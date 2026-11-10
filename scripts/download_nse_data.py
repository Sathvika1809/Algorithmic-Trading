#!/usr/bin/env python3
"""Download five years of daily NSE-listed equity/index data from Yahoo Finance.

Yahoo Finance is a third-party data source, not an official NSE feed. This script
is intended for educational/research backtesting; check provider terms before
redistributing downloaded data.
"""
from __future__ import annotations

import argparse
import csv
import sys
from datetime import date, timedelta
from pathlib import Path

try:
    import yfinance as yf
except ImportError:
    print("Missing dependency. Install it with: python -m pip install yfinance", file=sys.stderr)
    raise SystemExit(1)

DEFAULT_TICKERS = {
    "RELIANCE.NS": "RELIANCE",
    "TCS.NS": "TCS",
    "INFY.NS": "INFY",
    "HDFCBANK.NS": "HDFCBANK",
    "ICICIBANK.NS": "ICICIBANK",
    "^NSEI": "NIFTY50",
}

FIELDS = ["Date", "Open", "High", "Low", "Close", "Volume"]


def download_one(ticker: str, output_name: str, output_dir: Path, years: int) -> int:
    end = date.today() + timedelta(days=1)  # yfinance end date is exclusive
    start = end - timedelta(days=round(years * 365.25))
    print(f"Downloading {ticker}: {start.isoformat()} to {end.isoformat()} (end exclusive)")

    history = yf.Ticker(ticker).history(
        start=start.isoformat(),
        end=end.isoformat(),
        interval="1d",
        auto_adjust=True,
        actions=False,
    )
    if history.empty:
        print(f"WARNING: no rows returned for {ticker}; skipping", file=sys.stderr)
        return 0

    required = ["Open", "High", "Low", "Close", "Volume"]
    missing = [column for column in required if column not in history.columns]
    if missing:
        raise RuntimeError(f"{ticker}: missing columns from provider: {missing}")

    path = output_dir / f"{output_name}.csv"
    written = 0
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=FIELDS)
        writer.writeheader()
        for timestamp, row in history.iterrows():
            values = [row[column] for column in required]
            if any(value is None for value in values):
                continue
            try:
                open_price, high, low, close = (float(row[c]) for c in required[:4])
                volume = int(row["Volume"])
            except (TypeError, ValueError):
                continue
            if min(open_price, high, low, close) <= 0:
                continue
            writer.writerow({
                "Date": timestamp.strftime("%Y-%m-%d"),
                "Open": f"{open_price:.6f}",
                "High": f"{high:.6f}",
                "Low": f"{low:.6f}",
                "Close": f"{close:.6f}",
                "Volume": volume,
            })
            written += 1

    if written == 0:
        path.unlink(missing_ok=True)
        print(f"WARNING: {ticker} produced no valid rows", file=sys.stderr)
        return 0

    print(f"Saved {written} rows -> {path}")
    return written


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--years", type=int, default=5, help="history length in years (default: 5)")
    parser.add_argument("--output-dir", default="data/nse", help="CSV output directory")
    parser.add_argument("--tickers", nargs="*", help="optional Yahoo tickers, e.g. RELIANCE.NS TCS.NS")
    args = parser.parse_args()

    if args.years < 1 or args.years > 20:
        parser.error("--years must be between 1 and 20")

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    ticker_map = DEFAULT_TICKERS
    if args.tickers:
        ticker_map = {
            ticker: ("NIFTY50" if ticker == "^NSEI" else ticker.removesuffix(".NS"))
            for ticker in args.tickers
        }

    total = 0
    failures = []
    for ticker, output_name in ticker_map.items():
        try:
            total += download_one(ticker, output_name, output_dir, args.years)
        except Exception as exc:  # keep other symbols downloading if one fails
            failures.append((ticker, str(exc)))
            print(f"ERROR: {ticker}: {exc}", file=sys.stderr)

    print(f"\nDownload complete: {total} daily rows across {len(ticker_map)} requested symbols")
    if failures:
        print("Failed symbols:")
        for ticker, message in failures:
            print(f"  {ticker}: {message}")
        return 1
    return 0 if total else 1


if __name__ == "__main__":
    raise SystemExit(main())
