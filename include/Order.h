/*
Order - represents one Trade Order
Order ID: 101  - orderId
Symbol: BTC  -  symbol
Price: 80000  - price
Quantity: 2  -  quantity
Type: BUY  -  type
*/
#ifndef ORDER_H
#define ORDER_H

#include <string>
using namespace std;

class Order{
    private:
    // we dont want other classes to modify order's internal data
        int orderId;
        string symbol;
        double price;
        int quantity;
        string type;
    public:
        Order(); // default Constructor
        // Create Order with actual information
        Order(int id, const string& symbol,
        double price,int quantity, const string& type);
        // Controlled functions - Encapsulation
        int getOrderId() const;
        string getType() const;
        double getPrice() const;
        int getQuantity() const;
        string getSymbol() const;
        

        void display() const;
};

#endif

// const string & symbol - we dont unnecessarily copy the string
// when passing to the constructor, and const wont modify the original string
// Small Efficiency improvement
// const at the end means this fn is only going to read the object not modify it
// #ifndef ->include guards ->prevent the same header file from being included multiple times
// and causing compilation errors
// We are seperating declaration here and implementation in src/Order.cpp