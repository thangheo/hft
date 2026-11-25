// Tick-to-signal replay benchmark.
//
// Replays a recorded Binance diff-depth feed (scripts/record_binance.py) through the
// production code path, single-threaded and in-process (no network, no ZMQ):
//
//   raw frame ─► BinanceWebSocketClient (parse #1, re-serialize, copy)       stage "ws_client"
//             ─► OrderBookNormalizer + BinanceAdapter (parse #2) + OrderBook  stage "normalize_book"
//             ─► OrderBookAnalyzer + getOrderBookDelta + ImbalanceDetection   stage "strategy"
//
// Reports p50/p90/p99/p99.9/max latency and heap allocations per message for each
// stage, plus single-thread throughput. Only messages that were actually applied to
// a valid book are sampled (pre-snapshot / stale events take a cheap early-out and
// would flatter the numbers).
//
// Usage: replay_bench --feed data/feeds/binance_depth.tsv [--loops 5] [--cpu 2]
//                     [--max-msgs N] [--json out.json]
//
// --max-msgs limits each pass to the first N diff messages. Latency of the current code
// grows with uptime (see "strategy p50 by tick" output), so a fixed window keeps
// before/after comparisons fair.

#include "core/market_types.hpp"
#include "data_ingestion/normalization/order_book_normalizer.hpp"
#include "data_ingestion/websocket/exchanges/binance_websocket_client.hpp"
#include "strategy/core_analysis/order_book_analyzer.hpp"
#include "strategy/pattern/imbalance_detection_engine.hpp"

#include <nlohmann/json.hpp>
#include <pthread.h>
#include <sched.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <new>
#include <string>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// Allocation counting: replacing the global operator new in the executable also
// interposes it for the shared libraries, so this sees every heap allocation.
// ---------------------------------------------------------------------------
static thread_local uint64_t g_allocs = 0;

void* operator new(std::size_t n) {
    ++g_allocs;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {

using Clock = std::chrono::steady_clock;
using trading::core::ExchangeId;
using trading::core::MarketDataUpdate;
namespace di = trading::data_ingestion;
namespace st = trading::strategy;

inline int64_t nowNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        Clock::now().time_since_epoch()).count();
}

struct Record {
    bool snapshot;
    std::string symbol;   // "BTCUSDT"
    std::string pair;     // "BTC/USDT" (normalizer's standardized form)
    uint64_t last_id;     // "u" for diff events, "lastUpdateId" for snapshots
    std::string payload;  // raw frame / snapshot body
    std::vector<uint8_t> payload_bytes;  // snapshots only: what a REST client would hand over
};

std::string toPair(const std::string& sym) {
    static const char* quotes[] = {"USDT", "USDC", "BTC", "ETH"};
    for (const char* q : quotes) {
        std::string qs(q);
        if (sym.size() > qs.size() && sym.compare(sym.size() - qs.size(), qs.size(), qs) == 0)
            return sym.substr(0, sym.size() - qs.size()) + "/" + qs;
    }
    return sym;
}

std::vector<Record> loadFeed(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "cannot open feed: " << path << "\n";
        std::exit(1);
    }
    std::vector<Record> out;
    std::string line;
    while (std::getline(in, line)) {
        Record r;
        size_t t1 = line.find('\t');
        size_t t2 = line.find('\t', t1 + 1);
        if (line[0] == 'S') {
            size_t t3 = line.find('\t', t2 + 1);
            r.snapshot = true;
            r.symbol = line.substr(t2 + 1, t3 - t2 - 1);
            r.payload = line.substr(t3 + 1);
            r.last_id = nlohmann::json::parse(r.payload)["lastUpdateId"].get<uint64_t>();
            r.payload_bytes.assign(r.payload.begin(), r.payload.end());
        } else {
            r.snapshot = false;
            r.payload = line.substr(t2 + 1);
            auto j = nlohmann::json::parse(r.payload);
            r.symbol = j["data"]["s"].get<std::string>();
            r.last_id = j["data"]["u"].get<uint64_t>();
        }
        r.pair = toPair(r.symbol);
        out.push_back(std::move(r));
    }
    return out;
}

struct Stage {
    const char* name;
    std::vector<uint32_t> ns;
    std::vector<uint32_t> allocs;
};

uint32_t pct(std::vector<uint32_t>& v, double p) {
    if (v.empty()) return 0;
    size_t idx = static_cast<size_t>(p / 100.0 * (v.size() - 1) + 0.5);
    return v[idx];
}

constexpr size_t kBucket = 1000;  // applied ticks per bucket for the uptime curve

struct Counters {
    uint64_t diff_msgs = 0, applied = 0, not_applied = 0, imbalances = 0, snapshots = 0;
};

struct PerSymbol {
    std::shared_ptr<di::OrderBook> book;
    std::unique_ptr<st::OrderBookAnalyzer> analyzer;
    std::unique_ptr<st::ImbalanceDetectionEngine> detector;
};

// One full pass over the feed with a fresh pipeline. Samples are appended only if record==true.
void runPass(const std::vector<Record>& feed, bool record, uint64_t maxMsgs,
             Stage& ws, Stage& norm, Stage& strat, Stage& total, Counters& c,
             std::vector<std::vector<uint32_t>>& byTick) {
    byTick.clear();
    uint64_t passMsgs = 0, passApplied = 0;
    // Points captured inside the client callback
    int64_t t_cb_enter = 0, t_cb_exit = 0;
    uint64_t a_cb_enter = 0, a_cb_exit = 0;

    di::OrderBookNormalizer normalizer([](const di::OrderBookUpdate&) {});

    // Mirrors ExchangeManager::onWebSocketData (exchange_manager.cpp) exactly.
    auto onData = [&](const ExchangeId& exchange, const std::string& symbol,
                      const std::vector<uint8_t>& data) {
        t_cb_enter = nowNs();
        a_cb_enter = g_allocs;
        MarketDataUpdate update{};
        update.exchangeId = exchange;
        update.tradingPair = symbol;
        update.receivedTimestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        update.data = data;
        update.updateType = "orderbook";
        normalizer.processMarketData(update);
        t_cb_exit = nowNs();
        a_cb_exit = g_allocs;
    };
    di::BinanceWebSocketClient client("replay", "0", onData);

    std::unordered_map<std::string, PerSymbol> syms;
    st::ImbalanceDetectionConfig detCfg;

    for (const Record& r : feed) {
        PerSymbol& ps = syms[r.pair];

        if (r.snapshot) {
            // What a REST snapshot fetcher would hand to the normalizer
            MarketDataUpdate u{};
            u.exchangeId = ExchangeId::binance();
            u.tradingPair = r.symbol;
            u.updateType = "orderbook";
            u.data = r.payload_bytes;
            normalizer.processMarketData(u);
            ++c.snapshots;
            continue;
        }

        if (maxMsgs && passMsgs >= maxMsgs) break;
        ++passMsgs;
        ++c.diff_msgs;
        const int64_t t0 = nowNs();
        const uint64_t a0 = g_allocs;
        client.injectMessage(r.payload);
        const int64_t t3 = nowNs();
        const uint64_t a3 = g_allocs;

        if (!ps.book) {
            ps.book = normalizer.findOrderBook(ExchangeId::binance(), r.pair);
            ps.analyzer = std::make_unique<st::OrderBookAnalyzer>(ExchangeId::binance(), r.pair, 10, 0.0001);
            ps.detector = std::make_unique<st::ImbalanceDetectionEngine>(detCfg);
        }
        const bool applied = ps.book && ps.book->isValid() && ps.book->getSequenceNumber() == r.last_id;
        if (!applied) {
            ++c.not_applied;
            continue;
        }
        ++c.applied;

        // Mirrors the core of StrategyEngine::processOrderBook (strategy_engine.cpp:154)
        const int64_t t4 = nowNs();
        const uint64_t a4 = g_allocs;
        ps.analyzer->processOrderBook(*ps.book);
        auto metrics = ps.analyzer->getMetrics();
        auto delta = ps.book->getOrderBookDelta();
        ps.detector->analyzeMetrics(metrics);
        auto imbalances = ps.detector->getDetectedImbalances();
        const int64_t t5 = nowNs();
        const uint64_t a5 = g_allocs;
        c.imbalances += imbalances.size();
        if (passApplied / kBucket >= byTick.size()) byTick.emplace_back();
        byTick[passApplied / kBucket].push_back(static_cast<uint32_t>(t5 - t4));
        ++passApplied;

        if (record) {
            ws.ns.push_back(static_cast<uint32_t>(t_cb_enter - t0));
            ws.allocs.push_back(static_cast<uint32_t>(a_cb_enter - a0));
            norm.ns.push_back(static_cast<uint32_t>(t_cb_exit - t_cb_enter));
            norm.allocs.push_back(static_cast<uint32_t>(a_cb_exit - a_cb_enter));
            strat.ns.push_back(static_cast<uint32_t>(t5 - t4));
            strat.allocs.push_back(static_cast<uint32_t>(a5 - a4));
            total.ns.push_back(static_cast<uint32_t>((t3 - t0) + (t5 - t4)));
            total.allocs.push_back(static_cast<uint32_t>((a3 - a0) + (a5 - a4)));
        }
    }
}

int64_t clockOverheadNs() {
    std::vector<int64_t> d(200000);
    for (auto& x : d) {
        int64_t a = nowNs();
        int64_t b = nowNs();
        x = b - a;
    }
    std::nth_element(d.begin(), d.begin() + d.size() / 2, d.end());
    return d[d.size() / 2];
}

}  // namespace

int main(int argc, char** argv) {
    std::string feedPath = "data/feeds/binance_depth.tsv";
    std::string jsonPath;
    int loops = 5;
    int cpu = -1;
    uint64_t maxMsgs = 0;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) { std::cerr << "missing value for " << a << "\n"; std::exit(2); }
            return argv[++i];
        };
        if (a == "--feed") feedPath = next();
        else if (a == "--loops") loops = std::stoi(next());
        else if (a == "--cpu") cpu = std::stoi(next());
        else if (a == "--json") jsonPath = next();
        else if (a == "--max-msgs") maxMsgs = std::stoull(next());
        else { std::cerr << "unknown arg " << a << "\n"; return 2; }
    }
    if (loops < 2) loops = 2;

    if (cpu >= 0) {
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(cpu, &set);
        if (pthread_setaffinity_np(pthread_self(), sizeof(set), &set) != 0)
            std::cerr << "warning: could not pin to cpu " << cpu << "\n";
    }

    auto feed = loadFeed(feedPath);
    std::cerr << "loaded " << feed.size() << " records from " << feedPath << "\n";

    // Silence the pipeline's std::cout chatter (e.g. "Creating new order book").
    std::cout.setstate(std::ios::failbit);

    Stage ws{"ws_client", {}, {}}, norm{"normalize_book", {}, {}},
          strat{"strategy", {}, {}}, total{"total", {}, {}};
    Counters warm, meas;
    std::vector<std::vector<uint32_t>> byTick;  // strategy ns per applied-tick bucket (last pass)

    runPass(feed, false, maxMsgs, ws, norm, strat, total, warm, byTick);  // warm-up
    const int64_t start = nowNs();
    for (int l = 1; l < loops; ++l) runPass(feed, true, maxMsgs, ws, norm, strat, total, meas, byTick);
    const double wallS = (nowNs() - start) / 1e9;

    std::cout.clear();

    const int64_t clk = clockOverheadNs();
    const double msgsPerS = meas.diff_msgs / wallS;

    std::printf("\nfeed: %s  (%zu records, %d measured loops, max-msgs %lu)\n",
                feedPath.c_str(), feed.size(), loops - 1, maxMsgs);
    std::printf("per loop: %lu diff msgs, %lu applied, %lu not applied, %lu snapshots, %lu imbalances\n",
                meas.diff_msgs / (loops - 1), meas.applied / (loops - 1), meas.not_applied / (loops - 1),
                meas.snapshots / (loops - 1), meas.imbalances / (loops - 1));
    std::printf("throughput: %.0f msg/s (single thread, full pass incl. snapshots)\n", msgsPerS);
    std::printf("clock overhead: ~%ld ns per now() pair\n\n", clk);
    std::printf("%-16s %9s %9s %9s %9s %9s %10s\n", "stage (ns)", "p50", "p90", "p99", "p99.9", "max", "allocs/msg");

    nlohmann::json out;
    out["feed"] = feedPath;
    out["records"] = feed.size();
    out["measured_loops"] = loops - 1;
    out["max_msgs"] = maxMsgs;
    out["applied_per_loop"] = meas.applied / (loops - 1);
    out["not_applied_per_loop"] = meas.not_applied / (loops - 1);
    out["throughput_msg_per_s"] = msgsPerS;
    out["clock_overhead_ns"] = clk;

    for (Stage* s : {&ws, &norm, &strat, &total}) {
        std::sort(s->ns.begin(), s->ns.end());
        std::sort(s->allocs.begin(), s->allocs.end());
        uint32_t p50 = pct(s->ns, 50), p90 = pct(s->ns, 90), p99 = pct(s->ns, 99),
                 p999 = pct(s->ns, 99.9), mx = s->ns.empty() ? 0 : s->ns.back();
        uint32_t a50 = pct(s->allocs, 50);
        std::printf("%-16s %9u %9u %9u %9u %9u %10u\n", s->name, p50, p90, p99, p999, mx, a50);
        out["stages"][s->name] = {{"p50_ns", p50}, {"p90_ns", p90}, {"p99_ns", p99},
                                  {"p999_ns", p999}, {"max_ns", mx}, {"allocs_p50", a50},
                                  {"samples", s->ns.size()}};
    }

    std::printf("\nstrategy p50 (ns) by applied tick, last pass:\n");
    for (size_t b = 0; b < byTick.size(); ++b) {
        auto& v = byTick[b];
        std::sort(v.begin(), v.end());
        uint32_t p50 = pct(v, 50);
        std::printf("  ticks %6zu-%-6zu %9u\n", b * kBucket, b * kBucket + v.size() - 1, p50);
        out["strategy_p50_by_tick"].push_back({{"from", b * kBucket}, {"p50_ns", p50}});
    }

    if (!jsonPath.empty()) {
        std::ofstream(jsonPath) << out.dump(2) << "\n";
        std::fprintf(stderr, "\nwrote %s\n", jsonPath.c_str());
    }
    return 0;
}
