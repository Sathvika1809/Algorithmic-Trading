#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include "OrderList.h"
#include "CustomUnorderedMap.h"

class OrderBook
{
    private:
        OrderList buyOrders;
        OrderList sellOrders;

        CustomUnorderedMap orderMap;
    public:
        OrderBook();
        void addOrder(Order order);
        bool removeOrder(int orderId);
        Order* findOrder(int orderId);
        void displayOrders();
};
#endif