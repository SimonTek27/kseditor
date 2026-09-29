#pragma once
/** Native replacement for DeviceSettingsWidget (no QWidget). */
#include "NativeUiTypes.h"
#include "UiInput.h"
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

    void build(DrawList& dl, int screenW, int screenH, UiInput* input = nullptr) {
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

            bool hover = input && input->isHovering(rr);
            if (input && input->state().leftReleased() && rr.contains(input->state().x, input->state().y)) {
                m_selected = i;
                if (onDeviceSelected)
                    onDeviceSelected(i, row.name);
            }

            Color bg = (i == m_selected)
                ? Color::rgba(0.15f, 0.25f, 0.4f, 1.f)
                : (hover ? Color::rgba(0.14f, 0.16f, 0.22f, 1.f)
                         : Color::rgba(0.12f, 0.13f, 0.16f, 1.f));
            dl.addRectFilled(rr, bg);
            if (hover)
                dl.addRect(rr, Color::rgb(100, 180, 255), 1.f);

            std::string label = row.name + (row.connected ? "  [ON]" : "  [off]");
            dl.addText(rr.x + 10, rr.y + 8, label, Color::rgb(230, 230, 230), 1.1f);
            if (!row.detail.empty())
                dl.addText(rr.x + 10, rr.y + 24, row.detail, Color::rgb(160, 170, 180), 0.9f);
            rowY += 46;
        }

        // Close button
        Rect closeBtn{x + panelW - 100, y + panelH - 40, 80, 28};
        bool closeHover = input && input->isHovering(closeBtn);
        dl.addRectFilled(closeBtn, closeHover ? Color::rgba(0.3f, 0.15f, 0.15f, 1.f)
                                              : Color::rgba(0.2f, 0.12f, 0.12f, 1.f));
        dl.addText(closeBtn.x + 18, closeBtn.y + 6, "Close", Color::rgb(230, 200, 200), 1.f);
        if (input && input->state().leftReleased() && closeBtn.contains(input->state().x, input->state().y))
            m_visible = false;

        dl.addText(x + 16, y + panelH - 28, "Click row / Up-Down / Esc",
                   Color::rgb(140, 150, 160), 0.9f);
    }

    bool handleKey(int key) {
        if (!m_visible) return false;
        if (key == 27) { m_visible = false; return true; }
        if (key == 38) {
            if (m_selected > 0) --m_selected;
            return true;
        }
        if (key == 40) {
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

    /** Returns true if event was consumed (modal). */
    bool handleMouse(const MouseEvent& e) {
        if (!m_visible) return false;
        (void)e;
        return true; // modal: swallow mouse while open
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
