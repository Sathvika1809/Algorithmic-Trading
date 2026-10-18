
#include "include/MarketData.h"
#include "include/OrderBook.h"
#include "include/Order.h"
#include "include/Portfolio.h"
#include "include/BuyLowSellHigh.h"
#include "include/TradingEngine.h"
#include <iostream>


using namespace std;

int main()
{
    // =========================================
    // MARKET DATA
    // =========================================

    MarketData market;

    market.addPrice("AAPL", 180.5);
    market.addPrice("BTC", 63000);
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
    // =========================================
    // ORDER BOOK
    // =========================================

    OrderBook orderBook;

    Order order1(101, "BTC", 59500, 2, "BUY");
    Order order2(102, "BTC", 60500, 1, "SELL");
    Order order3(103, "ETH", 3900, 5, "BUY");
    Order order4(104, "ETH", 4100, 3, "SELL");

    orderBook.addOrder(order1);
    orderBook.addOrder(order2);
    orderBook.addOrder(order3);
    orderBook.addOrder(order4);
    // or u can write book.addOrder(Order(101,"BTC",60000,2,"BUY"));
    cout << "\n===== ORDER BOOK =====\n";

    orderBook.displayOrders();


    // =========================================
    // SEARCH ORDER
    // =========================================

    cout << "\n===== SEARCH ORDER =====\n";

    Order* foundOrder = orderBook.findOrder(102);

    if(foundOrder != nullptr)
    {
        cout << "Order found:\n";
        foundOrder->display();
    }
    else
    {
        cout << "Order not found\n";
    }


    // =========================================
    // REMOVE ORDER
    // =========================================

    cout << "\n===== REMOVE ORDER 102 =====\n";

    if(orderBook.removeOrder(102))
    {
        cout << "Order 102 removed successfully\n";
    }
    else
    {
        cout << "Order 102 not found\n";
    }


    // =========================================
    // DISPLAY AFTER REMOVAL
    // =========================================

    cout << "\n===== ORDER BOOK AFTER REMOVAL =====\n";

    orderBook.displayOrders();


    // =========================================
    // PORTFOLIO
    // =========================================

    cout << "\n===== PORTFOLIO =====\n";

    Portfolio portfolio(200000);

    // Buy BTC twice at different prices
    portfolio.buy("BTC", 60000, 1);
    portfolio.buy("BTC", 62000, 1);

    // Buy ETH
    portfolio.buy("ETH", 4000, 5);

    portfolio.display(market);


    // =========================================
    // PORTFOLIO QUANTITY
    // =========================================

    cout << "\n===== PORTFOLIO CHECK =====\n";

    cout << "BTC Quantity: "
         << portfolio.getQuantity("BTC") << endl;

    cout << "ETH Quantity: "
         << portfolio.getQuantity("ETH") << endl;

    cout << "Cash: "
         << portfolio.getCash() << endl;


    // =========================================
    // SELL TEST
    // =========================================

    cout << "\n===== SELL BTC =====\n";

    if(portfolio.sell("BTC", 65000, 1))
    {
        cout << "BTC sold successfully\n";
    }
    else
    {
        cout << "BTC sale failed\n";
    }

    portfolio.display(market);

    // =========================================
      // BUY LOW SELL HIGH STRATEGY
      // =========================================

    BuyLowSellHigh strategy("BTC",59000, 62000);

    strategy.generateSignal(market, portfolio);
    cout << "\n\n===== TESTING TRADING ENGINE =====\n";

    TradingEngine engine(200000);

    engine.setMarketPrice("BTC", 63000);
    engine.setMarketPrice("ETH", 4000);

    engine.executeBuy("BTC", 1);

    BuyLowSellHigh engineStrategy("ETH", 3500, 4500);

    engine.runStrategy(engineStrategy);

    engine.display();

    return 0;
}

