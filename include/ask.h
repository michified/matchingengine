#include <bits/stdc++.h>
using namespace std;

#include "order.h"
#include "pricelevel.h"

class AskBook {
private:
    int bestAskPrice;
    int totalVolumeTraded;
    vector<PriceLevel> priceLevels;
    vector<Order> orders;
    vector<int> nextPrices;
    vector<int> prevPrices;

    void removePriceLevel(int price) {
        int prevPrice = prevPrices[price];
        int nextPrice = nextPrices[price];

        if (prevPrice != INT_MAX) {
            nextPrices[prevPrice] = nextPrice;
        }
        if (nextPrice != INT_MAX) {
            prevPrices[nextPrice] = prevPrice;
        }

        nextPrices[price] = INT_MAX;
        prevPrices[price] = INT_MAX;

        if (bestAskPrice == price) {
            bestAskPrice = nextPrice;
        }
    }

public:
    AskBook(int maxPrice, int maxId) {
        bestAskPrice = INT_MAX;
        totalVolumeTraded = 0;
        priceLevels.resize(maxPrice + 1);
        orders.resize(maxId + 1);
        prevPrices.resize(maxPrice + 1, INT_MAX);
        nextPrices.resize(maxPrice + 1, INT_MAX);
    }

    void addOrder(int orderId, int qty, int price) {
        orders[orderId] = {orderId, qty, price, -1, -1};
        PriceLevel& level = priceLevels[price];

        if (level.firstOrderId == -1) {
            level.firstOrderId = orderId;
            level.lastOrderId = orderId;

            if (price < bestAskPrice) {
                nextPrices[price] = bestAskPrice;
                if (bestAskPrice != INT_MAX) {
                    prevPrices[bestAskPrice] = price;
                }
                bestAskPrice = price;
            } else {
                int currentPrice = bestAskPrice;

                while (nextPrices[currentPrice] != INT_MAX and nextPrices[currentPrice] < price) {
                    currentPrice = nextPrices[currentPrice];
                }

                int oldNext = nextPrices[currentPrice];
                nextPrices[price] = oldNext;
                prevPrices[price] = currentPrice;

                if (oldNext != INT_MAX) {
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

    void matchIncomingBid(int bidPrice, int& bidQty) {
        while (bidQty > 0 and bestAskPrice != INT_MAX and bestAskPrice <= bidPrice) {
            PriceLevel& level = priceLevels[bestAskPrice];
            int orderId = level.firstOrderId;

            if (orderId == -1) {
                bestAskPrice = nextPrices[bestAskPrice];
                continue;
            }

            Order& order = orders[orderId];
            int matchQty = min(bidQty, order.qty);
            bidQty -= matchQty;
            order.qty -= matchQty;
            totalVolumeTraded += matchQty;

            if (order.qty == 0) {
                level.firstOrderId = order.nextOrderId;

                if (level.firstOrderId == -1) {
                    level.lastOrderId = -1;
                    removePriceLevel(bestAskPrice);
                    continue;
                }
                orders[level.firstOrderId].prevOrderId = -1;
            }
        }

        if (bestAskPrice != INT_MAX and priceLevels[bestAskPrice].firstOrderId == -1) {
            removePriceLevel(bestAskPrice);
        }
    }

    int getBestAskPrice() const {
        return bestAskPrice;
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