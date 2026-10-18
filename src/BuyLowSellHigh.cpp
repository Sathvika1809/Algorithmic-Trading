#include "../include/BuyLowSellHigh.h"
#include <iostream>

using namespace std;

BuyLowSellHigh::BuyLowSellHigh(
    const string& symbol,
    double buyPrice,
    double sellPrice)
{
    this->symbol = symbol;
    buyThreshold = buyPrice;
    sellThreshold = sellPrice;
}

Signal BuyLowSellHigh::generateSignal(
    MarketData& market,
    Portfolio& /*portfolio*/)
{
    double curPrice = market.getPrice(symbol);

    if(curPrice == -1)
    {
        cout << symbol << " price not found\n";
        return Signal::HOLD;
    }

    cout << "\n===== BUY LOW SELL HIGH STRATEGY =====\n";

    cout << symbol << " Price: "
         << curPrice << endl;

    if(curPrice <= buyThreshold)
    {
        cout << "Signal: BUY " << symbol << '\n';
        return Signal::BUY;
    }
    else if(curPrice >= sellThreshold)
    {
        cout << "Signal: SELL " << symbol << '\n';
        return Signal::SELL;
    }
    else
    {
        cout << "Signal: HOLD " << symbol << '\n';
        return Signal::HOLD;
    }
}
    std::string BuyLowSellHigh::getSymbol()
    {
        return symbol;
    }