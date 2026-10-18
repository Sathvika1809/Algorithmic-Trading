#include "../include/OrderList.h"
#include <iostream>
#include <string>

static int g_total = 0;
static int g_failed = 0;

#define CHECK(cond)                                                      \
    do                                                                   \
    {                                                                    \
        ++g_total;                                                       \
        if (!(cond))                                                     \
        {                                                                \
            ++g_failed;                                                  \
            std::cout << "FAIL (line " << __LINE__ << "): " #cond "\n";  \
        }                                                                \
    } while (0)

static Order makeOrder(int id)
{
    return Order(id, "BTC", 100.0 + id, 1, "BUY");
}

// Empties the list from the front and returns the ids in order, e.g. "1,2,4,"
static std::string drain(OrderList& list)
{
    std::string result;
    while (!list.empty())
    {
        result += std::to_string(list.front()->getOrderId()) + ",";
        list.popFront();
    }
    return result;
}

void testEmptyList()
{
    OrderList l;
    CHECK(l.empty());
    CHECK(l.size() == 0);
    CHECK(l.front() == nullptr);
    CHECK(l.find(1) == nullptr);
    CHECK(!l.remove(1));
    CHECK(!l.popFront());
    CHECK(!l.removeNode(nullptr));
}

void testInsertKeepsArrivalOrder()
{
    OrderList l;
    l.insert(makeOrder(1));
    l.insert(makeOrder(2));
    l.insert(makeOrder(3));
    CHECK(l.size() == 3);
    CHECK(l.front()->getOrderId() == 1); // first in, first out
    CHECK(drain(l) == "1,2,3,");
    CHECK(l.empty());
}

void testInsertReturnsHandle()
{
    OrderList l;
    OrderList::Node* h = l.insert(makeOrder(7));
    CHECK(h != nullptr);
    CHECK(h->order.getOrderId() == 7);
}

void testFind()
{
    OrderList l;
    l.insert(makeOrder(1));
    l.insert(makeOrder(2));
    Order* o = l.find(2);
    CHECK(o != nullptr && o->getOrderId() == 2);
    CHECK(o != nullptr && o->getPrice() == 102.0);
    CHECK(l.find(99) == nullptr);
}

void testRemoveHeadMiddleTail()
{
    OrderList l;
    for (int i = 1; i <= 5; ++i)
    {
        l.insert(makeOrder(i));
    }
    CHECK(l.remove(3)); // middle
    CHECK(l.remove(1)); // head
    CHECK(l.remove(5)); // tail
    CHECK(!l.remove(3)); // already gone
    CHECK(l.size() == 2);
    CHECK(drain(l) == "2,4,");
}

void testRemoveOnlyElement()
{
    OrderList l;
    l.insert(makeOrder(1));
    CHECK(l.remove(1));
    CHECK(l.empty());
    CHECK(l.front() == nullptr);
    // list must be fully reset and usable again
    l.insert(makeOrder(2));
    CHECK(l.size() == 1);
    CHECK(drain(l) == "2,");
}

void testRemoveNodeByHandle()
{
    OrderList l;
    OrderList::Node* h1 = l.insert(makeOrder(1));
    OrderList::Node* h2 = l.insert(makeOrder(2));
    OrderList::Node* h3 = l.insert(makeOrder(3));
    CHECK(l.removeNode(h2)); // middle
    CHECK(l.size() == 2);
    CHECK(l.find(2) == nullptr);
    CHECK(l.removeNode(h3)); // tail
    CHECK(l.removeNode(h1)); // last remaining (head and tail)
    CHECK(l.empty());
}

void testTailStaysCorrectAfterRemovingTail()
{
    OrderList l;
    l.insert(makeOrder(1));
    l.insert(makeOrder(2));
    l.insert(makeOrder(3));
    l.remove(3);
    l.insert(makeOrder(4)); // must attach after 2, not after the deleted node
    CHECK(drain(l) == "1,2,4,");
}

void testPopFront()
{
    OrderList l;
    l.insert(makeOrder(1));
    l.insert(makeOrder(2));
    CHECK(l.popFront());
    CHECK(l.front()->getOrderId() == 2);
    CHECK(l.popFront());
    CHECK(!l.popFront());
    CHECK(l.empty());
}

void testManyOrders()
{
    OrderList l;
    for (int i = 0; i < 10000; ++i)
    {
        l.insert(makeOrder(i));
    }
    CHECK(l.size() == 10000);
    CHECK(l.front()->getOrderId() == 0);
    int count = 0;
    while (l.popFront())
    {
        ++count;
    }
    CHECK(count == 10000);
    CHECK(l.empty());
}

void testDestructorFreesRemainingNodes()
{
    // Nothing to assert: run with AddressSanitizer to see leaks.
    OrderList l;
    for (int i = 0; i < 100; ++i)
    {
        l.insert(makeOrder(i));
    }
    CHECK(l.size() == 100);
}

int main()
{
    testEmptyList();
    testInsertKeepsArrivalOrder();
    testInsertReturnsHandle();
    testFind();
    testRemoveHeadMiddleTail();
    testRemoveOnlyElement();
    testRemoveNodeByHandle();
    testTailStaysCorrectAfterRemovingTail();
    testPopFront();
    testManyOrders();
    testDestructorFreesRemainingNodes();

    std::cout << (g_total - g_failed) << "/" << g_total << " checks passed\n";
    if (g_failed == 0)
    {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }
    return 1;
}