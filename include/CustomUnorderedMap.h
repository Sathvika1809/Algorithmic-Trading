#ifndef CUSTOMERUNORDEREDMAP_H
#define CUSTOMERUNORDEREDMAP_H

#include "Order.h"

class CustomUnorderedMap
{
    private:
        struct Node{
            int key;
            Order value;
            Node* next;
            Node(int key,Order value);
        };
        Node** buckets;
        int capacity;

        int hashFunction(int key)const;
    public:
        CustomUnorderedMap(int size = 10);
        void insert(int key,Order value);
        Order* find(int key);
        void display();
        bool remove(int key);
        ~CustomUnorderedMap();
};
#endif