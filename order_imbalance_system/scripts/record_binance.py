#!/usr/bin/env python3
"""Record a Binance spot diff-depth feed (plus REST snapshots) for deterministic replay.

Follows Binance's book-sync procedure: subscribe first, then fetch a REST snapshot
per symbol, so the snapshot lands in the file *after* some buffered events and the
replayer must bridge it exactly like a live client would.

Output format (one record per line, tab-separated):
    S <recv_ns> <SYMBOL> <snapshot json>     REST /api/v3/depth snapshot
    D <recv_ns> <raw combined-stream frame>  {"stream": "...", "data": {...}}

recv_ns is time.time_ns() taken right after the frame was received.

Requires: pip install websockets aiohttp
Usage:    record_binance.py --minutes 15 --out data/feeds/binance_depth.tsv
"""
import argparse
import asyncio
import time

import aiohttp
import websockets

DEFAULT_SYMBOLS = [
    "btcusdt", "ethusdt", "bnbusdt", "solusdt", "xrpusdt",
    "dogeusdt", "adausdt", "trxusdt", "linkusdt", "avaxusdt",
]
WS_URL = "wss://stream.binance.com:9443/stream?streams="
REST_URL = "https://api.binance.com/api/v3/depth?symbol={}&limit=1000"


async def fetch_snapshots(symbols, out, delay_s):
    # Let some events buffer first so the replayer has to bridge U/u correctly.
    await asyncio.sleep(delay_s)
    async with aiohttp.ClientSession() as session:
        for sym in symbols:
            async with session.get(REST_URL.format(sym.upper())) as resp:
                body = await resp.text()
            out.write(f"S\t{time.time_ns()}\t{sym.upper()}\t{body}\n")


async def record(symbols, minutes, path):
    streams = "/".join(f"{s}@depth@100ms" for s in symbols)
    deadline = time.monotonic() + minutes * 60
    n = 0
    with open(path, "w", buffering=1 << 20) as out:
        async with websockets.connect(WS_URL + streams, max_size=None) as ws:
            snap_task = asyncio.create_task(fetch_snapshots(symbols, out, 2.0))
            try:
                while time.monotonic() < deadline:
                    frame = await ws.recv()
                    out.write(f"D\t{time.time_ns()}\t{frame}\n")
                    n += 1
                    if n % 10000 == 0:
                        print(f"{n} frames", flush=True)
            except websockets.ConnectionClosed as e:
                # Reconnecting would need fresh snapshots; keep the consistent prefix.
                print(f"connection closed after {n} frames: {e}", flush=True)
            await snap_task
    print(f"done: {n} frames -> {path}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--minutes", type=float, default=15)
    ap.add_argument("--symbols", default=",".join(DEFAULT_SYMBOLS))
    ap.add_argument("--out", default="data/feeds/binance_depth.tsv")
    args = ap.parse_args()
    asyncio.run(record(args.symbols.split(","), args.minutes, args.out))


if __name__ == "__main__":
    main()
