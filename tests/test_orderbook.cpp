#include "../include/OrderBook.h"
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

static Order buy(int id, double price, int qty, const char* sym = "BTC")
{
    return Order(id, sym, price, qty, "BUY");
}

static Order sell(int id, double price, int qty, const char* sym = "BTC")
{
    return Order(id, sym, price, qty, "SELL");
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

void testRejectsInvalidOrders()
{
    OrderBook b;
    CHECK(!b.addOrder(buy(1, 100.0, 0)));   // zero quantity
    CHECK(!b.addOrder(buy(2, 100.0, -5)));  // negative quantity
    CHECK(!b.addOrder(buy(3, 0.0, 1)));     // zero price
    CHECK(!b.addOrder(buy(4, -10.0, 1)));   // negative price
    CHECK(!b.addOrder(Order(5, "BTC", 100.0, 1, "HOLD"))); // bad type
    CHECK(b.orderCount() == 0);

    CHECK(b.addOrder(buy(6, 100.0, 1)));
    CHECK(!b.addOrder(buy(6, 101.0, 1)));   // duplicate id while resting
    CHECK(b.orderCount() == 1);
    CHECK(near(b.bestBid("BTC"), 100.0));   // duplicate did not replace it
}

void testNonCrossingOrdersRest()
{
    OrderBook b;
    CHECK(b.addOrder(buy(1, 100.0, 5)));
    CHECK(b.addOrder(sell(2, 101.0, 5)));
    CHECK(b.tradeCount() == 0);
    CHECK(b.orderCount() == 2);
    CHECK(near(b.bestBid("BTC"), 100.0));
    CHECK(near(b.bestAsk("BTC"), 101.0));
    CHECK(b.bestBid("ETH") == -1);          // unknown symbol
    CHECK(b.bestAsk("ETH") == -1);
}

void testExactFullMatch()
{
    OrderBook b;
    b.addOrder(sell(1, 100.0, 5));
    CHECK(b.addOrder(buy(2, 100.0, 5)));
    CHECK(b.tradeCount() == 1);
    CHECK(b.totalTradedQuantity() == 5);
    CHECK(b.orderCount() == 0);
    CHECK(b.bestBid("BTC") == -1);
    CHECK(b.bestAsk("BTC") == -1);
    CHECK(b.findOrder(1) == nullptr);
    CHECK(b.findOrder(2) == nullptr);
}

void testTradeUsesRestingOrderPrice()
{
    {
        OrderBook b;
        b.addOrder(sell(1, 100.0, 1));
        b.addOrder(buy(2, 105.0, 1)); // willing to pay 105, pays 100
        CHECK(b.trades().size() == 1);
        CHECK(near(b.trades()[0].price, 100.0));
    }
    {
        OrderBook b;
        b.addOrder(buy(1, 100.0, 1));
        b.addOrder(sell(2, 95.0, 1)); // willing to take 95, gets 100
        CHECK(b.trades().size() == 1);
        CHECK(near(b.trades()[0].price, 100.0));
    }
}

void testTradeRecordFields()
{
    OrderBook b;
    // incoming BUY is the aggressor
    b.addOrder(sell(10, 100.0, 3));
    b.addOrder(buy(11, 100.0, 3));
    // incoming SELL is the aggressor
    b.addOrder(buy(12, 50.0, 2, "ETH"));
    b.addOrder(sell(13, 50.0, 2, "ETH"));

    CHECK(b.trades().size() == 2);
    const Trade& t1 = b.trades()[0];
    CHECK(t1.symbol == "BTC");
    CHECK(t1.buyOrderId == 11 && t1.sellOrderId == 10);
    CHECK(t1.quantity == 3 && near(t1.price, 100.0));
    const Trade& t2 = b.trades()[1];
    CHECK(t2.symbol == "ETH");
    CHECK(t2.buyOrderId == 12 && t2.sellOrderId == 13);
    CHECK(t2.quantity == 2 && near(t2.price, 50.0));
}

void testPartialFillLeavesRestingRemainder()
{
    OrderBook b;
    b.addOrder(sell(1, 100.0, 10));
    b.addOrder(buy(2, 100.0, 4)); // takes 4 of 10
    CHECK(b.tradeCount() == 1);
    CHECK(b.totalTradedQuantity() == 4);
    CHECK(b.findOrder(1) != nullptr && b.findOrder(1)->getQuantity() == 6);
    CHECK(b.findOrder(2) == nullptr); // fully filled, never rested
    CHECK(b.orderCount() == 1);
    CHECK(near(b.bestAsk("BTC"), 100.0));
}

void testPartialFillRestsIncomingRemainder()
{
    OrderBook b;
    b.addOrder(sell(1, 100.0, 6));
    b.addOrder(buy(2, 100.0, 10)); // 6 trade, 4 rest as a bid
    CHECK(b.tradeCount() == 1);
    CHECK(b.totalTradedQuantity() == 6);
    CHECK(b.findOrder(1) == nullptr);
    CHECK(b.findOrder(2) != nullptr && b.findOrder(2)->getQuantity() == 4);
    CHECK(near(b.bestBid("BTC"), 100.0));
    CHECK(b.bestAsk("BTC") == -1);
    CHECK(b.orderCount() == 1);
}

void testPricePriority()
{
    OrderBook b;
    b.addOrder(sell(1, 102.0, 1));
    b.addOrder(sell(2, 100.0, 1));
    b.addOrder(sell(3, 101.0, 1));
    CHECK(near(b.bestAsk("BTC"), 100.0));

    b.addOrder(buy(10, 102.0, 1));
    b.addOrder(buy(11, 102.0, 1));
    b.addOrder(buy(12, 102.0, 1));
    CHECK(b.trades().size() == 3);
    // cheapest sell first: 100 (id 2), then 101 (id 3), then 102 (id 1)
    CHECK(b.trades()[0].sellOrderId == 2 && near(b.trades()[0].price, 100.0));
    CHECK(b.trades()[1].sellOrderId == 3 && near(b.trades()[1].price, 101.0));
    CHECK(b.trades()[2].sellOrderId == 1 && near(b.trades()[2].price, 102.0));
}

void testBuyPricePriority()
{
    OrderBook b;
    b.addOrder(buy(1, 98.0, 1));
    b.addOrder(buy(2, 100.0, 1));
    b.addOrder(buy(3, 99.0, 1));
    CHECK(near(b.bestBid("BTC"), 100.0));

    b.addOrder(sell(10, 98.0, 1));
    b.addOrder(sell(11, 98.0, 1));
    b.addOrder(sell(12, 98.0, 1));
    // highest bid first: 100 (id 2), 99 (id 3), 98 (id 1)
    CHECK(b.trades()[0].buyOrderId == 2);
    CHECK(b.trades()[1].buyOrderId == 3);
    CHECK(b.trades()[2].buyOrderId == 1);
}

void testTimePriorityAtSamePrice()
{
    OrderBook b;
    b.addOrder(sell(1, 100.0, 1));
    b.addOrder(sell(2, 100.0, 1));
    b.addOrder(sell(3, 100.0, 1));
    b.addOrder(buy(10, 100.0, 1));
    b.addOrder(buy(11, 100.0, 1));
    CHECK(b.trades()[0].sellOrderId == 1); // earliest first
    CHECK(b.trades()[1].sellOrderId == 2);
    CHECK(b.findOrder(3) != nullptr);
}

void testSweepsMultiplePriceLevels()
{
    OrderBook b;
    b.addOrder(sell(1, 100.0, 2));
    b.addOrder(sell(2, 101.0, 3));
    b.addOrder(sell(3, 102.0, 5));
    b.addOrder(buy(9, 101.0, 10)); // takes 2@100 and 3@101, rests 5@101

    CHECK(b.trades().size() == 2);
    CHECK(b.trades()[0].quantity == 2 && near(b.trades()[0].price, 100.0));
    CHECK(b.trades()[1].quantity == 3 && near(b.trades()[1].price, 101.0));
    CHECK(b.findOrder(9) != nullptr && b.findOrder(9)->getQuantity() == 5);
    CHECK(near(b.bestBid("BTC"), 101.0));
    CHECK(near(b.bestAsk("BTC"), 102.0));
    CHECK(b.findOrder(3)->getQuantity() == 5); // untouched
}

void testSymbolsAreIsolated()
{
    OrderBook b;
    b.addOrder(sell(1, 100.0, 5, "BTC"));
    b.addOrder(buy(2, 200.0, 5, "ETH")); // higher price but different symbol
    CHECK(b.tradeCount() == 0);
    CHECK(near(b.bestAsk("BTC"), 100.0));
    CHECK(near(b.bestBid("ETH"), 200.0));
    CHECK(b.bestBid("BTC") == -1);
    CHECK(b.orderCount() == 2);
}

void testCancel()
{
    OrderBook b;
    b.addOrder(buy(1, 100.0, 1));
    CHECK(b.removeOrder(1));
    CHECK(!b.removeOrder(1)); // already gone
    CHECK(!b.removeOrder(99)); // never existed
    CHECK(b.findOrder(1) == nullptr);
    CHECK(b.bestBid("BTC") == -1);
    CHECK(b.orderCount() == 0);
}

void testCancelKeepsPriorityOfOthers()
{
    OrderBook b;
    b.addOrder(buy(1, 100.0, 1));
    b.addOrder(buy(2, 100.0, 1));
    b.addOrder(buy(3, 100.0, 1));
    CHECK(b.removeOrder(2)); // middle of the queue
    b.addOrder(sell(10, 100.0, 1));
    b.addOrder(sell(11, 100.0, 1));
    CHECK(b.trades().size() == 2);
    CHECK(b.trades()[0].buyOrderId == 1);
    CHECK(b.trades()[1].buyOrderId == 3);
    CHECK(b.orderCount() == 0);
}

void testCancelWorksAfterPartialFill()
{
    OrderBook b;
    b.addOrder(sell(1, 100.0, 10));
    b.addOrder(buy(2, 100.0, 4));
    CHECK(b.removeOrder(1)); // cancel the remaining 6
    CHECK(b.orderCount() == 0);
    CHECK(b.bestAsk("BTC") == -1);
}

void testIdCanBeReusedAfterFill()
{
    OrderBook b;
    b.addOrder(sell(1, 100.0, 1));
    b.addOrder(buy(2, 100.0, 1)); // order 1 is filled and gone
    CHECK(b.addOrder(sell(1, 105.0, 1))); // same id is fine now
    CHECK(b.findOrder(1) != nullptr);
    CHECK(near(b.bestAsk("BTC"), 105.0));
}

void testFindOrderReturnsRestingData()
{
    OrderBook b;
    b.addOrder(buy(7, 123.5, 9, "ETH"));
    Order* o = b.findOrder(7);
    CHECK(o != nullptr);
    CHECK(o != nullptr && o->getSymbol() == "ETH");
    CHECK(o != nullptr && near(o->getPrice(), 123.5));
    CHECK(o != nullptr && o->getQuantity() == 9);
    CHECK(b.findOrder(8) == nullptr);
}

void testTradeLoggingCanBeDisabled()
{
    OrderBook b;
    b.setTradeLogging(false);
    b.addOrder(sell(1, 100.0, 5));
    b.addOrder(buy(2, 100.0, 5));
    CHECK(b.trades().empty());
    CHECK(b.tradeCount() == 1);
    CHECK(b.totalTradedQuantity() == 5);
}

void testManyOrders()
{
    OrderBook b;
    b.setTradeLogging(false);
    for (int i = 0; i < 1000; ++i)
    {
        b.addOrder(sell(i, 100.0 + (i % 10), 1));
    }
    CHECK(b.orderCount() == 1000);
    CHECK(near(b.bestAsk("BTC"), 100.0));
    for (int i = 0; i < 1000; ++i)
    {
        b.addOrder(buy(10000 + i, 200.0, 1));
    }
    CHECK(b.tradeCount() == 1000);
    CHECK(b.totalTradedQuantity() == 1000);
    CHECK(b.orderCount() == 0);
}

void testDestructorFreesRestingOrders()
{
    // Nothing to assert: run under AddressSanitizer to see leaks.
    OrderBook b;
    for (int i = 0; i < 50; ++i)
    {
        b.addOrder(buy(i, 90.0 - i, 1, "BTC"));
        b.addOrder(sell(1000 + i, 110.0 + i, 1, "ETH"));
    }
    CHECK(b.orderCount() == 100);
}

void testDisplayOrders()
{
    OrderBook b;
    b.addOrder(buy(101, 59500.0, 2));
    b.addOrder(sell(102, 60500.0, 1));

    CaptureCout capture;
    b.displayOrders();
    std::string out = capture.text();

    size_t buyHeader = out.find("===== BUY ORDERS =====");
    size_t id101 = out.find("Order ID: 101");
    size_t sellHeader = out.find("===== SELL ORDERS =====");
    size_t id102 = out.find("Order ID: 102");
    CHECK(buyHeader != std::string::npos);
    CHECK(buyHeader < id101);
    CHECK(id101 < sellHeader);
    CHECK(sellHeader < id102);
}

int main()
{
    testRejectsInvalidOrders();
    testNonCrossingOrdersRest();
    testExactFullMatch();
    testTradeUsesRestingOrderPrice();
    testTradeRecordFields();
    testPartialFillLeavesRestingRemainder();
    testPartialFillRestsIncomingRemainder();
    testPricePriority();
    testBuyPricePriority();
    testTimePriorityAtSamePrice();
    testSweepsMultiplePriceLevels();
    testSymbolsAreIsolated();
    testCancel();
    testCancelKeepsPriorityOfOthers();
    testCancelWorksAfterPartialFill();
    testIdCanBeReusedAfterFill();
    testFindOrderReturnsRestingData();
    testTradeLoggingCanBeDisabled();
    testManyOrders();
    testDestructorFreesRestingOrders();
    testDisplayOrders();

    std::cout << (g_total - g_failed) << "/" << g_total << " checks passed\n";
    if (g_failed == 0)
    {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }
    return 1;
}