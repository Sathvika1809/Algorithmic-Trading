#ifndef ARBITRAGESTRATEGY_H
#define ARBITRAGESTRATEGY_H

#include <string>
#include "MarketData.h"

// A risk-free-looking profit: the same asset is quoted at two different
// prices on two venues, so you buy it where it is cheap and sell it where it
// is expensive.
//
// Venues are simply two market symbols, for example "BTC-A" and "BTC-B".
// Both are kept in MarketData like any other symbol.
struct ArbitrageOpportunity
{
    bool found;
    std::string asset;      // what we hold in the portfolio, e.g. "BTC"
    std::string buyVenue;   // market symbol where it is cheaper
    std::string sellVenue;  // market symbol where it is more expensive
    double buyPrice;
    double sellPrice;
    int quantity;
    double profitPerUnit;   // after fees on both legs
    double expectedProfit;  // profitPerUnit * quantity

    ArbitrageOpportunity()
        : found(false), asset(), buyVenue(), sellVenue(),
          buyPrice(0.0), sellPrice(0.0), quantity(0),
          profitPerUnit(0.0), expectedProfit(0.0)
    {
    }
};

// Looks at two venues and reports whether a profitable trade exists.
//
// This does not derive from TradingStrategy: that interface returns ONE
// signal for ONE symbol, but arbitrage is two linked trades. The strategy
// only DECIDES (like the other strategies). The TradingEngine executes it.
class ArbitrageStrategy
{
    private:
        std::string asset;
        std::string venueA;
        std::string venueB;
        double feeRate;   // fraction charged on each leg, e.g. 0.001 = 0.1%
        double minProfit; // smallest profit per unit (after fees) worth trading
        int quantity;     // units to trade when an opportunity is found

    public:
        // Invalid values are repaired: a negative fee or minimum profit
        // becomes 0, and a quantity below 1 becomes 1.
        ArbitrageStrategy(const std::string& asset,
                          const std::string& venueA,
                          const std::string& venueB,
                          double feeRate = 0.0,
                          double minProfitPerUnit = 0.0,
                          int quantity = 1);

        // Returns the best direction (buy A / sell B, or buy B / sell A) if
        // its profit per unit, after fees on both legs, is strictly greater
        // than the minimum profit. Otherwise found is false.
        // Also returns "not found" if either venue has no valid price yet.
        ArbitrageOpportunity detect(const MarketData& market) const;

        const std::string& getAsset() const;
        double getFeeRate() const;
        int getQuantity() const;
};

#endif