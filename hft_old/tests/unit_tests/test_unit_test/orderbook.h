// orderbook.h
#ifndef ORDERBOOK_H
#define ORDERBOOK_H

class OrderBook {
public:
    void addOrder(int orderID, double price);
    void removeOrder(int orderID);
    bool orderExists(int orderID) const;
private:
    // Internal structure to store orders (e.g., a map or list)
};

#endif
