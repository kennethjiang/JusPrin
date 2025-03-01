#ifndef slic3r_GUI_JusPrinView3D_hpp_
#define slic3r_GUI_JusPrinView3D_hpp_

#include "../../GUI/GUI_Preview.hpp"
#include "JusPrinChatPanel.hpp"

// Forward declarations
class wxStaticBitmap;
class wxWindow;
class wxSizeEvent;

namespace Slic3r {
namespace GUI {

// Add near the top of the file, inside the Slic3r::GUI namespace
struct ChatPanelConfig {
    double height_ratio;
    double width_ratio;
};

// Move ActivationButtonNotificationBadge class definition to header
class ActivationButtonNotificationBadge : public wxPanel {
public:
    ActivationButtonNotificationBadge(wxWindow* parent, const wxString& text, const wxColour& bgColor)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(16, 16))
    {
        m_text = text;
        m_bgColor = bgColor;
#ifdef __APPLE__
        // Set transparent background
        SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
        SetBackgroundColour(wxTransparentColour);
#else
        SetBackgroundColour(*wxWHITE);
#endif

        Bind(wxEVT_PAINT, &ActivationButtonNotificationBadge::OnPaint, this);
    }

    // Add method to update text
    void SetText(const wxString& text) {
        m_text = text;
        Refresh();
    }

private:
    void OnPaint(wxPaintEvent&);
    wxString m_text;
    wxColour m_bgColor;
};

class ChatActivationButton : public wxPanel
{
public:
    ChatActivationButton(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
    void DoSetSize(int x, int y, int width, int height, int sizeFlags = wxSIZE_AUTO) override;
    void AddJoin(std::function<void(wxMouseEvent&)> do_some) { m_do = do_some; }

private:
    void OnPaint(wxPaintEvent& event);
    void OnMouseEnter(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);

private:
    bool                 m_isHovered{false};
    wxAnimationCtrlBase* m_animationCtrl{nullptr};
    std::function<void(wxMouseEvent&)> m_do{nullptr};
};

class JusPrinView3D : public View3D {
public:
    JusPrinView3D(wxWindow* parent, Bed3D& bed, Model* model, DynamicPrintConfig* config, BackgroundSlicingProcess* process);
    virtual ~JusPrinView3D();

    void changeChatPanelView(const std::string& viewMode);
    void setChatPanelVisibility(bool is_visible);
    void setChatPanelNotificationBadges(int red_badge, int orange_badge, int green_badge);
    std::string getChatPanelViewMode() const { return m_chatpanel_view_mode; }
    bool getChatPanelVisibility() const { return m_chat_panel->IsShown(); }

    JusPrinChatPanel* jusprinChatPanel() const { return m_chat_panel; }

protected:
    void OnSize(wxSizeEvent& evt);
    void OnCanvasMouseDown(SimpleEvent& evt);

private:
    void initOverlay();
    void showChatPanel();
    void hideChatPanel();
    void updateChatPanelRect();
    void updateActivationButtonRect();
    void showBadgesIfNecessary(); // This method depends on the bounds of the activation button. Needs to be called after updateActivationButtonRect()

    JusPrinChatPanel* m_chat_panel{nullptr};
    ChatActivationButton*   m_overlay_btn{nullptr};

    std::string m_chatpanel_view_mode{"large"}; // Default to large view

    // Badges
    int m_red_badge_count{0};
    int m_orange_badge_count{0};
    int m_green_badge_count{0};
    ActivationButtonNotificationBadge*    m_red_badge{nullptr};
    ActivationButtonNotificationBadge*    m_orange_badge{nullptr};
    ActivationButtonNotificationBadge*    m_green_badge{nullptr};

};

} // namespace GUI
} // namespace Slic3r

#endif
