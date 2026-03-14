#include "VirtualLinkList.h"
#include <wx/dc.h>
#include <wx/dcmemory.h>

wxBEGIN_EVENT_TABLE(VirtualLinkList, wxListCtrl)
    EVT_LIST_ITEM_ACTIVATED(wxID_ANY, VirtualLinkList::OnClick)
wxEND_EVENT_TABLE()

static wxBitmap MakeCheckBitmap(bool checked, int sz = 16) {
    wxBitmap bmp(sz, sz);
    wxMemoryDC dc(bmp);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    dc.SetPen(*wxBLACK_PEN);
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.DrawRectangle(1, 1, sz - 2, sz - 2);
    if (checked) {
        dc.SetPen(wxPen(*wxBLACK, 2));
        dc.DrawLine(3, sz/2, sz/2 - 1, sz - 4);
        dc.DrawLine(sz/2 - 1, sz - 4, sz - 3, 3);
    }
    dc.SelectObject(wxNullBitmap);
    return bmp;
}

VirtualLinkList::VirtualLinkList(wxWindow* parent, LinkStore& store)
    : wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                 wxLC_REPORT | wxLC_VIRTUAL | wxBORDER_SUNKEN),
      store_(store), img_list_(16, 16, true, 2) {
    img_list_.Add(MakeCheckBitmap(false));
    img_list_.Add(MakeCheckBitmap(true));
    SetImageList(&img_list_, wxIMAGE_LIST_SMALL);

    InsertColumn(0, wxT("\x540d\x79f0"), wxLIST_FORMAT_LEFT, 370);       // 名称
    InsertColumn(1, wxT("\x94fe\x63a5"), wxLIST_FORMAT_LEFT, 300);       // 链接
    InsertColumn(2, wxT("\x72b6\x6001"), wxLIST_FORMAT_LEFT, 110);       // 状态
}

wxString VirtualLinkList::OnGetItemText(long item, long column) const {
    if (item < 0 || (size_t)item >= store_.size()) return {};
    const auto& link = store_.at((size_t)item);
    switch (column) {
        case 0: return wxString::FromUTF8(link.name);
        case 1: return wxString::FromUTF8(link.quark_url);
        case 2: {
            switch (link.status.load()) {
                case TransferStatus::Pending:    return wxT("\x5f85\x8f6c\x5b58");     // 待转存
                case TransferStatus::InProgress: return wxT("\x8f6c\x5b58\x4e2d...");   // 转存中...
                case TransferStatus::Success:    return wxString::FromUTF8(link.status_detail);
                case TransferStatus::NoNewFiles: return wxT("\x65e0\x65b0\x6587\x4ef6"); // 无新文件
                case TransferStatus::Failed:     return wxT("\x5931\x8d25");             // 失败
                case TransferStatus::Skipped:    return wxT("\x8df3\x8fc7");             // 跳过
            }
        }
        default: return {};
    }
}

int VirtualLinkList::OnGetItemImage(long item) const {
    if (item < 0 || (size_t)item >= store_.size()) return 0;
    return store_.at((size_t)item).checked ? 1 : 0;
}

void VirtualLinkList::ToggleCheck(long item) {
    if (item >= 0 && (size_t)item < store_.size()) {
        auto& link = store_.at((size_t)item);
        link.checked = !link.checked;
        RefreshItem(item);
    }
}

bool VirtualLinkList::IsChecked(long item) const {
    if (item >= 0 && (size_t)item < store_.size())
        return store_.at((size_t)item).checked;
    return false;
}

void VirtualLinkList::UpdateItemCount() {
    SetItemCount((long)store_.size());
    Refresh();
}

void VirtualLinkList::OnClick(wxListEvent& evt) {
    ToggleCheck(evt.GetIndex());
}
