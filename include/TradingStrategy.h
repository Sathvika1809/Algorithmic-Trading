#ifndef TRADINGSTRATEGY_H
#define TRADINGSTRATEGY_H

#include <string>
#include "MarketData.h"
#include "Portfolio.h"

// What a strategy wants to do.
enum class Action
{
    BUY,
    SELL,
    HOLD
};

// What a strategy returns: the action, how many units, and a limit price.
// limitPrice == 0 means "no limit price, use the current market price".
// For HOLD, quantity is 0.
struct Signal
{
    Action action;
    int quantity;
    double limitPrice;

    Signal(Action a = Action::HOLD, int q = 0, double p = 0.0)
        : action(a), quantity(q), limitPrice(p)
    {
    }
};

class TradingStrategy
{
    public:
        // market and portfolio are read-only: a strategy only DECIDES.
        // The engine is the one that changes the portfolio.
        virtual Signal generateSignal(
            const MarketData& market,
            const Portfolio& portfolio
        ) = 0;
        virtual std::string getSymbol() const = 0;
        virtual ~TradingStrategy();
};

#endif
