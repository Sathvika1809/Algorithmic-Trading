#include "../include/TradingEngine.h"
#include <iostream>
using namespace std;

TradingEngine::TradingEngine(double initialCash)
    : portfolio(initialCash),
      nextOrderId(1001),
      nextMarketMakerId(1000000000),
      feeRate_(0.0)
{
}

void TradingEngine::setMarketPrice(
    const std::string& symbol,
    double price)
{
    market.addPrice(symbol, price);
}

bool TradingEngine::setFeeRate(double rate)
{
    if(rate < 0 || rate >= 1)
    {
        return false;
    }
    feeRate_ = rate;
    return true;
}

double TradingEngine::getFeeRate() const
{
    return feeRate_;
}

bool TradingEngine::execute(
    const std::string& marketSymbol,
    const std::string& assetSymbol,
    bool isBuy,
    int quantity,
    double limitPrice)
{
    const char* side = isBuy ? "BUY" : "SELL";
    const char* oppositeSide = isBuy ? "SELL" : "BUY";

    double marketPrice = market.getPrice(marketSymbol);

    if(marketPrice == -1)
    {
        cout << "Trading Engine: price not found\n";
        return false;
    }

    // Our order's limit: the market price unless a limit was given.
    double orderPrice = (limitPrice > 0) ? limitPrice : marketPrice;

    // Check everything BEFORE touching the book, so a rejected trade leaves
    // no trace in the order book or the trade log.
    bool ok = (quantity > 0 && marketPrice > 0);
    if(ok && isBuy)
    {
        // Fills happen at the market price plus the fee: the most we can pay.
        ok = (portfolio.getCash() >= marketPrice * (1 + feeRate_) * quantity);
    }
    else if(ok)
    {
        ok = (portfolio.getQuantity(assetSymbol) >= quantity);
    }

    int filled = 0;
    double filledValue = 0.0;

    if(ok)
    {
        int marketMakerId = nextMarketMakerId++;
        int ourId = nextOrderId++;

        // 1. The market maker offers the full quantity at the market price.
        orderBook.addOrder(
            Order(marketMakerId, marketSymbol, marketPrice, quantity,
                  oppositeSide));

        // Only trades from here on can involve our order.
        size_t firstTrade = orderBook.trades().size();

        // 2. Our order goes through the real matching engine.
        orderBook.addOrder(
            Order(ourId, marketSymbol, orderPrice, quantity, side));

        // 3. Read the actual fills back from the trade log.
        const vector<Trade>& log = orderBook.trades();
        for(size_t i = firstTrade; i < log.size(); i++)
        {
            int ourSideId = isBuy ? log[i].buyOrderId : log[i].sellOrderId;
            if(ourSideId == ourId)
            {
                filled += log[i].quantity;
                filledValue += log[i].quantity * log[i].price;
            }
        }

        // Immediate-or-cancel: whatever did not trade must not stay in the book.
        orderBook.removeOrder(ourId);
        orderBook.removeOrder(marketMakerId);
    }

    bool success = false;
    if(filled > 0)
    {
        // The portfolio sees the price including the fee: a buy costs a
        // little more, a sell brings in a little less.
        double averagePrice = filledValue / filled;
        double effectivePrice = isBuy
            ? averagePrice * (1 + feeRate_)
            : averagePrice * (1 - feeRate_);

        success = isBuy
            ? portfolio.buy(assetSymbol, effectivePrice, filled)
            : portfolio.sell(assetSymbol, effectivePrice, filled);
    }

    if(success)
    {
        cout << "Trading Engine: " << side << " executed for "
             << marketSymbol << endl;
    }
    else
    {
        cout << "Trading Engine: " << side << " failed for "
             << marketSymbol << endl;
    }

    return success;
}

bool TradingEngine::executeBuy(
    const std::string& symbol,
    int quantity,
    double limitPrice)
{
    return execute(symbol, symbol, true, quantity, limitPrice);
}

bool TradingEngine::executeSell(
    const std::string& symbol,
    int quantity,
    double limitPrice)
{
    return execute(symbol, symbol, false, quantity, limitPrice);
}

bool TradingEngine::executeArbitrage(const ArbitrageOpportunity& opportunity)
{
    if(!opportunity.found)
    {
        return false;
    }

    cout << "Trading Engine: ARBITRAGE " << opportunity.asset
         << ": buy on " << opportunity.buyVenue
         << " at " << opportunity.buyPrice
         << ", sell on " << opportunity.sellVenue
         << " at " << opportunity.sellPrice << endl;

    // Leg 1: buy where it is cheap.
    if(!execute(opportunity.buyVenue, opportunity.asset, true,
                opportunity.quantity, 0.0))
    {
        cout << "Trading Engine: ARBITRAGE failed, nothing traded\n";
        return false;
    }

    // Leg 2: sell where it is expensive.
    if(!execute(opportunity.sellVenue, opportunity.asset, false,
                opportunity.quantity, 0.0))
    {
        cout << "Trading Engine: ARBITRAGE leg 2 failed, still holding "
             << opportunity.asset << endl;
        return false;
    }

    cout << "Trading Engine: ARBITRAGE completed, expected profit "
         << opportunity.expectedProfit << endl;
    return true;
}

bool TradingEngine::runArbitrage(const ArbitrageStrategy& strategy)
{
    ArbitrageOpportunity opportunity = strategy.detect(market);

    if(!opportunity.found)
    {
        cout << "Trading Engine: no arbitrage opportunity\n";
        return false;
    }

    return executeArbitrage(opportunity);
}

void TradingEngine::runStrategy(TradingStrategy& strategy)
{
    Signal signal = strategy.generateSignal(market, portfolio);

    if(signal.action == Action::BUY)
    {
        cout << "Trading Engine: BUY signal received\n";
        executeBuy(strategy.getSymbol(), signal.quantity, signal.limitPrice);
    }
    else if(signal.action == Action::SELL)
    {
        cout << "Trading Engine: SELL signal received\n";
        executeSell(strategy.getSymbol(), signal.quantity, signal.limitPrice);
    }
    else
    {
        cout << "Trading Engine: HOLD signal received\n";
    }
}

const MarketData& TradingEngine::getMarket() const
{
    return market;
}

const OrderBook& TradingEngine::getOrderBook() const
{
    return orderBook;
}

const Portfolio& TradingEngine::getPortfolio() const
{
    return portfolio;
}

void TradingEngine::display() const
{
    cout << "\n===== TRADING ENGINE =====\n";

    cout << "\n--- MARKET DATA ---\n";
    market.display();

    cout << "\n--- ORDER BOOK ---\n";
    orderBook.displayOrders();

    cout << "\n--- PORTFOLIO ---\n";
    portfolio.display(market);
}