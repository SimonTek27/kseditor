#pragma once
/** Native replacement for DeviceSettingsWidget (no QWidget). */
#include "NativeUiTypes.h"
#include <string>
#include <vector>
#include <functional>

namespace ks {
namespace sim {
namespace ui {

struct DeviceInfoRow {
    std::string name;
    std::string detail;
    bool connected = false;
};

class DeviceSettingsOverlay {
public:
    void setVisible(bool v) { m_visible = v; }
    bool isVisible() const { return m_visible; }
    void toggle() { m_visible = !m_visible; }

    void setDevices(std::vector<DeviceInfoRow> rows) { m_rows = std::move(rows); }
    void setSelected(int index) {
        if (index >= 0 && index < static_cast<int>(m_rows.size()))
            m_selected = index;
    }
    int selected() const { return m_selected; }

    void build(DrawList& dl, int screenW, int screenH) {
        if (!m_visible) return;
        const float panelW = 420.f;
        const float panelH = 360.f;
        const float x = (screenW - panelW) * 0.5f;
        const float y = (screenH - panelH) * 0.5f;

        dl.addRectFilled({0, 0, (float)screenW, (float)screenH}, Color::rgba(0, 0, 0, 0.45f));
        dl.addRectFilled({x, y, panelW, panelH}, Color::rgba(0.08f, 0.09f, 0.12f, 0.95f));
        dl.addRect({x, y, panelW, panelH}, Color::rgb(80, 160, 255), 2.f);
        dl.addText(x + 16, y + 16, "Device Settings", Color::rgb(220, 230, 255), 1.4f);

        float rowY = y + 56;
        for (int i = 0; i < static_cast<int>(m_rows.size()); ++i) {
            const auto& row = m_rows[static_cast<size_t>(i)];
            Rect rr{x + 12, rowY, panelW - 24, 40};
            Color bg = (i == m_selected)
                ? Color::rgba(0.15f, 0.25f, 0.4f, 1.f)
                : Color::rgba(0.12f, 0.13f, 0.16f, 1.f);
            dl.addRectFilled(rr, bg);
            std::string label = row.name + (row.connected ? "  [ON]" : "  [off]");
            dl.addText(rr.x + 10, rr.y + 8, label, Color::rgb(230, 230, 230), 1.1f);
            if (!row.detail.empty())
                dl.addText(rr.x + 10, rr.y + 24, row.detail, Color::rgb(160, 170, 180), 0.9f);
            rowY += 46;
        }

        dl.addText(x + 16, y + panelH - 28, "Up/Down select  Enter confirm  Esc close",
                   Color::rgb(140, 150, 160), 0.9f);
    }

    bool handleKey(int key) {
        if (!m_visible) return false;
        // Virtual key codes: up=38 down=40 enter=13 esc=27 (Win32 VK)
        if (key == 27) { m_visible = false; return true; }
        if (key == 38) { // up
            if (m_selected > 0) --m_selected;
            return true;
        }
        if (key == 40) { // down
            if (m_selected + 1 < static_cast<int>(m_rows.size())) ++m_selected;
            return true;
        }
        if (key == 13) {
            if (onDeviceSelected && m_selected >= 0 && m_selected < static_cast<int>(m_rows.size()))
                onDeviceSelected(m_selected, m_rows[static_cast<size_t>(m_selected)].name);
            return true;
        }
        return false;
    }

    std::function<void(int index, const std::string& name)> onDeviceSelected;

private:
    bool m_visible = false;
    int m_selected = 0;
    std::vector<DeviceInfoRow> m_rows;
};

} // namespace ui
} // namespace sim
} // namespace ks
