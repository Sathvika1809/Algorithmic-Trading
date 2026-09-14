#include "include/MarketData.h"
#include <iostream>

using namespace std;

int main()
{
    MarketData market;

    market.addPrice("AAPL", 180.5);
    market.addPrice("BTC", 60000);
    market.addPrice("ETH", 4000);

    cout << "===== MARKET DATA =====\n";

    market.display();

    cout << "\n===== SEARCH PRICE =====\n";

    double price = market.getPrice("BTC");

    if (price != -1)
    {
        cout << "BTC Price: " << price << endl;
    }
    else
    {
        cout << "Symbol not found\n";
    }

    return 0;
}