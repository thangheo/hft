#ifndef MOCK_EXCHANGE_ADAPTER_H
#define MOCK_EXCHANGE_ADAPTER_H

#include "../adapters/exchange_adapter.h"  // Assuming this is the base interface path
#include <gmock/gmock.h>

class MockExchangeAdapter : public IExchangeAdapter {
public:
    MOCK_METHOD(bool, connect, (), (override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(void, subscribe, (const std::string& symbol), (override));
    MOCK_METHOD(void, unsubscribe, (const std::string& symbol), (override));
    MOCK_METHOD(void, setDataCallback, (const std::function<void(const std::string&)>& callback), (override));  // Added missing method
};

#endif // MOCK_EXCHANGE_ADAPTER_H
