#include "TransferWorker.h"
#include <set>

TransferWorker::TransferWorker(QuarkApi& api, std::string save_path, RateLimiter& limiter)
    : api_(api), save_path_(std::move(save_path)), limiter_(limiter) {}

TransferResult TransferWorker::execute(LinkItem& item, std::atomic<bool>& cancel) {
    TransferResult result;

    // Rate limit: wait before starting this transfer
    limiter_.acquire();

    // 1. Parse URL
    auto [pwd_id, passcode] = QuarkApi::parseShareUrl(item.quark_url);
    if (pwd_id.empty()) {
        result.message = "Invalid URL";
        return result;
    }
    if (cancel.load()) return result;

    // 2. Get stoken
    auto token_resp = api_.getStoken(pwd_id, passcode);
    if (!token_resp.contains("code") || token_resp["code"].get<int>() != 0) {
        result.message = token_resp.value("message", "stoken failed");
        return result;
    }
    auto stoken = token_resp["data"]["stoken"].get<std::string>();
    if (cancel.load()) return result;

    // 3. Get share file list
    auto detail = api_.getDetail(pwd_id, stoken);
    if (!detail.contains("code") || detail["code"].get<int>() != 0) {
        result.message = detail.value("message", "detail failed");
        return result;
    }
    auto& share_files = detail["data"]["list"];
    if (share_files.empty()) {
        result.message = "Empty share";
        return result;
    }
    if (cancel.load()) return result;

    // 4. Ensure target dir
    auto to_fid = api_.ensureDir(save_path_);
    if (to_fid.empty()) {
        result.message = "Cannot create dir";
        return result;
    }
    if (cancel.load()) return result;

    // 5. Get existing files for dedup
    auto existing = api_.lsDir(to_fid);
    std::set<std::string> existing_names;
    if (existing.contains("code") && existing["code"].get<int>() == 0) {
        for (auto& f : existing["data"]["list"]) {
            existing_names.insert(f["file_name"].get<std::string>());
        }
    }

    // 6. Filter new files
    std::vector<std::string> fid_list, fid_token_list;
    for (auto& f : share_files) {
        auto fname = f["file_name"].get<std::string>();
        if (existing_names.find(fname) == existing_names.end()) {
            fid_list.push_back(f["fid"].get<std::string>());
            fid_token_list.push_back(f["share_fid_token"].get<std::string>());
        }
    }
    if (fid_list.empty()) {
        result.success = true;
        result.message = "No new files";
        return result;
    }
    if (cancel.load()) return result;

    // 7. Save
    auto save_resp = api_.saveFile(fid_list, fid_token_list, to_fid, pwd_id, stoken);
    if (!save_resp.contains("code") || save_resp["code"].get<int>() != 0) {
        result.message = save_resp.value("message", "save failed");
        return result;
    }
    if (cancel.load()) return result;

    // 8. Poll task
    auto task_id = save_resp["data"]["task_id"].get<std::string>();
    auto task_resp = api_.queryTask(task_id);
    if (task_resp.contains("code") && task_resp["code"].get<int>() == 0) {
        result.success = true;
        result.file_count = (int)fid_list.size();
        result.message = "OK";
    } else {
        result.message = task_resp.value("message", "task failed");
    }
    return result;
}
