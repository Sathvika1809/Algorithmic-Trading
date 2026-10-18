
#ifndef TRADINGENGINE_H
#define TRADINGENGINE_H

#include "MarketData.h"
#include "OrderBook.h"
#include "Portfolio.h"
#include "TradingStrategy.h"

class TradingEngine
{
    private:
        MarketData market;
        OrderBook orderBook;
        Portfolio portfolio;

        int nextOrderId;

    public:
        TradingEngine(double initialCash);

        void setMarketPrice(
            const std::string& symbol,
            double price
        );

        void runStrategy(TradingStrategy& strategy);

        bool executeBuy(
            const std::string& symbol,
            int quantity
        );

        bool executeSell(
            const std::string& symbol,
            int quantity
        );

        void display();
};

#endif