
#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include <string>
#include <vector>
#include "MarketData.h"
using namespace std;

class Portfolio
{
    private:

        struct Holding
        {
            string symbol;
            int quantity;
            double averagePrice;
        };

        double cash;
        vector<Holding> holdings;

    public:

        Portfolio(double initialCash);

        bool buy(const string& symbol, double price, int quantity);

        bool sell(const string& symbol, double price, int quantity);

        int getQuantity(const string& symbol);

        double getCash() const;

        double getPortfolioValue(MarketData& market);

        void display(MarketData& market);
};

#endif

