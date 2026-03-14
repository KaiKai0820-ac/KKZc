#include "LinkStore.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <mutex>

using json = nlohmann::json;

// case-insensitive substring search
static bool containsCI(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    auto it = std::search(haystack.begin(), haystack.end(),
                          needle.begin(), needle.end(),
                          [](char a, char b) { return std::tolower((unsigned char)a) == std::tolower((unsigned char)b); });
    return it != haystack.end();
}

static bool isAiItem(const std::string& name) {
    return containsCI(name, "ai");
}

// SAX handler for streaming JSON array of link objects
class LinkSaxHandler : public json::json_sax_t {
public:
    LinkSaxHandler(std::vector<LinkItem>& items, std::shared_mutex& mtx,
                   LinkStore::ProgressCb progress_cb,
                   const std::set<std::string>* skip_urls = nullptr)
        : items_(items), mtx_(mtx), progress_cb_(std::move(progress_cb)), skip_urls_(skip_urls) {}

    bool null() override { return set_value(""); }
    bool boolean(bool) override { return true; }
    bool number_integer(json::number_integer_t) override { return true; }
    bool number_unsigned(json::number_unsigned_t) override { return true; }
    bool number_float(double, const json::string_t&) override { return true; }
    bool string(json::string_t& val) override { return set_value(val); }
    bool binary(json::binary_t&) override { return true; }

    bool start_object(std::size_t) override {
        if (depth_ == 1) { current_ = LinkItem{}; }
        depth_++;
        return true;
    }

    bool end_object() override {
        depth_--;
        if (depth_ == 1 && !current_.quark_url.empty()) {
            // Skip URLs already done
            if (skip_urls_ && skip_urls_->count(current_.quark_url)) {
                skipped_++;
                return true;
            }
            std::unique_lock lk(mtx_);
            items_.push_back(std::move(current_));
            auto sz = items_.size();
            lk.unlock();
            if (sz % 1000 == 0 && progress_cb_) progress_cb_(sz, 0);
        }
        return true;
    }

    bool start_array(std::size_t) override { depth_++; return true; }
    bool end_array() override { depth_--; return true; }
    bool key(json::string_t& key) override { current_key_ = key; return true; }

    bool parse_error(std::size_t, const std::string&, const json::exception& ex) override {
        error_ = ex.what();
        return false;
    }

    const std::string& error() const { return error_; }
    size_t skipped() const { return skipped_; }

private:
    bool set_value(const std::string& val) {
        if (depth_ == 2) {
            if (current_key_ == "\xe6\x97\xa5\xe6\x9c\x9f") current_.date = val;
            else if (current_key_ == "\xe5\x90\x8d\xe7\xa7\xb0") current_.name = val;
            else if (current_key_ == "\xe5\xa4\xb8\xe5\x85\x8b\xe7\xbd\x91\xe7\x9b\x98") current_.quark_url = val;
            else if (current_key_ == "\xe7\x99\xbe\xe5\xba\xa6\xe7\xbd\x91\xe7\x9b\x98") current_.baidu_url = val;
        }
        return true;
    }

    std::vector<LinkItem>& items_;
    std::shared_mutex& mtx_;
    LinkStore::ProgressCb progress_cb_;
    const std::set<std::string>* skip_urls_ = nullptr;
    LinkItem current_;
    std::string current_key_;
    std::string error_;
    size_t skipped_ = 0;
    int depth_ = 0;
};

void LinkStore::loadAsync(const std::string& path, ProgressCb progress_cb, DoneCb done_cb,
                          const std::set<std::string>* skip_urls) {
    std::thread([this, path, progress_cb = std::move(progress_cb), done_cb = std::move(done_cb), skip_urls] {
        {
            std::unique_lock lk(mtx_);
            items_.clear();
            view_.clear();
            items_.reserve(50000);
        }
        std::ifstream ifs(path);
        if (!ifs.is_open()) {
            if (done_cb) done_cb(false, "Cannot open file: " + path);
            return;
        }
        LinkSaxHandler handler(items_, mtx_, progress_cb, skip_urls);
        bool ok = json::sax_parse(ifs, &handler);
        {
            std::unique_lock lk(mtx_);
            rebuildView();
        }
        if (done_cb) done_cb(ok, handler.error());
    }).detach();
}

std::vector<size_t> LinkStore::checkedIndices() const {
    std::shared_lock lk(mtx_);
    std::vector<size_t> result;
    for (auto vi : view_) {
        if (!items_[vi].checked || items_[vi].quark_url.empty()) continue;
        // Skip already completed items
        auto st = items_[vi].status.load();
        if (st == TransferStatus::Success || st == TransferStatus::NoNewFiles) continue;
        result.push_back(vi);
    }
    return result;
}

void LinkStore::setAllChecked(bool v) {
    std::shared_lock lk(mtx_);
    for (auto vi : view_) items_[vi].checked = v;
}

void LinkStore::clear() {
    std::unique_lock lk(mtx_);
    items_.clear();
    view_.clear();
}

void LinkStore::applyFilter(const std::string& keyword, const std::string& tag) {
    std::unique_lock lk(mtx_);
    filter_keyword_ = keyword;
    filter_tag_ = tag;
    rebuildView();
}

void LinkStore::resetFilter() {
    std::unique_lock lk(mtx_);
    filter_keyword_.clear();
    filter_tag_ = "all";
    rebuildView();
}

// must be called with unique_lock held
void LinkStore::rebuildView() {
    view_.clear();
    view_.reserve(items_.size());
    for (size_t i = 0; i < items_.size(); i++) {
        const auto& item = items_[i];
        // tag filter
        if (filter_tag_ == "ai" && !isAiItem(item.name)) continue;
        if (filter_tag_ == "non-ai" && isAiItem(item.name)) continue;
        // keyword filter
        if (!filter_keyword_.empty() && !containsCI(item.name, filter_keyword_)) continue;
        view_.push_back(i);
    }
}
