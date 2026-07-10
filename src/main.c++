#include <bits/stdc++.h>
#include <x86intrin.h>
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
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    const int MAX_COMPANIES_LIMIT = 500;
    string filePath = "./src/data.txt";
    ifstream input(filePath);
    if (not input.is_open()) {
        cerr << "Unable to open data.txt at " << filePath << endl;
        return 1;
    }

    string line;
    unordered_map<string, int> tickerOrderCounts;
    unordered_map<int, string> globalIdToTickerMap;

    getline(input, line); // skip header

    cout << "Pass 1: Parsing file to discover top " << MAX_COMPANIES_LIMIT << " heaviest tickers..." << endl;
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
    sort(sortedCompanies.begin(), sortedCompanies.end(), [](const auto& a, const auto& b) {
        return a.second > b.second; 
    });

    size_t maxCompanies = min(sortedCompanies.size(), static_cast<size_t>(MAX_COMPANIES_LIMIT));
    set<string> topTickers;
    unordered_map<string, array<int, 3>> companyMap; 

    for (size_t i = 0; i < maxCompanies; ++i) {
        string ticker = sortedCompanies[i].first;
        topTickers.insert(ticker);
        companyMap[ticker] = {static_cast<int>(i), 0, INT_MAX}; 
    }

    globalIdToTickerMap.clear();
    input.clear();
    input.seekg(0, ios::beg);
    getline(input, line); 

    unordered_map<int, int> orderToEventIndexMap; 
    unordered_map<int, int> globalToLocalOrderIdMap;
    vector<MarketEvent> globalEvents;
    vector<int> locCodeToNextId(maxCompanies, 1);

    cout << "Pass 2: Streaming filtered events into RAM..." << endl;
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

                if (not topTickers.contains(ticker)) continue;

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

                orderToEventIndexMap[itchOrderId] = globalEvents.size();
                globalEvents.push_back(currentEvent);
                break;
            }
            case 'X': {
                int itchOrderId = stoi(line.substr(9, 9));
                int canceledQty = stoi(line.substr(18, 6));

                if (orderToEventIndexMap.find(itchOrderId) != orderToEventIndexMap.end()) {
                    int locCode = globalEvents[orderToEventIndexMap[itchOrderId]].locationCode;
                    int localOrderId = globalToLocalOrderIdMap[itchOrderId];

                    MarketEvent currentEvent;
                    currentEvent.type = 'X';
                    currentEvent.locationCode = locCode;
                    currentEvent.orderId = localOrderId;
                    currentEvent.qty = canceledQty;
                    currentEvent.price = 0;

                    globalEvents.push_back(currentEvent);
                }
                break;
            }
        }
    }
    input.close();
    cout << "Ingestion complete. Stored " << globalEvents.size() << " events." << endl;

    vector<array<int, 3>> globalCompanyConfigs(maxCompanies); 
    for (auto& elem : companyMap) {
        int locCode = elem.second[0];
        int maxPrice = elem.second[1];
        int minPrice = (elem.second[2] == INT_MAX) ? 0 : elem.second[2]; 
        int maxId = locCodeToNextId[locCode]; 
        globalCompanyConfigs[locCode] = {maxPrice, minPrice, maxId};
    }

    cout << "Sorting global events for contiguous slicing..." << endl;
    sort(globalEvents.begin(), globalEvents.end(), [](const MarketEvent& a, const MarketEvent& b) {
        return (a.locationCode == b.locationCode) ? (a.orderId < b.orderId) : (a.locationCode < b.locationCode);
    });

    ofstream csvOutput("averagetimes.csv", ios::trunc);
    csvOutput << "TestedCompaniesAmount,RunningAvgNs,RunningAvgThroughput_MMsgSec,CumulMedianCycles,CumulP95Cycles,CumulP999Cycles,CumulMaxCycles\n";

    ofstream txtSummary("sweep_summary.txt", ios::trunc);
    txtSummary << "=========================================================\n";
    txtSummary << "    MATCHING ENGINE PERCENTILE PERFORMANCE JOURNAL\n";
    txtSummary << "=========================================================\n";

    cout << "\nStarting Running Average & Percentile Slice Loop..." << endl;

    double runningTotalNsPerEvent = 0.0;
    double runningTotalThroughput = 0.0;
    int validTestsCount = 0;
    size_t currentIndex = 0;
    uint64_t globalMaxCycles = 0;

    vector<uint64_t> runningCyclesTracker;
    runningCyclesTracker.reserve(globalEvents.size()); 

    for (int k = 0; k < static_cast<int>(maxCompanies); ++k) {
        
        size_t startIndex = currentIndex;
        while (currentIndex < globalEvents.size() and globalEvents[currentIndex].locationCode == k) {
            currentIndex++;
        }
        size_t endIndex = currentIndex;
        size_t sliceSize = endIndex - startIndex;

        if (sliceSize == 0) continue;

        MarketManager marketManager(1);
        marketManager.initializeCompany(0, globalCompanyConfigs[k][0], globalCompanyConfigs[k][1], globalCompanyConfigs[k][2]);

        auto startBatch = chrono::high_resolution_clock::now();
        
        for (size_t i = startIndex; i < endIndex; ++i) {
            MarketEvent isolatedEvent = globalEvents[i];
            isolatedEvent.locationCode = 0; 
            
            uint64_t startTsc = __rdtsc();
            marketManager.processEvent(isolatedEvent);
            uint64_t endTsc = __rdtsc();
            
            uint64_t cycles = endTsc - startTsc;
            runningCyclesTracker.push_back(cycles);
            
            if (cycles > globalMaxCycles) {
                globalMaxCycles = cycles;
            }
        }
        DoNotOptimize(marketManager);
        ClobberMemory();
        auto endBatch = chrono::high_resolution_clock::now();

        auto durationNs = chrono::duration_cast<chrono::nanoseconds>(endBatch - startBatch).count();
        double singleAvgNs = static_cast<double>(durationNs) / sliceSize;
        double singleThroughput = (static_cast<double>(sliceSize) / durationNs) * 1000.0;

        validTestsCount++;
        runningTotalNsPerEvent += singleAvgNs;
        runningTotalThroughput += singleThroughput;

        double runningAvgNs = runningTotalNsPerEvent / validTestsCount;
        double runningAvgThroughput = runningTotalThroughput / validTestsCount;

        size_t totalProcessed = runningCyclesTracker.size();
        vector<uint64_t> tempCycles = runningCyclesTracker; // Copy to avoid mutating the master timeline
        
        size_t medIdx  = totalProcessed / 2;
        size_t p95Idx  = static_cast<size_t>(totalProcessed * 0.95);
        size_t p999Idx = static_cast<size_t>(totalProcessed * 0.999);

        nth_element(tempCycles.begin(), tempCycles.begin() + medIdx, tempCycles.end());
        uint64_t cumulMedian = tempCycles[medIdx];

        nth_element(tempCycles.begin() + medIdx, tempCycles.begin() + p95Idx, tempCycles.end());
        uint64_t cumulP95 = tempCycles[p95Idx];

        nth_element(tempCycles.begin() + p95Idx, tempCycles.begin() + p999Idx, tempCycles.end());
        uint64_t cumulP999 = tempCycles[p999Idx];

        csvOutput << validTestsCount << "," << runningAvgNs << "," << runningAvgThroughput << "," 
                  << cumulMedian << "," << cumulP95 << "," << cumulP999 << "," << globalMaxCycles << "\n";
        csvOutput.flush(); 
        
        txtSummary << "Tested Co Rank : " << setw(3) << (k + 1)
                   << " | Events in Slice: " << setw(7) << sliceSize
                   << " | P50 (Median): " << setw(4) << cumulMedian << " cycles"
                   << " | P95: " << setw(4) << cumulP95 << " cycles"
                   << " | P99.9: " << setw(5) << cumulP999 << " cycles"
                   << " | Max: " << setw(7) << globalMaxCycles << " cycles\n";
        txtSummary.flush(); 

        if ((k + 1) % 25 == 0 or (k + 1) == 1 or (k + 1) == static_cast<int>(maxCompanies)) {
            cout << "  -> Progress: " << setw(3) << (k + 1) << " Cos | Running Avg Latency: " 
                 << fixed << setprecision(2) << runningAvgNs << " ns | Cumul p99.9: " << cumulP999 << " cycles" << endl;
        }
    }

    csvOutput.close();
    txtSummary.close();

    cout << "\nSweep complete! Percentile metrics successfully generated." << endl;
    return 0;
}