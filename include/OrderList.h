// Doubly linked list of orders, oldest order at the front.
//
//  - insert at tail            : O(1)   (tail pointer, no walking the list)
//  - removeNode(handle)        : O(1)   (prev/next pointers, no searching)
//  - remove(orderId), find(id) : O(n)   (must search for the order first)
//  - front / popFront          : O(1)   (oldest order = first in line)
//
// The list owns its nodes and frees them in the destructor.
// It cannot be copied (copying an order list would silently duplicate orders).

#ifndef ORDERLIST_H
#define ORDERLIST_H

#include <cstddef>
#include "Order.h"

class OrderList
{
    public:
        struct Node
        {
            Order order;
            Node* prev;
            Node* next;
            explicit Node(const Order& o);
        };

    private:
        Node* head_;
        Node* tail_;
        std::size_t size_;

    public:
        OrderList();
        ~OrderList();

        OrderList(const OrderList&) = delete;
        OrderList& operator=(const OrderList&) = delete;

        // Adds the order at the back. Returns a handle to its node, which can
        // later be passed to removeNode() for an O(1) removal.
        Node* insert(const Order& order);

        // Returns a pointer to the order with this id, or nullptr.
        Order* find(int orderId);

        // Removes the order with this id. Returns false if it is not in the list.
        bool remove(int orderId);

        // Removes the node behind a handle returned by insert().
        // The handle must belong to THIS list and must not already be removed.
        bool removeNode(Node* node);

        // Oldest order in the list, or nullptr if empty.
        Order* front();

        // Removes the oldest order. Returns false if the list is empty.
        bool popFront();

        std::size_t size() const;
        bool empty() const;

        void display() const;
};

#endif