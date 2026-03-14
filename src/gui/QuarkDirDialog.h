#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include "api/QuarkApi.h"

class QuarkDirDialog : public wxDialog {
public:
    QuarkDirDialog(wxWindow* parent, QuarkApi& api);
    wxString GetPath() const { return current_path_; }

private:
    QuarkApi& api_;
    wxListCtrl* dir_list_;
    wxStaticText* lbl_path_;
    wxString current_path_ = "/";

    struct PathEntry { wxString name; std::string fid; };
    std::vector<PathEntry> path_stack_;

    void LoadDir(const std::string& fid, const wxString& path);
    void OnDblClick(wxListEvent& evt);
    void OnUp(wxCommandEvent& evt);
    void OnMkDir(wxCommandEvent& evt);

    wxDECLARE_EVENT_TABLE();
};
