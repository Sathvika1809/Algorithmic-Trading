#ifndef TRADINGENGINE_H
#define TRADINGENGINE_H

#include <string>
#include "MarketData.h"
#include "OrderBook.h"
#include "Portfolio.h"
#include "TradingStrategy.h"
#include "ArbitrageStrategy.h"

// Runs strategies and executes their trades through the order book.
//
// Every trade goes through the matching engine in OrderBook:
//
//   1. A simulated market maker posts liquidity at the current market price
//      on the opposite side (a SELL when we buy, a BUY when we sell).
//   2. Our own order is submitted to the book as an immediate-or-cancel
//      limit order. It matches by price-time priority like any other order.
//   3. The real fills are read back from the book's trade log, anything
//      left over is cancelled, and the portfolio is updated with exactly
//      what was filled.
//
// The limit price decides whether a trade happens:
//   BUY  fills only if limit >= market price (and pays the market price)
//   SELL fills only if limit <= market price (and receives the market price)
// A limit price of 0 means "use the market price".
//
// Fees: setFeeRate(f) charges a fraction f on every fill. A buy costs
// price * (1 + f), a sell brings in price * (1 - f). The default is 0.
//
// The engine needs the order book's trade log, so do not turn it off.
class TradingEngine
{
    private:
        MarketData market;
        OrderBook orderBook;
        Portfolio portfolio;

        int nextOrderId;        // ids for our own orders
        int nextMarketMakerId;  // ids for the simulated market maker
        double feeRate_;        // fraction charged on each fill

        // marketSymbol: which price and which order book to use (a venue).
        // assetSymbol:  what the portfolio buys or sells.
        // For normal trades both are the same symbol.
        bool execute(const std::string& marketSymbol,
                     const std::string& assetSymbol,
                     bool isBuy,
                     int quantity,
                     double limitPrice);

    public:
        TradingEngine(double initialCash);

        void setMarketPrice(
            const std::string& symbol,
            double price
        );

        // Fee as a fraction (0.001 = 0.1%). Must be from 0 up to (not
        // including) 1. Returns false and changes nothing otherwise.
        bool setFeeRate(double rate);
        double getFeeRate() const;

        void runStrategy(TradingStrategy& strategy);

        // limitPrice is optional; 0 means "market price".
        bool executeBuy(
            const std::string& symbol,
            int quantity,
            double limitPrice = 0.0
        );

        bool executeSell(
            const std::string& symbol,
            int quantity,
            double limitPrice = 0.0
        );

        // Asks the strategy for an opportunity and trades it if one exists.
        // Returns true only if both legs were executed.
        bool runArbitrage(const ArbitrageStrategy& strategy);

        // Leg 1: buy on the cheap venue. Leg 2: sell on the expensive venue.
        // Returns true only if both legs filled. If leg 1 fills and leg 2
        // does not, the portfolio keeps the asset (this is "legging risk").
        bool executeArbitrage(const ArbitrageOpportunity& opportunity);

        // Read-only views, mainly for tests and reporting.
        const MarketData& getMarket() const;
        const OrderBook& getOrderBook() const;
        const Portfolio& getPortfolio() const;

        void display() const;
};

#endif