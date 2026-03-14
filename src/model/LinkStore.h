#pragma once
#include "LinkItem.h"
#include <vector>
#include <string>
#include <functional>
#include <shared_mutex>
#include <thread>
#include <algorithm>
#include <set>

class LinkStore {
public:
    using ProgressCb = std::function<void(size_t loaded, size_t total)>;
    using DoneCb = std::function<void(bool ok, std::string err)>;

    void loadAsync(const std::string& path, ProgressCb progress_cb, DoneCb done_cb,
                   const std::set<std::string>* skip_urls = nullptr);

    // 筛选后的视图大小
    size_t size() const {
        std::shared_lock lk(mtx_);
        return view_.size();
    }

    // 通过视图索引访问
    LinkItem& at(size_t idx) {
        std::shared_lock lk(mtx_);
        return items_[view_[idx]];
    }

    const LinkItem& at(size_t idx) const {
        std::shared_lock lk(mtx_);
        return items_[view_[idx]];
    }

    size_t totalSize() const {
        std::shared_lock lk(mtx_);
        return items_.size();
    }

    std::vector<size_t> checkedIndices() const;
    void setAllChecked(bool v);
    void clear();

    // 筛选：keyword 为空则显示全部，tag 可选 "all"/"ai"/"non-ai"
    void applyFilter(const std::string& keyword, const std::string& tag);
    void resetFilter();

private:
    void rebuildView();

    std::vector<LinkItem> items_;
    std::vector<size_t> view_;  // 筛选后的索引映射
    std::string filter_keyword_;
    std::string filter_tag_ = "all";
    mutable std::shared_mutex mtx_;
};
