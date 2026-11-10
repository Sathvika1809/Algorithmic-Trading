# Algorithmic Trading System & NSE Historical Backtester

A C++17 algorithmic trading project implementing core trading-system components, including a custom hash map, order management, order book, portfolio management, and automated trading strategies. The project also includes an NSE historical-data backtesting extension to evaluate trading strategies using daily market data.

The system focuses on **data structures, object-oriented design, trade execution logic, portfolio management, and quantitative strategy evaluation**.

## Features

### 1. Core Trading System (C++17)

- **Custom Hash Map:** Templated hash map implementation using separate chaining for collision handling and dynamic resizing based on load factor.
- **Order Management:** Order representation and lifecycle management, including buy and sell orders.
- **Order List:** Linked-list-based order management for maintaining and processing orders.
- **Order Book:** Maintains buy and sell orders and supports matching against available liquidity.
- **Portfolio Management:** Tracks holdings, cash balance, executed trades, and portfolio value.
- **Market Data Processing:** Processes market data and maintains price-related information.
- **Trading Engine:** Coordinates order processing, trade execution, portfolio updates, and strategy execution.

### 2. Trading Strategies

The project implements strategy components for generating trading decisions:

- **Buy Low Sell High:** Identifies buying and selling opportunities based on configured price thresholds.
- **Arbitrage Strategy:** Detects potential price differences between market instruments.
- **Trading Strategy Interface:** Provides a common structure for implementing and extending trading strategies.

The trading engine uses synthetic liquidity for order matching. It is a simulation and is not connected to a live brokerage or exchange.

### 3. NSE Historical-Data Backtesting Extension

An additive extension that evaluates trading strategies using five years of historical daily OHLCV data.

**Data universe:**

| Instrument | Type |
|---|---|
| RELIANCE | NSE equity |
| TCS | NSE equity |
| INFY | NSE equity |
| HDFCBANK | NSE equity |
| ICICIBANK | NSE equity |
| NIFTY 50 (`^NSEI`) | Index benchmark |

**Data source:** Yahoo Finance through the Python `yfinance` library. This is a third-party public data interface, not an official NSE data feed. The project uses auto-adjusted OHLC values and daily bars.

Reference: https://ranaroussi.github.io/yfinance/

#### Backtesting Strategies

**BUY_LOW_SELL_HIGH**

- Calculates a 20-session Simple Moving Average (SMA).
- Buys when the closing price is at least 3% below the SMA.
- Sells when the closing price is at least 3% above the SMA.

**MA_CROSSOVER**

- Uses 20-session and 50-session Simple Moving Averages.
- Generates a buy signal when the short-term SMA crosses above the long-term SMA.
- Generates a sell signal when the short-term SMA crosses below the long-term SMA.

**BUY_AND_HOLD**

- Buys at the first available session's open.
- Holds the position until the final session.
- Liquidates at the final session's close.

**INDEX_BUY_AND_HOLD**

- Uses the NIFTY 50 price index as a market benchmark.
- Provides an index-return comparison rather than a directly executable investment strategy.

#### Execution Model

- Trading signals are generated after the session's close.
- Orders are executed at the next trading session's open.
- Supports long-only positions and whole-share quantities.
- Applies configurable transaction fees and slippage on both buy and sell transactions.
- Liquidates remaining positions at the final session's close, accounting for exit costs.

Default execution assumptions:

| Parameter | Value |
|---|---:|
| Initial capital | ₹100,000 |
| Transaction fee | 0.10% per side |
| Slippage | 0.05% per side |
| Short SMA | 20 sessions |
| Long SMA | 50 sessions |
| Mean-reversion band | 3% |
| Annualization factor | 252 trading sessions |

#### Performance Evaluation

The backtester calculates:

- **Net PnL:** Difference between final and initial portfolio equity.
- **Total Return:** Overall percentage return.
- **Annualized Return:** Compounded return annualized using 252 trading sessions per year.
- **Sharpe Ratio:** Annualized ratio of mean daily return to sample standard deviation of daily returns, assuming a zero risk-free rate.
- **Maximum Drawdown:** Largest peak-to-trough decline in the daily equity curve.
- **Closed Trades:** Number of completed trades.
- **Winning Trades and Win Rate:** Measures the proportion of profitable closed trades.

Daily returns are calculated from consecutive daily equity observations, excluding the first day's return. The final equity reflects liquidation after applicable exit costs.

## Backtesting Results

The following results are from the five-year historical backtest, using an initial capital of ₹100,000 for each individual run.

| Symbol | Strategy | Total Return | Sharpe Ratio |
|---|---|---:|---:|
| RELIANCE | Buy Low Sell High | 23.38% | 0.3380 |
| RELIANCE | MA Crossover | 0.97% | 0.0831 |
| RELIANCE | Buy and Hold | 14.64% | 0.2321 |
| TCS | Buy Low Sell High | -25.21% | -0.2770 |
| TCS | MA Crossover | -22.99% | -0.3191 |
| TCS | Buy and Hold | -18.01% | -0.0645 |
| INFY | Buy Low Sell High | -44.18% | -0.5626 |
| INFY | MA Crossover | -12.19% | -0.0765 |
| INFY | Buy and Hold | -22.68% | -0.0663 |
| HDFCBANK | Buy Low Sell High | 56.87% | 0.7328 |
| HDFCBANK | MA Crossover | -30.47% | -0.4409 |
| HDFCBANK | Buy and Hold | 9.67% | 0.1974 |
| ICICIBANK | Buy Low Sell High | 83.03% | 1.0722 |
| ICICIBANK | MA Crossover | 21.82% | 0.3541 |
| ICICIBANK | Buy and Hold | 111.50% | 0.8823 |
| NIFTY 50 | Index Buy and Hold | 35.06% | 0.5571 |

*Note: These are individual backtests, not a combined multi-stock portfolio. Historical performance does not guarantee future results.*

## Project Structure

```text
AlgorithmicTrading/
│
├── include/
│   └── NSEBacktest.h
│
├── src/
│   ├── ArbitrageStrategy.cpp
│   ├── BuyLowSellHigh.cpp
│   ├── MarketData.cpp
│   ├── NSEBacktest.cpp
│   ├── Order.cpp
│   ├── OrderBook.cpp
│   ├── OrderList.cpp
│   ├── Portfolio.cpp
│   ├── TradingEngine.cpp
│   └── TradingStrategy.cpp
│
├── scripts/
│   └── download_nse_data.py
│
├── data/
│   └── nse/
│       └── Historical daily OHLCV CSV files
│
├── results/
│   ├── backtest_metrics.csv
│   └── Daily equity-curve CSV files
│
├── backtest_main.cpp
├── main.cpp
├── build.ps1
├── run_backtest.ps1
├── requirements.txt
└── README.md
```

## Getting Started

### Prerequisites

- C++17-compatible compiler (GCC / MinGW recommended).
- Python 3.
- pip package manager.
- Internet connection for downloading historical market data.

### Build the Core Project

From the project directory:

```powershell
cd E:\AlgorithmicTrading

g++ -std=c++17 -Wall -Wextra main.cpp src\*.cpp -o trading.exe

.\trading.exe
```

### Run Unit Tests

The project includes test suites for its core components.

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 test
```

Test coverage includes:

- Arbitrage Strategy
- Trading Engine
- Custom Hash Map
- Market Data
- Order
- Order Book
- Order List
- Portfolio
- Trading Strategy

**Latest test execution: 767 checks passed across 9 test suites.**

### Run NSE Historical Backtests

The historical backtester has a separate entry point and does not replace the existing `main.cpp`.

Run:

```powershell
powershell -ExecutionPolicy Bypass -File .\run_backtest.ps1
```

The script checks or installs Python requirements, downloads five years of historical data, compiles the C++ backtesting engine, and executes the strategies.

Alternatively, execute the steps individually:

```powershell
python -m pip install -r requirements.txt

python .\scripts\download_nse_data.py --years 5

g++ -std=c++17 -Wall -Wextra -O2 backtest_main.cpp src\*.cpp -o backtest.exe

.\backtest.exe
```

### Generated Outputs

The downloader stores daily market data under:

```text
data/nse/
```

The backtesting engine generates:

```text
results/
    backtest_metrics.csv
    RELIANCE_BUY_LOW_SELL_HIGH_equity.csv
    RELIANCE_MA_CROSSOVER_equity.csv
    RELIANCE_BUY_AND_HOLD_equity.csv
    ...
    NIFTY50_INDEX_BUY_AND_HOLD_equity.csv
```

`backtest_metrics.csv` contains the performance metrics for each symbol and strategy. The equity-curve files contain daily cash, quantity, closing price, and portfolio equity.

## Configuration

Backtesting parameters can be modified through `BacktestConfig` in:

```text
include/NSEBacktest.h
```

The configuration includes initial capital, transaction fees, slippage, moving-average windows, and the mean-reversion band percentage.

The default ticker universe can be modified through `DEFAULT_TICKERS` in:

```text
scripts/download_nse_data.py
```

Example:

```powershell
python .\scripts\download_nse_data.py --years 5 --tickers RELIANCE.NS TCS.NS INFY.NS ^NSEI
```

## Testing and Validation

The project includes individual test suites for core trading components, covering data structures, order management, matching, portfolio operations, strategy logic, and engine behavior.

The historical backtesting extension additionally produces:
- Strategy-wise performance metrics.
- Daily equity curves.
- A NIFTY 50 index benchmark for comparison.

The historical results should be treated as research outputs under the stated data and execution assumptions, not as proof of live-market profitability.

## Limitations

- The default stock universe contains currently selected symbols and does not account for survivorship bias from delisted stocks.
- Yahoo Finance data may be revised, incomplete, or rate-limited.
- Auto-adjusted OHLC prices may differ from the exact historical execution prices available at the time.
- The NIFTY 50 benchmark is a price index, not a total-return index; dividends are not included.
- The backtester uses daily bars and does not reconstruct historical intraday trades or order-book liquidity.
- Fee and slippage assumptions are simplified placeholders and do not fully model Indian brokerage, STT, GST, stamp duty, exchange charges, or taxes.
- The existing order book uses synthetic liquidity and is not integrated with a live exchange or brokerage API.
- The project is intended for educational and research purposes, not investment advice.

## Technologies Used

- **C++17:** Trading engine, data structures, order book, portfolio management, and backtesting.
- **Python:** Historical market-data downloading and preprocessing.
- **yfinance:** Yahoo Finance historical market-data interface.
- **CSV:** Market-data input, performance metrics, and equity-curve output.
- **PowerShell:** Build, test, and backtesting automation.
- **GCC / MinGW:** C++ compilation.

## Future Improvements

- Implement multi-stock portfolio-level backtesting with shared capital allocation.
- Introduce out-of-sample testing and walk-forward strategy evaluation.
- Add more technical indicators and configurable strategy parameters.
- Incorporate more realistic Indian transaction costs and execution constraints.
- Compare against a NIFTY 50 Total Return Index benchmark.
- Add performance visualization for equity curves, drawdowns, and strategy comparisons.
- Explore integration with paper-trading APIs.

---

**Disclaimer:** This project is for educational and research purposes only. Backtested results are based on historical data and simplified assumptions and do not guarantee future performance.