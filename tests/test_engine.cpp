#include "../include/TradingEngine.h"
#include "../include/BuyLowSellHigh.h"
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
    std::string text() const { return sink.str(); }
};

void testInitialState()
{
    TradingEngine e(10000.0);
    CHECK(near(e.getPortfolio().getCash(), 10000.0));
    CHECK(e.getPortfolio().getQuantity("BTC") == 0);
    CHECK(e.getOrderBook().orderCount() == 0);
    CHECK(e.getOrderBook().tradeCount() == 0);
}

void testBuyGoesThroughOrderBook()
{
    SilenceCout quiet;
    TradingEngine e(10000.0);
    e.setMarketPrice("BTC", 100.0);

    CHECK(e.executeBuy("BTC", 3));
    CHECK(near(e.getPortfolio().getCash(), 9700.0));
    CHECK(e.getPortfolio().getQuantity("BTC") == 3);
    CHECK(near(e.getPortfolio().getAveragePrice("BTC"), 100.0));

    // The trade really happened inside the order book:
    const OrderBook& book = e.getOrderBook();
    CHECK(book.tradeCount() == 1);
    CHECK(book.trades().size() == 1);
    CHECK(book.trades()[0].symbol == "BTC");
    CHECK(book.trades()[0].quantity == 3);
    CHECK(near(book.trades()[0].price, 100.0));
    CHECK(book.totalTradedQuantity() == 3);
    // ... and nothing is left resting afterwards
    CHECK(book.orderCount() == 0);
}

void testSellGoesThroughOrderBook()
{
    SilenceCout quiet;
    TradingEngine e(10000.0);
    e.setMarketPrice("BTC", 100.0);
    e.executeBuy("BTC", 3);

    e.setMarketPrice("BTC", 110.0);
    CHECK(e.executeSell("BTC", 2));
    CHECK(near(e.getPortfolio().getCash(), 9700.0 + 220.0));
    CHECK(e.getPortfolio().getQuantity("BTC") == 1);
    // average buy price is unchanged by selling
    CHECK(near(e.getPortfolio().getAveragePrice("BTC"), 100.0));

    const OrderBook& book = e.getOrderBook();
    CHECK(book.tradeCount() == 2);
    CHECK(near(book.trades()[1].price, 110.0));
    CHECK(book.trades()[1].quantity == 2);
    CHECK(book.orderCount() == 0);
}

void testRejectedTradesLeaveNoTrace()
{
    SilenceCout quiet;
    TradingEngine e(1000.0);
    e.setMarketPrice("BTC", 100.0);

    CHECK(!e.executeBuy("DOGE", 1));    // no price for this symbol
    CHECK(!e.executeBuy("BTC", 0));     // zero quantity
    CHECK(!e.executeBuy("BTC", -2));    // negative quantity
    CHECK(!e.executeBuy("BTC", 11));    // costs 1100, only 1000 cash
    CHECK(!e.executeSell("BTC", 1));    // nothing owned
    CHECK(!e.executeSell("DOGE", 1));   // unknown symbol

    CHECK(near(e.getPortfolio().getCash(), 1000.0));
    CHECK(e.getPortfolio().getQuantity("BTC") == 0);
    CHECK(e.getOrderBook().tradeCount() == 0);   // book never touched
    CHECK(e.getOrderBook().orderCount() == 0);
}

void testSellMoreThanOwnedFails()
{
    SilenceCout quiet;
    TradingEngine e(10000.0);
    e.setMarketPrice("BTC", 100.0);
    e.executeBuy("BTC", 2);
    CHECK(!e.executeSell("BTC", 3));
    CHECK(e.getPortfolio().getQuantity("BTC") == 2);
    CHECK(e.getOrderBook().tradeCount() == 1); // only the original buy
}

void testBuyUsesAllCashExactly()
{
    SilenceCout quiet;
    TradingEngine e(1000.0);
    e.setMarketPrice("BTC", 100.0);
    CHECK(e.executeBuy("BTC", 10)); // exactly all the cash
    CHECK(near(e.getPortfolio().getCash(), 0.0));
}

void testBuyLimitPrice()
{
    SilenceCout quiet;
    TradingEngine e(10000.0);
    e.setMarketPrice("BTC", 100.0);

    // limit below the market price: nobody sells that cheap, no trade
    CHECK(!e.executeBuy("BTC", 1, 99.0));
    CHECK(near(e.getPortfolio().getCash(), 10000.0));
    CHECK(e.getOrderBook().tradeCount() == 0);
    CHECK(e.getOrderBook().orderCount() == 0); // unfilled orders were cancelled

    // limit equal to the market price: fills
    CHECK(e.executeBuy("BTC", 1, 100.0));
    CHECK(near(e.getPortfolio().getCash(), 9900.0));

    // limit above the market price: still pays only the market price
    CHECK(e.executeBuy("BTC", 1, 150.0));
    CHECK(near(e.getPortfolio().getCash(), 9800.0));
    CHECK(near(e.getOrderBook().trades()[1].price, 100.0));
    CHECK(e.getOrderBook().orderCount() == 0);
}

void testSellLimitPrice()
{
    SilenceCout quiet;
    TradingEngine e(10000.0);
    e.setMarketPrice("BTC", 100.0);
    e.executeBuy("BTC", 3);
    double cashAfterBuy = e.getPortfolio().getCash();

    // asking more than the market pays: no trade
    CHECK(!e.executeSell("BTC", 1, 101.0));
    CHECK(near(e.getPortfolio().getCash(), cashAfterBuy));
    CHECK(e.getPortfolio().getQuantity("BTC") == 3);
    CHECK(e.getOrderBook().orderCount() == 0);

    // asking less than the market pays: sells at the market price
    CHECK(e.executeSell("BTC", 1, 99.0));
    CHECK(near(e.getPortfolio().getCash(), cashAfterBuy + 100.0));
    CHECK(e.getPortfolio().getQuantity("BTC") == 2);
}

void testSymbolsAreIndependent()
{
    SilenceCout quiet;
    TradingEngine e(100000.0);
    e.setMarketPrice("BTC", 60000.0);
    e.setMarketPrice("ETH", 4000.0);
    CHECK(e.executeBuy("BTC", 1));
    CHECK(e.executeBuy("ETH", 5));
    CHECK(e.getPortfolio().getQuantity("BTC") == 1);
    CHECK(e.getPortfolio().getQuantity("ETH") == 5);
    CHECK(near(e.getPortfolio().getCash(), 100000.0 - 60000.0 - 20000.0));
    CHECK(e.getOrderBook().tradeCount() == 2);
    CHECK(e.getOrderBook().orderCount() == 0);
}

void testPriceChangesAffectLaterTrades()
{
    SilenceCout quiet;
    TradingEngine e(100000.0);
    e.setMarketPrice("BTC", 100.0);
    e.executeBuy("BTC", 1);
    e.setMarketPrice("BTC", 200.0);
    e.executeBuy("BTC", 1);
    CHECK(e.getPortfolio().getQuantity("BTC") == 2);
    CHECK(near(e.getPortfolio().getAveragePrice("BTC"), 150.0));
    CHECK(near(e.getOrderBook().trades()[0].price, 100.0));
    CHECK(near(e.getOrderBook().trades()[1].price, 200.0));
}

void testBookStaysCleanOverManyTrades()
{
    SilenceCout quiet;
    TradingEngine e(1000000.0);
    e.setMarketPrice("BTC", 100.0);
    bool allOk = true;
    for (int i = 0; i < 200; ++i)
    {
        if (!e.executeBuy("BTC", 2)) allOk = false;
        if (!e.executeSell("BTC", 1)) allOk = false;
        if (e.getOrderBook().orderCount() != 0) allOk = false;
    }
    CHECK(allOk);
    CHECK(e.getPortfolio().getQuantity("BTC") == 200);
    CHECK(e.getOrderBook().tradeCount() == 400);
    CHECK(e.getOrderBook().totalTradedQuantity() == 600); // 200*2 + 200*1
}

void testRunStrategyBuysAndSells()
{
    SilenceCout quiet;
    TradingEngine e(1000000.0);
    BuyLowSellHigh s("BTC", 59000.0, 62000.0, 2);

    e.setMarketPrice("BTC", 58000.0); // low: strategy says BUY 2
    e.runStrategy(s);
    CHECK(e.getPortfolio().getQuantity("BTC") == 2);
    CHECK(e.getOrderBook().tradeCount() == 1);

    e.setMarketPrice("BTC", 60000.0); // in between: HOLD
    e.runStrategy(s);
    CHECK(e.getPortfolio().getQuantity("BTC") == 2);
    CHECK(e.getOrderBook().tradeCount() == 1);

    e.setMarketPrice("BTC", 70000.0); // high: strategy says SELL 2
    e.runStrategy(s);
    CHECK(e.getPortfolio().getQuantity("BTC") == 0);
    CHECK(e.getOrderBook().tradeCount() == 2);
    CHECK(e.getOrderBook().orderCount() == 0);
}

void testRunStrategyWithoutPriceDoesNothing()
{
    SilenceCout quiet;
    TradingEngine e(1000.0);
    BuyLowSellHigh s("BTC", 59000.0, 62000.0);
    e.runStrategy(s); // no price known yet: HOLD
    CHECK(e.getPortfolio().getQuantity("BTC") == 0);
    CHECK(e.getOrderBook().tradeCount() == 0);
}

void testDisplayShowsAllSections()
{
    SilenceCout capture;
    TradingEngine e(1000.0);
    e.setMarketPrice("BTC", 100.0);
    e.executeBuy("BTC", 1);
    e.display();
    std::string out = capture.text();
    CHECK(out.find("===== TRADING ENGINE =====") != std::string::npos);
    CHECK(out.find("--- MARKET DATA ---") != std::string::npos);
    CHECK(out.find("--- ORDER BOOK ---") != std::string::npos);
    CHECK(out.find("--- PORTFOLIO ---") != std::string::npos);
    CHECK(out.find("Trading Engine: BUY executed for BTC") != std::string::npos);
}

int main()
{
    testInitialState();
    testBuyGoesThroughOrderBook();
    testSellGoesThroughOrderBook();
    testRejectedTradesLeaveNoTrace();
    testSellMoreThanOwnedFails();
    testBuyUsesAllCashExactly();
    testBuyLimitPrice();
    testSellLimitPrice();
    testSymbolsAreIndependent();
    testPriceChangesAffectLaterTrades();
    testBookStaysCleanOverManyTrades();
    testRunStrategyBuysAndSells();
    testRunStrategyWithoutPriceDoesNothing();
    testDisplayShowsAllSections();

    std::cout << (g_total - g_failed) << "/" << g_total << " checks passed\n";
    if (g_failed == 0)
    {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }
    return 1;
}