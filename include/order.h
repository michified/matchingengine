#pragma once

struct Order {
    int orderId;
    int qty;
    int price;

    int prevOrderId = -1;
    int nextOrderId = -1;
};
