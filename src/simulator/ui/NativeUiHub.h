#pragma once
/**
 * Composes menu / dashboard / telemetry / device / multiplayer overlays
 * into one UiRenderer batch per frame.
 */
#include "UiRenderer.h"
#include "DeviceSettingsOverlay.h"
#include "MultiplayerOverlay.h"
#include "../GameMenuOverlay.h"
#include "../DashboardOverlay.h"
#include "../TelemetryOverlay.h"
#include <memory>

namespace ks {
namespace sim {
namespace ui {

class NativeUiHub {
public:
    NativeUiHub() {
        m_menu = std::make_unique<GameMenuOverlay>();
        m_dash = std::make_unique<DashboardOverlay>();
        m_telem = std::make_unique<TelemetryOverlay>();
        m_devices = std::make_unique<DeviceSettingsOverlay>();
        m_mp = std::make_unique<MultiplayerOverlay>();
    }

    GameMenuOverlay& menu() { return *m_menu; }
    DashboardOverlay& dashboard() { return *m_dash; }
    TelemetryOverlay& telemetry() { return *m_telem; }
    DeviceSettingsOverlay& devices() { return *m_devices; }
    MultiplayerOverlay& multiplayer() { return *m_mp; }
    UiRenderer& renderer() { return m_renderer; }

    void resize(int w, int h) { m_renderer.setViewport(w, h); }

    /** Build draw list for this frame. Call after physics/input update. */
    void renderFrame(int width, int height) {
        m_renderer.setViewport(width, height);
        m_renderer.beginFrame();
        auto& dl = m_renderer.list();

        // In-game HUD first (behind menus)
        if (m_dash->isVisible())
            m_dash->render(width, height); // legacy stub may no-op; also emit to dl below
        buildDashboard(dl, width, height);

        if (m_telem->isVisible())
            buildTelemetry(dl, width, height);

        // Modal overlays
        if (m_menu->isVisible())
            buildMenu(dl, width, height);

        m_devices->build(dl, width, height);
        m_mp->build(dl, width, height);

        m_renderer.endFrame();
    }

    bool handleKey(int key) {
        if (m_devices->isVisible() && m_devices->handleKey(key)) return true;
        if (m_mp->isVisible() && m_mp->handleKey(key)) return true;
        if (m_menu->isVisible() && m_menu->handleKeyPress(key)) return true;
        // F1 devices, F2 multiplayer, Esc menu
        if (key == 112) { m_devices->toggle(); return true; } // F1
        if (key == 113) { m_mp->toggle(); return true; }      // F2
        if (key == 27 && !m_menu->isVisible()) {
            m_menu->setVisible(true);
            return true;
        }
        return false;
    }

    bool blocksDrivingInput() const {
        return m_menu->isInputBlocked() || m_devices->isVisible() || m_mp->isVisible();
    }

private:
    void buildDashboard(DrawList& dl, int w, int h) {
        if (!m_dash->isVisible()) return;
        // Pull public-ish values via update pattern — use last telemetry mirror
        const float barW = 220.f;
        const float x = 24.f;
        const float y = static_cast<float>(h) - 120.f;
        dl.addRectFilled({x, y, barW, 88}, Color::rgba(0, 0, 0, 0.55f));
        dl.addText(x + 10, y + 8, "DASH", Color::rgb(200, 210, 220), 1.0f);
        // Throttle / brake bars filled from telemetry overlay state
        dl.addProgressBar({x + 10, y + 36, barW - 20, 12}, m_telem->m_throttle,
                          Color::rgb(80, 200, 100), Color::rgba(0.2f, 0.2f, 0.2f, 1.f));
        dl.addProgressBar({x + 10, y + 54, barW - 20, 12}, m_telem->m_brake,
                          Color::rgb(220, 80, 80), Color::rgba(0.2f, 0.2f, 0.2f, 1.f));
        dl.addText(x + 10, y + 72,
                   "SPD " + std::to_string(static_cast<int>(m_telem->m_speed * 3.6f)) +
                   "  RPM " + std::to_string(static_cast<int>(m_telem->m_rpm)),
                   Color::rgb(230, 230, 230), 1.0f);
        (void)w;
    }

    void buildTelemetry(DrawList& dl, int w, int h) {
        const float panelW = 280.f;
        const float x = static_cast<float>(w) - panelW - 16.f;
        const float y = 16.f;
        dl.addRectFilled({x, y, panelW, 160.f}, Color::rgba(0, 0, 0, 0.5f));
        dl.addText(x + 10, y + 8, "TELEMETRY", Color::rgb(180, 200, 255), 1.1f);
        dl.addText(x + 10, y + 32,
                   "LatG " + std::to_string(m_telem->m_lateralG).substr(0, 5),
                   Color::rgb(220, 220, 220), 1.0f);
        dl.addText(x + 10, y + 52,
                   "LonG " + std::to_string(m_telem->m_longitudinalG).substr(0, 5),
                   Color::rgb(220, 220, 220), 1.0f);
        dl.addProgressBar({x + 10, y + 80, panelW - 20, 10}, (m_telem->m_steering + 1.f) * 0.5f,
                          Color::rgb(100, 160, 255), Color::rgba(0.15f, 0.15f, 0.15f, 1.f));
        dl.addText(x + 10, y + 100, "Steer", Color::rgb(160, 170, 180), 0.9f);
        (void)h;
    }

    void buildMenu(DrawList& dl, int w, int h) {
        // Lightweight panel; detailed items still driven by GameMenuOverlay state machine
        const float panelW = 360.f;
        const float panelH = 400.f;
        const float x = (w - panelW) * 0.5f;
        const float y = (h - panelH) * 0.5f;
        dl.addRectFilled({0, 0, (float)w, (float)h}, Color::rgba(0, 0, 0, 0.55f));
        dl.addRectFilled({x, y, panelW, panelH}, Color::rgba(0.06f, 0.07f, 0.1f, 0.96f));
        dl.addRect({x, y, panelW, panelH}, Color::rgb(90, 140, 255), 2.f);
        dl.addText(x + 20, y + 20, "KS Simulator", Color::rgb(220, 230, 255), 1.6f);
        dl.addText(x + 20, y + 55, "Esc close  Enter select", Color::rgb(140, 150, 170), 0.95f);
        m_menu->render(w, h); // keeps existing menu logic hooks
    }

    UiRenderer m_renderer;
    std::unique_ptr<GameMenuOverlay> m_menu;
    std::unique_ptr<DashboardOverlay> m_dash;
    std::unique_ptr<TelemetryOverlay> m_telem;
    std::unique_ptr<DeviceSettingsOverlay> m_devices;
    std::unique_ptr<MultiplayerOverlay> m_mp;
};

} // namespace ui
} // namespace sim
} // namespace ks
