#include "../include/NSEBacktest.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace
{
    bool parseCsvRow(const std::string& line, DailyBar& bar)
    {
        std::stringstream stream(line);
        std::string field;
        std::vector<std::string> fields;
        while (std::getline(stream, field, ',')) fields.push_back(field);
        if (fields.size() < 6 || fields[0] == "Date") return false;
        try
        {
            bar.date = fields[0];
            bar.open = std::stod(fields[1]);
            bar.high = std::stod(fields[2]);
            bar.low = std::stod(fields[3]);
            bar.close = std::stod(fields[4]);
            bar.volume = std::stoll(fields[5]);
        }
        catch (...) { return false; }
        return !bar.date.empty() && bar.open > 0 && bar.close > 0;
    }

    double averageClose(const std::vector<DailyBar>& bars, int endIndex, int window)
    {
        if (window <= 0 || endIndex + 1 < window) return -1.0;
        double sum = 0.0;
        for (int i = endIndex - window + 1; i <= endIndex; ++i)
            sum += bars[static_cast<std::size_t>(i)].close;
        return sum / window;
    }

    enum class Signal { HOLD, BUY, SELL };

    Signal signalFor(const std::vector<DailyBar>& bars, int i,
                     const std::string& strategy, const BacktestConfig& cfg,
                     bool holding)
    {
        if (strategy == "BUY_LOW_SELL_HIGH")
        {
            const double sma = averageClose(bars, i, cfg.shortWindow);
            if (sma <= 0) return Signal::HOLD;
            if (!holding && bars[static_cast<std::size_t>(i)].close <= sma * (1.0 - cfg.bandPct))
                return Signal::BUY;
            if (holding && bars[static_cast<std::size_t>(i)].close >= sma * (1.0 + cfg.bandPct))
                return Signal::SELL;
            return Signal::HOLD;
        }

        if (strategy == "MA_CROSSOVER")
        {
            if (i < 1) return Signal::HOLD;
            const double shortNow = averageClose(bars, i, cfg.shortWindow);
            const double longNow = averageClose(bars, i, cfg.longWindow);
            const double shortPrev = averageClose(bars, i - 1, cfg.shortWindow);
            const double longPrev = averageClose(bars, i - 1, cfg.longWindow);
            if (shortNow < 0 || longNow < 0 || shortPrev < 0 || longPrev < 0)
                return Signal::HOLD;
            if (!holding && shortPrev <= longPrev && shortNow > longNow)
                return Signal::BUY;
            if (holding && shortPrev >= longPrev && shortNow < longNow)
                return Signal::SELL;
        }
        return Signal::HOLD;
    }

    struct TradeState
    {
        int quantity = 0;
        double entryCostIncludingFee = 0.0;
    };

    void writeEquityHeader(std::ofstream& out)
    {
        out << "Date,Cash,Quantity,Close,Equity\n";
    }
}

bool loadDailyBars(const std::string& path, std::vector<DailyBar>& bars,
                   std::string& error)
{
    std::ifstream input(path);
    if (!input)
    {
        error = "cannot open " + path;
        return false;
    }

    bars.clear();
    std::string line;
    std::getline(input, line); // header
    while (std::getline(input, line))
    {
        DailyBar bar;
        if (parseCsvRow(line, bar)) bars.push_back(bar);
    }
    if (bars.size() < 60)
    {
        error = "need at least 60 valid daily rows; found " + std::to_string(bars.size());
        return false;
    }
    std::sort(bars.begin(), bars.end(), [](const DailyBar& a, const DailyBar& b) {
        return a.date < b.date;
    });
    return true;
}

BacktestMetrics runBacktest(const std::string& symbol,
                            const std::vector<DailyBar>& bars,
                            const std::string& strategy,
                            const BacktestConfig& cfg,
                            const std::string& equityOutputPath)
{
    BacktestMetrics result;
    result.symbol = symbol;
    result.strategy = strategy;
    result.initialCash = cfg.initialCash;
    result.finalEquity = cfg.initialCash;
    if (bars.size() < 2 || cfg.initialCash <= 0) return result;

    std::ofstream equityOut(equityOutputPath);
    if (equityOut) writeEquityHeader(equityOut);

    double cash = cfg.initialCash;
    TradeState position;
    Signal pending = Signal::HOLD;
    if (strategy == "BUY_AND_HOLD")
    {
        const double fill = bars.front().open * (1.0 + cfg.slippageRate);
        const double unitCost = fill * (1.0 + cfg.feeRate);
        const int quantity = static_cast<int>(std::floor(cash / unitCost));
        if (quantity > 0)
        {
            const double gross = fill * quantity;
            const double fee = gross * cfg.feeRate;
            cash -= gross + fee;
            position.quantity = quantity;
            position.entryCostIncludingFee = gross + fee;
        }
    }
    std::vector<double> equityValues;
    std::vector<double> dailyReturns;
    double previousEquity = cfg.initialCash;
    double peakEquity = cfg.initialCash;
    double maxDrawdown = 0.0;
    int closedTrades = 0;
    int winningTrades = 0;

    for (std::size_t i = 0; i < bars.size(); ++i)
    {
        const DailyBar& bar = bars[i];

        // A signal formed after yesterday's close is executed at today's open.
        if (i > 0 && pending == Signal::BUY && position.quantity == 0)
        {
            const double fill = bar.open * (1.0 + cfg.slippageRate);
            const double unitCost = fill * (1.0 + cfg.feeRate);
            const int quantity = static_cast<int>(std::floor(cash / unitCost));
            if (quantity > 0)
            {
                const double gross = fill * quantity;
                const double fee = gross * cfg.feeRate;
                cash -= gross + fee;
                position.quantity = quantity;
                position.entryCostIncludingFee = gross + fee;
            }
        }
        else if (i > 0 && pending == Signal::SELL && position.quantity > 0)
        {
            const double fill = bar.open * (1.0 - cfg.slippageRate);
            const double gross = fill * position.quantity;
            const double fee = gross * cfg.feeRate;
            const double netProceeds = gross - fee;
            cash += netProceeds;
            ++closedTrades;
            if (netProceeds - position.entryCostIncludingFee > 0) ++winningTrades;
            position.quantity = 0;
            position.entryCostIncludingFee = 0.0;
        }

        double equity = cash + position.quantity * bar.close;
        // Liquidate any remaining position at the final close so reported PnL
        // is net of exit costs rather than an unrealized mark-to-market value.
        if (i + 1 == bars.size() && position.quantity > 0)
        {
            const double fill = bar.close * (1.0 - cfg.slippageRate);
            const double gross = fill * position.quantity;
            const double fee = gross * cfg.feeRate;
            const double netProceeds = gross - fee;
            cash += netProceeds;
            ++closedTrades;
            if (netProceeds - position.entryCostIncludingFee > 0) ++winningTrades;
            position.quantity = 0;
            position.entryCostIncludingFee = 0.0;
            equity = cash;
        }
        equityValues.push_back(equity);
        if (i>0 && previousEquity > 0)
            dailyReturns.push_back(equity / previousEquity - 1.0);
        previousEquity = equity;
        peakEquity = std::max(peakEquity, equity);
        if (peakEquity > 0)
            maxDrawdown = std::max(maxDrawdown, (peakEquity - equity) / peakEquity);

        if (equityOut)
            equityOut << bar.date << ',' << std::fixed << std::setprecision(2)
                      << cash << ',' << position.quantity << ',' << bar.close << ',' << equity << '\n';

        // The signal is computed using this day's close and can only fill tomorrow.
        pending = signalFor(bars, static_cast<int>(i), strategy, cfg, position.quantity > 0);
    }

    result.finalEquity = equityValues.empty() ? cfg.initialCash : equityValues.back();
    result.netPnl = result.finalEquity - cfg.initialCash;
    result.totalReturnPct = (result.finalEquity / cfg.initialCash - 1.0) * 100.0;
    const double years = std::max(1.0 / 252.0,
        static_cast<double>(bars.size() - 1) / 252.0);
    if (result.finalEquity > 0)
        result.annualizedReturnPct = (std::pow(result.finalEquity / cfg.initialCash, 1.0 / years) - 1.0) * 100.0;
    result.maxDrawdownPct = maxDrawdown * 100.0;
    result.closedTrades = closedTrades;
    result.winningTrades = winningTrades;
    result.winRatePct = closedTrades ? 100.0 * winningTrades / closedTrades : 0.0;

    if (dailyReturns.size() > 1)
    {
        double mean = 0.0;
        for (double value : dailyReturns) mean += value;
        mean /= dailyReturns.size();
        double variance = 0.0;
        for (double value : dailyReturns) variance += (value - mean) * (value - mean);
        variance /= (dailyReturns.size() - 1);
        const double standardDeviation = std::sqrt(variance);
        result.sharpeRatio = standardDeviation > 0
            ? (mean / standardDeviation) * std::sqrt(252.0) : 0.0;
    }
    return result;
}

bool writeMetricsCsv(const std::string& path,
                     const std::vector<BacktestMetrics>& metrics)
{
    std::ofstream out(path);
    if (!out) return false;
    out << "Symbol,Strategy,InitialCash,FinalEquity,NetPnL,TotalReturnPct,AnnualizedReturnPct,SharpeRatio,MaxDrawdownPct,ClosedTrades,WinningTrades,WinRatePct\n";
    out << std::fixed << std::setprecision(4);
    for (const BacktestMetrics& m : metrics)
        out << m.symbol << ',' << m.strategy << ',' << m.initialCash << ','
            << m.finalEquity << ',' << m.netPnl << ',' << m.totalReturnPct << ','
            << m.annualizedReturnPct << ',' << m.sharpeRatio << ','
            << m.maxDrawdownPct << ',' << m.closedTrades << ','
            << m.winningTrades << ',' << m.winRatePct << '\n';
    return true;
}
