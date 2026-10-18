#include "../include/OrderList.h"
#include <iostream>

using namespace std;

OrderList::Node::Node(const Order& o)
    : order(o), prev(nullptr), next(nullptr)
{
}

OrderList::OrderList()
    : head_(nullptr), tail_(nullptr), size_(0)
{
}

OrderList::~OrderList()
{
    Node* cur = head_;
    while (cur != nullptr)
    {
        Node* nextNode = cur->next;
        delete cur;
        cur = nextNode;
    }
}

OrderList::Node* OrderList::insert(const Order& order)
{
    Node* newNode = new Node(order);
    if (tail_ == nullptr)
    {
        // list was empty
        head_ = newNode;
        tail_ = newNode;
    }
    else
    {
        newNode->prev = tail_;
        tail_->next = newNode;
        tail_ = newNode;
    }
    ++size_;
    return newNode;
}

Order* OrderList::find(int orderId)
{
    for (Node* cur = head_; cur != nullptr; cur = cur->next)
    {
        if (cur->order.getOrderId() == orderId)
        {
            return &(cur->order);
        }
    }
    return nullptr;
}

bool OrderList::removeNode(Node* node)
{
    if (node == nullptr)
    {
        return false;
    }

    // Reconnect the neighbours around the node.
    if (node->prev != nullptr)
    {
        node->prev->next = node->next;
    }
    else
    {
        head_ = node->next; // node was the head
    }

    if (node->next != nullptr)
    {
        node->next->prev = node->prev;
    }
    else
    {
        tail_ = node->prev; // node was the tail
    }

    delete node;
    --size_;
    return true;
}

bool OrderList::remove(int orderId)
{
    for (Node* cur = head_; cur != nullptr; cur = cur->next)
    {
        if (cur->order.getOrderId() == orderId)
        {
            return removeNode(cur);
        }
    }
    return false;
}

Order* OrderList::front()
{
    return (head_ != nullptr) ? &(head_->order) : nullptr;
}

bool OrderList::popFront()
{
    if (head_ == nullptr)
    {
        return false;
    }
    return removeNode(head_);
}

size_t OrderList::size() const
{
    return size_;
}

bool OrderList::empty() const
{
    return size_ == 0;
}

void OrderList::display() const
{
    for (const Node* cur = head_; cur != nullptr; cur = cur->next)
    {
        cur->order.display();
        cout << "-----------" << endl;
    }
}