#pragma once
#include <string>
#include <atomic>
#include <cstdint>

enum class TransferStatus : uint8_t {
    Pending,
    InProgress,
    Success,
    NoNewFiles,
    Failed,
    Skipped
};

struct LinkItem {
    std::string date;
    std::string name;
    std::string quark_url;
    std::string baidu_url;
    bool checked = true;
    std::atomic<TransferStatus> status{TransferStatus::Pending};
    std::string status_detail;

    LinkItem() = default;
    LinkItem(std::string d, std::string n, std::string q, std::string b)
        : date(std::move(d)), name(std::move(n)), quark_url(std::move(q)), baidu_url(std::move(b)) {}
    LinkItem(LinkItem&& o) noexcept
        : date(std::move(o.date)), name(std::move(o.name)),
          quark_url(std::move(o.quark_url)), baidu_url(std::move(o.baidu_url)),
          checked(o.checked), status(o.status.load()), status_detail(std::move(o.status_detail)) {}
    LinkItem& operator=(LinkItem&& o) noexcept {
        date = std::move(o.date); name = std::move(o.name);
        quark_url = std::move(o.quark_url); baidu_url = std::move(o.baidu_url);
        checked = o.checked; status.store(o.status.load());
        status_detail = std::move(o.status_detail);
        return *this;
    }
};
