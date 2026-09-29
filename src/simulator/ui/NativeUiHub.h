#pragma once
/**
 * Composes overlays into one UiRenderer batch; routes keyboard + mouse.
 * Main menu: cinematic racing layout (full scrim, large type, accent bar).
 * Product branding: ksim only — no third-party titles.
 */
#include "UiRenderer.h"
#include "TextRenderer.h"
#include "UiInput.h"
#include "DeviceSettingsOverlay.h"
#include "MultiplayerOverlay.h"
#include "../GameMenuOverlay.h"
#include "../DashboardOverlay.h"
#include "../TelemetryOverlay.h"
#include <memory>
#include <string>
#include <cstdio>
#include <algorithm>
#include <cmath>

namespace ks {
namespace sim {
namespace ui {

class NativeUiHub {
public:
    NativeUiHub()
        : m_text(std::make_shared<FontAtlas>())
    {
        m_menu = std::make_unique<GameMenuOverlay>();
        m_dash = std::make_unique<DashboardOverlay>();
        m_telem = std::make_unique<TelemetryOverlay>();
        m_devices = std::make_unique<DeviceSettingsOverlay>();
        m_mp = std::make_unique<MultiplayerOverlay>();
        m_renderer.setFont(m_text.atlasPtr());
    }

    GameMenuOverlay& menu() { return *m_menu; }
    DashboardOverlay& dashboard() { return *m_dash; }
    TelemetryOverlay& telemetry() { return *m_telem; }
    DeviceSettingsOverlay& devices() { return *m_devices; }
    MultiplayerOverlay& multiplayer() { return *m_mp; }
    UiRenderer& renderer() { return m_renderer; }
    TextRenderer& text() { return m_text; }
    UiInput& input() { return m_input; }

    void resize(int w, int h) {
        m_viewW = w;
        m_viewH = h;
        m_renderer.setViewport(w, h);
    }

    void renderFrame(int width, int height) {
        m_viewW = width;
        m_viewH = height;
        m_renderer.setViewport(width, height);

        m_input.beginFrame();
        m_renderer.beginFrame();
        auto& dl = m_renderer.list();

        if (m_dash->isVisible()) {
            m_dash->render(width, height);
            buildDashboard(dl, width, height);
        }
        if (m_telem->isVisible())
            buildTelemetry(dl, width, height);

        m_menu->render(width, height);
        if (m_menu->isVisible() || m_menu->fadeAlpha() > 0.01f)
            buildCinematicMenu(dl, width, height);

        m_devices->build(dl, width, height, &m_input);
        m_mp->build(dl, width, height, &m_input);

        m_renderer.endFrame();
        m_input.endFrame();
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

    bool handleMouse(const MouseEvent& e) {
        m_input.inject(e);
        if (e.type == MouseEventType::Down && e.button == MouseButton::Left)
            m_input.notePressPosition();

        if (m_devices->isVisible() && m_devices->handleMouse(e)) return true;
        if (m_mp->isVisible() && m_mp->handleMouse(e)) return true;

        if (m_menu->isVisible() || m_menu->fadeAlpha() > 0.5f) {
            updateMenuHover(e.x, e.y, m_viewW, m_viewH);
            if (e.type == MouseEventType::Up && e.button == MouseButton::Left &&
                m_menu->hoverIndex() >= 0) {
                m_menu->setSelectedIndex(m_menu->hoverIndex());
                m_menu->handleClick({(int)e.x, (int)e.y}, m_viewW, m_viewH);
            }
            return true;
        }
        return false;
    }

    bool handleMouseMove(float x, float y) {
        MouseEvent e; e.type = MouseEventType::Move; e.x = x; e.y = y;
        return handleMouse(e);
    }

    bool handleMouseButton(MouseButton b, bool down, float x, float y) {
        MouseEvent e;
        e.type = down ? MouseEventType::Down : MouseEventType::Up;
        e.button = b; e.x = x; e.y = y;
        return handleMouse(e);
    }

    bool handleMouseWheel(float delta, float x, float y) {
        MouseEvent e; e.type = MouseEventType::Wheel; e.wheelDelta = delta; e.x = x; e.y = y;
        return handleMouse(e);
    }

    bool blocksDrivingInput() const {
        return m_menu->isInputBlocked() || m_devices->isVisible() || m_mp->isVisible();
    }

private:
    // Layout for cinematic menu (left rail + detail pane)
    struct MenuLayout {
        float left = 72.f;
        float top = 140.f;
        float rowH = 44.f;
        float listW = 420.f;
        float accentW = 4.f;
    };

    void updateMenuHover(float mx, float my, int w, int h) {
        (void)w; (void)h;
        MenuLayout L;
        const auto& items = m_menu->items();
        int hover = -1;
        float iy = L.top;
        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            if (items[i].isSeparator) {
                iy += L.rowH * 0.45f;
                continue;
            }
            Rect rr{L.left, iy, L.listW, L.rowH - 4.f};
            if (rr.contains(mx, my))
                hover = i;
            iy += L.rowH;
        }
        m_menu->setHoverIndex(hover);
        if (hover >= 0)
            m_menu->setSelectedIndex(hover);
    }

    void buildCinematicMenu(DrawList& dl, int w, int h) {
        const float a = m_menu->fadeAlpha();
        if (a <= 0.01f) return;

        const float t = m_menu->transitionProgress();
        const float slide = (1.f - t) * 40.f;

        // Full-screen dark scrim + subtle left vignette bar
        dl.addRectFilled({0, 0, (float)w, (float)h}, Color::rgba(0.f, 0.f, 0.f, 0.72f * a));
        dl.addRectFilled({0, 0, 8.f, (float)h}, Color::rgba(0.85f, 0.12f, 0.12f, 0.9f * a));

        // Top brand strip
        m_text.draw(dl, 72.f + slide, 48.f, "KSIM",
                    TextStyle{TextAlign::Left, Color::rgba(1, 1, 1, a), 2.4f, true});
        m_text.draw(dl, 72.f + slide, 88.f, m_menu->sectionTitle().c_str(),
                    TextStyle{TextAlign::Left, Color::rgba(0.75f, 0.75f, 0.78f, a), 1.1f, false});

        // Context line (track / car)
        char ctx[128];
        std::snprintf(ctx, sizeof(ctx), "%s  ·  %s",
                      m_menu->carName().empty() ? "Vehicle" : m_menu->carName().c_str(),
                      m_menu->trackName().empty() ? "Circuit" : m_menu->trackName().c_str());
        m_text.draw(dl, 72.f + slide, static_cast<float>(h) - 48.f, ctx,
                    TextStyle{TextAlign::Left, Color::rgba(0.55f, 0.55f, 0.6f, a), 1.f, false});

        MenuLayout L;
        L.left += slide;
        const auto& items = m_menu->items();
        const int sel = m_menu->selectedIndex();
        float iy = L.top;

        std::string desc;
        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            const auto& it = items[i];
            if (it.isSeparator) {
                // thin rule
                dl.addRectFilled({L.left, iy + L.rowH * 0.15f, L.listW * 0.55f, 1.f},
                                 Color::rgba(1, 1, 1, 0.12f * a));
                iy += L.rowH * 0.45f;
                continue;
            }

            const bool selected = (i == sel);
            const bool hover = (i == m_menu->hoverIndex());

            if (selected || hover) {
                // selection plate
                dl.addRectFilled({L.left - 12.f, iy - 2.f, L.listW + 24.f, L.rowH - 2.f},
                                 Color::rgba(1.f, 1.f, 1.f, 0.06f * a));
                // accent bar
                dl.addRectFilled({L.left - 12.f, iy - 2.f, L.accentW, L.rowH - 2.f},
                                 Color::rgba(0.9f, 0.15f, 0.15f, a));
            }

            Color titleCol = selected
                ? Color::rgba(1.f, 1.f, 1.f, a)
                : Color::rgba(0.72f, 0.72f, 0.75f, a);
            m_text.draw(dl, L.left + 8.f, iy + 10.f, it.text.c_str(),
                        TextStyle{TextAlign::Left, titleCol, selected ? 1.35f : 1.2f, selected});

            if (selected)
                desc = it.description;

            iy += L.rowH;
        }

        // Right detail pane
        if (!desc.empty()) {
            const float px = static_cast<float>(w) * 0.52f;
            const float py = L.top;
            const float pw = static_cast<float>(w) - px - 64.f;
            if (pw > 120.f) {
                dl.addRectFilled({px, py, pw, 160.f}, Color::rgba(0.05f, 0.05f, 0.07f, 0.85f * a));
                dl.addRectFilled({px, py, 3.f, 160.f}, Color::rgba(0.9f, 0.15f, 0.15f, 0.7f * a));
                m_text.draw(dl, px + 20.f, py + 24.f, "INFO",
                            TextStyle{TextAlign::Left, Color::rgba(0.9f, 0.25f, 0.25f, a), 1.f, true});
                m_text.draw(dl, px + 20.f, py + 56.f, desc.c_str(),
                            TextStyle{TextAlign::Left, Color::rgba(0.85f, 0.85f, 0.88f, a), 1.15f, false});
            }
        }

        // Footer hints
        m_text.draw(dl, static_cast<float>(w) - 280.f, static_cast<float>(h) - 48.f,
                    "ENTER  select    ESC  back",
                    TextStyle{TextAlign::Left, Color::rgba(0.5f, 0.5f, 0.55f, a), 0.95f, false});
    }

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

        char line[64];
        std::snprintf(line, sizeof(line), "SPD %d  RPM %d",
                      static_cast<int>(m_telem->m_speed * 3.6f),
                      static_cast<int>(m_telem->m_rpm));
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

    UiRenderer m_renderer;
    TextRenderer m_text;
    UiInput m_input;
    int m_viewW = 1280, m_viewH = 720;

    std::unique_ptr<GameMenuOverlay> m_menu;
    std::unique_ptr<DashboardOverlay> m_dash;
    std::unique_ptr<TelemetryOverlay> m_telem;
    std::unique_ptr<DeviceSettingsOverlay> m_devices;
    std::unique_ptr<MultiplayerOverlay> m_mp;
};

} // namespace ui
} // namespace sim
} // namespace ks
