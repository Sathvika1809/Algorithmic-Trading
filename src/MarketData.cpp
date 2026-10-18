#include "../include/MarketData.h"
#include <iostream>
using namespace std;

MarketData::MarketData()
{
    // table and symbols start empty
}

void MarketData::addPrice(const string& symbol, double price)
{
    SymbolData* existing = table.find(symbol);
    if (existing != nullptr)
    {
        existing->lastPrice = price;
        existing->history.push_back(price);
        return;
    }

    SymbolData fresh;
    fresh.lastPrice = price;
    fresh.history.push_back(price);
    table.insert(symbol, fresh);
    symbols.push_back(symbol);
}

double MarketData::getPrice(const string& symbol) const
{
    const SymbolData* data = table.find(symbol);
    if (data == nullptr)
    {
        return -1;
    }
    return data->lastPrice;
}

bool MarketData::hasPrice(const string& symbol) const
{
    return table.find(symbol) != nullptr;
}

size_t MarketData::historySize(const string& symbol) const
{
    const SymbolData* data = table.find(symbol);
    if (data == nullptr)
    {
        return 0;
    }
    return data->history.size();
}

double MarketData::getMovingAverage(const string& symbol, size_t window) const
{
    const SymbolData* data = table.find(symbol);
    if (data == nullptr || window == 0 || data->history.size() < window)
    {
        return -1;
    }

    double sum = 0.0;
    for (size_t i = data->history.size() - window; i < data->history.size(); ++i)
    {
        sum += data->history[i];
    }
    return sum / static_cast<double>(window);
}

void MarketData::display() const
{
    for (size_t i = 0; i < symbols.size(); i++)
    {
        cout << "Symbol: " << symbols[i] << '\n';
        cout << "Price: " << getPrice(symbols[i]) << '\n';
        cout << "-----------\n";
    }
}