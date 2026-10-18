// Tests for the Order class.

#include "../include/Order.h"
#include <iostream>
#include <sstream>
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
            std::cerr << "FAIL (line " << __LINE__ << "): " #cond "\n";  \
        }                                                                \
    } while (0)

// Captures everything printed to cout while it is alive.
struct CaptureCout
{
    std::ostringstream sink;
    std::streambuf* old;
    CaptureCout() : sink(), old(std::cout.rdbuf(sink.rdbuf())) {}
    ~CaptureCout() { std::cout.rdbuf(old); }
    std::string text() const { return sink.str(); }
};

void testDefaultConstructor()
{
    Order o;
    CHECK(o.getOrderId() == 0);
    CHECK(o.getSymbol() == "");
    CHECK(o.getPrice() == 0.0);
    CHECK(o.getQuantity() == 0);
    CHECK(o.getType() == "");
}

void testParameterizedConstructor()
{
    Order o(101, "BTC", 59500.5, 2, "BUY");
    CHECK(o.getOrderId() == 101);
    CHECK(o.getSymbol() == "BTC");
    CHECK(o.getPrice() == 59500.5);
    CHECK(o.getQuantity() == 2);
    CHECK(o.getType() == "BUY");
}

void testSetQuantity()
{
    Order o(1, "ETH", 4000.0, 10, "SELL");
    o.setQuantity(4);
    CHECK(o.getQuantity() == 4);
    o.setQuantity(0);
    CHECK(o.getQuantity() == 0);
    // nothing else changes
    CHECK(o.getOrderId() == 1);
    CHECK(o.getSymbol() == "ETH");
    CHECK(o.getPrice() == 4000.0);
    CHECK(o.getType() == "SELL");
}

void testCopyIsIndependent()
{
    Order a(5, "BTC", 100.0, 3, "BUY");
    Order b = a;
    b.setQuantity(1);
    CHECK(a.getQuantity() == 3);
    CHECK(b.getQuantity() == 1);
    CHECK(b.getOrderId() == 5);
}

void testDisplayOutput()
{
    Order o(7, "BTC", 100.5, 3, "BUY");

    CaptureCout capture;
    o.display();
    std::string out = capture.text();

    std::string expected =
        "Order ID: 7\n"
        "Symbol: BTC\n"
        "Price: 100.5\n"
        "Quantity: 3\n"
        "Type: BUY\n";
    CHECK(out == expected);
}

int main()
{
    testDefaultConstructor();
    testParameterizedConstructor();
    testSetQuantity();
    testCopyIsIndependent();
    testDisplayOutput();

    std::cout << (g_total - g_failed) << "/" << g_total << " checks passed\n";
    if (g_failed == 0)
    {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }
    return 1;
}
