#pragma once
#include <wx/listctrl.h>
#include "model/LinkStore.h"

class VirtualLinkList : public wxListCtrl {
public:
    VirtualLinkList(wxWindow* parent, LinkStore& store);

    wxString OnGetItemText(long item, long column) const override;
    int OnGetItemImage(long item) const override;

    void ToggleCheck(long item);
    bool IsChecked(long item) const;
    void UpdateItemCount();

private:
    LinkStore& store_;
    wxImageList img_list_;

    void OnClick(wxListEvent& evt);
    wxDECLARE_EVENT_TABLE();
};
