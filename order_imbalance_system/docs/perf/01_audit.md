# Ngày 1: Audit data flow, ownership, concurrency

Phạm vi: đường tick-to-order cho Binance depth update. Mọi nhận định dưới đây đều có tham chiếu
`file:line` (tại commit `2d73d60`). Những nhận định có dán nhãn **[đã xác nhận]** đã được chạy thử.

## TL;DR

1. **Hot path chưa được nối với nhau.** Từng stage tồn tại riêng lẻ, nhưng không có chỗ nào gọi
   `StrategyEngine::processOrderBook`, `OrderBookPublisher::publishOrderBookUpdate` hay
   `PreTradeRiskFilter::validateSignal`. `main.cpp` bị comment toàn bộ. Có **2 pipeline ingestion
   song song** (`ExchangeManager`+`OrderBookNormalizer` và `DataSourceManager`+`protocol/BinanceAdapter`).
2. **Pipeline A sẽ deadlock ngay ở message đầu tiên** [đã xác nhận]: `OrderBook` gọi callback khi
   đang giữ `std::mutex`, còn callback do normalizer đăng ký lại lock chính mutex đó.
3. **Logic sequence sai với giao thức Binance**: book có thể bị coi là hợp lệ trong khi đã lệch, và
   không bao giờ lấy REST snapshot.
4. **Chi phí trên mỗi message rất lớn so với yêu cầu low-latency**: parse JSON 2 lần, serialize lại 1
   lần, khoảng 5 lần copy buffer, copy **toàn bộ 2 `std::map`** mỗi update (mà không ai đọc), và
   gọi `system_clock::now()` cho mỗi price level.

→ Ngày 2 sẽ **dựng một harness in-process** nối các stage thành chuỗi tick-to-order, thay vì
benchmark cái pipeline không chạy được hiện tại. Trước khi benchmark cần sửa B1 và B2.

---

## 1. Data flow (pipeline A, như đã thiết kế)

```
[io thread của WebSocketClientBase]  (mọi bước dưới đây chạy đồng bộ trên MỘT thread)
 ws.read(flat_buffer)                                  websocket_client_base.cpp:237
   └─ buffers_to_string → std::string msg      COPY#1  websocket_client_base.cpp:244
 BinanceWebSocketClient::onWebSocketMessage
   ├─ json::parse(message)                     PARSE#1 binance_websocket_client.cpp:239
   ├─ json stream_data = data["data"]          COPY#2  (copy cây JSON con)   :297
   └─ processOrderBookData
        ├─ data.dump() → std::string           SERIALIZE                      :353
        └─ vector<uint8_t>(str)                COPY#3                         :354
 ExchangeManager::onWebSocketData
   ├─ receivedTimestamp = system_clock::now()  (đo SAU parse + dump)  exchange_manager.cpp:92
   └─ update.data = data                       COPY#4                         :94
 OrderBookNormalizer::processMarketData
   ├─ exchange_adapters_.find(toString())      (tra map theo string)  order_book_normalizer.cpp:54
   ├─ BinanceAdapter::normalize
   │    ├─ std::string(update.data)            COPY#5                         :193
   │    ├─ json::parse                         PARSE#2                        :196
   │    └─ stod() mỗi level → vector<PriceLevel> (push_back, không reserve)   :219-236
   ├─ standardizeTradingPair                   (tạo vector<string> 8 phần tử mỗi lần)  :258
   ├─ getOrCreateOrderBook                     (nối chuỗi key + mutex + map<string>)   :109-117
   ├─ OrderBook::applyUpdate                   (xem §3)
   │    └─ notifyCallback → callback → book.getSnapshot()   ⚠ DEADLOCK (B1)
   └─ callback_(normalized)                    (callback lần 2 cho cùng message)  :98
 → OrderBookPublisher::publishOrderBookUpdate  (KHÔNG có ai gọi)
   ├─ stringstream JSON                        SERIALIZE              order_book_publisher.cc:~395
   └─ ZeroMQPublisher::publish
        ├─ compressed_data = data              COPY (kể cả khi không nén)  :86
        ├─ mutex + std::queue + condvar        hand-off sang worker thread  :95-119
        └─ zmq::message_t(data)                COPY                         :200

[StrategyEngine::processOrderBook]             (KHÔNG có ai gọi) strategy_engine.cpp:154
   ├─ OrderBookAnalyzer: getBids/getAsks       (lock + cấp phát vector mỗi lần)
   ├─ orderBook.getOrderBookDelta()            (tham số mặc định → copy TOÀN BỘ book)  :176
   ├─ ImbalanceDetectionEngine::analyzeMetrics
   └─ tradingPairStates_.find(...)             (không lock, xem B4)               :201
[PreTradeRiskFilter::validateSignal]           (KHÔNG có ai gọi)
   └─ 5 bước kiểm tra + LOG_INFO đồng bộ cho mỗi quyết định
[OrderGenerator::createOrderFromSignal]        (mutex + stringstream để sinh client id)
```

## 2. Bản đồ thread

| Thread | Tạo ở | Làm gì |
|---|---|---|
| WS io thread | `websocket_client_base.cpp:115` | read → parse → normalize → OrderBook → callback. Blocking read, **mọi xử lý chạy inline** |
| WS ping thread | `:118` | ping định kỳ, dùng chung `write_mutex_` với phía ghi |
| Connection monitor | `exchange_manager.cpp:73` | kiểm tra kết nối, lock `mutex_` |
| ZMQ publisher worker | `order_book_publisher.cc:40` | lấy từ queue (mutex+condvar) → `zmq send` |
| Strategy maintenance | `strategy_engine.cpp:113` | `sleep 100ms`, dọn signal, lock `mutex_` |

Nhận xét: phần parse và cập nhật book chạy trên thread I/O, nên mọi độ trễ ở đây trực tiếp
làm chậm việc đọc socket (head-of-line blocking).

## 3. Ownership

| Object | Ai tạo | Ai giữ | Ghi chú |
|---|---|---|---|
| Buffer WS (`flat_buffer`) | io thread (biến local) | io thread | dùng lại được, nhưng nội dung bị copy ra `std::string` ngay sau đó |
| `nlohmann::json` | client, và **lần nữa** ở adapter | biến local | mỗi node là một lần cấp phát heap |
| `MarketDataUpdate` | `ExchangeManager` (trên stack) | truyền bằng `const&` | `data` là `vector<uint8_t>` copy |
| `OrderBookUpdate` | `BinanceAdapter::normalize` (trả về theo giá trị) | normalizer | chứa 2 `vector<PriceLevel>` cấp phát mới mỗi message |
| `OrderBook` | `make_shared` trong normalizer | `map<string, shared_ptr<OrderBook>>` | `shared_ptr` không cần thiết vì vòng đời gắn với normalizer |
| `OrderBookDelta` | `applyUpdate` (stack) | **copy** vào `deque` 100 phần tử | 4 vector + 1 string mỗi delta, cộng thêm 1 lần copy khi lưu |
| `previous_bids_/asks_` | `captureOrderBookState` | `OrderBook` | **copy cả map mỗi update, không ai đọc** (`order_book.cpp:282`) |
| `Order` | `OrderGenerator` | `shared_ptr<Order>` | |
| Logger | singleton | `getLogger()` trả `shared_ptr` **theo giá trị** | mỗi lần log tốn 2 thao tác atomic refcount |

## 4. Concurrency

| Lock / atomic | Bảo vệ | Vấn đề |
|---|---|---|
| `OrderBook::mutex_` (`std::mutex`) | `bids_`, `asks_`, delta | **giữ lock khi gọi callback → deadlock (B1)**. `sequence_number_` bị đọc **ngoài** lock (`:84`, `:145`) nhưng ghi trong lock → data race (B3) |
| `OrderBook::is_valid_` (`atomic<bool>`) | trạng thái hợp lệ | bị kiểm tra và ghi theo kiểu check-then-act ngoài lock |
| `OrderBookNormalizer::mutex_` | `order_books_` | lock ở mỗi message chỉ để tra map |
| `ZeroMQPublisher::queue_mutex_` + condvar | `message_queue_` | mỗi message: 1 lock, 1 `notify_one` (có thể futex syscall). `stats_` (struct `uint64_t` thường) bị ghi từ thread producer **không có lock** (`:88-90`) → data race (B3) |
| `StrategyEngine::mutex_` | `tradingPairStates_` ... | `processOrderBook` truy cập `tradingPairStates_` **không lock** (`:201`) trong khi `initializeTradingPair` insert vào map (`:641`) → data race (B4) |
| `OrderBookAnalyzer::mutex_` | metrics | lock mỗi tick, chưa thấy contention vì chỉ có 1 writer |
| Logger `mutex_` toàn cục | `std::cout` | log đồng bộ, dùng `std::endl` → flush và gọi `write(2)` cho **mỗi dòng log** (`logger.cpp:31-48`) |

---

## 5. Bug correctness (sửa trước khi benchmark)

| # | Bug | Bằng chứng | Mức độ |
|---|---|---|---|
| B1 | Deadlock: `notifyCallback()` chạy khi đang giữ `mutex_`; callback do normalizer đăng ký gọi `getSnapshot()`, hàm này lock lại cùng mutex | `order_book.cpp:134,197` + `order_book_normalizer.cpp:141-144`. **[đã xác nhận]**: chương trình test treo, `timeout 5` trả exit 124 | Pipeline A treo ở message đầu tiên |
| B2 | Logic sequence sai với Binance diff-depth: event mang `U` (first) và `u` (final), `u` tăng theo **số thay đổi** chứ không tăng 1. Code chỉ dùng `u` và coi `seq > last+1` là có gap. Ngoài ra không lấy REST snapshot, và book chưa từng hợp lệ (`is_valid_=false`) thì **bỏ qua luôn bước kiểm tra gap** | `order_book_normalizer.cpp:211`, `order_book.cpp:150` | Book có thể sai mà không ai biết |
| B3 | Data race: `sequence_number_` trong OrderBook; `stats_` trong publisher | xem §4 | UB, sẽ được TSan bắt ở ngày 4 |
| B4 | Data race: `tradingPairStates_` trong StrategyEngine | `strategy_engine.cpp:201` vs `:641` | UB |
| B5 | `exchange_timestamp` không bao giờ được gán (bỏ qua field `E`) → luôn bằng 0; `receivedTimestamp` lấy sau khi đã parse | `exchange_manager.cpp:89-95` | Không đo được latency thật |
| B6 | Callback chạy 2 lần cho mỗi message, một lần kèm copy toàn bộ book | `order_book_normalizer.cpp:98,144` | Lãng phí, và subscriber thấy trùng dữ liệu |
| B7 | `SequenceGapFilter` lưu sequence bằng `int` | `filter.hpp:149` | Tràn số với sequence của Binance |
| B8 | `OrderBookAnalyzer(config)` (viết tạm lúc sửa build) gán exchange = UNKNOWN, pair = "" | `order_book_analyzer.cpp` | Metrics gắn nhãn sai |

## 6. Giả thuyết bottleneck (xếp theo mức nghi ngờ) và cách đo

Đặt `N` = số level trong một update, `B` = tổng số level của book.

| # | Giả thuyết | Chi phí ước lượng mỗi msg | Đo bằng gì (ngày 2/3) |
|---|---|---|---|
| H1 | Parse JSON 2 lần + dump 1 lần + khoảng 5 lần copy buffer | O(kích thước msg) × 3, vài chục đến vài trăm lần cấp phát | microbench từng stage; đếm alloc/msg; flamegraph |
| H2 | `captureOrderBookState` copy 2 `std::map` | **O(B) lần cấp phát** mỗi update, với B có thể lên tới hàng nghìn | alloc/msg theo B; so sánh trước/sau khi bỏ |
| H3 | `std::map<double>` dạng node: mỗi lần `find`/`insert`/`erase` là pointer chasing | cache miss × N × log B | `perf stat` L1/LLC miss; so với flat array theo tick |
| H4 | Delta: 4 vector + `symbol_.toString()` + copy vào deque | khoảng 6 lần cấp phát mỗi msg | đếm alloc |
| H5 | `system_clock::now()` trong `updateBidLevel/AskLevel` | N lần gọi vDSO (khoảng 20 ns mỗi lần) | microbench, `perf` |
| H6 | Strategy: `getBids/getAsks` cấp phát, `getOrderBookDelta()` copy toàn bộ book, `Symbol::fromString` mỗi tick | O(B) mỗi tick | microbench `processOrderBook` |
| H7 | Log đồng bộ trên đường risk (`cout` + `endl` + mutex + atomic của `shared_ptr`) | 1 syscall mỗi quyết định | `perf trace -s`, `strace -c`; off-CPU flamegraph |
| H8 | Publisher: copy khi không nén, JSON bằng `stringstream`, mutex+condvar | vài lần cấp phát + có thể futex | `perf lock`, đếm context switch |
| H9 | Key dạng string: nối key + `map<string>` + `vector<string>` trong `standardizeTradingPair` | khoảng 10 lần cấp phát mỗi msg | đếm alloc |

H1, H2 và H3 được dự đoán là chiếm phần lớn thời gian. Ngày 3 sẽ xác nhận hoặc bác bỏ bằng số đo.

## 7. Quyết định cho ngày 2

1. **Sửa B1 và B2 trước** (callback đưa ra ngoài lock; dùng đúng `U`/`u` của Binance). Nếu không sửa
   thì chẳng có gì để đo.
2. **Hot path dùng để demo** sẽ là một harness in-process, chạy trên 1 thread và có thể replay:
   ```
   raw Binance JSON (file) → parse → OrderBook::applyUpdate → OrderBookAnalyzer
                           → ImbalanceDetectionEngine → PreTradeRiskFilter → OrderGenerator
   ```
   Harness sẽ có timestamp cho từng stage. Pipeline A/B với WebSocket và ZMQ nằm ngoài phần đo:
   network jitter không phải thứ chúng ta tối ưu.
3. Đặt `receivedTimestamp` ngay sau `ws.read()` để đo đúng (B5).
