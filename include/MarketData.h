#ifndef MARKETDATA_H
#define MARKETDATA_H

#include <string>
#include <vector>
using namespace std;

class MarketData
{
    private:
        struct PriceData
        {
            string symbol;
            double price;
        };
        vector<PriceData>data;
    public: 
        MarketData();
        void addPrice(const string& symbol, double price);
        double getPrice(const string& symbol);
        void display();
};
#endif