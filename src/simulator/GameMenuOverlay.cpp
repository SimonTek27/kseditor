#include "GameMenuOverlay.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ks::sim {

GameMenuOverlay::GameMenuOverlay()
{
    buildMainMenu();
}

void GameMenuOverlay::setVisible(bool visible)
{
    if (m_visible == visible) return;
    m_visible = visible;
    m_menuDirty = true;
    if (visible) {
        m_fadeAlpha = 0.0f;
        m_transitionProgress = 0.0f;
    }
}

void GameMenuOverlay::toggleVisible()
{
    setVisible(!m_visible);
}

void GameMenuOverlay::setProfileField(const std::string& fieldName, const std::string& value)
{
    if (fieldName == "NAME") m_profile.name = value;
    else if (fieldName == "BIO") m_profile.bio = value;
    else if (fieldName == "NATIONALITY") m_profile.nationality = value;
    m_menuDirty = true;
}

void GameMenuOverlay::setNationalityFromList(int index)
{
    static const char* nationalities[] = {
        "Italian", "German", "British", "French", "Spanish", "American",
        "Japanese", "Brazilian", "Australian", "Canadian", "Dutch",
        "Finnish", "Swedish", "Belgian", "Swiss", "Austrian"
    };
    if (index >= 0 && index < 16) {
        m_profile.nationality = nationalities[index];
        m_menuDirty = true;
    }
}

void GameMenuOverlay::buildMainMenu()
{
    m_items.clear();
    m_items.push_back({ "SINGLEPLAYER", "Race modes, practice and championships",
        [this]() { switchMenu(MenuState::Singleplayer); } });
    m_items.push_back({ "MULTIPLAYER", "Online racing with rated and casual events",
        [this]() { switchMenu(MenuState::Multiplayer); } });
    m_items.push_back({ "GARAGE", "Car setup, tuning and tire management",
        [this]() { switchMenu(MenuState::Garage); } });
    m_items.push_back({ "REPLAY", "Watch, record and manage race replays",
        [this]() { switchMenu(MenuState::Replay); } });
    m_items.push_back({ "CONTENT MANAGER", "Browse, download and update content",
        [this]() { switchMenu(MenuState::ContentManager); } });
    m_items.push_back({ "", "", nullptr, true });
    m_items.push_back({ "PROFILE", "Driver info, stats and quick shortcuts",
        [this]() { switchMenu(MenuState::Profile); } });
    m_items.push_back({ "SETTINGS", "Graphics, audio, display and controls",
        [this]() { switchMenu(MenuState::Settings); } });
    m_items.push_back({ "", "", nullptr, true });
    m_items.push_back({ "DEV MODE", "Exit and launch ksEditor editor",
        [this]() { switchMenu(MenuState::DevModeConfirm); } });
    m_items.push_back({ "QUIT", "Exit the simulator",
        [this]() { switchMenu(MenuState::QuitConfirm); } });
}

void GameMenuOverlay::buildSingleplayerMenu()
{
    m_items.clear();
    m_items.push_back({ "PRACTICE", "Free practice on any track",
        [this]() { if (onStartDrivingRequested) onStartDrivingRequested(); setVisible(false); } });
    m_items.push_back({ "QUICK RACE", "Single race against AI opponents",
        [this]() { if (onStartDrivingRequested) onStartDrivingRequested(); setVisible(false); } });
    m_items.push_back({ "TIME TRIAL", "Race against the clock for best laps",
        [this]() { if (onStartDrivingRequested) onStartDrivingRequested(); setVisible(false); } });
    m_items.push_back({ "", "", nullptr, true });
    m_items.push_back({ "BACK", "Return to main menu",
        [this]() { goBack(); } });
}

void GameMenuOverlay::buildMultiplayerMenu()
{
    m_items.clear();
    m_items.push_back({ "RATED", "Competitive ranked racing with ELO", [this]() {} });
    m_items.push_back({ "CASUAL EVENT", "Relaxed racing, no rank impact", [this]() {} });
    m_items.push_back({ "", "", nullptr, true });
    m_items.push_back({ "BACK", "Return to main menu", [this]() { goBack(); } });
}

void GameMenuOverlay::buildProfileMenu()
{
    m_items.clear();
    m_items.push_back({ "NAME", m_profile.name,
        [this]() { if (onTextInputRequested) onTextInputRequested("NAME", m_profile.name); } });
    m_items.push_back({ "NATIONALITY", m_profile.nationality,
        [this]() { if (onNationalityInputRequested) onNationalityInputRequested(); } });
    m_items.push_back({ "RACE NUMBER", std::to_string(m_profile.raceNumber),
        [this]() { m_profile.raceNumber = (m_profile.raceNumber % 99) + 1; } });
    m_items.push_back({ "BIO", m_profile.bio,
        [this]() { if (onTextInputRequested) onTextInputRequested("BIO", m_profile.bio); } });
    m_items.push_back({ "", "", nullptr, true });
    m_items.push_back({ "HELMET DESIGN", "#" + std::to_string(m_profile.helmetDesign + 1),
        [this]() { m_profile.helmetDesign = (m_profile.helmetDesign + 1) % 10; } });
    m_items.push_back({ "", "", nullptr, true });
    m_items.push_back({ "BACK", "Return to main menu", [this]() { goBack(); } });
}

void GameMenuOverlay::buildGarageMenu()
{
    m_items.clear();
    m_items.push_back({ "CAR SETUP", "Adjust aerodynamics, gearing, brakes",
        [this]() { if (onOpenSetupGarageRequested) onOpenSetupGarageRequested(); } });
    m_items.push_back({ "BACK", "Return to main menu", [this]() { goBack(); } });
}

void GameMenuOverlay::buildReplayMenu()
{
    m_items.clear();
    m_items.push_back({ "LOAD REPLAY", "Select a saved replay file",
        [this]() { if (onLoadReplayRequested) onLoadReplayRequested(); } });
    m_items.push_back({ "BACK", "Return to main menu", [this]() { goBack(); } });
}

void GameMenuOverlay::buildContentManagerMenu()
{
    m_items.clear();
    m_items.push_back({ "TRACKS", "Browse and download new tracks",
        [this]() { if (onOpenContentBrowserRequested) onOpenContentBrowserRequested("tracks"); } });
    m_items.push_back({ "CARS", "Browse and download new vehicles",
        [this]() { if (onOpenContentBrowserRequested) onOpenContentBrowserRequested("cars"); } });
    m_items.push_back({ "BACK", "Return to main menu", [this]() { goBack(); } });
}

void GameMenuOverlay::buildSettingsMenu()
{
    m_items.clear();
    m_items.push_back({ "GRAPHICS", "Resolution, VSync, quality presets", [this]() {} });
    m_items.push_back({ "AUDIO", "Engine, wind, crowd volume levels",
        [this]() { if (onOpenSettingsPanelRequested) onOpenSettingsPanelRequested("audio"); } });
    m_items.push_back({ "BACK", "Return to main menu", [this]() { goBack(); } });
}

void GameMenuOverlay::buildControlsMenu()
{
    m_items.clear();
    m_items.push_back({ "KEYBOARD BINDINGS", "Reassign driving controls", [this]() {} });
    m_items.push_back({ "BACK", "Return to settings", [this]() { switchMenu(MenuState::Settings); } });
}

void GameMenuOverlay::buildDevModeConfirm()
{
    m_items.clear();
    m_items.push_back({ "YES, OPEN EDITOR", "Exit simulator and launch ksEditor",
        [this]() { if (onDevModeRequested) onDevModeRequested(); } });
    m_items.push_back({ "CANCEL", "Return to menu", [this]() { goBack(); } });
}

void GameMenuOverlay::buildQuitConfirm()
{
    m_items.clear();
    m_items.push_back({ "YES, QUIT", "Exit ksEditor Simulator",
        [this]() { if (onExitRequested) onExitRequested(); } });
    m_items.push_back({ "CANCEL", "Return to menu", [this]() { goBack(); } });
}

void GameMenuOverlay::switchMenu(MenuState state)
{
    m_previousState = m_currentState;
    m_currentState = state;
    m_selectedIndex = 0;
    m_hoverIndex = -1;
    m_transitionProgress = 0.0f;

    switch (state) {
    case MenuState::Main: buildMainMenu(); break;
    case MenuState::Singleplayer: buildSingleplayerMenu(); break;
    case MenuState::Multiplayer: buildMultiplayerMenu(); break;
    case MenuState::Profile: buildProfileMenu(); break;
    case MenuState::Garage: buildGarageMenu(); break;
    case MenuState::Replay: buildReplayMenu(); break;
    case MenuState::ContentManager: buildContentManagerMenu(); break;
    case MenuState::Settings: buildSettingsMenu(); break;
    case MenuState::Controls: buildControlsMenu(); break;
    case MenuState::DevModeConfirm: buildDevModeConfirm(); break;
    case MenuState::QuitConfirm: buildQuitConfirm(); break;
    }
}

void GameMenuOverlay::goBack()
{
    if (m_currentState == MenuState::Main) {
        setVisible(false);
        return;
    }
    if (m_currentState == MenuState::Controls) {
        switchMenu(MenuState::Settings);
        return;
    }
    if (m_currentState == MenuState::DevModeConfirm || m_currentState == MenuState::QuitConfirm) {
        switchMenu(MenuState::Main);
        return;
    }
    switchMenu(MenuState::Main);
}

void GameMenuOverlay::render(int width, int height)
{
    if (!m_visible && m_fadeAlpha <= 0.01f) return;

    m_animationTime += 0.02f;

    if (m_visible && m_fadeAlpha < 1.0f) {
        m_fadeAlpha = std::min(1.0f, m_fadeAlpha + 0.08f);
    } else if (!m_visible && m_fadeAlpha > 0.0f) {
        m_fadeAlpha = std::max(0.0f, m_fadeAlpha - 0.08f);
    }

    if (m_transitionProgress < 1.0f) {
        m_transitionProgress = std::min(1.0f, m_transitionProgress + 0.06f);
    }
    // Stubbed: No QPainter available in Win32/Vulkan path.
    // Menu will be rendered via Vulkan overlay or ImGui in the future.
}

bool GameMenuOverlay::handleKeyPress(int key)
{
    if (!m_visible) return false;

    bool isProfile = (m_currentState == MenuState::Profile);
    bool isConfirm = (m_currentState == MenuState::QuitConfirm || m_currentState == MenuState::DevModeConfirm);
    int maxFieldIndex = isProfile ? 6 : (isConfirm ? 1 : 0);

    // Use Windows virtual key codes
    switch (key) {
    case 0x26: { // VK_UP
        if (isProfile || isConfirm) {
            m_selectedIndex--;
            if (m_selectedIndex < 0) m_selectedIndex = maxFieldIndex;
        } else {
            int idx = m_selectedIndex;
            do {
                idx--;
                if (idx < 0) idx = static_cast<int>(m_items.size()) - 1;
            } while (m_items[idx].isSeparator && idx != m_selectedIndex);
            m_selectedIndex = idx;
        }
        return true;
    }
    case 0x28: { // VK_DOWN
        if (isProfile || isConfirm) {
            m_selectedIndex++;
            if (m_selectedIndex > maxFieldIndex) m_selectedIndex = 0;
        } else {
            int idx = m_selectedIndex;
            do {
                idx++;
                if (idx >= static_cast<int>(m_items.size())) idx = 0;
            } while (m_items[idx].isSeparator && idx != m_selectedIndex);
            m_selectedIndex = idx;
        }
        return true;
    }
    case 0x0D: // VK_RETURN
    case 0x60: { // VK_NUMPAD0 used as enter alternative
        if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size())) {
            const auto& item = m_items[m_selectedIndex];
            if (item.enabled && item.action) {
                item.action();
            }
        }
        return true;
    }
    case 0x1B: { // VK_ESCAPE
        goBack();
        return true;
    }
    default:
        break;
    }
    return false;
}

void GameMenuOverlay::handleMouseMove(const SimPoint& pos, int widgetWidth, int widgetHeight)
{
    (void)pos; (void)widgetWidth; (void)widgetHeight;
    if (!m_visible) return;
    // Stubbed: no mouse in pure keyboard mode
}

void GameMenuOverlay::handleClick(const SimPoint& pos, int widgetWidth, int widgetHeight)
{
    (void)pos; (void)widgetWidth; (void)widgetHeight;
    if (!m_visible) return;
    // Stubbed: no mouse in pure keyboard mode
}

} // namespace ks::sim
