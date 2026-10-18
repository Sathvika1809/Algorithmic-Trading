#include "../include/BuyLowSellHigh.h"
#include <iostream>

using namespace std;

BuyLowSellHigh::BuyLowSellHigh(
    const string& symbol,
    double buyPrice,
    double sellPrice,
    int quantity)
    : symbol(symbol),
      buyThreshold(buyPrice),
      sellThreshold(sellPrice),
      quantity(quantity > 0 ? quantity : 1)   // never trade 0 or negative units
{
}

Signal BuyLowSellHigh::generateSignal(
    const MarketData& market,
    const Portfolio& /*portfolio*/)
{
    double curPrice = market.getPrice(symbol);

    if(curPrice == -1)
    {
        cout << symbol << " price not found\n";
        return Signal(Action::HOLD, 0, 0.0);
    }

    cout << "\n===== BUY LOW SELL HIGH STRATEGY =====\n";

    cout << symbol << " Price: "
         << curPrice << endl;

    if(curPrice <= buyThreshold)
    {
        cout << "Signal: BUY " << symbol << '\n';
        return Signal(Action::BUY, quantity, 0.0);
    }
    else if(curPrice >= sellThreshold)
    {
        cout << "Signal: SELL " << symbol << '\n';
        return Signal(Action::SELL, quantity, 0.0);
    }
    else
    {
        cout << "Signal: HOLD " << symbol << '\n';
        return Signal(Action::HOLD, 0, 0.0);
    }
}

string BuyLowSellHigh::getSymbol() const
{
    return symbol;
}
