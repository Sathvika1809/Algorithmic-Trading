#include "../include/MarketData.h"
#include <cmath>
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

static bool near(double a, double b)
{
    return std::fabs(a - b) < 1e-9;
}

// Captures everything printed to cout while it is alive.
struct CaptureCout
{
    std::ostringstream sink;
    std::streambuf* old;
    CaptureCout() : sink(), old(std::cout.rdbuf(sink.rdbuf())) {}
    ~CaptureCout() { std::cout.rdbuf(old); }
    std::string text() const { return sink.str(); }
};

void testAddAndGet()
{
    MarketData m;
    m.addPrice("BTC", 60000.0);
    m.addPrice("ETH", 4000.0);
    CHECK(near(m.getPrice("BTC"), 60000.0));
    CHECK(near(m.getPrice("ETH"), 4000.0));
    CHECK(m.getPrice("DOGE") == -1); // unknown symbol
}

void testHasPrice()
{
    MarketData m;
    CHECK(!m.hasPrice("BTC"));
    m.addPrice("BTC", 1.0);
    CHECK(m.hasPrice("BTC"));
    CHECK(!m.hasPrice("ETH"));
}

void testUpdateKeepsLatestAndBuildsHistory()
{
    MarketData m;
    m.addPrice("BTC", 100.0);
    m.addPrice("BTC", 110.0);
    m.addPrice("BTC", 105.0);
    CHECK(near(m.getPrice("BTC"), 105.0)); // latest wins
    CHECK(m.historySize("BTC") == 3);
    CHECK(m.historySize("ETH") == 0); // unknown symbol
}

void testSymbolsAreIndependent()
{
    MarketData m;
    m.addPrice("BTC", 100.0);
    m.addPrice("ETH", 10.0);
    m.addPrice("BTC", 120.0);
    CHECK(m.historySize("BTC") == 2);
    CHECK(m.historySize("ETH") == 1);
    CHECK(near(m.getPrice("ETH"), 10.0));
}

void testMovingAverage()
{
    MarketData m;
    m.addPrice("BTC", 10.0);
    m.addPrice("BTC", 20.0);
    m.addPrice("BTC", 30.0);
    m.addPrice("BTC", 40.0);
    CHECK(near(m.getMovingAverage("BTC", 1), 40.0));
    CHECK(near(m.getMovingAverage("BTC", 2), 35.0)); // (30+40)/2
    CHECK(near(m.getMovingAverage("BTC", 4), 25.0)); // (10+20+30+40)/4
    CHECK(m.getMovingAverage("BTC", 5) == -1);       // not enough history
    CHECK(m.getMovingAverage("BTC", 0) == -1);       // window of 0
    CHECK(m.getMovingAverage("ETH", 2) == -1);       // unknown symbol
}

void testMovingAverageUsesOnlyRecentWindow()
{
    MarketData m;
    for (int i = 1; i <= 100; ++i)
    {
        m.addPrice("BTC", static_cast<double>(i));
    }
    // last 3 prices are 98, 99, 100
    CHECK(near(m.getMovingAverage("BTC", 3), 99.0));
}

void testManySymbols()
{
    MarketData m;
    for (int i = 0; i < 500; ++i)
    {
        m.addPrice("SYM" + std::to_string(i), i * 1.5);
    }
    bool allOk = true;
    for (int i = 0; i < 500; ++i)
    {
        if (!near(m.getPrice("SYM" + std::to_string(i)), i * 1.5))
        {
            allOk = false;
        }
    }
    CHECK(allOk);
    CHECK(m.getPrice("SYM500") == -1);
}

void testDisplayKeepsInsertionOrderWithoutDuplicates()
{
    MarketData m;
    m.addPrice("A", 1.0);
    m.addPrice("B", 2.0);
    m.addPrice("A", 3.0); // update, must not create a second "A" entry

    CaptureCout capture;
    m.display();
    std::string out = capture.text();

    std::string expected =
        "Symbol: A\nPrice: 3\n-----------\n"
        "Symbol: B\nPrice: 2\n-----------\n";
    CHECK(out == expected);
}

void testCopyIsIndependent()
{
    MarketData a;
    a.addPrice("X", 1.0);
    MarketData b = a;
    b.addPrice("X", 2.0);
    b.addPrice("Y", 5.0);
    CHECK(near(a.getPrice("X"), 1.0));
    CHECK(a.historySize("X") == 1);
    CHECK(!a.hasPrice("Y"));
    CHECK(near(b.getPrice("X"), 2.0));
}

void testConstAccess()
{
    MarketData a;
    a.addPrice("X", 7.0);
    const MarketData& c = a;
    CHECK(near(c.getPrice("X"), 7.0));
    CHECK(c.hasPrice("X"));
    CHECK(c.historySize("X") == 1);
    CHECK(c.getMovingAverage("X", 1) == 7.0);
}

int main()
{
    testAddAndGet();
    testHasPrice();
    testUpdateKeepsLatestAndBuildsHistory();
    testSymbolsAreIndependent();
    testMovingAverage();
    testMovingAverageUsesOnlyRecentWindow();
    testManySymbols();
    testDisplayKeepsInsertionOrderWithoutDuplicates();
    testCopyIsIndependent();
    testConstAccess();

    std::cout << (g_total - g_failed) << "/" << g_total << " checks passed\n";
    if (g_failed == 0)
    {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }
    return 1;
}