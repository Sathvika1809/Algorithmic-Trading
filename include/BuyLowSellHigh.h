#ifndef BUYLOWSLLHIGH_H
#define BUYLOWSLLHIGH_H

#include "TradingStrategy.h"
#include <string>
class BuyLowSellHigh : public TradingStrategy
{
    private:
        string symbol;
        double buyThreshold;
        double sellThreshold;

    public:

        BuyLowSellHigh(const string& symbol,double buyPrice, double sellPrice);

        Signal generateSignal(
            MarketData& market,
            Portfolio& portfolio
        ) override;
        std::string getSymbol() override;
};

#endif