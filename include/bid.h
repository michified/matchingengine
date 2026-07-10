#include <bits/stdc++.h>
using namespace std;

#include "order.h"
#include "pricelevel.h"

class BidBook {
private:
    int bestBidPrice;
    int totalVolumeTraded;
    vector<PriceLevel> priceLevels;
    vector<Order> orders;
    vector<int> nextPrices;
    vector<int> prevPrices;

    void removePriceLevel(int price) {
        int prevPrice = prevPrices[price];
        int nextPrice = nextPrices[price];

        if (prevPrice != INT_MIN) {
            nextPrices[prevPrice] = nextPrice;
        }
        if (nextPrice != INT_MIN) {
            prevPrices[nextPrice] = prevPrice;
        }

        nextPrices[price] = INT_MIN;
        prevPrices[price] = INT_MIN;

        if (bestBidPrice == price) {
            bestBidPrice = nextPrice;
        }
    }

public:
    BidBook(int maxPrice, int maxId) {
        bestBidPrice = INT_MIN;
        totalVolumeTraded = 0;
        priceLevels.resize(maxPrice + 1);
        orders.resize(maxId + 1);
        prevPrices.resize(maxPrice + 1, INT_MIN);
        nextPrices.resize(maxPrice + 1, INT_MIN);
    }

    void addOrder(int orderId, int qty, int price) {
        orders[orderId] = {orderId, qty, price, -1, -1};
        PriceLevel& level = priceLevels[price];

        if (level.firstOrderId == -1) {
            level.firstOrderId = orderId;
            level.lastOrderId = orderId;

            if (price > bestBidPrice) {
                nextPrices[price] = bestBidPrice;
                if (bestBidPrice != INT_MIN) {
                    prevPrices[bestBidPrice] = price;
                }
                bestBidPrice = price;
            } else {
                int currentPrice = bestBidPrice;

                while (nextPrices[currentPrice] != INT_MIN and nextPrices[currentPrice] > price) {
                    currentPrice = nextPrices[currentPrice];
                }

                int oldNext = nextPrices[currentPrice];
                nextPrices[price] = oldNext;
                prevPrices[price] = currentPrice;
                
                if (oldNext != INT_MIN) {
                    prevPrices[oldNext] = price;
                }
                nextPrices[currentPrice] = price;
            }
        } else {
            int lastId = level.lastOrderId;
            orders[lastId].nextOrderId = orderId;
            orders[orderId].prevOrderId = lastId;
            level.lastOrderId = orderId;
        }
    }

    int removeOrder(int orderId, int qty) {
        Order& order = orders[orderId];
        if (order.qty <= 0) return 0;
        
        int qtyToRemove = min(order.qty, qty);
        order.qty -= qtyToRemove;

        if (order.qty > 0) return qtyToRemove;

        PriceLevel& level = priceLevels[order.price];

        if (orderId == level.firstOrderId and orderId == level.lastOrderId) {
            level.firstOrderId = -1;
            level.lastOrderId = -1;
        }
        else if (orderId == level.firstOrderId) {
            level.firstOrderId = order.nextOrderId;
            if (level.firstOrderId != -1) {
                orders[level.firstOrderId].prevOrderId = -1;
            }
        }
        else if (orderId == level.lastOrderId) {
            level.lastOrderId = order.prevOrderId;
            if (level.lastOrderId != -1) {
                orders[level.lastOrderId].nextOrderId = -1;
            }
        }
        else {
            if (order.prevOrderId != -1) orders[order.prevOrderId].nextOrderId = order.nextOrderId;
            if (order.nextOrderId != -1) orders[order.nextOrderId].prevOrderId = order.prevOrderId;
        }

        if (level.firstOrderId == -1 and level.lastOrderId == -1) {
            removePriceLevel(order.price);
        }
        
        return qtyToRemove;
    }

    void matchIncomingAsk(int askPrice, int& askQty) {
        while (askQty > 0 and bestBidPrice != INT_MIN and bestBidPrice >= askPrice) {
            PriceLevel& level = priceLevels[bestBidPrice];
            int orderId = level.firstOrderId;

            if (orderId == -1) {   
                bestBidPrice = nextPrices[bestBidPrice];
                continue;
            }

            Order& order = orders[orderId];
            int matchQty = min(askQty, order.qty);
            askQty -= matchQty;
            order.qty -= matchQty;
            totalVolumeTraded += matchQty;

            if (order.qty == 0) {
                level.firstOrderId = order.nextOrderId;

                if (level.firstOrderId == -1) {
                    level.lastOrderId = -1;
                    removePriceLevel(bestBidPrice);
                    continue;
                }
                orders[level.firstOrderId].prevOrderId = -1;
            }
        }

        if (bestBidPrice != INT_MIN and priceLevels[bestBidPrice].firstOrderId == -1) {
            removePriceLevel(bestBidPrice);
        }
    }

    int getBestBidPrice() const {
        return bestBidPrice;
    }

    int getTotalVolumeTraded() const {
        return totalVolumeTraded;
    }

    int getRemainingVolume() { // -1 if there is a negative qty
        int remainingVolume = 0;
        for (Order& order : orders) {
            if (order.qty < 0) return -1;
            remainingVolume += order.qty;
        }
        return remainingVolume;
    }
};