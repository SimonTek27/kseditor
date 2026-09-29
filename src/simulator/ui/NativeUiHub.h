#pragma once
/**
 * Composes menu / dashboard / telemetry / device / multiplayer overlays
 * into one UiRenderer batch per frame. Text via TextRenderer + FontAtlas.
 */
#include "UiRenderer.h"
#include "TextRenderer.h"
#include "DeviceSettingsOverlay.h"
#include "MultiplayerOverlay.h"
#include "../GameMenuOverlay.h"
#include "../DashboardOverlay.h"
#include "../TelemetryOverlay.h"
#include <memory>
#include <string>

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
        // Share font atlas between renderer and text API
        m_text = TextRenderer(m_renderer.atlasPtr ? nullptr : nullptr);
        // Keep renderer font; TextRenderer uses its own default atlas — rebind:
        m_text = TextRenderer(std::make_shared<FontAtlas>());
        m_renderer.setFont(m_text.atlasPtr());
    }

    GameMenuOverlay& menu() { return *m_menu; }
    DashboardOverlay& dashboard() { return *m_dash; }
    TelemetryOverlay& telemetry() { return *m_telem; }
    DeviceSettingsOverlay& devices() { return *m_devices; }
    MultiplayerOverlay& multiplayer() { return *m_mp; }
    UiRenderer& renderer() { return m_renderer; }
    TextRenderer& text() { return m_text; }

    void resize(int w, int h) { m_renderer.setViewport(w, h); }

    void renderFrame(int width, int height) {
        m_renderer.setViewport(width, height);
        m_renderer.beginFrame();
        auto& dl = m_renderer.list();

        if (m_dash->isVisible()) {
            m_dash->render(width, height);
            buildDashboard(dl, width, height);
        }
        if (m_telem->isVisible())
            buildTelemetry(dl, width, height);
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
        if (key == 112) { m_devices->toggle(); return true; }
        if (key == 113) { m_mp->toggle(); return true; }
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
    void buildDashboard(DrawList& dl, int /*w*/, int h) {
        const float barW = 240.f;
        const float x = 24.f;
        const float y = static_cast<float>(h) - 130.f;

        dl.addRectFilled({x, y, barW, 100}, Color::rgba(0, 0, 0, 0.6f));
        dl.addRect({x, y, barW, 100}, Color::rgb(60, 100, 160), 1.f);

        m_text.draw(dl, x + 12, y + 8, "DASH", TextRenderer::hudLabel());

        dl.addProgressBar({x + 12, y + 36, barW - 24, 14}, m_telem->m_throttle,
                          Color::rgb(80, 210, 100), Color::rgba(0.15f, 0.15f, 0.15f, 1.f));
        dl.addProgressBar({x + 12, y + 54, barW - 24, 14}, m_telem->m_brake,
                          Color::rgb(230, 70, 70), Color::rgba(0.15f, 0.15f, 0.15f, 1.f));

        const int spd = static_cast<int>(m_telem->m_speed * 3.6f);
        const int rpm = static_cast<int>(m_telem->m_rpm);
        char line[64];
        std::snprintf(line, sizeof(line), "SPD %d  RPM %d", spd, rpm);
        m_text.draw(dl, x + 12, y + 74, line, TextRenderer::hudValue());
    }

    void buildTelemetry(DrawList& dl, int w, int /*h*/) {
        const float panelW = 300.f;
        const float x = static_cast<float>(w) - panelW - 16.f;
        const float y = 16.f;

        dl.addRectFilled({x, y, panelW, 170.f}, Color::rgba(0, 0, 0, 0.55f));
        dl.addRect({x, y, panelW, 170.f}, Color::rgb(80, 120, 200), 1.f);

        m_text.draw(dl, x + 12, y + 10, "TELEMETRY", TextRenderer::hudLabel());

        char buf[64];
        std::snprintf(buf, sizeof(buf), "Lat G  %.2f", m_telem->m_lateralG);
        m_text.draw(dl, x + 12, y + 36, buf, TextRenderer::hudValue());
        std::snprintf(buf, sizeof(buf), "Lon G  %.2f", m_telem->m_longitudinalG);
        m_text.draw(dl, x + 12, y + 56, buf, TextRenderer::hudValue());

        dl.addProgressBar({x + 12, y + 88, panelW - 24, 12},
                          (m_telem->m_steering + 1.f) * 0.5f,
                          Color::rgb(100, 170, 255), Color::rgba(0.12f, 0.12f, 0.12f, 1.f));
        m_text.draw(dl, x + 12, y + 110, "STEER", TextRenderer::hudLabel());

        std::snprintf(buf, sizeof(buf), "THR %.0f%%  BRK %.0f%%",
                      m_telem->m_throttle * 100.f, m_telem->m_brake * 100.f);
        m_text.draw(dl, x + 12, y + 140, buf, TextRenderer::hudLabel());
    }

    void buildMenu(DrawList& dl, int w, int h) {
        const float panelW = 380.f;
        const float panelH = 420.f;
        const float x = (w - panelW) * 0.5f;
        const float y = (h - panelH) * 0.5f;

        dl.addRectFilled({0, 0, (float)w, (float)h}, Color::rgba(0, 0, 0, 0.55f));
        dl.addRectFilled({x, y, panelW, panelH}, Color::rgba(0.06f, 0.07f, 0.1f, 0.96f));
        dl.addRect({x, y, panelW, panelH}, Color::rgb(90, 140, 255), 2.f);

        TextStyle title = TextRenderer::menuTitle();
        title.align = TextAlign::Left;
        m_text.draw(dl, x + 24, y + 24, "KS Simulator", title);

        m_text.draw(dl, x + 24, y + 70, "Esc close   Enter select",
                    TextRenderer::menuItem());

        // Simple static items (state machine still in GameMenuOverlay)
        const char* items[] = {
            "Start Driving",
            "Load Track",
            "Garage",
            "Multiplayer",
            "Settings",
            "Quit"
        };
        float iy = y + 120.f;
        for (const char* it : items) {
            dl.addRectFilled({x + 20, iy - 4, panelW - 40, 28}, Color::rgba(0.12f, 0.14f, 0.18f, 1.f));
            m_text.draw(dl, x + 32, iy + 4, it, TextRenderer::menuItem());
            iy += 36.f;
        }

        m_menu->render(w, h);
    }

    UiRenderer m_renderer;
    TextRenderer m_text;
    std::unique_ptr<GameMenuOverlay> m_menu;
    std::unique_ptr<DashboardOverlay> m_dash;
    std::unique_ptr<TelemetryOverlay> m_telem;
    std::unique_ptr<DeviceSettingsOverlay> m_devices;
    std::unique_ptr<MultiplayerOverlay> m_mp;
};

} // namespace ui
} // namespace sim
} // namespace ks
