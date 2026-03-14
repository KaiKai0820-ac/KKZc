#pragma once
#include "api/QuarkApi.h"
#include "model/LinkItem.h"
#include "RateLimiter.h"
#include <atomic>
#include <string>

struct TransferResult {
    bool success = false;
    int file_count = 0;
    std::string message;
};

class TransferWorker {
public:
    TransferWorker(QuarkApi& api, std::string save_path, RateLimiter& limiter);
    TransferResult execute(LinkItem& item, std::atomic<bool>& cancel);

private:
    QuarkApi& api_;
    std::string save_path_;
    RateLimiter& limiter_;
};
