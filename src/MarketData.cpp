#include "../include/MarketData.h"
#include <iostream>
using namespace std;
MarketData::MarketData()
{
    //vector is automatically initialized as empty
}

void MarketData::addPrice(const string& symbol,double price)
{
    PriceData newData;
    newData.symbol = symbol;
    newData.price = price;
    data.push_back(newData);
}

double MarketData::getPrice(const string& symbol)
{
    for(int i=0;i<data.size();i++){
        if(data[i].symbol == symbol)
        {
            return data[i].price;
        }
    }
    return -1;
}
void MarketData::display()
{
    for (int i = 0; i < data.size(); i++)
    {
        cout << "Symbol: " << data[i].symbol << '\n';
        cout << "Price: " << data[i].price << '\n';
        cout << "-----------\n";
    }
}