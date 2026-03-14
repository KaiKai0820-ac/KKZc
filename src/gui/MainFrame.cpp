#include "MainFrame.h"
#include "worker/TransferWorker.h"
#include <wx/filedlg.h>
#include <fstream>
#include <chrono>
#include <ctime>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

enum {
    ID_LOGIN = wxID_HIGHEST + 100,
    ID_BROWSE, ID_LOAD_JSON, ID_SELECT_ALL,
    ID_START, ID_STOP,
    ID_FILTER_INPUT, ID_FILTER_TAG, ID_RATE_CHOICE,
    ID_LOAD_COOKIE, ID_EXPORT_LOG
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_BUTTON(ID_LOGIN, MainFrame::OnLogin)
    EVT_BUTTON(ID_BROWSE, MainFrame::OnBrowse)
    EVT_BUTTON(ID_LOAD_JSON, MainFrame::OnLoadJson)
    EVT_BUTTON(ID_SELECT_ALL, MainFrame::OnSelectAll)
    EVT_BUTTON(ID_START, MainFrame::OnStart)
    EVT_BUTTON(ID_STOP, MainFrame::OnStop)
    EVT_BUTTON(ID_LOAD_COOKIE, MainFrame::OnLoadCookieFile)
    EVT_BUTTON(ID_EXPORT_LOG, MainFrame::OnExportLog)
    EVT_TEXT(ID_FILTER_INPUT, MainFrame::OnFilterChanged)
    EVT_CHOICE(ID_FILTER_TAG, MainFrame::OnFilterChanged)
wxEND_EVENT_TABLE()

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, wxT("\x5938\x514b\x7f51\x76d8\x6279\x91cf\x8f6c\x5b58"),
              wxDefaultPosition, wxSize(950, 720)) {
    BuildUI();
    LoadDoneUrls();
    LoadCookie();
    Centre();
}

void MainFrame::BuildUI() {
    auto* panel = new wxPanel(this);
    auto* vbox = new wxBoxSizer(wxVERTICAL);

    // Cookie
    auto* ck_box = new wxStaticBoxSizer(wxHORIZONTAL, panel, wxT("\x8d26\x53f7\x8bbe\x7f6e"));
    cookie_input_ = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    auto* btn_load_ck = new wxButton(panel, ID_LOAD_COOKIE, wxT("\x5bfc\x5165"), wxDefaultPosition, wxSize(60, -1)); // 导入
    auto* btn_login = new wxButton(panel, ID_LOGIN, wxT("\x9a8c\x8bc1\x767b\x5f55"), wxDefaultPosition, wxSize(90, -1));
    lbl_account_ = new wxStaticText(panel, wxID_ANY, wxT("\x672a\x767b\x5f55"));
    ck_box->Add(cookie_input_, 1, wxEXPAND | wxRIGHT, 5);
    ck_box->Add(btn_load_ck, 0, wxRIGHT, 5);
    ck_box->Add(btn_login, 0, wxRIGHT, 5);
    ck_box->Add(lbl_account_, 0, wxALIGN_CENTER_VERTICAL);
    vbox->Add(ck_box, 0, wxEXPAND | wxALL, 8);

    // Save path
    auto* path_box = new wxStaticBoxSizer(wxHORIZONTAL, panel, wxT("\x4fdd\x5b58\x8def\x5f84"));
    path_input_ = new wxTextCtrl(panel, wxID_ANY, wxT("/\x6765\x81ea\xff1a\x5206\x4eab"));
    auto* btn_browse = new wxButton(panel, ID_BROWSE, wxT("\x6d4f\x89c8..."), wxDefaultPosition, wxSize(70, -1));
    path_box->Add(path_input_, 1, wxEXPAND | wxRIGHT, 5);
    path_box->Add(btn_browse, 0);
    vbox->Add(path_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Filter bar
    auto* filter_box = new wxStaticBoxSizer(wxHORIZONTAL, panel, wxT("\x7b5b\x9009")); // 筛选
    filter_box->Add(new wxStaticText(panel, wxID_ANY, wxT("\x5206\x7c7b:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 3); // 分类:
    wxString tags[] = {wxT("\x5168\x90e8"), wxT("AI \x77ed\x5267"), wxT("\x975e AI")}; // 全部, AI 短剧, 非 AI
    filter_tag_ = new wxChoice(panel, ID_FILTER_TAG, wxDefaultPosition, wxSize(100, -1), 3, tags);
    filter_tag_->SetSelection(0);
    filter_box->Add(filter_tag_, 0, wxRIGHT, 10);
    filter_box->Add(new wxStaticText(panel, wxID_ANY, wxT("\x641c\x7d22:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 3); // 搜索:
    filter_input_ = new wxTextCtrl(panel, ID_FILTER_INPUT, wxEmptyString, wxDefaultPosition, wxSize(200, -1));
    filter_box->Add(filter_input_, 1, wxEXPAND);
    vbox->Add(filter_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Link list toolbar
    auto* list_box = new wxStaticBoxSizer(wxVERTICAL, panel, wxT("\x8f6c\x5b58\x5217\x8868"));
    auto* toolbar = new wxBoxSizer(wxHORIZONTAL);
    toolbar->Add(new wxButton(panel, ID_LOAD_JSON, wxT("\x5bfc\x5165 links.json")), 0, wxRIGHT, 5);
    toolbar->Add(new wxButton(panel, ID_SELECT_ALL, wxT("\x5168\x9009/\x53d6\x6d88")), 0, wxRIGHT, 10);
    lbl_count_ = new wxStaticText(panel, wxID_ANY, wxT("\x5171 0 \x6761"));
    toolbar->Add(lbl_count_, 0, wxALIGN_CENTER_VERTICAL);
    toolbar->AddStretchSpacer();

    // Rate control
    toolbar->Add(new wxStaticText(panel, wxID_ANY, wxT("\x8f6c\x5b58\x901f\x5ea6:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 3); // 转存速度:
    wxString rates[] = {
        wxT("\x706b\x529b\x5168\x5f00"),  // 火力全开
        wxT("20\x90e8/\x5206\x949f"),     // 20部/分钟
        wxT("30\x90e8/\x5206\x949f"),     // 30部/分钟
        wxT("10\x90e8/\x5206\x949f"),     // 10部/分钟
        wxT("5\x90e8/\x5206\x949f"),      // 5部/分钟
    };
    rate_choice_ = new wxChoice(panel, ID_RATE_CHOICE, wxDefaultPosition, wxSize(120, -1), 5, rates);
    rate_choice_->SetSelection(0);
    toolbar->Add(rate_choice_, 0, wxRIGHT, 10);

    toolbar->Add(new wxStaticText(panel, wxID_ANY, wxT("\x5e76\x53d1:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 3); // 并发:
    spin_threads_ = new wxSpinCtrl(panel, wxID_ANY, "4", wxDefaultPosition, wxSize(55, -1), wxSP_ARROW_KEYS, 1, 16, 4);
    toolbar->Add(spin_threads_, 0, wxRIGHT, 10);

    toolbar->Add(new wxStaticText(panel, wxID_ANY, wxT("\x6570\x91cf:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 3); // 数量:
    spin_limit_ = new wxSpinCtrl(panel, wxID_ANY, "0", wxDefaultPosition, wxSize(70, -1), wxSP_ARROW_KEYS, 0, 99999, 0);
    spin_limit_->SetToolTip(wxT("0 = \x5168\x90e8\x8f6c\x5b58")); // 0 = 全部转存
    toolbar->Add(spin_limit_, 0);

    list_box->Add(toolbar, 0, wxEXPAND | wxBOTTOM, 5);

    list_ctrl_ = new VirtualLinkList(panel, store_);
    list_box->Add(list_ctrl_, 1, wxEXPAND);
    vbox->Add(list_box, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);

    // Bottom: log + buttons
    auto* bottom = new wxBoxSizer(wxHORIZONTAL);
    log_text_ = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                               wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL);
    log_text_->SetMinSize(wxSize(-1, 100));
    bottom->Add(log_text_, 1, wxEXPAND | wxRIGHT, 5);

    auto* btn_panel = new wxBoxSizer(wxVERTICAL);
    btn_start_ = new wxButton(panel, ID_START, wxT("\x5f00\x59cb\x8f6c\x5b58"), wxDefaultPosition, wxSize(100, 40));
    btn_start_->SetBackgroundColour(wxColour(76, 175, 80));
    btn_start_->SetForegroundColour(*wxWHITE);
    btn_stop_ = new wxButton(panel, ID_STOP, wxT("\x505c\x6b62"), wxDefaultPosition, wxSize(100, 30));
    btn_stop_->Disable();
    auto* btn_export = new wxButton(panel, ID_EXPORT_LOG, wxT("\x5bfc\x51fa\x65e5\x5fd7"), wxDefaultPosition, wxSize(100, 30)); // 导出日志
    btn_panel->Add(btn_start_, 0, wxBOTTOM, 5);
    btn_panel->Add(btn_stop_, 0, wxBOTTOM, 5);
    btn_panel->Add(btn_export, 0);
    bottom->Add(btn_panel, 0, wxALIGN_BOTTOM);
    vbox->Add(bottom, 0, wxEXPAND | wxALL, 8);

    gauge_ = new wxGauge(panel, wxID_ANY, 100);
    vbox->Add(gauge_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    panel->SetSizer(vbox);
}

void MainFrame::LoadCookie() {
    struct { const char* path; const char* key; } paths[] = {
        {"cookies.json", "cookies"},
        {"config/quark_config.json", "cookie"},
    };
    for (auto& [path, key] : paths) {
        try {
            std::ifstream ifs(path);
            if (!ifs.is_open()) continue;
            auto cfg = json::parse(ifs);
            std::string ck;
            if (cfg[key].is_array()) ck = cfg[key][0].get<std::string>();
            else ck = cfg[key].get<std::string>();
            if (!ck.empty()) {
                cookie_input_->SetValue(wxString::FromUTF8(ck));
                Log(wxString::Format(wxT("\x4ece %s \x52a0\x8f7d\x4e86 cookie"), path));
                return;
            }
        } catch (...) {}
    }
}

void MainFrame::Log(const wxString& msg) {
    log_text_->AppendText(msg + "\n");
}

void MainFrame::ApplyFilter() {
    auto keyword = filter_input_->GetValue().ToStdString(wxConvUTF8);
    int sel = filter_tag_->GetSelection();
    std::string tag = "all";
    if (sel == 1) tag = "ai";
    else if (sel == 2) tag = "non-ai";
    store_.applyFilter(keyword, tag);
    list_ctrl_->UpdateItemCount();
    lbl_count_->SetLabel(wxString::Format(wxT("\x7b5b\x9009 %zu / \x5171 %zu \x6761"),
                                          store_.size(), store_.totalSize())); // 筛选 N / 共 N 条
}

void MainFrame::OnFilterChanged(wxCommandEvent&) {
    ApplyFilter();
}

void MainFrame::OnLogin(wxCommandEvent&) {
    auto ck = cookie_input_->GetValue().ToStdString(wxConvUTF8);
    if (ck.empty()) { wxMessageBox(wxT("\x8bf7\x5148\x8f93\x5165 cookie")); return; }
    api_ = std::make_unique<QuarkApi>(ck);
    auto resp = api_->getAccountInfo();
    if (resp.contains("data") && resp["data"].contains("nickname")) {
        lbl_account_->SetLabel(wxString::FromUTF8("\xe5\xb7\xb2\xe7\x99\xbb\xe5\xbd\x95: " + api_->nickname()));
        lbl_account_->SetForegroundColour(wxColour(0, 128, 0));
        Log(wxString::FromUTF8("\xe7\x99\xbb\xe5\xbd\x95\xe6\x88\x90\xe5\x8a\x9f: " + api_->nickname()));
    } else {
        lbl_account_->SetLabel(wxT("\x767b\x5f55\x5931\x8d25"));
        lbl_account_->SetForegroundColour(*wxRED);
        api_.reset();
        Log(wxT("cookie \x65e0\x6548"));
    }
}

void MainFrame::OnLoadCookieFile(wxCommandEvent&) {
    wxFileDialog dlg(this, wxT("\x9009\x62e9 Cookie \x6587\x4ef6"), "", "", // 选择 Cookie 文件
                     "JSON (*.json)|*.json|All (*.*)|*.*", wxFD_OPEN);
    if (dlg.ShowModal() != wxID_OK) return;
    auto path = dlg.GetPath().ToStdString(wxConvUTF8);
    try {
        std::ifstream ifs(path);
        auto cfg = json::parse(ifs);
        std::string ck;
        // 尝试多种 key
        for (auto& key : {"cookies", "cookie"}) {
            if (!cfg.contains(key)) continue;
            if (cfg[key].is_array()) ck = cfg[key][0].get<std::string>();
            else if (cfg[key].is_string()) ck = cfg[key].get<std::string>();
            if (!ck.empty()) break;
        }
        if (!ck.empty()) {
            cookie_input_->SetValue(wxString::FromUTF8(ck));
            Log(wxString::FromUTF8("\xe4\xbb\x8e\xe6\x96\x87\xe4\xbb\xb6\xe5\x8a\xa0\xe8\xbd\xbd\xe4\xba\x86 cookie")); // 从文件加载了 cookie
        } else {
            wxMessageBox(wxT("\x672a\x627e\x5230 cookie \x5b57\x6bb5")); // 未找到 cookie 字段
        }
    } catch (std::exception& e) {
        wxMessageBox(wxString::FromUTF8(std::string("\xe8\xa7\xa3\xe6\x9e\x90\xe5\xa4\xb1\xe8\xb4\xa5: ") + e.what())); // 解析失败:
    }
}

void MainFrame::OnBrowse(wxCommandEvent&) {
    if (!api_) { OnLogin(*(wxCommandEvent*)nullptr); if (!api_) return; }
    QuarkDirDialog dlg(this, *api_);
    if (dlg.ShowModal() == wxID_OK) {
        path_input_->SetValue(dlg.GetPath());
    }
}

void MainFrame::OnLoadJson(wxCommandEvent&) {
    wxFileDialog dlg(this, wxT("\x9009\x62e9\x94fe\x63a5\x6587\x4ef6"), "", "",
                     "JSON (*.json)|*.json", wxFD_OPEN);
    if (dlg.ShowModal() != wxID_OK) return;
    auto path = dlg.GetPath().ToStdString(wxConvUTF8);
    size_t skipped_count = done_urls_.size();
    Log(wxString::Format(wxT("\x6b63\x5728\x52a0\x8f7d... (\x5df2\x8df3\x8fc7 %zu \x6761\x5df2\x5b8c\x6210)"), skipped_count));
    // 正在加载... (已跳过 N 条已完成)

    store_.loadAsync(path,
        [this](size_t loaded, size_t) {
            CallAfter([this, loaded] {
                list_ctrl_->SetItemCount((long)loaded);
                lbl_count_->SetLabel(wxString::Format(wxT("\x5df2\x52a0\x8f7d %zu \x6761"), loaded));
            });
        },
        [this, skipped_count](bool ok, std::string err) {
            CallAfter([this, ok, err, skipped_count] {
                list_ctrl_->UpdateItemCount();
                auto sz = store_.size();
                lbl_count_->SetLabel(wxString::Format(wxT("\x5171 %zu \x6761"), sz));
                Log(ok ? wxString::Format(wxT("\x52a0\x8f7d\x5b8c\x6210, %zu \x6761\x5f85\x8f6c\x5b58, %zu \x6761\x5df2\x8df3\x8fc7"),
                         sz, skipped_count)
                    // 加载完成, N 条待转存, N 条已跳过
                       : wxString::FromUTF8("\xe5\x8a\xa0\xe8\xbd\xbd\xe5\xa4\xb1\xe8\xb4\xa5: " + err));
            });
        },
        &done_urls_
    );
}

void MainFrame::OnSelectAll(wxCommandEvent&) {
    bool first_checked = store_.size() > 0 && store_.at(0).checked;
    store_.setAllChecked(!first_checked);
    list_ctrl_->Refresh();
}

void MainFrame::OnStart(wxCommandEvent&) {
    if (running_) return;
    if (!api_) { OnLogin(*(wxCommandEvent*)nullptr); if (!api_) return; }

    auto indices = store_.checkedIndices();
    if (indices.empty()) {
        wxMessageBox(wxT("\x8bf7\x52fe\x9009\x8981\x8f6c\x5b58\x7684\x94fe\x63a5"));
        return;
    }

    // Apply limit: 0 = all
    int limit = spin_limit_->GetValue();
    if (limit > 0 && (size_t)limit < indices.size()) {
        indices.resize((size_t)limit);
    }

    // Set rate limiter based on choice
    int rate_sel = rate_choice_->GetSelection();
    int rates[] = {0, 20, 30, 10, 5};  // 0 = unlimited
    limiter_.setRate(rates[rate_sel]);

    running_ = true;
    cancel_.store(false);
    completed_.store(0);
    success_count_.store(0);
    fail_count_.store(0);
    btn_start_->Disable();
    btn_stop_->Enable();
    gauge_->SetRange((int)indices.size());
    gauge_->SetValue(0);

    int n_threads = spin_threads_->GetValue();
    auto save_path = path_input_->GetValue().ToStdString(wxConvUTF8);

    wxString rate_desc = rate_choice_->GetStringSelection();
    Log(wxString::Format(wxT("\x5f00\x59cb\x8f6c\x5b58 %zu \x6761, \x5e76\x53d1 %d, \x901f\x5ea6: %s"),
                         indices.size(), n_threads, rate_desc));
    // 开始转存 N 条, 并发 N, 速度: xxx

    pool_ = std::make_unique<ThreadPool>((size_t)n_threads);

    auto total = indices.size();
    for (auto idx : indices) {
        pool_->submit([this, idx, save_path, total] {
            if (cancel_.load()) {
                // Still count toward completion
                int done = ++completed_;
                CallAfter([this, done, total] {
                    gauge_->SetValue(done);
                    if ((size_t)done >= total) {
                        running_ = false;
                        btn_start_->Enable();
                        btn_stop_->Disable();
                    }
                });
                return;
            }

            auto& item = store_.at(idx);
            item.status.store(TransferStatus::InProgress);
            CallAfter([this, idx] { list_ctrl_->RefreshItem((long)idx); });

            TransferWorker worker(*api_, save_path, limiter_);
            auto result = worker.execute(item, cancel_);

            if (result.success && result.file_count > 0) {
                item.status.store(TransferStatus::Success);
                item.status_detail = "\xe6\x88\x90\xe5\x8a\x9f(" + std::to_string(result.file_count) + "\xe4\xb8\xaa)";
                AppendDoneUrl(item.quark_url);
                ++success_count_;
            } else if (result.success) {
                item.status.store(TransferStatus::NoNewFiles);
                AppendDoneUrl(item.quark_url);
                ++success_count_;
            } else {
                item.status.store(TransferStatus::Failed);
                item.status_detail = result.message;
                ++fail_count_;
            }

            int done = ++completed_;
            CallAfter([this, idx, done, total] {
                list_ctrl_->RefreshItem((long)idx);
                gauge_->SetValue(done);
                if ((size_t)done >= total) {
                    running_ = false;
                    btn_start_->Enable();
                    btn_stop_->Disable();
                    int s = success_count_.load(), f = fail_count_.load();
                    Log(wxString::Format(wxT("\x5b8c\x6210! \x6210\x529f %d, \x5931\x8d25 %d"), s, f));
                    // 完成! 成功 N, 失败 N
                    ExportSessionLog();
                }
            });
        });
    }
}

void MainFrame::LoadDoneUrls() {
    std::ifstream ifs("done_urls.txt");
    if (!ifs.is_open()) return;
    std::string line;
    while (std::getline(ifs, line)) {
        if (!line.empty()) done_urls_.insert(line);
    }
    if (!done_urls_.empty()) {
        Log(wxString::Format(wxT("\x52a0\x8f7d\x4e86 %zu \x6761\x5df2\x5b8c\x6210\x8bb0\x5f55"), done_urls_.size()));
        // 加载了 N 条已完成记录
    }
}

void MainFrame::AppendDoneUrl(const std::string& url) {
    {
        std::lock_guard lk(done_mtx_);
        if (!done_urls_.insert(url).second) return;  // already exists
    }
    // Append to file
    std::ofstream ofs("done_urls.txt", std::ios::app);
    if (ofs.is_open()) ofs << url << "\n";
}

void MainFrame::ExportSessionLog() {
    // Get timestamp for filename
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", std::localtime(&t));
    std::string filename = std::string("transfer_log_") + buf + ".txt";

    std::ofstream ofs(filename, std::ios::out);
    if (!ofs.is_open()) return;

    ofs << "Transfer Log - " << buf << "\n";
    ofs << "Success: " << success_count_.load() << ", Failed: " << fail_count_.load() << "\n\n";

    auto total = store_.totalSize();
    for (size_t i = 0; i < total; i++) {
        // Access raw items through view-independent totalSize
        // We need direct access, but at() goes through view. Use a simple approach:
        // iterate all checked indices we processed
    }

    // Just log done_urls
    ofs << "=== Completed URLs ===\n";
    {
        std::lock_guard lk(done_mtx_);
        for (auto& url : done_urls_) {
            ofs << url << "\n";
        }
    }
    ofs << "\n=== Total: " << done_urls_.size() << " ===\n";

    Log(wxString::FromUTF8("\xe6\x97\xa5\xe5\xbf\x97\xe5\xb7\xb2\xe4\xbf\x9d\xe5\xad\x98: " + filename));
    // 日志已保存: filename
}

void MainFrame::OnExportLog(wxCommandEvent&) {
    ExportSessionLog();
}

void MainFrame::OnStop(wxCommandEvent&) {
    cancel_.store(true);
    Log(wxT("\x6b63\x5728\x505c\x6b62..."));
    // 在后台等线程池排空，然后恢复按钮
    std::thread([this] {
        pool_.reset();  // 等所有 worker 结束
        CallAfter([this] {
            running_ = false;
            btn_start_->Enable();
            btn_stop_->Disable();
            Log(wxString::Format(wxT("\x5df2\x505c\x6b62, \x5b8c\x6210 %d \x6761"), completed_.load())); // 已停止, 完成 N 条
        });
    }).detach();
}
