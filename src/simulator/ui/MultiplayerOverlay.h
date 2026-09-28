#pragma once
/** Native replacement for MultiplayerWidget (no QWidget). */
#include "NativeUiTypes.h"
#include <string>
#include <vector>
#include <functional>

namespace ks {
namespace sim {
namespace ui {

struct LobbyRow {
    std::string name;
    int players = 0;
    int maxPlayers = 16;
    std::string track;
};

class MultiplayerOverlay {
public:
    void setVisible(bool v) { m_visible = v; }
    bool isVisible() const { return m_visible; }
    void toggle() { m_visible = !m_visible; }

    void setHost(const std::string& host) { m_host = host; }
    void setPort(int port) { m_port = port; }
    void setLobbies(std::vector<LobbyRow> rows) { m_lobbies = std::move(rows); }

    void build(DrawList& dl, int screenW, int screenH) {
        if (!m_visible) return;
        const float panelW = 520.f;
        const float panelH = 420.f;
        const float x = (screenW - panelW) * 0.5f;
        const float y = (screenH - panelH) * 0.5f;

        dl.addRectFilled({0, 0, (float)screenW, (float)screenH}, Color::rgba(0, 0, 0, 0.5f));
        dl.addRectFilled({x, y, panelW, panelH}, Color::rgba(0.07f, 0.08f, 0.11f, 0.96f));
        dl.addRect({x, y, panelW, panelH}, Color::rgb(60, 200, 120), 2.f);
        dl.addText(x + 16, y + 16, "Multiplayer", Color::rgb(200, 255, 220), 1.5f);

        dl.addText(x + 16, y + 52,
                   "Host: " + m_host + "  Port: " + std::to_string(m_port),
                   Color::rgb(180, 190, 200), 1.0f);

        // Buttons
        m_hostBtn = {x + 16, y + 80, 140, 36};
        m_joinBtn = {x + 170, y + 80, 140, 36};
        m_refreshBtn = {x + 324, y + 80, 140, 36};
        drawButton(dl, m_hostBtn, "Host", m_focus == 0);
        drawButton(dl, m_joinBtn, "Join", m_focus == 1);
        drawButton(dl, m_refreshBtn, "Refresh", m_focus == 2);

        float rowY = y + 140;
        dl.addText(x + 16, rowY - 20, "Lobbies", Color::rgb(160, 170, 180), 1.0f);
        for (int i = 0; i < static_cast<int>(m_lobbies.size()); ++i) {
            const auto& L = m_lobbies[static_cast<size_t>(i)];
            Rect rr{x + 12, rowY, panelW - 24, 36};
            Color bg = (i == m_lobbySel)
                ? Color::rgba(0.12f, 0.28f, 0.2f, 1.f)
                : Color::rgba(0.11f, 0.12f, 0.15f, 1.f);
            dl.addRectFilled(rr, bg);
            std::string line = L.name + "  " + std::to_string(L.players) + "/" +
                               std::to_string(L.maxPlayers) + "  " + L.track;
            dl.addText(rr.x + 10, rr.y + 10, line, Color::rgb(230, 235, 240), 1.0f);
            rowY += 42;
        }

        dl.addText(x + 16, y + panelH - 28, "Tab focus  Enter action  Esc close",
                   Color::rgb(140, 150, 160), 0.9f);
    }

    bool handleKey(int key) {
        if (!m_visible) return false;
        if (key == 27) { m_visible = false; return true; }
        if (key == 9) { // tab
            m_focus = (m_focus + 1) % 3;
            return true;
        }
        if (key == 38 && m_lobbySel > 0) { --m_lobbySel; return true; }
        if (key == 40 && m_lobbySel + 1 < static_cast<int>(m_lobbies.size())) {
            ++m_lobbySel; return true;
        }
        if (key == 13) {
            if (m_focus == 0 && onHost) onHost(m_port);
            else if (m_focus == 1 && onJoin) {
                std::string lobby = m_lobbySel >= 0 && m_lobbySel < static_cast<int>(m_lobbies.size())
                    ? m_lobbies[static_cast<size_t>(m_lobbySel)].name : m_host;
                onJoin(lobby, m_port);
            } else if (m_focus == 2 && onRefresh) onRefresh();
            return true;
        }
        return false;
    }

    std::function<void(int port)> onHost;
    std::function<void(const std::string& host, int port)> onJoin;
    std::function<void()> onRefresh;

private:
    void drawButton(DrawList& dl, const Rect& r, const char* label, bool focused) {
        Color bg = focused ? Color::rgba(0.2f, 0.45f, 0.3f, 1.f)
                           : Color::rgba(0.14f, 0.16f, 0.18f, 1.f);
        dl.addRectFilled(r, bg);
        dl.addRect(r, focused ? Color::rgb(80, 220, 140) : Color::rgb(70, 80, 90), 1.5f);
        dl.addText(r.x + 36, r.y + 10, label, Color::rgb(240, 245, 250), 1.1f);
    }

    bool m_visible = false;
    std::string m_host = "127.0.0.1";
    int m_port = 7777;
    int m_focus = 0;
    int m_lobbySel = 0;
    std::vector<LobbyRow> m_lobbies;
    Rect m_hostBtn, m_joinBtn, m_refreshBtn;
};

} // namespace ui
} // namespace sim
} // namespace ks
