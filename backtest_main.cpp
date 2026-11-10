#include "include/NSEBacktest.h"

#include <filesystem>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
    const std::string dataDir = argc > 1 ? argv[1] : "data/nse";
    const std::string resultsDir = argc > 2 ? argv[2] : "results";
    fs::create_directories(resultsDir);

    BacktestConfig config;
    std::vector<std::string> stocks = {
        "RELIANCE", "TCS", "INFY", "HDFCBANK", "ICICIBANK"
    };
    const std::vector<std::string> strategies = {
        "BUY_LOW_SELL_HIGH", "MA_CROSSOVER"
    };
    std::vector<BacktestMetrics> allMetrics;

    std::cout << "NSE historical-data backtest (daily bars)\n"
              << "Initial capital: Rs " << config.initialCash
              << " | Fee/side: " << config.feeRate * 100.0 << "%"
              << " | Slippage/side: " << config.slippageRate * 100.0 << "%\n\n";

    for (const std::string& symbol : stocks)
    {
        const std::string inputPath = (fs::path(dataDir) / (symbol + ".csv")).string();
        std::vector<DailyBar> bars;
        std::string error;
        if (!loadDailyBars(inputPath, bars, error))
        {
            std::cerr << "Skipping " << symbol << ": " << error << '\n';
            continue;
        }
        std::cout << symbol << ": " << bars.size() << " sessions, "
                  << bars.front().date << " to " << bars.back().date << '\n';
        for (const std::string& strategy : strategies)
        {
            const std::string equityPath = (fs::path(resultsDir) /
                (symbol + "_" + strategy + "_equity.csv")).string();
            BacktestMetrics metrics = runBacktest(symbol, bars, strategy, config, equityPath);
            allMetrics.push_back(metrics);
        }

        // Per-stock buy-and-hold benchmark, with the same fee/slippage assumptions.
        const std::string holdEquityPath = (fs::path(resultsDir) /
            (symbol + "_BUY_AND_HOLD_equity.csv")).string();
        allMetrics.push_back(runBacktest(symbol, bars, "BUY_AND_HOLD", config, holdEquityPath));
    }

    // NIFTY 50 index benchmark, represented as an index investment proxy.
    {
        std::vector<DailyBar> indexBars;
        std::string error;
        const std::string path = (fs::path(dataDir) / "NIFTY50.csv").string();
        if (loadDailyBars(path, indexBars, error))
        {
            const std::string indexEquityPath = (fs::path(resultsDir) / "NIFTY50_INDEX_BUY_AND_HOLD_equity.csv").string();
            BacktestMetrics index = runBacktest("NIFTY50", indexBars, "BUY_AND_HOLD", config, indexEquityPath);
            index.strategy = "INDEX_BUY_AND_HOLD";
            allMetrics.push_back(index);
            std::cout << "NIFTY50: " << indexBars.size() << " sessions (index reference)\n";
        }
        else std::cerr << "NIFTY50 benchmark unavailable: " << error << '\n';
    }

    const std::string metricsPath = (fs::path(resultsDir) / "backtest_metrics.csv").string();
    if (!writeMetricsCsv(metricsPath, allMetrics))
    {
        std::cerr << "Could not write " << metricsPath << '\n';
        return 1;
    }

    std::cout << "\n" << std::left << std::setw(12) << "SYMBOL"
              << std::setw(22) << "STRATEGY" << std::right
              << std::setw(13) << "NET PnL" << std::setw(11) << "RETURN%"
              << std::setw(10) << "SHARPE" << std::setw(12) << "MAX DD%"
              << std::setw(10) << "TRADES" << std::setw(10) << "WIN%" << '\n';
    for (const BacktestMetrics& m : allMetrics)
        std::cout << std::left << std::setw(12) << m.symbol
                  << std::setw(22) << m.strategy << std::right
                  << std::setw(13) << std::fixed << std::setprecision(2) << m.netPnl
                  << std::setw(11) << m.totalReturnPct
                  << std::setw(10) << m.sharpeRatio
                  << std::setw(12) << m.maxDrawdownPct
                  << std::setw(10) << m.closedTrades
                  << std::setw(10) << m.winRatePct << '\n';

    std::cout << "\nSaved metrics: " << metricsPath
              << "\nSaved daily equity curves under: " << resultsDir << '\n';
    return 0;
}
