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
    CHECK(s.generateSignal(m, p).action == Action::BUY); // below buy threshold

    m.addPrice("BTC", 59000.0);
    CHECK(s.generateSignal(m, p).action == Action::BUY); // exactly at buy threshold

    m.addPrice("BTC", 60000.0);
    CHECK(s.generateSignal(m, p).action == Action::HOLD); // in between

    m.addPrice("BTC", 62000.0);
    CHECK(s.generateSignal(m, p).action == Action::SELL); // exactly at sell threshold

    m.addPrice("BTC", 70000.0);
    CHECK(s.generateSignal(m, p).action == Action::SELL); // above sell threshold
}

void testSignalQuantity()
{
    MarketData m;
    Portfolio p(100000.0);
    SilenceCout quiet;

    // default quantity is 1
    BuyLowSellHigh defaultQty("BTC", 59000.0, 62000.0);
    m.addPrice("BTC", 58000.0);
    CHECK(defaultQty.generateSignal(m, p).quantity == 1);

    // custom quantity is carried on BUY and SELL signals
    BuyLowSellHigh five("BTC", 59000.0, 62000.0, 5);
    CHECK(five.generateSignal(m, p).quantity == 5);
    m.addPrice("BTC", 70000.0);
    CHECK(five.generateSignal(m, p).quantity == 5);

    // HOLD carries quantity 0
    m.addPrice("BTC", 60000.0);
    Signal hold = five.generateSignal(m, p);
    CHECK(hold.action == Action::HOLD);
    CHECK(hold.quantity == 0);

    // invalid quantity falls back to 1
    BuyLowSellHigh bad("BTC", 59000.0, 62000.0, -3);
    m.addPrice("BTC", 50000.0);
    CHECK(bad.generateSignal(m, p).quantity == 1);
}

void testUnknownSymbolHolds()
{
    MarketData m;
    m.addPrice("BTC", 50000.0);
    Portfolio p(100000.0);
    BuyLowSellHigh s("XYZ", 10.0, 20.0);
    SilenceCout quiet;
    Signal sig = s.generateSignal(m, p);
    CHECK(sig.action == Action::HOLD);
    CHECK(sig.quantity == 0);
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
    CHECK(base->generateSignal(m, p).action == Action::BUY);
    CHECK(base->getSymbol() == "ETH");
}

int main()
{
    testMarketDataAddAndGet();
    testMarketDataUpdatesExistingSymbol();
    testSignalsAroundThresholds();
    testSignalQuantity();
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
