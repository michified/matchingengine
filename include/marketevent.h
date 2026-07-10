#pragma once

struct MarketEvent {
    char type; // S: sell, B: buy, X: cancel
    int locationCode;
    int orderId; // independent for each company
    int qty;
    int price;
};