#include "../include/OrderBook.h"
#include <algorithm>
#include <iostream>

using namespace std;

OrderBook::OrderBook()
    : tradeCount_(0), tradedQuantity_(0), logTrades_(true)
{
}

OrderBook::~OrderBook()
{
    // The order lists live inside heap-allocated SymbolBooks; free them all.
    for (size_t i = 0; i < bookList.size(); ++i)
    {
        delete bookList[i];
    }
}

OrderBook::SymbolBook* OrderBook::getOrCreateBook(const string& symbol)
{
    SymbolBook** found = books.find(symbol);
    if (found != nullptr)
    {
        return *found;
    }

    SymbolBook* created = new SymbolBook();
    created->symbol = symbol;
    books.insert(symbol, created);
    bookList.push_back(created);
    return created;
}

void OrderBook::recordTrade(const string& symbol, int buyId, int sellId,
                            double price, int quantity)
{
    ++tradeCount_;
    tradedQuantity_ += quantity;
    if (logTrades_)
    {
        Trade t;
        t.symbol = symbol;
        t.buyOrderId = buyId;
        t.sellOrderId = sellId;
        t.price = price;
        t.quantity = quantity;
        tradeLog.push_back(t);
    }
}

void OrderBook::match(SymbolBook* book, Order& incoming, bool incomingIsBuy)
{
    OrderList& opposite = incomingIsBuy ? book->sells : book->buys;

    while (incoming.getQuantity() > 0)
    {
        Order* resting = opposite.front(); // best price, oldest first
        if (resting == nullptr)
        {
            break; // nothing on the other side
        }

        bool crosses = incomingIsBuy
            ? (incoming.getPrice() >= resting->getPrice())
            : (incoming.getPrice() <= resting->getPrice());
        if (!crosses)
        {
            break; // best opposite price is not good enough
        }

        int qty = min(incoming.getQuantity(), resting->getQuantity());
        double tradePrice = resting->getPrice(); // resting order's price

        int buyId = incomingIsBuy ? incoming.getOrderId() : resting->getOrderId();
        int sellId = incomingIsBuy ? resting->getOrderId() : incoming.getOrderId();
        recordTrade(incoming.getSymbol(), buyId, sellId, tradePrice, qty);

        incoming.setQuantity(incoming.getQuantity() - qty);
        resting->setQuantity(resting->getQuantity() - qty);

        if (resting->getQuantity() == 0)
        {
            // Fully filled: remove it. Read the id first, because the node
            // (and therefore `resting`) is deleted by removeNode.
            int restingId = resting->getOrderId();
            OrderRef* ref = orderIndex.find(restingId);
            opposite.removeNode(ref->node);
            orderIndex.remove(restingId);
        }
    }
}

void OrderBook::rest(SymbolBook* book, const Order& order, bool isBuy)
{
    OrderList& sameSide = isBuy ? book->buys : book->sells;
    OrderList::Node* node = sameSide.insertSorted(order, isBuy);

    OrderRef ref;
    ref.node = node;
    ref.book = book;
    ref.isBuy = isBuy;
    orderIndex.insert(order.getOrderId(), ref);
}

bool OrderBook::addOrder(Order order)
{
    bool isBuy;
    if (order.getType() == "BUY")
    {
        isBuy = true;
    }
    else if (order.getType() == "SELL")
    {
        isBuy = false;
    }
    else
    {
        return false;
    }

    if (order.getQuantity() <= 0 || order.getPrice() <= 0)
    {
        return false;
    }

    if (orderIndex.find(order.getOrderId()) != nullptr)
    {
        return false; // id already resting
    }

    SymbolBook* book = getOrCreateBook(order.getSymbol());

    match(book, order, isBuy);

    if (order.getQuantity() > 0)
    {
        rest(book, order, isBuy);
    }
    return true;
}

bool OrderBook::removeOrder(int orderId)
{
    OrderRef* found = orderIndex.find(orderId);
    if (found == nullptr)
    {
        return false;
    }

    OrderRef ref = *found; // copy before the index entry is removed
    OrderList& list = ref.isBuy ? ref.book->buys : ref.book->sells;
    list.removeNode(ref.node); // O(1): no searching
    orderIndex.remove(orderId);
    return true;
}

Order* OrderBook::findOrder(int orderId)
{
    OrderRef* found = orderIndex.find(orderId);
    if (found == nullptr)
    {
        return nullptr;
    }
    return &(found->node->order);
}

double OrderBook::bestBid(const string& symbol) const
{
    SymbolBook* const* found = books.find(symbol);
    if (found == nullptr)
    {
        return -1;
    }
    Order* top = (*found)->buys.front();
    return top != nullptr ? top->getPrice() : -1;
}

double OrderBook::bestAsk(const string& symbol) const
{
    SymbolBook* const* found = books.find(symbol);
    if (found == nullptr)
    {
        return -1;
    }
    Order* top = (*found)->sells.front();
    return top != nullptr ? top->getPrice() : -1;
}

size_t OrderBook::orderCount() const
{
    return orderIndex.size();
}

const vector<Trade>& OrderBook::trades() const
{
    return tradeLog;
}

void OrderBook::setTradeLogging(bool enabled)
{
    logTrades_ = enabled;
}

size_t OrderBook::tradeCount() const
{
    return tradeCount_;
}

long long OrderBook::totalTradedQuantity() const
{
    return tradedQuantity_;
}

void OrderBook::displayOrders() const
{
    cout << "\n===== BUY ORDERS =====\n";
    for (size_t i = 0; i < bookList.size(); ++i)
    {
        bookList[i]->buys.display();
    }

    cout << "\n===== SELL ORDERS =====\n";
    for (size_t i = 0; i < bookList.size(); ++i)
    {
        bookList[i]->sells.display();
    }
}