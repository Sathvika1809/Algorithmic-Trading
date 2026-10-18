// Tests for MarketData and the BuyLowSellHigh strategy.

#include "../include/MarketData.h"
#include "../include/Portfolio.h"
#include "../include/BuyLowSellHigh.h"
#include <iostream>
#include <sstream>
#include <string>

static int g_total = 0;
static int g_failed = 0;

// Failures go to cerr so they stay visible while cout is silenced below.
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

// The strategy prints to cout. This hides that output while it is alive.
struct SilenceCout
{
    std::ostringstream sink;
    std::streambuf* old;
    SilenceCout() : sink(), old(std::cout.rdbuf(sink.rdbuf())) {}
    ~SilenceCout() { std::cout.rdbuf(old); }
};

// ---------- MarketData ----------

void testMarketDataAddAndGet()
{
    MarketData m;
    m.addPrice("BTC", 60000.0);
    m.addPrice("ETH", 4000.0);
    CHECK(m.getPrice("BTC") == 60000.0);
    CHECK(m.getPrice("ETH") == 4000.0);
    CHECK(m.getPrice("DOGE") == -1); // missing symbol returns -1
}

void testMarketDataUpdatesExistingSymbol()
{
    MarketData m;
    m.addPrice("BTC", 60000.0);
    m.addPrice("BTC", 61000.0); // same symbol: update, not a duplicate
    CHECK(m.getPrice("BTC") == 61000.0);
}

// ---------- BuyLowSellHigh ----------

void testSignalsAroundThresholds()
{
    MarketData m;
    Portfolio p(100000.0);
    BuyLowSellHigh s("BTC", 59000.0, 62000.0);
    SilenceCout quiet;

    m.addPrice("BTC", 58000.0);
    CHECK(s.generateSignal(m, p) == Signal::BUY); // below buy threshold

    m.addPrice("BTC", 59000.0);
    CHECK(s.generateSignal(m, p) == Signal::BUY); // exactly at buy threshold

    m.addPrice("BTC", 60000.0);
    CHECK(s.generateSignal(m, p) == Signal::HOLD); // in between

    m.addPrice("BTC", 62000.0);
    CHECK(s.generateSignal(m, p) == Signal::SELL); // exactly at sell threshold

    m.addPrice("BTC", 70000.0);
    CHECK(s.generateSignal(m, p) == Signal::SELL); // above sell threshold
}

void testUnknownSymbolHolds()
{
    MarketData m;
    m.addPrice("BTC", 50000.0);
    Portfolio p(100000.0);
    BuyLowSellHigh s("XYZ", 10.0, 20.0);
    SilenceCout quiet;
    CHECK(s.generateSignal(m, p) == Signal::HOLD);
}

void testGetSymbol()
{
    BuyLowSellHigh s("ETH", 3500.0, 4500.0);
    CHECK(s.getSymbol() == "ETH");
}

void testWorksThroughBasePointer()
{
    MarketData m;
    m.addPrice("ETH", 3000.0);
    Portfolio p(100000.0);
    BuyLowSellHigh concrete("ETH", 3500.0, 4500.0);
    TradingStrategy* base = &concrete;
    SilenceCout quiet;
    CHECK(base->generateSignal(m, p) == Signal::BUY);
    CHECK(base->getSymbol() == "ETH");
}

int main()
{
    testMarketDataAddAndGet();
    testMarketDataUpdatesExistingSymbol();
    testSignalsAroundThresholds();
    testUnknownSymbolHolds();
    testGetSymbol();
    testWorksThroughBasePointer();

    std::cout << (g_total - g_failed) << "/" << g_total << " checks passed\n";
    if (g_failed == 0)
    {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }
    return 1;
}