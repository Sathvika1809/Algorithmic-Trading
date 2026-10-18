#include "../include/OrderBook.h"
#include <iostream>

using namespace std;

OrderBook::OrderBook()
{
    //  Automatically the object calls its constructor
}

OrderBook::~OrderBook()
{
    
}

void OrderBook::addOrder(Order order)
{
    if(order.getType() == "BUY")
    {
        buyOrders.insert(order);
    }else if(order.getType() == "SELL")
    {
        sellOrders.insert(order);
    }
    orderMap.insert(order.getOrderId(),order);
}
bool OrderBook::removeOrder(int orderId)
{
    Order* order = orderMap.find(orderId);
    if(order == nullptr){
        return false;
    }
    string type = order->getType();
    orderMap.remove(orderId);
    if(type == "BUY"){
        buyOrders.remove(orderId);
    }
    else if(type == "SELL"){
        sellOrders.remove(orderId);
    }
    return true;
}
Order* OrderBook::findOrder(int orderId)
{
    return orderMap.find(orderId);
}
void OrderBook::displayOrders()
{
    cout << "\n===== BUY ORDERS =====\n";
    buyOrders.display();

    cout << "\n===== SELL ORDERS =====\n";
    sellOrders.display();
}