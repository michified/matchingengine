#include <bits/stdc++.h>
using namespace std;

#include "marketmanager.h"
#include "marketevent.h"

template <class T>
__attribute__((always_inline)) inline void DoNotOptimize(const T& value) {
    asm volatile("" : : "g"(value) : "memory");
}

__attribute__((always_inline)) inline void ClobberMemory() {
    asm volatile("" : : : "memory");
}

string trim_ticker(string str) {
    str.erase(find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
        return not isspace(ch);
    }).base(), str.end());
    return str;
}

int main() {
    const int numCompanies = 10; // default
    ios_base::sync_with_stdio(false);
    
    string filePath = "./src/data.txt";
    ifstream input(filePath);
    if (not input.is_open()) {
        cerr << "Unable to open data.txt" << endl;
        return 1;
    }

    string line;
    unordered_map<string, int> tickerOrderCounts;
    unordered_map<int, string> globalIdToTickerMap;

    getline(input, line); // skip header

    cout << "Pass 1: Scanning file to find top " << numCompanies << " heaviest companies..." << endl;
    while (getline(input, line)) {
        if (line.length() < 9) continue;
        char type = line[8];

        if (type == 'A') {
            int itchOrderId = stoi(line.substr(9, 9));
            string ticker = trim_ticker(line.substr(25, 8));
            
            tickerOrderCounts[ticker]++;
            globalIdToTickerMap[itchOrderId] = ticker;
        } else if (type == 'E') {
            int itchOrderId = stoi(line.substr(9, 9));
            if (globalIdToTickerMap.contains(itchOrderId)) {
                tickerOrderCounts[globalIdToTickerMap[itchOrderId]]++;
            }
        }
    }

    vector<pair<string, int>> sortedCompanies(tickerOrderCounts.begin(), tickerOrderCounts.end());

    // process companies sequentially
    sort(sortedCompanies.begin(), sortedCompanies.end(), [](const auto& a, const auto& b) {
        return a.second > b.second; 
    });

    size_t targetSize = min(sortedCompanies.size(), static_cast<size_t>(numCompanies));
    set<string> top100Tickers;
    unordered_map<string, array<int, 3>> companyMap; 

    for (size_t i = 0; i < targetSize; ++i) {
        string ticker = sortedCompanies[i].first;
        top100Tickers.insert(ticker);
        companyMap[ticker] = {static_cast<int>(i), 0, INT_MAX};
    }

    globalIdToTickerMap.clear();

    input.clear();
    input.seekg(0, ios::beg);
    getline(input, line); 

    unordered_map<int, int> orderToEventIndexMap; 
    unordered_map<int, int> globalToLocalOrderIdMap;
    vector<MarketEvent> events;

    vector<int> locCodeToNextId(targetSize, 1);

    cout << "\nPass 2: Streaming filtered events into RAM..." << endl;
    while (getline(input, line)) {
        if (line.length() < 9) continue;
        char type = line[8];

        switch (type) {
            case 'A': {
                int itchOrderId = stoi(line.substr(9, 9));
                char side        = line[18];
                int qty          = stoi(line.substr(19, 6));
                string ticker    = trim_ticker(line.substr(25, 8));
                int price        = stoi(line.substr(33, 10));

                if (not top100Tickers.contains(ticker)) continue;

                int locCode = companyMap[ticker][0];
                
                companyMap[ticker][1] = max(companyMap[ticker][1], price);
                companyMap[ticker][2] = min(companyMap[ticker][2], price);
                
                int localOrderId = locCodeToNextId[locCode]++;
                globalToLocalOrderIdMap[itchOrderId] = localOrderId;

                MarketEvent currentEvent;
                currentEvent.type = side;
                currentEvent.locationCode = locCode;
                currentEvent.orderId = localOrderId;
                currentEvent.qty = qty;
                currentEvent.price = price;

                orderToEventIndexMap[itchOrderId] = events.size();
                events.push_back(currentEvent);
                break;
            }

            case 'X': {
                int itchOrderId = stoi(line.substr(9, 9));
                int canceledQty = stoi(line.substr(18, 6));

                if (orderToEventIndexMap.find(itchOrderId) != orderToEventIndexMap.end()) {
                    int locCode = events[orderToEventIndexMap[itchOrderId]].locationCode;
                    int localOrderId = globalToLocalOrderIdMap[itchOrderId];

                    MarketEvent currentEvent;
                    currentEvent.type = 'X';
                    currentEvent.locationCode = locCode;
                    currentEvent.orderId = localOrderId;
                    currentEvent.qty = canceledQty;
                    currentEvent.price = 0;

                    events.push_back(currentEvent);
                }
                break;
            }
            
            case 'P': 
                break; 

            case 'E': 
                break;
        }
    }
    input.close();
    cout << "Filtering Complete. Ingested " << events.size() << " target events into RAM safely." << endl;

    MarketManager marketManager(companyMap.size());
    vector<array<int, 3>> companyConfigs(companyMap.size()); 
    
    for (auto& elem : companyMap) {
        int locCode = elem.second[0];
        int maxPrice = elem.second[1];
        int minPrice = (elem.second[2] == INT_MAX) ? 0 : elem.second[2]; 
        int maxId = locCodeToNextId[locCode]; 
        companyConfigs[locCode] = {maxPrice, minPrice, maxId};
    }

    for (size_t i = 0; i < companyConfigs.size(); ++i) {
        marketManager.initializeCompany(i, companyConfigs[i][0], companyConfigs[i][1], companyConfigs[i][2]);
    }

    vector<unsigned long long> cycleLatencies;
    cycleLatencies.resize(events.size());

    cout << "Starting matching engine benchmark over " << events.size() << " events..." << endl;
    sort(events.begin(), events.end(), [](const MarketEvent& a, const MarketEvent& b) {
        return (a.locationCode == b.locationCode) ? (a.orderId < b.orderId) : (a.locationCode < b.locationCode);
    });

    auto wallClockStart = chrono::high_resolution_clock::now();

    for (size_t i = 0; i < events.size(); ++i) {
        unsigned long long elapsedCycles = marketManager.processEvent(events[i]);
        cycleLatencies[i] = elapsedCycles;
    }
    DoNotOptimize(marketManager);
    ClobberMemory();

    auto wallClockEnd = chrono::high_resolution_clock::now();
    auto totalDurationInNs = chrono::duration_cast<chrono::nanoseconds>(wallClockEnd - wallClockStart).count();
    double totalDurationInMs = totalDurationInNs / 1'000'000.0;

    vector<unsigned long long> bidLatencies;
    vector<unsigned long long> askLatencies;
    vector<unsigned long long> cancelLatencies;

    bidLatencies.reserve(events.size());
    askLatencies.reserve(events.size());
    cancelLatencies.reserve(events.size());

    for (size_t i = 0; i < events.size(); ++i) {
        if (events[i].type == 'B') {
            bidLatencies.push_back(cycleLatencies[i]);
        } else if (events[i].type == 'S') {
            askLatencies.push_back(cycleLatencies[i]);
        } else if (events[i].type == 'X') {
            cancelLatencies.push_back(cycleLatencies[i]);
        }
    }

    pair<int, int> passedFailed = marketManager.validateCompanies();

    auto printMetrics = [](const string& name, vector<unsigned long long>& lats) {
        if (lats.empty()) {
            cout << "-----------------------------------------" << endl;
            cout << name << " LATENCIES: No events recorded." << endl;
            return;
        }
        sort(lats.begin(), lats.end());
        unsigned long long minC = lats.front();
        unsigned long long maxC = lats.back();
        unsigned long long p50 = lats[lats.size() * 0.50];
        unsigned long long p95 = lats[lats.size() * 0.95];
        unsigned long long p999 = lats[lats.size() * 0.999];
        
        unsigned long long total = accumulate(lats.begin(), lats.end(), 0ULL);
        double avg = static_cast<double>(total) / lats.size();

        cout << "-----------------------------------------" << endl;
        cout << name << " LATENCIES (" << lats.size() << " events):" << endl;
        cout << "  Min Latency   : " << minC << " cycles" << endl;
        cout << "  Avg Latency   : " << avg << " cycles" << endl;
        cout << "  Median (p50)  : " << p50 << " cycles" << endl;
        cout << "  Tail (p95)    : " << p95 << " cycles" << endl;
        cout << "  Worst (p99.9) : " << p999 << " cycles" << endl;
        cout << "  Max Outlier   : " << maxC << " cycles" << endl;
    };

    double millionsOfEventsPerSecond = (static_cast<double>(events.size()) / totalDurationInNs) * 1000.0;
    double averageNsPerEvent = static_cast<double>(totalDurationInNs) / events.size();

    cout << "\n=========================================" << endl;
    cout << "      MATCHING ENGINE BENCHMARK REPORT    " << endl;
    cout << "=========================================" << endl;
    cout << "Total Operational Runtime : " << totalDurationInMs << " ms" << endl;
    cout << "System Throughput         : " << millionsOfEventsPerSecond << " Million Msg/sec" << endl;
    cout << "Avg Macro Wall Clock Time : " << averageNsPerEvent << " ns per event" << endl;
    
    printMetrics("GLOBAL CPU CYCLE", cycleLatencies);
    printMetrics("BID (BUY)", bidLatencies);
    printMetrics("ASK (SELL)", askLatencies);
    printMetrics("CANCEL (X)", cancelLatencies);

    cout << "-----------------------------------------" << endl;
    cout << "VALIDATION STATE CHECK:" << endl;
    cout << "  Correct Companies      : " << passedFailed.first << endl;
    cout << "  Incorrect Companies    : " << passedFailed.second << endl;
    
    if (passedFailed.second == 0) {
        cout << "  STATUS                 : SUCCESS" << endl;
    } else {
        cout << "  STATUS                 : FAILED" << endl;
    }
    cout << "=========================================" << endl;
    return 0;
}