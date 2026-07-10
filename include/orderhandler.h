#pragma once

#include "ask.h"
#include "bid.h"

class OrderHandler {
private:
    BidBook bids;
    AskBook asks;
    int maxPrice;
    int maxId;
    vector<bool> orderInAsk;
    
    int totalInjected = 0;
    int totalCancelled = 0;

public:    
    OrderHandler(int maxPrice, int maxId) : bids(maxPrice, maxId), asks(maxPrice, maxId) {
        this->maxPrice = maxPrice;
        this->maxId = maxId;
        orderInAsk.resize(maxId + 1, false);
    }

    void buy(int orderId, int qty, int price) {
        totalInjected += qty;
        asks.matchIncomingBid(price, qty);
        if (qty > 0) {
            bids.addOrder(orderId, qty, price);
            orderInAsk[orderId] = false;
        }
    }

    void sell(int orderId, int qty, int price) {
        totalInjected += qty;
        bids.matchIncomingAsk(price, qty);
        if (qty > 0) {
            asks.addOrder(orderId, qty, price);
            orderInAsk[orderId] = true;
        }
    }

    int cancel(int orderId, int qty) {
        int cancelled = 0;
        if (orderInAsk[orderId]) {
            cancelled = asks.removeOrder(orderId, qty);
        } else {
            cancelled = bids.removeOrder(orderId, qty);
        }
        totalCancelled += cancelled;
        return cancelled;
    }

    array<int, 8> getStats() {
        return {
            asks.getBestAskPrice(), 
            bids.getBestBidPrice(), 
            asks.getTotalVolumeTraded(), 
            bids.getTotalVolumeTraded(), 
            asks.getRemainingVolume(), 
            bids.getRemainingVolume(),
            totalCancelled,
            totalInjected
        };
    }
};