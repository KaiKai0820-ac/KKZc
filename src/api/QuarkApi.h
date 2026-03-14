#pragma once
#include <string>
#include <vector>
#include <utility>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class QuarkApi {
public:
    explicit QuarkApi(std::string cookie);

    json getAccountInfo();
    json getStoken(const std::string& pwd_id, const std::string& passcode = "");
    json getDetail(const std::string& pwd_id, const std::string& stoken,
                   const std::string& pdir_fid = "0");
    json getFids(const std::vector<std::string>& file_paths);
    json mkDir(const std::string& dir_path);
    json lsDir(const std::string& pdir_fid);
    json saveFile(const std::vector<std::string>& fid_list,
                  const std::vector<std::string>& fid_token_list,
                  const std::string& to_pdir_fid,
                  const std::string& pwd_id,
                  const std::string& stoken);
    json queryTask(const std::string& task_id);

    std::string ensureDir(const std::string& path);
    static std::pair<std::string, std::string> parseShareUrl(const std::string& url);

    const std::string& nickname() const { return nickname_; }

private:
    std::string cookie_;
    std::string nickname_;

    static constexpr const char* BASE = "https://drive-pc.quark.cn";
    static constexpr const char* UA =
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) quark-cloud-drive/3.14.2 Chrome/112.0.5615.165 "
        "Electron/24.1.3.8 Safari/537.36 Channel/pckk_other_ch";

    json doGet(const std::string& url, const std::string& query);
    json doPost(const std::string& url, const std::string& query, const json& body);
    static std::string buildQuery(const std::vector<std::pair<std::string,std::string>>& params);
    static std::string urlEncode(const std::string& s);
};
