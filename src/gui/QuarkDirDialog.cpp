#include "QuarkDirDialog.h"

enum { ID_BTN_UP = wxID_HIGHEST + 1, ID_BTN_MKDIR, ID_DIR_LIST };

wxBEGIN_EVENT_TABLE(QuarkDirDialog, wxDialog)
    EVT_LIST_ITEM_ACTIVATED(ID_DIR_LIST, QuarkDirDialog::OnDblClick)
    EVT_BUTTON(ID_BTN_UP, QuarkDirDialog::OnUp)
    EVT_BUTTON(ID_BTN_MKDIR, QuarkDirDialog::OnMkDir)
wxEND_EVENT_TABLE()

QuarkDirDialog::QuarkDirDialog(wxWindow* parent, QuarkApi& api)
    : wxDialog(parent, wxID_ANY, wxT("\x9009\x62e9\x7f51\x76d8\x76ee\x5f55"), // 选择网盘目录
               wxDefaultPosition, wxSize(500, 450)),
      api_(api) {
    auto* vbox = new wxBoxSizer(wxVERTICAL);

    lbl_path_ = new wxStaticText(this, wxID_ANY, wxT("\x5f53\x524d: /")); // 当前: /
    vbox->Add(lbl_path_, 0, wxEXPAND | wxALL, 8);

    dir_list_ = new wxListCtrl(this, ID_DIR_LIST, wxDefaultPosition, wxDefaultSize,
                               wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SUNKEN);
    dir_list_->InsertColumn(0, wxT("\x6587\x4ef6\x5939"), wxLIST_FORMAT_LEFT, 350); // 文件夹
    dir_list_->InsertColumn(1, "fid", wxLIST_FORMAT_LEFT, 0);
    vbox->Add(dir_list_, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);

    auto* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->Add(new wxButton(this, ID_BTN_UP, wxT("\x8fd4\x56de\x4e0a\x7ea7")), 0, wxRIGHT, 5); // 返回上级
    btn_sizer->Add(new wxButton(this, ID_BTN_MKDIR, wxT("\x65b0\x5efa\x6587\x4ef6\x5939")), 0, wxRIGHT, 5); // 新建文件夹
    btn_sizer->AddStretchSpacer();
    btn_sizer->Add(new wxButton(this, wxID_OK, wxT("\x9009\x62e9\x6b64\x76ee\x5f55")), 0, wxRIGHT, 5); // 选择此目录
    btn_sizer->Add(new wxButton(this, wxID_CANCEL, wxT("\x53d6\x6d88")), 0); // 取消
    vbox->Add(btn_sizer, 0, wxEXPAND | wxALL, 8);

    SetSizer(vbox);
    path_stack_.push_back({"/", "0"});
    LoadDir("0", "/");
}

void QuarkDirDialog::LoadDir(const std::string& fid, const wxString& path) {
    dir_list_->DeleteAllItems();
    wxBusyCursor wait;
    try {
        auto resp = api_.lsDir(fid);
        if (resp.contains("code") && resp["code"].get<int>() == 0) {
            for (auto& f : resp["data"]["list"]) {
                if (f.value("dir", false)) {
                    long idx = dir_list_->GetItemCount();
                    dir_list_->InsertItem(idx, wxString::FromUTF8(f["file_name"].get<std::string>()));
                    dir_list_->SetItem(idx, 1, wxString::FromUTF8(f["fid"].get<std::string>()));
                }
            }
        }
    } catch (...) {}
    current_path_ = path;
    lbl_path_->SetLabel(wxT("\x5f53\x524d: ") + path); // 当前:
}

void QuarkDirDialog::OnDblClick(wxListEvent& evt) {
    long idx = evt.GetIndex();
    wxString name = dir_list_->GetItemText(idx, 0);
    std::string fid = dir_list_->GetItemText(idx, 1).ToStdString();
    wxString new_path = current_path_;
    if (!new_path.EndsWith("/")) new_path += "/";
    new_path += name;
    path_stack_.push_back({name, fid});
    LoadDir(fid, new_path);
}

void QuarkDirDialog::OnUp(wxCommandEvent&) {
    if (path_stack_.size() <= 1) return;
    path_stack_.pop_back();
    wxString path = "/";
    for (size_t i = 1; i < path_stack_.size(); i++) {
        path += path_stack_[i].name;
        if (i + 1 < path_stack_.size()) path += "/";
    }
    LoadDir(path_stack_.back().fid, path);
}

void QuarkDirDialog::OnMkDir(wxCommandEvent&) {
    wxTextEntryDialog dlg(this, wxT("\x65b0\x6587\x4ef6\x5939\x540d\x79f0:"), // 新文件夹名称:
                          wxT("\x65b0\x5efa\x6587\x4ef6\x5939")); // 新建文件夹
    if (dlg.ShowModal() == wxID_OK) {
        wxString dir_name = dlg.GetValue().Trim();
        if (!dir_name.empty()) {
            wxString new_path = current_path_;
            if (!new_path.EndsWith("/")) new_path += "/";
            new_path += dir_name;
            wxBusyCursor wait;
            auto resp = api_.mkDir(new_path.ToStdString(wxConvUTF8));
            if (resp.contains("code") && resp["code"].get<int>() == 0) {
                LoadDir(path_stack_.back().fid, current_path_);
            } else {
                wxMessageBox(wxString::FromUTF8(resp.value("message", "failed")),
                             wxT("\x9519\x8bef")); // 错误
            }
        }
    }
}
