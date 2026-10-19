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

OrderList::Node* OrderList::insertSorted(const Order& order, bool descendingPrice)
{
    // The list is sorted, best price first. "At least as good" nodes (better
    // or equal price than the new order) form a prefix of the list, and the
    // new order goes right after the last of them. That keeps equal prices in
    // arrival order (price-time priority).
    //
    // We look for the end of that prefix from both sides at once:
    //   - from the tail, going backwards: the first node that is at least as
    //     good IS the end of the prefix;
    //   - from the head, going forwards: the first node that is strictly
    //     worse marks the start of the rest, so its predecessor is the end.
    // Whichever side finishes first decides. A new order with the worst price
    // finishes at once from the tail, and one near the best price (the common
    // case) finishes quickly from the head.
    const double newPrice = order.getPrice();

    Node* back = tail_;
    Node* front = head_;
    Node* after = nullptr; // the node the new order goes after (nullptr = first)

    for (;;)
    {
        // tail side
        if (back == nullptr)
        {
            after = nullptr; // every node is strictly worse: new order goes first
            break;
        }
        double backPrice = back->order.getPrice();
        bool backAtLeastAsGood = descendingPrice ? (backPrice >= newPrice)
                                                 : (backPrice <= newPrice);
        if (backAtLeastAsGood)
        {
            after = back;
            break;
        }
        back = back->prev;

        // head side
        if (front == nullptr)
        {
            after = tail_; // walked past the end: every node is at least as good
            break;
        }
        double frontPrice = front->order.getPrice();
        bool frontAtLeastAsGood = descendingPrice ? (frontPrice >= newPrice)
                                                  : (frontPrice <= newPrice);
        if (!frontAtLeastAsGood)
        {
            after = front->prev; // first strictly worse node: go right before it
            break;
        }
        front = front->next;
    }

    Node* newNode = new Node(order);
    if (after == nullptr)
    {
        // new order has the best price: it becomes the head
        newNode->next = head_;
        if (head_ != nullptr)
        {
            head_->prev = newNode;
        }
        else
        {
            tail_ = newNode;
        }
        head_ = newNode;
    }
    else
    {
        newNode->prev = after;
        newNode->next = after->next;
        if (after->next != nullptr)
        {
            after->next->prev = newNode;
        }
        else
        {
            tail_ = newNode;
        }
        after->next = newNode;
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