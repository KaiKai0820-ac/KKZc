#include "QuarkApi.h"
#include <curl/curl.h>
#include <regex>
#include <random>
#include <chrono>
#include <thread>
#include <sstream>

static size_t writeCallback(char* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(ptr, size * nmemb);
    return size * nmemb;
}

static CURL* getThreadCurl() {
    thread_local CURL* curl = [] {
        CURL* c = curl_easy_init();
        curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(c, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 1L);
        return c;
    }();
    return curl;
}

QuarkApi::QuarkApi(std::string cookie) : cookie_(std::move(cookie)) {}

std::string QuarkApi::urlEncode(const std::string& s) {
    CURL* c = curl_easy_init();
    char* out = curl_easy_escape(c, s.c_str(), (int)s.size());
    std::string result(out);
    curl_free(out);
    curl_easy_cleanup(c);
    return result;
}

std::string QuarkApi::buildQuery(const std::vector<std::pair<std::string,std::string>>& params) {
    std::string q;
    for (auto& [k, v] : params) {
        if (!q.empty()) q += "&";
        q += k + "=" + urlEncode(v);
    }
    return q;
}

json QuarkApi::doGet(const std::string& url, const std::string& query) {
    CURL* curl = getThreadCurl();
    std::string full_url = url;
    if (!query.empty()) full_url += "?" + query;

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("cookie: " + cookie_).c_str());
    headers = curl_slist_append(headers, "content-type: application/json");
    headers = curl_slist_append(headers, (std::string("user-agent: ") + UA).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        return json{{"status", 500}, {"code", 1}, {"message", curl_easy_strerror(res)}};
    }
    try { return json::parse(response); }
    catch (...) { return json{{"status", 500}, {"code", 1}, {"message", "JSON parse error"}}; }
}

json QuarkApi::doPost(const std::string& url, const std::string& query, const json& body) {
    CURL* curl = getThreadCurl();
    std::string full_url = url;
    if (!query.empty()) full_url += "?" + query;

    std::string body_str = body.dump();
    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("cookie: " + cookie_).c_str());
    headers = curl_slist_append(headers, "content-type: application/json");
    headers = curl_slist_append(headers, (std::string("user-agent: ") + UA).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body_str.size());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        return json{{"status", 500}, {"code", 1}, {"message", curl_easy_strerror(res)}};
    }
    try { return json::parse(response); }
    catch (...) { return json{{"status", 500}, {"code", 1}, {"message", "JSON parse error"}}; }
}

json QuarkApi::getAccountInfo() {
    auto q = buildQuery({{"fr","pc"},{"platform","pc"}});
    auto resp = doGet("https://pan.quark.cn/account/info", q);
    if (resp.contains("data") && resp["data"].contains("nickname")) {
        nickname_ = resp["data"]["nickname"].get<std::string>();
    }
    return resp;
}

json QuarkApi::getStoken(const std::string& pwd_id, const std::string& passcode) {
    auto q = buildQuery({{"pr","ucpro"},{"fr","pc"}});
    return doPost(std::string(BASE) + "/1/clouddrive/share/sharepage/token", q,
                  {{"pwd_id", pwd_id}, {"passcode", passcode}});
}

json QuarkApi::getDetail(const std::string& pwd_id, const std::string& stoken,
                         const std::string& pdir_fid) {
    json merged_resp;
    std::vector<json> all_files;
    int page = 1;
    while (true) {
        auto q = buildQuery({
            {"pr","ucpro"},{"fr","pc"},{"pwd_id",pwd_id},{"stoken",stoken},
            {"pdir_fid",pdir_fid},{"force","0"},{"_page",std::to_string(page)},
            {"_size","50"},{"_fetch_banner","0"},{"_fetch_share","0"},
            {"_fetch_total","1"},{"_sort","file_type:asc,updated_at:desc"}
        });
        auto resp = doGet(std::string(BASE) + "/1/clouddrive/share/sharepage/detail", q);
        if (!resp.contains("code") || resp["code"].get<int>() != 0) return resp;
        auto& list = resp["data"]["list"];
        if (list.empty()) { merged_resp = resp; break; }
        for (auto& f : list) all_files.push_back(std::move(f));
        int total = resp["metadata"]["_total"].get<int>();
        if ((int)all_files.size() >= total) { merged_resp = resp; break; }
        page++;
    }
    merged_resp["data"]["list"] = std::move(all_files);
    return merged_resp;
}

json QuarkApi::getFids(const std::vector<std::string>& file_paths) {
    auto q = buildQuery({{"pr","ucpro"},{"fr","pc"},{"uc_param_str",""}});
    return doPost(std::string(BASE) + "/1/clouddrive/file/info/path_list", q,
                  {{"file_path", file_paths}, {"namespace", "0"}});
}

json QuarkApi::mkDir(const std::string& dir_path) {
    auto q = buildQuery({{"pr","ucpro"},{"fr","pc"},{"uc_param_str",""}});
    return doPost(std::string(BASE) + "/1/clouddrive/file", q,
                  {{"pdir_fid","0"},{"file_name",""},{"dir_path",dir_path},{"dir_init_lock",false}});
}

json QuarkApi::lsDir(const std::string& pdir_fid) {
    json merged_resp;
    std::vector<json> all_files;
    int page = 1;
    while (true) {
        auto q = buildQuery({
            {"pr","ucpro"},{"fr","pc"},{"uc_param_str",""},{"pdir_fid",pdir_fid},
            {"_page",std::to_string(page)},{"_size","50"},{"_fetch_total","1"},
            {"_fetch_sub_dirs","0"},{"_sort","file_type:asc,updated_at:desc"}
        });
        auto resp = doGet(std::string(BASE) + "/1/clouddrive/file/sort", q);
        if (!resp.contains("code") || resp["code"].get<int>() != 0) return resp;
        auto& list = resp["data"]["list"];
        if (list.empty()) { merged_resp = resp; break; }
        for (auto& f : list) all_files.push_back(std::move(f));
        int total = resp["metadata"]["_total"].get<int>();
        if ((int)all_files.size() >= total) { merged_resp = resp; break; }
        page++;
    }
    merged_resp["data"]["list"] = std::move(all_files);
    return merged_resp;
}

json QuarkApi::saveFile(const std::vector<std::string>& fid_list,
                        const std::vector<std::string>& fid_token_list,
                        const std::string& to_pdir_fid,
                        const std::string& pwd_id,
                        const std::string& stoken) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(1.0, 5.0);
    int dt = (int)(dis(gen) * 60 * 1000);
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    auto q = buildQuery({
        {"pr","ucpro"},{"fr","pc"},{"uc_param_str",""},{"app","clouddrive"},
        {"__dt",std::to_string(dt)},{"__t",std::to_string(ts)}
    });
    return doPost(std::string(BASE) + "/1/clouddrive/share/sharepage/save", q, {
        {"fid_list", fid_list}, {"fid_token_list", fid_token_list},
        {"to_pdir_fid", to_pdir_fid}, {"pwd_id", pwd_id},
        {"stoken", stoken}, {"pdir_fid", "0"}, {"scene", "link"}
    });
}

json QuarkApi::queryTask(const std::string& task_id) {
    int retry = 0;
    while (true) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(1.0, 5.0);
        int dt = (int)(dis(gen) * 60 * 1000);
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

        auto q = buildQuery({
            {"pr","ucpro"},{"fr","pc"},{"uc_param_str",""},
            {"task_id",task_id},{"retry_index",std::to_string(retry)},
            {"__dt",std::to_string(dt)},{"__t",std::to_string(ts)}
        });
        auto resp = doGet(std::string(BASE) + "/1/clouddrive/task", q);
        if (!resp.contains("status") || resp["status"].get<int>() != 200) return resp;
        if (resp["data"]["status"].get<int>() == 2) return resp;
        retry++;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

std::string QuarkApi::ensureDir(const std::string& path) {
    if (path == "/") return "0";
    auto resp = getFids({path});
    if (resp.contains("code") && resp["code"].get<int>() == 0 && !resp["data"].empty()) {
        return resp["data"][0]["fid"].get<std::string>();
    }
    auto mk = mkDir(path);
    if (mk.contains("code") && mk["code"].get<int>() == 0) {
        return mk["data"]["fid"].get<std::string>();
    }
    return "";
}

std::pair<std::string, std::string> QuarkApi::parseShareUrl(const std::string& url) {
    std::string pwd_id, passcode;
    std::regex id_re(R"(/s/(\w+))");
    std::regex pwd_re(R"(pwd=(\w+))");
    std::smatch m;
    if (std::regex_search(url, m, id_re)) pwd_id = m[1].str();
    if (std::regex_search(url, m, pwd_re)) passcode = m[1].str();
    return {pwd_id, passcode};
}
