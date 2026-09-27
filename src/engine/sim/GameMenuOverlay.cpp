#include "GameMenuOverlay.h"

#include <algorithm>
#include <cmath>

namespace ks {
namespace sim {

GameMenuOverlay::GameMenuOverlay() {
    buildMainMenu();
    m_menuDirty = true;
}

void GameMenuOverlay::setVisible(bool visible) {
    if (m_visible == visible) return;
    m_visible = visible;
    m_menuDirty = true;
    m_ui.markDirty();
    if (visible) m_fadeAlpha = 0.f;
}

void GameMenuOverlay::toggleVisible() { setVisible(!m_visible); }

void GameMenuOverlay::switchMenu(MenuState state) {
    m_previousState = m_currentState;
    m_currentState = state;
    m_selectedIndex = 0;
    m_hoverIndex = -1;
    m_menuDirty = true;
    m_ui.markDirty();
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

void GameMenuOverlay::goBack() {
    if (m_currentState == MenuState::Main) return;
    switchMenu(MenuState::Main);
}

void GameMenuOverlay::buildMainMenu() {
    m_items.clear();
    m_items.push_back({"Start Driving", "Begin session", [this]{
        if (onStartDrivingRequested) onStartDrivingRequested();
        setVisible(false);
    }});
    m_items.push_back({"Singleplayer", "Career / practice", [this]{ switchMenu(MenuState::Singleplayer); }});
    m_items.push_back({"Multiplayer", "Online", [this]{ switchMenu(MenuState::Multiplayer); }});
    m_items.push_back({"Garage", "Car & setup", [this]{ switchMenu(MenuState::Garage); }});
    m_items.push_back({"Profile", m_profile.name, [this]{ switchMenu(MenuState::Profile); }});
    m_items.push_back({"Settings", "Graphics & controls", [this]{ switchMenu(MenuState::Settings); }});
    m_items.push_back({"Quit", "Exit simulator", [this]{ switchMenu(MenuState::QuitConfirm); }});
}

void GameMenuOverlay::buildSingleplayerMenu() {
    m_items.clear();
    m_items.push_back({"Practice", m_trackName.empty() ? "Select track" : m_trackName,
        [this]{ if (onLoadTrackRequested) onLoadTrackRequested(); }});
    m_items.push_back({"Load Car", m_carName.empty() ? "Select car" : m_carName,
        [this]{ if (onLoadCarRequested) onLoadCarRequested(); }});
    m_items.push_back({"Back", "", [this]{ goBack(); }});
}

void GameMenuOverlay::buildMultiplayerMenu() {
    m_items.clear();
    m_items.push_back({"Join Server", "Not connected", []{}});
    m_items.push_back({"Back", "", [this]{ goBack(); }});
}

void GameMenuOverlay::buildProfileMenu() {
    m_items.clear();
    m_items.push_back({"Name: " + m_profile.name, "Edit name", [this]{
        if (onTextInputRequested) onTextInputRequested("name", m_profile.name);
    }});
    m_items.push_back({"Nationality: " + m_profile.nationality, "Change", [this]{
        if (onNationalityInputRequested) onNationalityInputRequested();
    }});
    m_items.push_back({"Back", "", [this]{ goBack(); }});
}

void GameMenuOverlay::buildGarageMenu() {
    m_items.clear();
    m_items.push_back({"Open Garage", "", [this]{ if (onOpenGarageRequested) onOpenGarageRequested(); }});
    m_items.push_back({"Setup", "", [this]{ if (onOpenSetupGarageRequested) onOpenSetupGarageRequested(); }});
    m_items.push_back({"Back", "", [this]{ goBack(); }});
}

void GameMenuOverlay::buildReplayMenu() {
    m_items.clear();
    m_items.push_back({"Load Replay", "", [this]{ if (onLoadReplayRequested) onLoadReplayRequested(); }});
    m_items.push_back({"Record", "", [this]{ if (onRecordReplayRequested) onRecordReplayRequested(); }});
    m_items.push_back({"Back", "", [this]{ goBack(); }});
}

void GameMenuOverlay::buildContentManagerMenu() {
    m_items.clear();
    m_items.push_back({"Cars", "", [this]{
        if (onOpenContentBrowserRequested) onOpenContentBrowserRequested("cars"); }});
    m_items.push_back({"Tracks", "", [this]{
        if (onOpenContentBrowserRequested) onOpenContentBrowserRequested("tracks"); }});
    m_items.push_back({"Back", "", [this]{ goBack(); }});
}

void GameMenuOverlay::buildSettingsMenu() {
    m_items.clear();
    m_items.push_back({"Graphics", "", [this]{
        if (onOpenSettingsPanelRequested) onOpenSettingsPanelRequested("graphics"); }});
    m_items.push_back({"Controls", "", [this]{ switchMenu(MenuState::Controls); }});
    m_items.push_back({"Fullscreen", "", [this]{
        if (onToggleFullscreenRequested) onToggleFullscreenRequested(); }});
    m_items.push_back({"Back", "", [this]{ goBack(); }});
}

void GameMenuOverlay::buildControlsMenu() {
    m_items.clear();
    m_items.push_back({"Preset " + std::to_string(m_profile.controlsPreset), "", []{}});
    m_items.push_back({"Back", "", [this]{ goBack(); }});
}

void GameMenuOverlay::buildDevModeConfirm() {
    m_items.clear();
    m_items.push_back({"Enable Dev Mode", "", [this]{
        if (onDevModeRequested) onDevModeRequested(); goBack(); }});
    m_items.push_back({"Cancel", "", [this]{ goBack(); }});
}

void GameMenuOverlay::buildQuitConfirm() {
    m_items.clear();
    m_items.push_back({"Confirm Quit", "", [this]{ if (onExitRequested) onExitRequested(); }});
    m_items.push_back({"Cancel", "", [this]{ goBack(); }});
}

void GameMenuOverlay::setProfileField(const std::string& fieldName, const std::string& value) {
    if (fieldName == "name") m_profile.name = value;
    else if (fieldName == "nationality") m_profile.nationality = value;
    else if (fieldName == "bio") m_profile.bio = value;
    m_menuDirty = true;
    m_ui.markDirty();
    if (m_currentState == MenuState::Profile) buildProfileMenu();
    if (onProfileChanged) onProfileChanged(m_profile);
}

void GameMenuOverlay::setNationalityFromList(int index) {
    static const char* kNats[] = {
        "Italian","British","German","French","Spanish","American","Japanese","Brazilian"};
    if (index >= 0 && index < 8) setProfileField("nationality", kNats[index]);
}

bool GameMenuOverlay::handleKeyPress(int key) {
    if (!m_visible) return false;
    if (key == 0x1B) {
        if (m_currentState != MenuState::Main) goBack();
        else setVisible(false);
        return true;
    }
    if (key == 0x26) {
        if (m_selectedIndex > 0) { --m_selectedIndex; m_ui.markDirty(); }
        return true;
    }
    if (key == 0x28) {
        if (m_selectedIndex + 1 < static_cast<int>(m_items.size())) {
            ++m_selectedIndex; m_ui.markDirty();
        }
        return true;
    }
    if (key == 0x0D) {
        if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size())) {
            auto& it = m_items[static_cast<size_t>(m_selectedIndex)];
            if (it.enabled && it.action) it.action();
        }
        return true;
    }
    return false;
}

void GameMenuOverlay::handleMouseMove(const SimPoint& pos, int widgetWidth, int widgetHeight) {
    if (!m_visible || widgetHeight <= 0) return;
    const float panelTop = widgetHeight * 0.25f;
    const float rowH = 40.f;
    int hover = -1;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        float y = panelTop + i * rowH;
        if (pos.y >= y && pos.y < y + rowH) { hover = i; break; }
    }
    if (hover != m_hoverIndex) {
        m_hoverIndex = hover;
        if (hover >= 0) m_selectedIndex = hover;
        m_ui.markDirty();
    }
    (void)widgetWidth;
}

void GameMenuOverlay::handleClick(const SimPoint& pos, int widgetWidth, int widgetHeight) {
    handleMouseMove(pos, widgetWidth, widgetHeight);
    if (m_hoverIndex >= 0 && m_hoverIndex < static_cast<int>(m_items.size())) {
        auto& it = m_items[static_cast<size_t>(m_hoverIndex)];
        if (it.enabled && it.action) it.action();
    }
}

void GameMenuOverlay::buildDrawList(UiRenderer& ui) {
    if (!m_visible) return;
    const int w = ui.width();
    const int h = ui.height();
    const float alpha = std::clamp(m_fadeAlpha, 0.f, 1.f);

    ui.addQuad({0, 0, static_cast<float>(w), static_cast<float>(h),
                UiColor::rgba(0.f, 0.f, 0.f, 0.55f * alpha), 0});

    const float panelW = std::min(420.f, w * 0.4f);
    const float panelH = std::min(480.f, h * 0.7f);
    const float panelX = (w - panelW) * 0.5f;
    const float panelY = (h - panelH) * 0.5f;

    ui.addQuad({panelX, panelY, panelW, panelH,
                UiColor::rgba(0.08f, 0.09f, 0.12f, 0.92f * alpha), 1});
    ui.addQuad({panelX, panelY, 4.f, panelH,
                UiColor::rgba(0.2f, 0.55f, 1.f, 0.9f * alpha), 2});
    ui.addText({panelX + 24.f, panelY + 20.f, 22.f, "KS Simulator",
                UiColor::rgba(1.f, 1.f, 1.f, alpha), 3});

    const float rowH = 40.f;
    const float listY = panelY + 60.f;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        const auto& it = m_items[static_cast<size_t>(i)];
        const float y = listY + i * rowH;
        if (i == m_selectedIndex) {
            ui.addQuad({panelX + 8.f, y, panelW - 16.f, rowH - 4.f,
                        UiColor::rgba(0.2f, 0.4f, 0.75f, 0.55f * alpha), 2});
        }
        ui.addText({panelX + 28.f, y + 8.f, 16.f, it.text,
                    UiColor::rgba(1.f, 1.f, 1.f, (it.enabled ? 1.f : 0.4f) * alpha), 3});
        if (!it.description.empty()) {
            ui.addText({panelX + 28.f, y + 24.f, 11.f, it.description,
                        UiColor::rgba(0.7f, 0.75f, 0.8f, 0.8f * alpha), 3});
        }
    }
}

void GameMenuOverlay::render(int width, int height) {
    if (!m_visible) {
        m_fadeAlpha = std::max(0.f, m_fadeAlpha - 0.15f);
        if (m_fadeAlpha <= 0.f) return;
    } else {
        m_fadeAlpha = std::min(1.f, m_fadeAlpha + 0.12f);
    }

    if (width != m_lastW || height != m_lastH) {
        m_lastW = width;
        m_lastH = height;
        m_ui.setScreenSize(width, height);
        m_ui.markDirty();
    }

    if (m_ui.isDirty() || m_menuDirty) {
        m_ui.begin();
        buildDrawList(m_ui);
        m_ui.end();
        m_menuDirty = false;
    }

    m_ui.flush(nullptr);
}

} // namespace sim
} // namespace ks
