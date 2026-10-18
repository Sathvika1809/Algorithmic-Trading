#ifndef MARKETDATA_H
#define MARKETDATA_H

#include <cstddef>
#include <string>
#include <vector>
#include "CustomUnorderedMap.h"
using namespace std;

// Latest price and full price history for each symbol.
//
// Symbols are stored in our own CustomUnorderedMap, so addPrice and getPrice
// are O(1) on average (the old vector needed a linear search, O(n)).
// A separate list remembers the order symbols were first added, so display()
// prints them in a stable, predictable order.
class MarketData
{
    private:
        struct SymbolData
        {
            double lastPrice;
            vector<double> history; // oldest first, newest last
        };

        CustomUnorderedMap<string, SymbolData> table;
        vector<string> symbols; // insertion order, used by display()

    public:
        MarketData();

        // Records a new price. Creates the symbol if it is new, otherwise
        // updates its latest price and appends to its history.
        void addPrice(const string& symbol, double price);

        // Latest price, or -1 if the symbol is unknown.
        double getPrice(const string& symbol) const;

        bool hasPrice(const string& symbol) const;

        // Number of prices recorded for the symbol (0 if unknown).
        size_t historySize(const string& symbol) const;

        // Average of the last `window` prices. Returns -1 if the symbol is
        // unknown, window is 0, or fewer than `window` prices exist yet.
        double getMovingAverage(const string& symbol, size_t window) const;

        void display() const;
};
#endif