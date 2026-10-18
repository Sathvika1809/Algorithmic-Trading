#include "../include/ArbitrageStrategy.h"

using namespace std;

ArbitrageStrategy::ArbitrageStrategy(
    const string& asset,
    const string& venueA,
    const string& venueB,
    double feeRate,
    double minProfitPerUnit,
    int quantity)
    : asset(asset),
      venueA(venueA),
      venueB(venueB),
      feeRate(feeRate > 0 ? feeRate : 0.0),
      minProfit(minProfitPerUnit > 0 ? minProfitPerUnit : 0.0),
      quantity(quantity > 0 ? quantity : 1)
{
}

ArbitrageOpportunity ArbitrageStrategy::detect(const MarketData& market) const
{
    ArbitrageOpportunity none; // found == false

    double priceA = market.getPrice(venueA);
    double priceB = market.getPrice(venueB);

    // -1 means "unknown symbol"; zero or less is not a usable price either.
    if(priceA <= 0 || priceB <= 0)
    {
        return none;
    }

    // Fees are paid on both legs: buying costs a little more than the
    // quoted price, selling brings in a little less.
    double profitBuyAsellB = priceB * (1 - feeRate) - priceA * (1 + feeRate);
    double profitBuyBsellA = priceA * (1 - feeRate) - priceB * (1 + feeRate);

    ArbitrageOpportunity best;
    best.asset = asset;
    best.quantity = quantity;

    if(profitBuyAsellB >= profitBuyBsellA)
    {
        best.buyVenue = venueA;
        best.sellVenue = venueB;
        best.buyPrice = priceA;
        best.sellPrice = priceB;
        best.profitPerUnit = profitBuyAsellB;
    }
    else
    {
        best.buyVenue = venueB;
        best.sellVenue = venueA;
        best.buyPrice = priceB;
        best.sellPrice = priceA;
        best.profitPerUnit = profitBuyBsellA;
    }

    if(best.profitPerUnit <= minProfit)
    {
        return none;
    }

    best.found = true;
    best.expectedProfit = best.profitPerUnit * quantity;
    return best;
}

const string& ArbitrageStrategy::getAsset() const
{
    return asset;
}

double ArbitrageStrategy::getFeeRate() const
{
    return feeRate;
}

int ArbitrageStrategy::getQuantity() const
{
    return quantity;
}