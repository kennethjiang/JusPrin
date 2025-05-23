#include "JusPrinLoginDialog.hpp"

#include <wx/sizer.h>
#include <wx/toolbar.h>
#include <wx/textdlg.h>

#include <boost/lexical_cast.hpp>
#include <nlohmann/json.hpp>

#include "../I18N.hpp"
#include "../GUI_App.hpp"
#include "../wxExtensions.hpp"
#include "../../libslic3r/AppConfig.hpp"
#include "libslic3r_version.h"
#include "slic3r/GUI/Widgets/WebView.hpp"

using namespace nlohmann;

namespace Slic3r { namespace GUI {

JusPrinLoginDialog::JusPrinLoginDialog()
    : wxDialog((wxWindow*)(wxGetApp().mainframe), wxID_ANY, "JusPrin Login")
{
    SetBackgroundColour(*wxWHITE);

    // Set up the URL for JusPrin login
    wxString auth_url = wxGetApp().app_config->get("jusprin_server", "auth_url");
    if (auth_url.IsEmpty()) {
        // If auth_url not found, derive from base_url
        wxString base_url = wxGetApp().app_config->get_with_default("jusprin_server", "base_url", "https://app.obico.io");
        // Remove trailing slash if present
        if (base_url.EndsWith("/")) {
            base_url = base_url.Left(base_url.Length() - 1);
        }
        auth_url = base_url + "/accounts/login/?hide_navbar=true&next=/o/authorize/%3Fresponse_type%3Dtoken%26client_id%3DJusPrin";
    }
    m_networkOk = false;

    // Create the webview
    m_browser = WebView::CreateWebView(this, "");
    if (m_browser == nullptr) {
        wxLogError("Could not create JusPrin login webview");
        return;
    }
    m_browser->Hide();
    m_browser->SetSize(0, 0);

    // Connect the webview events
    Bind(wxEVT_WEBVIEW_NAVIGATING, &JusPrinLoginDialog::OnNavigationRequest, this, m_browser->GetId());
    Bind(wxEVT_WEBVIEW_NAVIGATED, &JusPrinLoginDialog::OnNavigationComplete, this, m_browser->GetId());
    Bind(wxEVT_WEBVIEW_LOADED, &JusPrinLoginDialog::OnDocumentLoaded, this, m_browser->GetId());
    Bind(wxEVT_WEBVIEW_ERROR, &JusPrinLoginDialog::OnError, this, m_browser->GetId());
    Bind(wxEVT_WEBVIEW_NEWWINDOW, &JusPrinLoginDialog::OnNewWindow, this, m_browser->GetId());
    Bind(wxEVT_WEBVIEW_TITLE_CHANGED, &JusPrinLoginDialog::OnTitleChanged, this, m_browser->GetId());
    Bind(wxEVT_WEBVIEW_FULLSCREEN_CHANGED, &JusPrinLoginDialog::OnFullScreenChanged, this, m_browser->GetId());
    Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &JusPrinLoginDialog::OnScriptMessage, this, m_browser->GetId());

    // Set up the dialog
    SetTitle(_L("JusPrin Login"));
    wxSize pSize = FromDIP(wxSize(650, 840));
    SetSize(pSize);

    int screenheight = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y, NULL);
    int screenwidth = wxSystemSettings::GetMetric(wxSYS_SCREEN_X, NULL);
    int MaxY = (screenheight - pSize.y) > 0 ? (screenheight - pSize.y) / 2 : 0;
    wxPoint tmpPT((screenwidth - pSize.x) / 2, MaxY);
    Move(tmpPT);

    m_browser->LoadURL(auth_url);
   //wxGetApp().UpdateDlgDarkUI(this);
}

bool JusPrinLoginDialog::run()
{
    return (ShowModal() == wxID_OK);
}

void JusPrinLoginDialog::load_url(wxString& url)
{
    m_browser->LoadURL(url);
    m_browser->SetFocus();
    UpdateState();
}

void JusPrinLoginDialog::UpdateState()
{
    // Update UI state if needed
}

void JusPrinLoginDialog::OnNavigationRequest(wxWebViewEvent& evt)
{
    wxString tmpUrl = evt.GetURL();
    if (tmpUrl.Contains("authorized/") && tmpUrl.Contains("access_token=")) {
        wxString access_token;
        int start = tmpUrl.Find("access_token=");
        if (start != wxNOT_FOUND) {
            start += 13; // length of "access_token="
            int end = tmpUrl.find('&', start);
            if (end != wxNOT_FOUND) {
                access_token = tmpUrl.SubString(start, end - 1);
            } else {
                access_token = tmpUrl.SubString(start, tmpUrl.Length() - 1);
            }
            std::string oauth_token = access_token.ToStdString();

            if (!oauth_token.empty()) {
                wxGetApp().app_config->set("jusprin_server", "access_token", oauth_token);
                wxGetApp().app_config->save();
                wxGetApp().update_oauth_access_token();

                EndModal(wxID_OK);
            } else {
                EndModal(wxID_CANCEL);
            }
        } else {
            EndModal(wxID_CANCEL);
        }
    }

    UpdateState();
}

void JusPrinLoginDialog::OnNavigationComplete(wxWebViewEvent& evt)
{
    m_browser->Show();
    Layout();
    UpdateState();
}

void JusPrinLoginDialog::OnDocumentLoaded(wxWebViewEvent& evt)
{
    wxString tmpUrl = evt.GetURL();
    if (tmpUrl.Contains("obico.io")) {
        m_networkOk = true;
    }
    UpdateState();
}

void JusPrinLoginDialog::OnNewWindow(wxWebViewEvent& evt)
{
    wxString flag = evt.GetNavigationAction() == wxWEBVIEW_NAV_ACTION_USER ? " (user)" : " (other)";
    m_browser->LoadURL(evt.GetURL());
    UpdateState();
}

void JusPrinLoginDialog::OnTitleChanged(wxWebViewEvent& evt)
{
    // SetTitle(evt.GetString());
}

void JusPrinLoginDialog::OnFullScreenChanged(wxWebViewEvent& evt)
{
    ShowFullScreen(evt.GetInt() != 0);
}

void JusPrinLoginDialog::OnError(wxWebViewEvent& evt)
{
    if (evt.GetInt() == wxWEBVIEW_NAV_ERR_CONNECTION && !m_networkOk) {
        ShowErrorPage();
    }
    UpdateState();
}

void JusPrinLoginDialog::OnScriptMessage(wxWebViewEvent& evt)
{
    wxString str_input = evt.GetString();
    try {
        json j = json::parse(into_u8(str_input));
        wxString strCmd = j["command"];

        if (strCmd == "jusprint_login") {
            // Handle JusPrin login
            // wxGetApp().handle_jusprint_login(j.dump());
            Close();
        }
        // Add other JusPrin-specific commands as needed
    } catch (std::exception& e) {
        wxMessageBox(e.what(), "Parse JSON failed", wxICON_WARNING);
        Close();
    }
}

void JusPrinLoginDialog::RunScript(const wxString& javascript)
{
    m_javascript = javascript;
    if (m_browser) {
        WebView::RunScript(m_browser, javascript);
    }
}

bool JusPrinLoginDialog::ShowErrorPage()
{
    wxString ErrorUrl = from_u8((boost::filesystem::path(resources_dir()) / "web/login/error.html").make_preferred().string());
    load_url(ErrorUrl);
    return true;
}

}} // namespace Slic3r::GUI
