#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include "Order.h"
#include "OrderList.h"
#include "CustomUnorderedMap.h"

class OrderBook
{
    private:
        OrderList buyOrders;
        OrderList sellOrders;

        CustomUnorderedMap<int, Order> orderMap;
    public:
        OrderBook();
        ~OrderBook();
        void addOrder(Order order);
        bool removeOrder(int orderId);
        Order* findOrder(int orderId);
        void displayOrders();
};
#endif