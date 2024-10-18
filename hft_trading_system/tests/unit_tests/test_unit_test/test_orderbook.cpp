// test_orderbook.cpp
#include <gtest/gtest.h>
#include "orderbook.h"

TEST(OrderBookTest, AddOrderTest) {
    OrderBook ob;
    ob.addOrder(1, 100.0);
    ASSERT_TRUE(ob.orderExists(1));  // Expect the order to exist after adding it
}

TEST(OrderBookTest, RemoveOrderTest) {
    OrderBook ob;
    ob.addOrder(1, 100.0);
    ob.removeOrder(1);
    ASSERT_FALSE(ob.orderExists(1));  // Expect the order to no longer exist
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
