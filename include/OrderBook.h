// Limit order book with a matching engine.
//
//  - One buy list and one sell list per symbol, each kept sorted by price
//    (best first) and then by arrival time: price-time priority.
//  - An incoming order first trades against the opposite side while prices
//    cross. Whatever is left over rests in the book.
//  - A trade always happens at the price of the order that was already
//    resting in the book (the incoming order gets any price improvement).
//  - orderIndex (our CustomUnorderedMap) maps order id -> its list node, so
//    find and cancel by id are O(1) with no list scanning.
//
// Complexity (n = resting orders on one side of one symbol):
//    best bid / best ask : O(1)
//    match               : O(1) per trade (always at the front of the list)
//    rest a new order    : O(n) worst case (sorted insert)
//    cancel / find by id : O(1) average

#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <cstddef>
#include <string>
#include <vector>
#include "Order.h"
#include "OrderList.h"
#include "CustomUnorderedMap.h"

struct Trade
{
    std::string symbol;
    int buyOrderId;
    int sellOrderId;
    double price;
    int quantity;
};

class OrderBook
{
    private:
        struct SymbolBook
        {
            std::string symbol;
            OrderList buys;  // highest price first
            OrderList sells; // lowest price first
        };

        struct OrderRef
        {
            OrderList::Node* node;
            SymbolBook* book;
            bool isBuy;
        };

        CustomUnorderedMap<std::string, SymbolBook*> books;
        std::vector<SymbolBook*> bookList; // owns the books; also display order
        CustomUnorderedMap<int, OrderRef> orderIndex;

        std::vector<Trade> tradeLog;
        std::size_t tradeCount_;
        long long tradedQuantity_;
        bool logTrades_;

        SymbolBook* getOrCreateBook(const std::string& symbol);
        void match(SymbolBook* book, Order& incoming, bool incomingIsBuy);
        void rest(SymbolBook* book, const Order& order, bool isBuy);
        void recordTrade(const std::string& symbol, int buyId, int sellId,
                         double price, int quantity);

    public:
        OrderBook();
        ~OrderBook();

        // The book owns heap memory, so copying it is not allowed.
        OrderBook(const OrderBook&) = delete;
        OrderBook& operator=(const OrderBook&) = delete;

        // Submits a limit order. Returns false (and changes nothing) if the
        // order is invalid: type not "BUY"/"SELL", price or quantity <= 0, or
        // an order with the same id is already resting in the book.
        // The order may trade immediately, rest in the book, or both.
        bool addOrder(Order order);

        // Cancels a resting order by id. Returns false if it is not resting.
        bool removeOrder(int orderId);

        // Returns the resting order with this id, or nullptr.
        // Do not change its price through this pointer (that would break the
        // sort order); quantity is managed by the book itself.
        Order* findOrder(int orderId);

        // Best resting buy / sell price for a symbol, or -1 if there is none.
        double bestBid(const std::string& symbol) const;
        double bestAsk(const std::string& symbol) const;

        // Number of orders currently resting in the book (all symbols).
        std::size_t orderCount() const;

        // Trade history. Turn logging off for large benchmarks; the counters
        // below keep working either way.
        const std::vector<Trade>& trades() const;
        void setTradeLogging(bool enabled);
        std::size_t tradeCount() const;
        long long totalTradedQuantity() const;

        void displayOrders() const;
};
#endif