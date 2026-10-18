#ifndef TRADINGSTRATEGY_H
#define TRADINGSTRATEGY_H

#include "../include/MarketData.h"
#include "../include/Portfolio.h"
#include <string>
enum class Signal
{
    BUY,
    SELL,
    HOLD
};

class TradingStrategy
{
    public:
        virtual Signal generateSignal(
            MarketData& market,
            Portfolio& portfolio
        ) = 0;
        virtual std::string getSymbol() = 0;
        virtual ~TradingStrategy();
};

#endif