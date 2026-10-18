#include "../include/Portfolio.h"
#include <iostream>

using namespace std;

Portfolio::Portfolio(double initialCash)
{
    cash = initialCash;
}

bool Portfolio::buy(const string& symbol, double price, int quantity)
{
    // Check for invalid quantity or price
    if(price <= 0 || quantity <= 0)
    {
        return false;
    }

    double totalCost = price * quantity;

    // Check if we have enough cash
    if(totalCost > cash)
    {
        return false;
    }

    // Check if we already own this asset
    for(size_t i = 0; i < holdings.size(); i++)
    {
        if(holdings[i].symbol == symbol)
        {
            // Calculate new average price
            double oldValue =
                holdings[i].quantity * holdings[i].averagePrice;

            double newValue = quantity * price;

            int newQuantity = holdings[i].quantity + quantity;

            holdings[i].averagePrice =
                (oldValue + newValue) / newQuantity;

            holdings[i].quantity = newQuantity;

            cash -= totalCost;

            return true;
        }
    }

    // Asset does not exist yet
    Holding newHolding;

    newHolding.symbol = symbol;
    newHolding.quantity = quantity;
    newHolding.averagePrice = price;

    holdings.push_back(newHolding);

    cash -= totalCost;

    return true;
}

bool Portfolio::sell(const string& symbol, double price, int quantity)
{
    // Check for invalid quantity or price
    if(price <= 0 || quantity <= 0)
    {
        return false;
    }

    // Find the asset
    for(size_t i = 0; i < holdings.size(); i++)
    {
        if(holdings[i].symbol == symbol)
        {
            // Check whether we own enough
            if(quantity > holdings[i].quantity)
            {
                return false;
            }

            double totalValue = price * quantity;

            holdings[i].quantity -= quantity;

            cash += totalValue;

            // Remove holding if quantity becomes zero
            if(holdings[i].quantity == 0)
            {
                holdings.erase(holdings.begin() + i);
            }

            return true;
        }
    }

    // Asset not found
    return false;
}

int Portfolio::getQuantity(const string& symbol) const
{
    for(size_t i = 0; i < holdings.size(); i++)
    {
        if(holdings[i].symbol == symbol)
        {
            return holdings[i].quantity;
        }
    }

    return 0;
}

double Portfolio::getAveragePrice(const string& symbol) const
{
    for(size_t i = 0; i < holdings.size(); i++)
    {
        if(holdings[i].symbol == symbol)
        {
            return holdings[i].averagePrice;
        }
    }

    return 0.0;
}

double Portfolio::getCash() const
{
    return cash;
}

double Portfolio::getPortfolioValue(const MarketData& market) const
{
    double totalValue = cash;

    for(size_t i = 0; i < holdings.size(); i++)
    {
        double cur = market.getPrice(holdings[i].symbol);
        if(cur != -1){
            totalValue += holdings[i].quantity * cur;
        }
    }

    return totalValue;
}

void Portfolio::display(const MarketData& market) const
{
    cout << "\n===== PORTFOLIO =====\n";

    cout << "Cash: " << cash << endl;

    if(holdings.empty())
    {
        cout << "No holdings\n";
    }
    else
    {
        for(size_t i = 0; i < holdings.size(); i++)
        {
            cout << "Symbol: "
                 << holdings[i].symbol << endl;

            cout << "Quantity: "
                 << holdings[i].quantity << endl;

            cout << "Average Price: "
                 << holdings[i].averagePrice << endl;

            cout << "-----------\n";
        }
    }

    cout << "Portfolio Value: "
         << getPortfolioValue(market) << endl;
}
