#include <wx/wx.h>
#include "api/CurlInit.h"
#include "gui/MainFrame.h"

class App : public wxApp {
public:
    bool OnInit() override {
        auto* frame = new MainFrame();
        frame->Show();
        return true;
    }
};

static CurlInit g_curl_init;

wxIMPLEMENT_APP(App);
