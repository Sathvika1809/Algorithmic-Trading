// Benchmark and simulation program for the trading system.
//
// Build (from the project root, with optimisation ON, it matters):
//   g++ -std=c++17 -O2 -Wall -Wextra bench/benchmark.cpp src/*.cpp -o benchmark.exe
// Run:
//   .\benchmark.exe          full run (takes roughly 10-40 seconds)
//   .\benchmark.exe quick    about 10x smaller, for a fast check
//
// Every random number comes from a fixed seed, so two runs on the same
// machine do the same work. Timings still vary a little between runs, and
// they depend on your computer: always quote numbers from YOUR machine.

#include "../include/CustomUnorderedMap.h"
#include "../include/OrderList.h"
#include "../include/OrderBook.h"
#include "../include/TradingEngine.h"
#include "../include/BuyLowSellHigh.h"
#include "../include/ArbitrageStrategy.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

typedef std::chrono::steady_clock Clock;

static double nsSince(Clock::time_point start)
{
    return std::chrono::duration<double, std::nano>(Clock::now() - start).count();
}

// The engine and strategies print status lines. This hides them during the
// simulations so only the results are shown.
struct SilenceCout
{
    std::ostringstream sink;
    std::streambuf* old;
    SilenceCout() : sink(), old(std::cout.rdbuf(sink.rdbuf())) {}
    ~SilenceCout() { std::cout.rdbuf(old); }
};

static void printHeader(const std::string& title)
{
    std::cout << "\n==================================================\n";
    std::cout << title << "\n";
    std::cout << "==================================================\n";
}

// ============================================================
// 1. Our hash map versus std::unordered_map
// ============================================================

struct MapTimes
{
    double insertNs;
    double hitNs;
    double missNs;
    double removeNs;
};

static MapTimes benchCustomMap(const std::vector<int>& keys,
                               const std::vector<int>& missKeys,
                               long long& checksum)
{
    const size_t n = keys.size();
    CustomUnorderedMap<int, int> m;
    MapTimes t;
    long long sum = 0;

    Clock::time_point t0 = Clock::now();
    for (size_t i = 0; i < n; ++i)
    {
        m.insert(keys[i], keys[i]);
    }
    t.insertNs = nsSince(t0) / static_cast<double>(n);

    t0 = Clock::now();
    for (size_t i = 0; i < n; ++i)
    {
        const int* v = m.find(keys[i]);
        if (v != nullptr)
        {
            sum += *v;
        }
    }
    t.hitNs = nsSince(t0) / static_cast<double>(n);

    t0 = Clock::now();
    for (size_t i = 0; i < n; ++i)
    {
        if (m.find(missKeys[i]) == nullptr) // key was never inserted
        {
            ++sum;
        }
    }
    t.missNs = nsSince(t0) / static_cast<double>(n);

    t0 = Clock::now();
    for (size_t i = 0; i < n; ++i)
    {
        if (m.remove(keys[i]))
        {
            ++sum;
        }
    }
    t.removeNs = nsSince(t0) / static_cast<double>(n);

    checksum += sum + (m.empty() ? 1 : 0);
    return t;
}

static MapTimes benchStdMap(const std::vector<int>& keys,
                            const std::vector<int>& missKeys,
                            long long& checksum)
{
    const size_t n = keys.size();
    std::unordered_map<int, int> m;
    MapTimes t;
    long long sum = 0;

    Clock::time_point t0 = Clock::now();
    for (size_t i = 0; i < n; ++i)
    {
        m.emplace(keys[i], keys[i]);
    }
    t.insertNs = nsSince(t0) / static_cast<double>(n);

    t0 = Clock::now();
    for (size_t i = 0; i < n; ++i)
    {
        std::unordered_map<int, int>::const_iterator it = m.find(keys[i]);
        if (it != m.end())
        {
            sum += it->second;
        }
    }
    t.hitNs = nsSince(t0) / static_cast<double>(n);

    t0 = Clock::now();
    for (size_t i = 0; i < n; ++i)
    {
        if (m.find(missKeys[i]) == m.end())
        {
            ++sum;
        }
    }
    t.missNs = nsSince(t0) / static_cast<double>(n);

    t0 = Clock::now();
    for (size_t i = 0; i < n; ++i)
    {
        sum += static_cast<long long>(m.erase(keys[i]));
    }
    t.removeNs = nsSince(t0) / static_cast<double>(n);

    checksum += sum + (m.empty() ? 1 : 0);
    return t;
}

static MapTimes minTimes(const MapTimes& a, const MapTimes& b)
{
    MapTimes t;
    t.insertNs = std::min(a.insertNs, b.insertNs);
    t.hitNs = std::min(a.hitNs, b.hitNs);
    t.missNs = std::min(a.missNs, b.missNs);
    t.removeNs = std::min(a.removeNs, b.removeNs);
    return t;
}

static void printMapRow(const std::string& name, double custom, double stdNs)
{
    std::cout << std::left << std::setw(14) << name
              << std::right << std::fixed << std::setprecision(1)
              << std::setw(14) << custom
              << std::setw(14) << stdNs
              << std::setprecision(2) << std::setw(12) << (stdNs / custom) << "x\n";
}

static void runMapPattern(const std::string& title,
                          const std::vector<int>& keys,
                          const std::vector<int>& missKeys,
                          long long& checksum)
{
    MapTimes custom = benchCustomMap(keys, missKeys, checksum);
    MapTimes standard = benchStdMap(keys, missKeys, checksum);
    for (int run = 1; run < 3; ++run)
    {
        custom = minTimes(custom, benchCustomMap(keys, missKeys, checksum));
        standard = minTimes(standard, benchStdMap(keys, missKeys, checksum));
    }

    std::cout << title << "\n";
    std::cout << std::left << std::setw(14) << "operation"
              << std::right << std::setw(14) << "ours ns/op"
              << std::setw(14) << "std ns/op"
              << std::setw(13) << "speedup" << "\n";
    printMapRow("insert", custom.insertNs, standard.insertNs);
    printMapRow("find (hit)", custom.hitNs, standard.hitNs);
    printMapRow("find (miss)", custom.missNs, standard.missNs);
    printMapRow("remove", custom.removeNs, standard.removeNs);
    std::cout << "\n";
}

static void benchHashMaps(int n, long long& checksum)
{
    printHeader("1. HASH MAP: our CustomUnorderedMap vs std::unordered_map");
    std::cout << n << " integer keys, best of 3 runs. Speedup above 1.00x means OUR\n";
    std::cout << "map is faster; below 1.00x means the standard library is faster.\n";
    std::cout << "Two key patterns, because the result depends on the keys.\n\n";

    const size_t count = static_cast<size_t>(n);
    std::mt19937 rng(1);

    // Pattern A: ids 0..n-1 in shuffled order (like order ids). Misses are
    // ids above the range.
    std::vector<int> seqKeys(count);
    std::vector<int> seqMiss(count);
    for (size_t i = 0; i < count; ++i)
    {
        seqKeys[i] = static_cast<int>(i);
    }
    std::shuffle(seqKeys.begin(), seqKeys.end(), rng);
    for (size_t i = 0; i < count; ++i)
    {
        seqMiss[i] = seqKeys[i] + n;
    }
    runMapPattern("A) sequential ids, shuffled (best case for our hash):",
                  seqKeys, seqMiss, checksum);

    // Pattern B: random 31-bit keys. The miss keys are random numbers that
    // are checked to be absent from the key set (setup only, not timed).
    std::unordered_set<int> used;
    std::vector<int> rndKeys;
    std::vector<int> rndMiss;
    rndKeys.reserve(count);
    rndMiss.reserve(count);
    while (rndKeys.size() < count)
    {
        int k = static_cast<int>(rng() & 0x7fffffffu);
        if (used.insert(k).second)
        {
            rndKeys.push_back(k);
        }
    }
    while (rndMiss.size() < count)
    {
        int k = static_cast<int>(rng() & 0x7fffffffu);
        if (used.find(k) == used.end())
        {
            rndMiss.push_back(k);
        }
    }
    runMapPattern("B) random keys (a fairer, harder test):", rndKeys, rndMiss, checksum);

    std::cout << "Memory note: our map keeps load factor <= 0.75 (more buckets), which\n";
    std::cout << "helps speed but costs memory.\n";
}

// ============================================================
// 2. Cancelling an order: scan the list vs jump straight to it
// ============================================================

static void benchCancel(int n, long long& checksum)
{
    std::vector<int> cancelOrder(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i)
    {
        cancelOrder[static_cast<size_t>(i)] = i;
    }
    std::mt19937 rng(2);
    std::shuffle(cancelOrder.begin(), cancelOrder.end(), rng);

    long long sum = 0;

    // (a) the old way: search the list for the id, then unlink it
    double scanNs = 0.0;
    {
        OrderList list;
        for (int i = 0; i < n; ++i)
        {
            list.insert(Order(i, "BTC", 100.0, 1, "BUY"));
        }
        Clock::time_point t0 = Clock::now();
        for (size_t i = 0; i < cancelOrder.size(); ++i)
        {
            if (list.remove(cancelOrder[i]))
            {
                ++sum;
            }
        }
        scanNs = nsSince(t0) / static_cast<double>(n);
        sum += list.empty() ? 1 : 0;
    }

    // (b) the order book: hash map finds the node, then O(1) unlink
    double bookNs = 0.0;
    {
        OrderBook book;
        book.setTradeLogging(false);
        for (int i = 0; i < n; ++i)
        {
            // falling prices: every new order is the worst one, so resting
            // it is O(1) and the setup stays fast
            book.addOrder(Order(i, "BTC", 1000000.0 - i, 1, "BUY"));
        }
        Clock::time_point t0 = Clock::now();
        for (size_t i = 0; i < cancelOrder.size(); ++i)
        {
            if (book.removeOrder(cancelOrder[i]))
            {
                ++sum;
            }
        }
        bookNs = nsSince(t0) / static_cast<double>(n);
        sum += (book.orderCount() == 0) ? 1 : 0;
    }

    checksum += sum;

    std::cout << std::right << std::setw(10) << n
              << std::fixed << std::setprecision(1)
              << std::setw(18) << scanNs
              << std::setw(18) << bookNs
              << std::setprecision(1) << std::setw(12) << (scanNs / bookNs) << "x\n";
}

static void benchCancels(const std::vector<int>& sizes, long long& checksum)
{
    printHeader("2. CANCELLING ORDERS: list scan vs order book (hash map + handle)");
    std::cout << "Cancel every resting order, in random order. Time per cancel.\n";
    std::cout << "Scanning gets slower as the book grows; the order book does not.\n\n";
    std::cout << std::right << std::setw(10) << "orders"
              << std::setw(18) << "list scan ns"
              << std::setw(18) << "order book ns"
              << std::setw(13) << "speedup" << "\n";
    for (size_t i = 0; i < sizes.size(); ++i)
    {
        benchCancel(sizes[i], checksum);
    }
}

// ============================================================
// 3. Order book throughput and latency
// ============================================================

// One step of the generated order flow: a new order, and the id of an older
// order to cancel right afterwards (0 = no cancel). Every order is cancelled
// `lifetime` steps after it was placed, unless it already traded. That keeps
// the number of resting orders (the book "depth") roughly bounded, like a
// real book where stale orders get pulled.
struct FlowStep
{
    Order order;
    int cancelId;
};

static std::vector<FlowStep> makeFlow(size_t steps, size_t lifetime, unsigned seed)
{
    std::mt19937 rng(seed);
    std::vector<FlowStep> flow;
    flow.reserve(steps);
    double mid = 10000.0;
    for (size_t i = 0; i < steps; ++i)
    {
        if (rng() % 8u == 0)
        {
            mid += ((rng() & 1u) != 0) ? 1.0 : -1.0; // the mid price drifts slowly
        }
        bool isBuy = (rng() & 1u) != 0;
        // offset -3..+6 ticks from the mid; negative offsets cross the spread
        double offset = static_cast<double>(static_cast<int>(rng() % 10u)) - 3.0;
        double price = isBuy ? (mid - offset) : (mid + offset);
        int qty = 1 + static_cast<int>(rng() % 10u);

        FlowStep step;
        step.order = Order(static_cast<int>(i) + 1, "BTC", price, qty,
                           isBuy ? "BUY" : "SELL");
        step.cancelId = (i >= lifetime) ? static_cast<int>(i - lifetime) + 1 : 0;
        flow.push_back(step);
    }
    return flow;
}

static long long percentileNs(const std::vector<long long>& sorted, double p)
{
    size_t index = static_cast<size_t>(p * static_cast<double>(sorted.size() - 1));
    return sorted[index];
}

static void benchOrderBookAtDepth(size_t steps, size_t lifetime, long long& checksum)
{
    std::vector<FlowStep> flow = makeFlow(steps, lifetime, 7);
    long long submittedQty = 0;
    for (size_t i = 0; i < flow.size(); ++i)
    {
        submittedQty += flow[i].order.getQuantity();
    }

    OrderBook book;
    book.setTradeLogging(false);
    size_t cancelled = 0;
    size_t cancelAttempts = 0;
    size_t maxDepth = 0;

    Clock::time_point t0 = Clock::now();
    for (size_t i = 0; i < flow.size(); ++i)
    {
        book.addOrder(flow[i].order);
        if (flow[i].cancelId != 0)
        {
            ++cancelAttempts;
            if (book.removeOrder(flow[i].cancelId))
            {
                ++cancelled;
            }
        }
    }
    double totalNs = nsSince(t0);

    // depth is measured in a second, untimed pass so it cannot slow the timing
    {
        OrderBook probe;
        probe.setTradeLogging(false);
        for (size_t i = 0; i < flow.size(); ++i)
        {
            probe.addOrder(flow[i].order);
            if (flow[i].cancelId != 0)
            {
                probe.removeOrder(flow[i].cancelId);
            }
            maxDepth = std::max(maxDepth, probe.orderCount());
        }
    }

    double operations = static_cast<double>(flow.size() + cancelAttempts);
    double opsPerSec = operations / (totalNs / 1e9);
    double tradedShare = 100.0 * 2.0 * static_cast<double>(book.totalTradedQuantity())
                         / static_cast<double>(submittedQty);

    std::cout << std::right << std::fixed
              << std::setw(10) << lifetime
              << std::setw(11) << maxDepth
              << std::setprecision(0) << std::setw(14) << opsPerSec
              << std::setprecision(0) << std::setw(12) << (totalNs / operations)
              << std::setw(10) << book.tradeCount()
              << std::setprecision(1) << std::setw(10) << tradedShare << "\n";

    checksum += static_cast<long long>(book.tradeCount()) + book.totalTradedQuantity()
                + static_cast<long long>(book.orderCount())
                + static_cast<long long>(cancelled);
}

static void benchOrderBook(size_t steps, size_t latencySteps, long long& checksum)
{
    printHeader("3. ORDER BOOK: matching engine speed");
    std::cout << steps << " new orders per row (buy/sell limit orders near a drifting\n";
    std::cout << "mid price, quantity 1-10), each cancelled after the given number of\n";
    std::cout << "steps if it has not traded. Trade log off. The deeper the book, the\n";
    std::cout << "more orders rest in it.\n\n";
    std::cout << std::right << std::setw(10) << "lifetime"
              << std::setw(11) << "max depth"
              << std::setw(14) << "ops/second"
              << std::setw(12) << "ns/op"
              << std::setw(10) << "trades"
              << std::setw(10) << "traded%" << "\n";

    benchOrderBookAtDepth(steps, 100, checksum);
    benchOrderBookAtDepth(steps, 1000, checksum);
    benchOrderBookAtDepth(steps, 10000, checksum);

    std::cout << "\n(an operation is one new order or one cancel; max depth = most\n";
    std::cout << "orders resting in the book at any moment)\n";

    // ---- latency of single orders, on the 1000-lifetime flow ----
    std::vector<FlowStep> flow = makeFlow(latencySteps, 1000, 11);
    OrderBook book;
    book.setTradeLogging(false);
    std::vector<long long> latencies(flow.size());
    for (size_t i = 0; i < flow.size(); ++i)
    {
        Clock::time_point s = Clock::now();
        book.addOrder(flow[i].order);
        latencies[i] = static_cast<long long>(nsSince(s));
        if (flow[i].cancelId != 0)
        {
            book.removeOrder(flow[i].cancelId);
        }
    }
    std::sort(latencies.begin(), latencies.end());

    std::cout << "\nLatency of adding ONE order (" << flow.size() << " orders, lifetime 1000).\n";
    std::cout << "Includes roughly 20-100 ns of timer overhead per measurement.\n\n";
    std::cout << std::right;
    std::cout << "  median (p50) : " << std::setw(9) << percentileNs(latencies, 0.50) << " ns\n";
    std::cout << "  p90          : " << std::setw(9) << percentileNs(latencies, 0.90) << " ns\n";
    std::cout << "  p99          : " << std::setw(9) << percentileNs(latencies, 0.99) << " ns\n";
    std::cout << "  p99.9        : " << std::setw(9) << percentileNs(latencies, 0.999) << " ns\n";
    std::cout << "  worst (max)  : " << std::setw(9) << latencies.back() << " ns\n";
    std::cout << "(the worst case is often the operating system pausing the program,\n";
    std::cout << "not the matching code)\n";
    checksum += latencies.back() + static_cast<long long>(book.tradeCount());
}

// ============================================================
// 4. Strategy simulations on generated prices
// ============================================================

struct SimResult
{
    int ticks;
    int buys;
    int sells;
    int wins;
    double startValue;
    double endValue;
    double maxDrawdownPct;
};

// Price follows exp(x), where x is pulled back towards log(60000) with
// strength theta, pushed by a constant drift, and shaken by random noise
// each tick.
//   theta > 0           : mean-reverting prices
//   theta == 0, drift 0 : a plain random walk
//   theta == 0, drift < 0: a slow downtrend
static SimResult simulateBuyLowSellHigh(int ticks, double theta, double drift,
                                        unsigned seed)
{
    const double startCash = 1000000.0;
    const double fee = 0.0005; // 0.05% per fill
    const double mean = 60000.0;

    SilenceCout quiet;
    TradingEngine engine(startCash);
    engine.setFeeRate(fee);
    BuyLowSellHigh strategy("BTC", 59500.0, 60500.0, 1);

    std::mt19937 rng(seed);
    std::normal_distribution<double> noise(0.0, 1.0);
    double logPrice = std::log(mean);

    SimResult r;
    r.ticks = ticks;
    r.buys = 0;
    r.sells = 0;
    r.wins = 0;
    r.startValue = startCash;
    r.endValue = startCash;
    r.maxDrawdownPct = 0.0;
    double peak = startCash;

    for (int t = 0; t < ticks; ++t)
    {
        logPrice += theta * (std::log(mean) - logPrice) + drift + 0.003 * noise(rng);
        double price = std::exp(logPrice);
        engine.setMarketPrice("BTC", price);

        int qtyBefore = engine.getPortfolio().getQuantity("BTC");
        double avgBefore = engine.getPortfolio().getAveragePrice("BTC");
        engine.runStrategy(strategy);
        int qtyAfter = engine.getPortfolio().getQuantity("BTC");

        if (qtyAfter > qtyBefore)
        {
            ++r.buys;
        }
        else if (qtyAfter < qtyBefore)
        {
            ++r.sells;
            if (price * (1.0 - fee) > avgBefore)
            {
                ++r.wins; // sold for more than it cost (fees included)
            }
        }

        double value = engine.getPortfolio().getPortfolioValue(engine.getMarket());
        r.endValue = value;
        if (value > peak)
        {
            peak = value;
        }
        double drawdown = 100.0 * (peak - value) / peak;
        if (drawdown > r.maxDrawdownPct)
        {
            r.maxDrawdownPct = drawdown;
        }
    }
    return r;
}

static void printSimRow(const std::string& name, const SimResult& r)
{
    double winRate = (r.sells > 0) ? 100.0 * r.wins / r.sells : 0.0;
    double returnPct = 100.0 * (r.endValue - r.startValue) / r.startValue;
    std::cout << std::left << std::setw(24) << name
              << std::right << std::fixed
              << std::setw(7) << r.buys
              << std::setw(7) << r.sells
              << std::setw(7) << (r.buys - r.sells)
              << std::setprecision(1) << std::setw(8) << winRate
              << std::setprecision(1) << std::setw(10) << returnPct
              << std::setprecision(1) << std::setw(14) << r.maxDrawdownPct << "\n";
}

struct ArbResult
{
    int ticks;
    int opportunities;
    int executed;
    double profit;
    double expectedProfit;
    int leftoverQty;
};

static ArbResult simulateArbitrage(int ticks, unsigned seed)
{
    const double startCash = 1000000.0;
    const double fee = 0.001; // 0.1% per fill, so a round trip costs 0.2%

    SilenceCout quiet;
    TradingEngine engine(startCash);
    engine.setFeeRate(fee);
    ArbitrageStrategy strategy("BTC", "BTC-A", "BTC-B", fee, 0.0, 1);

    std::mt19937 rng(seed);
    std::normal_distribution<double> noise(0.0, 1.0);
    double logBase = std::log(60000.0);

    ArbResult r;
    r.ticks = ticks;
    r.opportunities = 0;
    r.executed = 0;
    r.profit = 0.0;
    r.expectedProfit = 0.0;
    r.leftoverQty = 0;

    for (int t = 0; t < ticks; ++t)
    {
        // one true price, and each venue quotes it with its own small error
        logBase += 0.002 * noise(rng);
        double base = std::exp(logBase);
        engine.setMarketPrice("BTC-A", base * (1.0 + 0.001 * noise(rng)));
        engine.setMarketPrice("BTC-B", base * (1.0 + 0.001 * noise(rng)));

        ArbitrageOpportunity opp = strategy.detect(engine.getMarket());
        if (opp.found)
        {
            ++r.opportunities;
            if (engine.executeArbitrage(opp))
            {
                ++r.executed;
                r.expectedProfit += opp.expectedProfit;
            }
        }
    }
    r.profit = engine.getPortfolio().getCash() - startCash;
    r.leftoverQty = engine.getPortfolio().getQuantity("BTC");
    return r;
}

static void benchSimulations(int ticks)
{
    printHeader("4. STRATEGY SIMULATIONS on generated (synthetic) prices");
    std::cout << ticks << " price ticks per scenario, 1,000,000 starting cash, fixed seed.\n";
    std::cout << "These prices are made up by a random generator. They test that the\n";
    std::cout << "machinery works end to end. They are NOT evidence of real profits.\n\n";

    SimResult mr = simulateBuyLowSellHigh(ticks, 0.05, 0.0, 42);
    SimResult rw = simulateBuyLowSellHigh(ticks, 0.0, 0.0, 42);
    SimResult dt = simulateBuyLowSellHigh(ticks, 0.0, -0.00003, 42);

    std::cout << "Buy-low / sell-high (buy <= 59,500, sell >= 60,500, 0.05% fee, 1 unit)\n";
    std::cout << std::left << std::setw(24) << "scenario"
              << std::right << std::setw(7) << "buys"
              << std::setw(7) << "sells"
              << std::setw(7) << "held"
              << std::setw(8) << "win %"
              << std::setw(10) << "return %"
              << std::setw(14) << "max drawdown%" << "\n";
    printSimRow("mean-reverting prices", mr);
    printSimRow("random-walk prices", rw);
    printSimRow("slow downtrend", dt);
    std::cout << "\n(win % counts only closed trades: it ignores units still held, which\n";
    std::cout << "is why the downtrend can show 100% wins and still lose money.)\n";
    std::cout << "The strategy assumes prices come back to normal. The first row looks\n";
    std::cout << "great only because those prices were generated to come back. With\n";
    std::cout << "no such pull, or in a downtrend, the strategy has no edge and the\n";
    std::cout << "drawdown shows the risk of buying a falling price.\n";

    ArbResult ar = simulateArbitrage(ticks, 43);
    std::cout << "\nArbitrage between two venues (0.1% fee per fill, 1 unit per trade)\n";
    std::cout << "  ticks                  : " << ar.ticks << "\n";
    std::cout << "  opportunities found    : " << ar.opportunities << "\n";
    std::cout << "  trades executed        : " << ar.executed << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  profit after fees      : " << ar.profit << "\n";
    std::cout << "  profit the strategy expected: " << ar.expectedProfit << "\n";
    std::cout << "  units left over        : " << ar.leftoverQty << "\n";
}

// ============================================================

int main(int argc, char** argv)
{
    bool quick = (argc > 1 && std::string(argv[1]) == "quick");

    int mapKeys = quick ? 100000 : 1000000;
    std::vector<int> cancelSizes;
    if (quick)
    {
        cancelSizes.push_back(500);
        cancelSizes.push_back(2000);
        cancelSizes.push_back(5000);
    }
    else
    {
        cancelSizes.push_back(1000);
        cancelSizes.push_back(10000);
        cancelSizes.push_back(30000);
    }
    size_t flowSteps = quick ? 20000 : 200000;
    size_t latencySteps = quick ? 20000 : 200000;
    int simTicks = quick ? 2000 : 20000;

    std::cout << "Trading system benchmark" << (quick ? " (quick mode)" : "") << "\n";
    std::cout << "Numbers depend on your computer. Close other programs for steady results.\n";

    long long checksum = 0; // keeps the compiler from skipping the work

    benchHashMaps(mapKeys, checksum);
    benchCancels(cancelSizes, checksum);
    benchOrderBook(flowSteps, latencySteps, checksum);
    benchSimulations(simTicks);

    std::cout << "\n(checksum " << checksum << ": proves the work above really ran)\n";
    return 0;
}