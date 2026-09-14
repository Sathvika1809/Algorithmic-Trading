#include "../include/Order.h"
#include <iostream>
using namespace std;
Order::Order()
{
    orderId = 0;
    symbol = "";
    price = 0.0;
    quantity = 0;
    type = "";
}
Order::Order(int id,const string& symbol,double price,int quantity,const string& type){
    orderId = id;
    this->symbol = symbol;
    this->price = price;
    this->quantity = quantity;
    this->type = type;
}

int Order::getOrderId() const
{
    return orderId;
}

string Order::getSymbol() const
{
    return symbol;
}

double Order::getPrice() const
{
    return price;
}

int Order::getQuantity() const
{
    return quantity;
}
string Order:: getType() const
{
    return type;
}
void Order::display() const
{
    cout<<"Order ID: "<<orderId<<'\n';
    cout<<"Symbol: "<<symbol<<'\n';
    cout<<"Price: "<<price<<'\n';
    cout<<"Quantity: "<<quantity<<'\n';
    cout<<"Type: "<<type<<'\n';
}