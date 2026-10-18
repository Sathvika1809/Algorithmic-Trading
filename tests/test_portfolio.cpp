#include "../include/Portfolio.h"
#include "../include/MarketData.h"
#include <cmath>
#include <iostream>

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

void testInitialState()
{
    Portfolio p(1000.0);
    CHECK(near(p.getCash(), 1000.0));
    CHECK(p.getQuantity("BTC") == 0);
    CHECK(near(p.getAveragePrice("BTC"), 0.0));
}

void testBuyReducesCash()
{
    Portfolio p(1000.0);
    CHECK(p.buy("BTC", 100.0, 3));
    CHECK(near(p.getCash(), 700.0));
    CHECK(p.getQuantity("BTC") == 3);
    CHECK(near(p.getAveragePrice("BTC"), 100.0));
}

void testBuyRejectsInvalidInput()
{
    Portfolio p(1000.0);
    CHECK(!p.buy("BTC", 0.0, 1));
    CHECK(!p.buy("BTC", -5.0, 1));
    CHECK(!p.buy("BTC", 100.0, 0));
    CHECK(!p.buy("BTC", 100.0, -2));
    CHECK(near(p.getCash(), 1000.0));
    CHECK(p.getQuantity("BTC") == 0);
}

void testBuyRejectsInsufficientCash()
{
    Portfolio p(1000.0);
    CHECK(!p.buy("BTC", 600.0, 2)); // costs 1200
    CHECK(near(p.getCash(), 1000.0)); // cash untouched
    CHECK(p.getQuantity("BTC") == 0);
    CHECK(p.buy("BTC", 500.0, 2)); // exactly all the cash is allowed
    CHECK(near(p.getCash(), 0.0));
}

void testAveragePriceOnRepeatedBuys()
{
    Portfolio p(500000.0);
    CHECK(p.buy("BTC", 60000.0, 1));
    CHECK(p.buy("BTC", 62000.0, 1));
    CHECK(p.getQuantity("BTC") == 2);
    CHECK(near(p.getAveragePrice("BTC"), 61000.0));

    // weighted: 2 @ 61000 plus 2 @ 63000 -> 62000
    CHECK(p.buy("BTC", 63000.0, 2));
    CHECK(p.getQuantity("BTC") == 4);
    CHECK(near(p.getAveragePrice("BTC"), 62000.0));
}

void testSell()
{
    Portfolio p(1000.0);
    p.buy("BTC", 100.0, 5); // cash 500
    CHECK(p.sell("BTC", 120.0, 2)); // +240
    CHECK(near(p.getCash(), 740.0));
    CHECK(p.getQuantity("BTC") == 3);
    // average buy price does not change when selling
    CHECK(near(p.getAveragePrice("BTC"), 100.0));
}

void testSellRejections()
{
    Portfolio p(1000.0);
    p.buy("BTC", 100.0, 2);
    CHECK(!p.sell("BTC", 100.0, 3)); // more than owned
    CHECK(!p.sell("ETH", 100.0, 1)); // never owned
    CHECK(!p.sell("BTC", 0.0, 1)); // bad price
    CHECK(!p.sell("BTC", 100.0, 0)); // bad quantity
    CHECK(p.getQuantity("BTC") == 2);
    CHECK(near(p.getCash(), 800.0));
}

void testSellEverythingRemovesHolding()
{
    Portfolio p(1000.0);
    p.buy("BTC", 100.0, 2);
    CHECK(p.sell("BTC", 150.0, 2));
    CHECK(p.getQuantity("BTC") == 0);
    CHECK(near(p.getAveragePrice("BTC"), 0.0));
    CHECK(near(p.getCash(), 1100.0));
    // buying again after a full exit starts a fresh holding
    CHECK(p.buy("BTC", 200.0, 1));
    CHECK(near(p.getAveragePrice("BTC"), 200.0));
}

void testMultipleSymbolsAreIndependent()
{
    Portfolio p(10000.0);
    p.buy("BTC", 1000.0, 2);
    p.buy("ETH", 100.0, 5);
    CHECK(p.getQuantity("BTC") == 2);
    CHECK(p.getQuantity("ETH") == 5);
    p.sell("BTC", 1000.0, 2); // removes BTC, must not disturb ETH
    CHECK(p.getQuantity("BTC") == 0);
    CHECK(p.getQuantity("ETH") == 5);
    CHECK(near(p.getAveragePrice("ETH"), 100.0));
}

void testPortfolioValue()
{
    MarketData m;
    m.addPrice("BTC", 70000.0);
    m.addPrice("ETH", 4000.0);

    Portfolio p(100000.0);
    p.buy("BTC", 60000.0, 1); // cash 40000
    p.buy("ETH", 4000.0, 5); // cash 20000
    // 20000 cash + 70000 BTC + 20000 ETH
    CHECK(near(p.getPortfolioValue(m), 110000.0));

    // a held symbol with no market price adds nothing instead of breaking
    Portfolio q(1000.0);
    q.buy("DOGE", 1.0, 10); // cash 990
    CHECK(near(q.getPortfolioValue(m), 990.0));
}

int main()
{
    testInitialState();
    testBuyReducesCash();
    testBuyRejectsInvalidInput();
    testBuyRejectsInsufficientCash();
    testAveragePriceOnRepeatedBuys();
    testSell();
    testSellRejections();
    testSellEverythingRemovesHolding();
    testMultipleSymbolsAreIndependent();
    testPortfolioValue();

    std::cout << (g_total - g_failed) << "/" << g_total << " checks passed\n";
    if (g_failed == 0)
    {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }
    return 1;
}