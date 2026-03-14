#pragma once
#include <wx/wx.h>
#include <wx/spinctrl.h>
#include "VirtualLinkList.h"
#include "QuarkDirDialog.h"
#include "model/LinkStore.h"
#include "api/QuarkApi.h"
#include "worker/ThreadPool.h"
#include "worker/RateLimiter.h"
#include <memory>
#include <atomic>
#include <set>
#include <mutex>

class MainFrame : public wxFrame {
public:
    MainFrame();

private:
    wxTextCtrl* cookie_input_;
    wxStaticText* lbl_account_;
    wxTextCtrl* path_input_;
    VirtualLinkList* list_ctrl_;
    wxStaticText* lbl_count_;
    wxTextCtrl* log_text_;
    wxButton* btn_start_;
    wxButton* btn_stop_;
    wxGauge* gauge_;
    wxSpinCtrl* spin_threads_;
    wxSpinCtrl* spin_limit_;

    // Filter controls
    wxTextCtrl* filter_input_;
    wxChoice* filter_tag_;

    // Rate control
    wxChoice* rate_choice_;

    LinkStore store_;
    std::unique_ptr<QuarkApi> api_;
    std::unique_ptr<ThreadPool> pool_;
    RateLimiter limiter_;
    std::atomic<bool> cancel_{false};
    std::atomic<int> completed_{0};
    std::atomic<int> success_count_{0};
    std::atomic<int> fail_count_{0};
    bool running_ = false;

    // Done URL tracking
    std::set<std::string> done_urls_;
    std::mutex done_mtx_;
    void LoadDoneUrls();
    void AppendDoneUrl(const std::string& url);
    void ExportSessionLog();

    void BuildUI();
    void LoadCookie();
    void Log(const wxString& msg);
    void ApplyFilter();

    void OnLogin(wxCommandEvent&);
    void OnLoadCookieFile(wxCommandEvent&);
    void OnBrowse(wxCommandEvent&);
    void OnLoadJson(wxCommandEvent&);
    void OnSelectAll(wxCommandEvent&);
    void OnStart(wxCommandEvent&);
    void OnStop(wxCommandEvent&);
    void OnFilterChanged(wxCommandEvent&);
    void OnExportLog(wxCommandEvent&);

    wxDECLARE_EVENT_TABLE();
};
