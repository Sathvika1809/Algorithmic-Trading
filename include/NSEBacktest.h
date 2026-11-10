#ifndef NSE_BACKTEST_H
#define NSE_BACKTEST_H

#include <string>
#include <vector>

struct DailyBar
{
    std::string date;
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    long long volume = 0;
};

struct BacktestConfig
{
    double initialCash = 100000.0;
    double feeRate = 0.001;       // 0.10% per side; configurable assumption
    double slippageRate = 0.0005; // 0.05% per side; configurable assumption
    int shortWindow = 20;
    int longWindow = 50;
    double bandPct = 0.03;        // +/- 3% around 20-day SMA
};

struct BacktestMetrics
{
    std::string symbol;
    std::string strategy;
    double initialCash = 0.0;
    double finalEquity = 0.0;
    double netPnl = 0.0;
    double totalReturnPct = 0.0;
    double annualizedReturnPct = 0.0;
    double sharpeRatio = 0.0;
    double maxDrawdownPct = 0.0;
    int closedTrades = 0;
    int winningTrades = 0;
    double winRatePct = 0.0;
};

bool loadDailyBars(const std::string& path, std::vector<DailyBar>& bars,
                   std::string& error);
BacktestMetrics runBacktest(const std::string& symbol,
                            const std::vector<DailyBar>& bars,
                            const std::string& strategy,
                            const BacktestConfig& config,
                            const std::string& equityOutputPath);
bool writeMetricsCsv(const std::string& path,
                     const std::vector<BacktestMetrics>& metrics);

#endif
