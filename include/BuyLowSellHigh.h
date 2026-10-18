#ifndef BUYLOWSELLHIGH_H
#define BUYLOWSELLHIGH_H

#include <string>
#include "TradingStrategy.h"

class BuyLowSellHigh : public TradingStrategy
{
    private:
        std::string symbol;
        double buyThreshold;
        double sellThreshold;
        int quantity;   // units to trade on every signal

    public:
        // quantity is optional (default 1) so old code keeps working.
        BuyLowSellHigh(const std::string& symbol,
                       double buyPrice,
                       double sellPrice,
                       int quantity = 1);

        Signal generateSignal(
            const MarketData& market,
            const Portfolio& portfolio
        ) override;

        std::string getSymbol() const override;
};

#endif
