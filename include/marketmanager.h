#pragma once

#include <bits/stdc++.h>
#include <x86intrin.h>
using namespace std;

#include "orderhandler.h"
#include "marketevent.h"

class MarketManager {
private:
    vector<OrderHandler> companies;
    vector<int> minPrices;
    int correctExecutions = 0;
    int wrongExecutions = 0;

public:
    MarketManager(int numCompanies) {
        companies.reserve(numCompanies);
        minPrices.reserve(numCompanies);
    }

    void initializeCompany(int companyId, int maxPrice, int minPrice, int maxId) {
        if (companyId != (int)companies.size()) return;
        companies.push_back(OrderHandler(maxPrice - minPrice + 1, maxId));
        minPrices.push_back(minPrice);
    }

    unsigned long long processEvent(MarketEvent& event) { 
        OrderHandler& orderHandler = companies[event.locationCode];
        unsigned long long startTime = __rdtsc();

        switch (event.type) {
            case 'B':
                orderHandler.buy(event.orderId, event.qty, event.price - minPrices[event.locationCode]);
                break;
            case 'S':
                orderHandler.sell(event.orderId, event.qty, event.price - minPrices[event.locationCode]);
                break;
            case 'X':
                orderHandler.cancel(event.orderId, event.qty);
                break;
        }

        unsigned long long endTime = __rdtsc();
        return endTime - startTime;
    }

    pair<int, int> getExecutionStats() const {
        return {correctExecutions, wrongExecutions};
    }

    pair<int, int> validateCompanies() {
        int companiesPassed = 0;
        int companiesFailed = 0;
        int i = 0;

        for (OrderHandler& orderHandler : companies) {
            array<int, 8> stats = orderHandler.getStats();
            
            int bestAsk = stats[0];
            int bestBid = stats[1];
            int askTraded = stats[2];
            int bidTraded = stats[3];
            int askRemaining = stats[4];
            int bidRemaining = stats[5];
            int totalCancelled = stats[6];
            int totalInjected = stats[7];

            int totalTraded = askTraded + bidTraded;
            int totalRemaining = askRemaining + bidRemaining;

            bool spreadInvalid = (bestAsk <= bestBid);
            bool negativeQty = (askRemaining == -1 or bidRemaining == -1);
            bool mathFails = (totalRemaining + totalCancelled + (2 * totalTraded) != totalInjected);

            if (spreadInvalid or negativeQty or mathFails) {
                companiesFailed++;
                cout << "Company #" << i << ": ";
                if (spreadInvalid) cout << "ask <= bid | ";
                if (negativeQty) cout << "negative qty | ";
                if (mathFails) {
                    cout << "Math fails! Injected: " << totalInjected 
                         << " != " << (totalRemaining + totalCancelled + (2 * totalTraded))
                         << " (Rem: " << totalRemaining 
                         << " + Canc: " << totalCancelled 
                         << " + 2xTrd: " << (2 * totalTraded) << ")";
                }
                cout << endl;
            } else {
                companiesPassed++;
            }
            i++;
        }

        return {companiesPassed, companiesFailed};
    }
};