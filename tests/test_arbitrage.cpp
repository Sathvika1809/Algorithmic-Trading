#include "../include/ArbitrageStrategy.h"
#include "../include/TradingEngine.h"
#include <cmath>
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

static bool near(double a, double b)
{
    return std::fabs(a - b) < 1e-9;
}

// The engine prints status lines. This hides them while it is alive.
struct SilenceCout
{
    std::ostringstream sink;
    std::streambuf* old;
    SilenceCout() : sink(), old(std::cout.rdbuf(sink.rdbuf())) {}
    ~SilenceCout() { std::cout.rdbuf(old); }
};

// ------------------------- detection -------------------------

void testNothingWithoutBothPrices()
{
    MarketData m;
    ArbitrageStrategy s("BTC", "BTC-A", "BTC-B");
    CHECK(!s.detect(m).found); // no prices at all
    m.addPrice("BTC-A", 100.0);
    CHECK(!s.detect(m).found); // only one venue priced
}

void testNothingWhenPricesEqual()
{
    MarketData m;
    m.addPrice("BTC-A", 100.0);
    m.addPrice("BTC-B", 100.0);
    ArbitrageStrategy s("BTC", "BTC-A", "BTC-B");
    CHECK(!s.detect(m).found);
}

void testFindsOpportunityWhenAIsCheaper()
{
    MarketData m;
    m.addPrice("BTC-A", 100.0);
    m.addPrice("BTC-B", 102.0);
    ArbitrageStrategy s("BTC", "BTC-A", "BTC-B");
    ArbitrageOpportunity o = s.detect(m);
    CHECK(o.found);
    CHECK(o.asset == "BTC");
    CHECK(o.buyVenue == "BTC-A");
    CHECK(o.sellVenue == "BTC-B");
    CHECK(near(o.buyPrice, 100.0));
    CHECK(near(o.sellPrice, 102.0));
    CHECK(o.quantity == 1);
    CHECK(near(o.profitPerUnit, 2.0));
    CHECK(near(o.expectedProfit, 2.0));
}

void testFindsOpportunityWhenBIsCheaper()
{
    MarketData m;
    m.addPrice("BTC-A", 102.0);
    m.addPrice("BTC-B", 100.0);
    ArbitrageStrategy s("BTC", "BTC-A", "BTC-B");
    ArbitrageOpportunity o = s.detect(m);
    CHECK(o.found);
    CHECK(o.buyVenue == "BTC-B");
    CHECK(o.sellVenue == "BTC-A");
    CHECK(near(o.profitPerUnit, 2.0));
}

void testFeesCanRemoveTheOpportunity()
{
    MarketData m;
    ArbitrageStrategy s("BTC", "BTC-A", "BTC-B", 0.01); // 1% per leg

    // spread of 1: 101*0.99 - 100*1.01 = -1.01, not profitable
    m.addPrice("BTC-A", 100.0);
    m.addPrice("BTC-B", 101.0);
    CHECK(!s.detect(m).found);

    // spread of 3: 103*0.99 - 100*1.01 = 0.97, profitable
    m.addPrice("BTC-B", 103.0);
    ArbitrageOpportunity o = s.detect(m);
    CHECK(o.found);
    CHECK(near(o.profitPerUnit, 0.97));
}

void testMinimumProfitThreshold()
{
    MarketData m;
    m.addPrice("BTC-A", 100.0);
    m.addPrice("BTC-B", 102.0); // profit per unit 2.0

    ArbitrageStrategy exactly("BTC", "BTC-A", "BTC-B", 0.0, 2.0);
    CHECK(!exactly.detect(m).found); // must be strictly above the minimum

    ArbitrageStrategy lower("BTC", "BTC-A", "BTC-B", 0.0, 1.9);
    CHECK(lower.detect(m).found);
}

void testQuantityIsCarried()
{
    MarketData m;
    m.addPrice("BTC-A", 100.0);
    m.addPrice("BTC-B", 102.0);
    ArbitrageStrategy s("BTC", "BTC-A", "BTC-B", 0.0, 0.0, 5);
    ArbitrageOpportunity o = s.detect(m);
    CHECK(o.quantity == 5);
    CHECK(near(o.expectedProfit, 10.0));
    CHECK(s.getQuantity() == 5);
    CHECK(s.getAsset() == "BTC");
}

void testInvalidSettingsAreRepaired()
{
    MarketData m;
    m.addPrice("BTC-A", 100.0);
    m.addPrice("BTC-B", 102.0);
    // negative fee -> 0, zero quantity -> 1, negative minimum profit -> 0
    ArbitrageStrategy s("BTC", "BTC-A", "BTC-B", -0.5, -3.0, 0);
    CHECK(near(s.getFeeRate(), 0.0));
    CHECK(s.getQuantity() == 1);
    CHECK(near(s.detect(m).profitPerUnit, 2.0));
}

void testUsesLatestPrices()
{
    MarketData m;
    ArbitrageStrategy s("BTC", "BTC-A", "BTC-B");
    m.addPrice("BTC-A", 100.0);
    m.addPrice("BTC-B", 102.0);
    CHECK(s.detect(m).found);
    m.addPrice("BTC-B", 100.0); // gap closes
    CHECK(!s.detect(m).found);
}

// ------------------------- execution -------------------------

void testArbitrageExecutesBothLegsThroughTheBook()
{
    SilenceCout quiet;
    TradingEngine e(10000.0);
    e.setMarketPrice("BTC-A", 100.0);
    e.setMarketPrice("BTC-B", 102.0);
    ArbitrageStrategy s("BTC", "BTC-A", "BTC-B", 0.0, 0.0, 5);

    CHECK(e.runArbitrage(s));
    CHECK(near(e.getPortfolio().getCash(), 10010.0)); // 5 * (102 - 100)
    CHECK(e.getPortfolio().getQuantity("BTC") == 0);  // no inventory left

    const OrderBook& book = e.getOrderBook();
    CHECK(book.tradeCount() == 2);
    CHECK(book.trades()[0].symbol == "BTC-A"); // leg 1: buy cheap
    CHECK(near(book.trades()[0].price, 100.0));
    CHECK(book.trades()[1].symbol == "BTC-B"); // leg 2: sell dear
    CHECK(near(book.trades()[1].price, 102.0));
    CHECK(book.orderCount() == 0);
}

void testArbitrageInTheOtherDirection()
{
    SilenceCout quiet;
    TradingEngine e(10000.0);
    e.setMarketPrice("BTC-A", 102.0);
    e.setMarketPrice("BTC-B", 100.0);
    ArbitrageStrategy s("BTC", "BTC-A", "BTC-B", 0.0, 0.0, 5);

    CHECK(e.runArbitrage(s));
    CHECK(near(e.getPortfolio().getCash(), 10010.0));
    CHECK(e.getOrderBook().trades()[0].symbol == "BTC-B"); // buys the cheap one
    CHECK(e.getOrderBook().trades()[1].symbol == "BTC-A");
}

void testNoOpportunityMeansNoTrade()
{
    SilenceCout quiet;
    TradingEngine e(10000.0);
    e.setMarketPrice("BTC-A", 100.0);
    e.setMarketPrice("BTC-B", 100.0);
    ArbitrageStrategy s("BTC", "BTC-A", "BTC-B");

    CHECK(!e.runArbitrage(s));
    CHECK(near(e.getPortfolio().getCash(), 10000.0));
    CHECK(e.getOrderBook().tradeCount() == 0);
}

void testArbitrageWithFeesMatchesExpectedProfit()
{
    SilenceCout quiet;
    TradingEngine e(10000.0);
    CHECK(e.setFeeRate(0.01));
    e.setMarketPrice("BTC-A", 100.0);
    e.setMarketPrice("BTC-B", 103.0);
    ArbitrageStrategy s("BTC", "BTC-A", "BTC-B", 0.01);

    ArbitrageOpportunity o = s.detect(e.getMarket());
    CHECK(o.found);
    CHECK(e.runArbitrage(s));
    // buy costs 100 * 1.01 = 101, sell brings in 103 * 0.99 = 101.97
    CHECK(near(e.getPortfolio().getCash(), 10000.0 + o.expectedProfit));
    CHECK(near(e.getPortfolio().getCash(), 10000.97));
    CHECK(e.getPortfolio().getQuantity("BTC") == 0);
}

void testArbitrageFailsWhenLegOneIsUnaffordable()
{
    SilenceCout quiet;
    TradingEngine e(10.0); // far too little cash for 100 per unit
    e.setMarketPrice("BTC-A", 100.0);
    e.setMarketPrice("BTC-B", 102.0);
    ArbitrageStrategy s("BTC", "BTC-A", "BTC-B");

    CHECK(!e.runArbitrage(s));
    CHECK(near(e.getPortfolio().getCash(), 10.0));
    CHECK(e.getPortfolio().getQuantity("BTC") == 0);
    CHECK(e.getOrderBook().tradeCount() == 0);
}

void testRepeatedArbitrageAccumulatesProfit()
{
    SilenceCout quiet;
    TradingEngine e(10000.0);
    e.setMarketPrice("BTC-A", 100.0);
    e.setMarketPrice("BTC-B", 102.0);
    ArbitrageStrategy s("BTC", "BTC-A", "BTC-B", 0.0, 0.0, 5);

    bool allOk = true;
    for (int i = 0; i < 50; ++i)
    {
        if (!e.runArbitrage(s)) allOk = false;
        if (e.getOrderBook().orderCount() != 0) allOk = false;
    }
    CHECK(allOk);
    CHECK(near(e.getPortfolio().getCash(), 10000.0 + 50 * 10.0));
    CHECK(e.getPortfolio().getQuantity("BTC") == 0);
    CHECK(e.getOrderBook().tradeCount() == 100);
}

void testExecuteArbitrageRejectsNotFound()
{
    SilenceCout quiet;
    TradingEngine e(10000.0);
    ArbitrageOpportunity none; // found == false
    CHECK(!e.executeArbitrage(none));
    CHECK(e.getOrderBook().tradeCount() == 0);
}

// ---------------------------- fees ----------------------------

void testSetFeeRateValidation()
{
    TradingEngine e(1000.0);
    CHECK(near(e.getFeeRate(), 0.0)); // default
    CHECK(!e.setFeeRate(-0.1));
    CHECK(!e.setFeeRate(1.0));
    CHECK(!e.setFeeRate(2.0));
    CHECK(near(e.getFeeRate(), 0.0)); // unchanged by rejected values
    CHECK(e.setFeeRate(0.01));
    CHECK(near(e.getFeeRate(), 0.01));
    CHECK(e.setFeeRate(0.0));
}

void testFeeOnPlainBuyAndSell()
{
    SilenceCout quiet;
    TradingEngine e(10000.0);
    e.setFeeRate(0.01);
    e.setMarketPrice("BTC", 100.0);

    CHECK(e.executeBuy("BTC", 10));
    CHECK(near(e.getPortfolio().getCash(), 10000.0 - 1010.0)); // 10 * 100 * 1.01
    // the cost basis includes the fee
    CHECK(near(e.getPortfolio().getAveragePrice("BTC"), 101.0));

    CHECK(e.executeSell("BTC", 5));
    CHECK(near(e.getPortfolio().getCash(), 8990.0 + 495.0)); // 5 * 100 * 0.99
}

void testFeeIsIncludedInAffordabilityCheck()
{
    SilenceCout quiet;
    TradingEngine e(100.5); // enough for 100 but not for 100 + 1% fee
    e.setFeeRate(0.01);
    e.setMarketPrice("BTC", 100.0);
    CHECK(!e.executeBuy("BTC", 1));
    CHECK(near(e.getPortfolio().getCash(), 100.5));
    CHECK(e.getOrderBook().tradeCount() == 0);

    TradingEngine f(100.5); // same cash, no fee: affordable
    f.setMarketPrice("BTC", 100.0);
    CHECK(f.executeBuy("BTC", 1));
}

int main()
{
    testNothingWithoutBothPrices();
    testNothingWhenPricesEqual();
    testFindsOpportunityWhenAIsCheaper();
    testFindsOpportunityWhenBIsCheaper();
    testFeesCanRemoveTheOpportunity();
    testMinimumProfitThreshold();
    testQuantityIsCarried();
    testInvalidSettingsAreRepaired();
    testUsesLatestPrices();

    testArbitrageExecutesBothLegsThroughTheBook();
    testArbitrageInTheOtherDirection();
    testNoOpportunityMeansNoTrade();
    testArbitrageWithFeesMatchesExpectedProfit();
    testArbitrageFailsWhenLegOneIsUnaffordable();
    testRepeatedArbitrageAccumulatesProfit();
    testExecuteArbitrageRejectsNotFound();

    testSetFeeRateValidation();
    testFeeOnPlainBuyAndSell();
    testFeeIsIncludedInAffordabilityCheck();

    std::cout << (g_total - g_failed) << "/" << g_total << " checks passed\n";
    if (g_failed == 0)
    {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }
    return 1;
}