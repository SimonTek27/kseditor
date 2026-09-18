#pragma once

#include <string>
#include <functional>
#include <vector>

namespace ks::sim {

struct SimPoint { int x = 0; int y = 0; };
struct SimRect { int x = 0; int y = 0; int w = 0; int h = 0; bool contains(const SimPoint& p) const { return p.x >= x && p.x < x+w && p.y >= y && p.y < y+h; } };

enum class MenuState {
    Main,
    Singleplayer,
    Multiplayer,
    Profile,
    Garage,
    Replay,
    ContentManager,
    Settings,
    Controls,
    DevModeConfirm,
    QuitConfirm
};

struct DriverProfile {
    std::string name = "Player";
    std::string nationality = "Italian";
    int raceNumber = 1;
    std::string bio = "Racing enthusiast";
    int helmetDesign = 0;
    int controlsPreset = 0;
    int lastReplayIndex = -1;
    int wins = 0;
    int poles = 0;
    int podiums = 0;
    int totalRaces = 0;
    float bestLapTime = 0.0f;
};

struct MenuItem {
    std::string text;
    std::string description;
    std::function<void()> action;
    bool isSeparator = false;
    bool enabled = true;
};

class GameMenuOverlay {
public:
    GameMenuOverlay();

    // Stubbed - no QPainter in pure Win32/Vulkan path
    void render(int width, int height);
    bool handleKeyPress(int key);
    void handleMouseMove(const SimPoint& pos, int widgetWidth, int widgetHeight);
    void handleClick(const SimPoint& pos, int widgetWidth, int widgetHeight);

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible);
    void toggleVisible();

    void setTrackName(const std::string& name) { m_trackName = name; }
    void setCarName(const std::string& name) { m_carName = name; }
    void setProfileField(const std::string& fieldName, const std::string& value);
    void setNationalityFromList(int index);

    bool isInputBlocked() const { return m_visible; }

    DriverProfile& profile() { return m_profile; }
    const DriverProfile& profile() const { return m_profile; }

    std::function<void()> onExitRequested;
    std::function<void()> onStartDrivingRequested;
    std::function<void()> onLoadTrackRequested;
    std::function<void()> onLoadCarRequested;
    std::function<void()> onResetRequested;
    std::function<void()> onToggleFullscreenRequested;
    std::function<void(const DriverProfile&)> onProfileChanged;
    std::function<void()> onDevModeRequested;
    std::function<void(const std::string&, const std::string&)> onTextInputRequested;
    std::function<void()> onNationalityInputRequested;
    std::function<void()> onOpenGarageRequested;
    std::function<void()> onOpenSetupGarageRequested;
    std::function<void()> onLoadReplayRequested;
    std::function<void()> onRecordReplayRequested;
    std::function<void(const std::string&)> onOpenContentBrowserRequested;
    std::function<void(const std::string&)> onOpenSettingsPanelRequested;

private:
    void buildMainMenu();
    void buildSingleplayerMenu();
    void buildMultiplayerMenu();
    void buildProfileMenu();
    void buildGarageMenu();
    void buildReplayMenu();
    void buildContentManagerMenu();
    void buildSettingsMenu();
    void buildControlsMenu();
    void buildDevModeConfirm();
    void buildQuitConfirm();
    void switchMenu(MenuState state);
    void goBack();

    bool m_visible = true;
    bool m_menuDirty = true;
    MenuState m_currentState = MenuState::Main;
    MenuState m_previousState = MenuState::Main;
    int m_selectedIndex = 0;
    int m_hoverIndex = -1;
    float m_animationTime = 0.0f;
    float m_fadeAlpha = 0.0f;
    float m_transitionProgress = 1.0f;
    std::string m_trackName;
    std::string m_carName;

    DriverProfile m_profile;
    std::vector<MenuItem> m_items;
};

} // namespace ks::sim
