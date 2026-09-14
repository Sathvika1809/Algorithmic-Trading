// Custom Linked list

#ifndef ORDERLIST_H
#define ORDERLIST_H

#include "Order.h"

class OrderList
{
    private:
        struct Node
        {
            Order order;
            Node* next;
            Node(Order o);
        };

        Node* head;

    public:
        
        OrderList();

        void insert(Order order);

        void display();
        Order* find(int orderId);
        bool remove(int orderId);
};
        
#endif