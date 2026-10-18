#include "../include/CustomUnorderedMap.h"
#include <cstddef>
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

// Forces every key into the same bucket so we can test long chains.
struct ConstantHash
{
    std::size_t operator()(int) const { return 0; }
};

void testBasicInsertFind()
{
    CustomUnorderedMap<int, int> m;
    CHECK(m.empty());
    CHECK(m.insert(1, 10));
    CHECK(m.insert(2, 20));
    CHECK(m.size() == 2);
    CHECK(m.find(1) != nullptr && *m.find(1) == 10);
    CHECK(m.find(2) != nullptr && *m.find(2) == 20);
    CHECK(m.find(3) == nullptr);
}

void testUpdateExistingKey()
{
    CustomUnorderedMap<int, int> m;
    CHECK(m.insert(5, 50));
    CHECK(!m.insert(5, 99)); // update, not a new key
    CHECK(m.size() == 1);
    CHECK(*m.find(5) == 99);
}

void testRemove()
{
    CustomUnorderedMap<int, int> m;
    m.insert(1, 1);
    m.insert(2, 2);
    CHECK(m.remove(1));
    CHECK(!m.remove(1)); // already gone
    CHECK(!m.remove(42)); // never existed
    CHECK(m.size() == 1);
    CHECK(m.find(1) == nullptr);
    CHECK(m.find(2) != nullptr);
}

void testNegativeAndZeroKeys()
{
    CustomUnorderedMap<int, int> m;
    m.insert(-5, 1);
    m.insert(-17, 2);
    m.insert(0, 3);
    CHECK(*m.find(-5) == 1);
    CHECK(*m.find(-17) == 2);
    CHECK(*m.find(0) == 3);
}

void testResize()
{
    CustomUnorderedMap<int, int> m(4);
    for (int i = 0; i < 1000; ++i)
    {
        m.insert(i, i * 2);
    }
    CHECK(m.size() == 1000);
    CHECK(m.bucketCount() > 4);
    CHECK(m.loadFactor() <= 0.75);
    bool allOk = true;
    for (int i = 0; i < 1000; ++i)
    {
        int* v = m.find(i);
        if (v == nullptr || *v != i * 2)
        {
            allOk = false;
        }
    }
    CHECK(allOk);
}

void testPointerStableAcrossRehash()
{
    CustomUnorderedMap<int, int> m(2);
    m.insert(1, 10);
    int* p = m.find(1);
    for (int i = 100; i < 300; ++i)
    {
        m.insert(i, i);
    }
    CHECK(m.find(1) == p);
    CHECK(*p == 10);
}

void testCollisionChain()
{
    CustomUnorderedMap<int, int, ConstantHash> m;
    for (int i = 1; i <= 5; ++i)
    {
        m.insert(i, i * 10);
    }
    // chain (head -> tail) is 5,4,3,2,1
    CHECK(m.remove(3)); // middle
    CHECK(m.remove(5)); // head
    CHECK(m.remove(1)); // tail
    CHECK(m.size() == 2);
    CHECK(m.find(2) != nullptr && *m.find(2) == 20);
    CHECK(m.find(4) != nullptr && *m.find(4) == 40);
    CHECK(m.find(3) == nullptr);
    CHECK(m.find(5) == nullptr);
    CHECK(m.find(1) == nullptr);
}

void testStringKeys()
{
    CustomUnorderedMap<std::string, double> prices;
    prices.insert("BTC", 60000.0);
    prices.insert("ETH", 4000.0);
    CHECK(prices.find("BTC") != nullptr && *prices.find("BTC") == 60000.0);
    CHECK(prices.find("DOGE") == nullptr);
    CHECK(prices.remove("ETH"));
    CHECK(prices.size() == 1);
}

void testCopyConstructorIsDeep()
{
    CustomUnorderedMap<int, int> a;
    for (int i = 1; i <= 20; ++i)
    {
        a.insert(i, i);
    }
    CustomUnorderedMap<int, int> b(a);
    b.insert(1, 999);
    b.remove(2);
    CHECK(*a.find(1) == 1);
    CHECK(a.find(2) != nullptr);
    CHECK(a.size() == 20);
    CHECK(b.size() == 19);
}

void testAssignment()
{
    CustomUnorderedMap<int, int> a;
    for (int i = 1; i <= 10; ++i)
    {
        a.insert(i, i);
    }
    CustomUnorderedMap<int, int> c;
    c.insert(77, 77);
    c = a;
    CHECK(c.size() == 10);
    CHECK(c.find(77) == nullptr);
    CHECK(*c.find(5) == 5);
    c.remove(5);
    CHECK(a.find(5) != nullptr); // a unaffected

    // self-assignment must not break anything
    CustomUnorderedMap<int, int>& ref = c;
    c = ref;
    CHECK(c.size() == 9);
    CHECK(c.find(1) != nullptr);
}

void testConstFind()
{
    CustomUnorderedMap<int, int> a;
    a.insert(1, 11);
    const CustomUnorderedMap<int, int>& cr = a;
    CHECK(cr.find(1) != nullptr && *cr.find(1) == 11);
    CHECK(cr.find(2) == nullptr);
}

void testZeroCapacityRequested()
{
    CustomUnorderedMap<int, int> z(0);
    CHECK(z.insert(1, 1));
    CHECK(z.insert(2, 2));
    CHECK(*z.find(2) == 2);
}

int main()
{
    testBasicInsertFind();
    testUpdateExistingKey();
    testRemove();
    testNegativeAndZeroKeys();
    testResize();
    testPointerStableAcrossRehash();
    testCollisionChain();
    testStringKeys();
    testCopyConstructorIsDeep();
    testAssignment();
    testConstFind();
    testZeroCapacityRequested();

    std::cout << (g_total - g_failed) << "/" << g_total << " checks passed\n";
    if (g_failed == 0)
    {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }
    return 1;
}