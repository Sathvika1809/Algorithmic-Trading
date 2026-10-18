#include "../include/TradingEngine.h"
#include <iostream>
using namespace std;
TradingEngine::TradingEngine(double initialCash)
    : portfolio(initialCash), nextOrderId(1001)
{
}

void TradingEngine::setMarketPrice(
    const std::string& symbol,
    double price)
{
    market.addPrice(symbol, price);
}

bool TradingEngine::executeBuy(
    const std::string& symbol,
    int quantity)
{
    double price = market.getPrice(symbol);

    if(price == -1)
    {
        cout << "Trading Engine: price not found\n";
        return false;
    }

    bool success = portfolio.buy(symbol, price, quantity);

    if(success)
    {
        cout << "Trading Engine: BUY executed for "
             << symbol << endl;
    }
    else
    {
        cout << "Trading Engine: BUY failed for "
             << symbol << endl;
    }

    return success;
}

bool TradingEngine::executeSell(
    const std::string& symbol,
    int quantity)
{
    double price = market.getPrice(symbol);

    if(price == -1)
    {
        cout << "Trading Engine: price not found\n";
        return false;
    }

    bool success = portfolio.sell(symbol, price, quantity);

    if(success)
    {
        cout << "Trading Engine: SELL executed for "
             << symbol << endl;
    }
    else
    {
        cout << "Trading Engine: SELL failed for "
             << symbol << endl;
    }

    return success;
}

void TradingEngine::runStrategy(TradingStrategy& strategy)
{
    Signal signal = strategy.generateSignal(market, portfolio);

    if(signal.action == Action::BUY)
    {
        cout << "Trading Engine: BUY signal received\n";
        executeBuy(strategy.getSymbol(), signal.quantity);
    }
    else if(signal.action == Action::SELL)
    {
        cout << "Trading Engine: SELL signal received\n";
        executeSell(strategy.getSymbol(), signal.quantity);
    }
    else
    {
        cout << "Trading Engine: HOLD signal received\n";
    }
}

void TradingEngine::display()
{
    cout << "\n===== TRADING ENGINE =====\n";

    cout << "\n--- MARKET DATA ---\n";
    market.display();

    cout << "\n--- ORDER BOOK ---\n";
    orderBook.displayOrders();

    cout << "\n--- PORTFOLIO ---\n";
    portfolio.display(market);
}
