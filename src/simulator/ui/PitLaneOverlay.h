#pragma once
/**
 * Pit lane service board — cinematic ksim UI (Qt-free).
 * Live session actions: tyres, fuel, wings, confirm, progress.
 */
#include "NativeUiTypes.h"
#include "TextRenderer.h"
#include "UiInput.h"
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>

namespace ks {
namespace sim {
namespace ui {

enum class PitState : int {
    Closed = 0,
    Approach,
    InBox,
    Servicing,
    Ready
};

enum class TyreCompound : int {
    Soft = 0,
    Medium,
    Hard,
    Count
};

struct PitRequest {
    bool changeTyres = false;
    TyreCompound compound = TyreCompound::Medium;
    bool refuel = false;
    float fuelLitres = 0.f;      // target add
    bool frontWing = false;
    bool rearWing = false;
    bool damageCheck = false;
};

class PitLaneOverlay {
public:
    void setVisible(bool v) {
        if (v && m_state == PitState::Closed)
            m_state = PitState::Approach;
        if (!v) {
            m_state = PitState::Closed;
            m_fade = 0.f;
        } else {
            m_fade = std::max(m_fade, 0.01f);
        }
    }
    bool isVisible() const { return m_state != PitState::Closed || m_fade > 0.01f; }
    void toggle() { setVisible(m_state == PitState::Closed); }

    void setState(PitState s) { m_state = s; }
    PitState state() const { return m_state; }

    void setSessionInfo(int lap, int position) {
        m_lap = lap;
        m_position = position;
    }
    void setCarName(const std::string& n) { m_carName = n; }

    PitRequest& request() { return m_req; }
    const PitRequest& request() const { return m_req; }

    float estimatedSeconds() const { return computeEstimate(m_req); }
    float progress01() const { return m_progress; }

    /** Advance servicing timer; call each frame when Servicing. */
    void update(float dt) {
        const float target = (m_state != PitState::Closed) ? 1.f : 0.f;
        m_fade += ((target > m_fade) ? 1.f : -1.f) * dt * 6.f;
        m_fade = std::clamp(m_fade, 0.f, 1.f);

        if (m_state == PitState::Servicing) {
            const float total = std::max(0.5f, m_serviceTotal);
            m_serviceElapsed += dt;
            m_progress = std::clamp(m_serviceElapsed / total, 0.f, 1.f);
            if (m_progress >= 1.f) {
                m_state = PitState::Ready;
                m_progress = 1.f;
                if (onServiceComplete)
                    onServiceComplete(m_req);
            }
        }
    }

    bool handleKey(int key) {
        if (m_state == PitState::Closed && m_fade < 0.5f) return false;

        if (key == 0x1B) { // ESC
            if (m_state == PitState::Servicing)
                return true; // lock board during work
            setVisible(false);
            return true;
        }

        if (m_state == PitState::Servicing)
            return true;

        const int rowCount = 6; // 5 services + confirm
        switch (key) {
        case 0x26: // UP
            m_selected = (m_selected - 1 + rowCount) % rowCount;
            return true;
        case 0x28: // DOWN
            m_selected = (m_selected + 1) % rowCount;
            return true;
        case 0x25: // LEFT
            adjustSelected(-1);
            return true;
        case 0x27: // RIGHT
            adjustSelected(+1);
            return true;
        case 0x20: // SPACE
            toggleSelected();
            return true;
        case 0x0D: // ENTER
            if (m_selected == 5)
                confirmAndStart();
            else
                toggleSelected();
            return true;
        default:
            break;
        }
        return false;
    }

    void build(DrawList& dl, TextRenderer& text, int w, int h, UiInput* input = nullptr) {
        update(1.f / 60.f);
        const float a = m_fade;
        if (a <= 0.01f) return;

        auto sty = [&](const Color& c, float sc, bool bold = false) {
            TextStyle s;
            s.color = c;
            s.scale = sc;
            s.shadow = true;
            s.outline = bold;
            return s;
        };

        dl.addRectFilled({0, 0, (float)w, (float)h}, Color::rgba(0, 0, 0, 0.75f * a));
        dl.addRectFilled({0, 0, 8.f, (float)h}, Color::rgba(0.85f, 0.12f, 0.12f, 0.95f * a));

        text.draw(dl, 72.f, 36.f, "KSIM", sty(Color::rgba(1, 1, 1, a), 2.2f, true));
        text.draw(dl, 72.f, 72.f, "PIT LANE", sty(Color::rgba(0.75f, 0.75f, 0.78f, a), 1.15f));

        char hdr[64];
        std::snprintf(hdr, sizeof(hdr), "LAP %d  ·  P%d", m_lap, m_position);
        text.draw(dl, (float)w - 280.f, 48.f, hdr,
                  sty(Color::rgba(0.85f, 0.85f, 0.88f, a), 1.15f));

        // Status chip
        const char* st =
            m_state == PitState::Approach ? "ON APPROACH" :
            m_state == PitState::InBox ? "IN BOX" :
            m_state == PitState::Servicing ? "SERVICING" :
            m_state == PitState::Ready ? "READY" : "";
        Color stCol = m_state == PitState::Ready
            ? Color::rgba(0.2f, 0.85f, 0.35f, a)
            : m_state == PitState::Servicing
                ? Color::rgba(0.95f, 0.75f, 0.2f, a)
                : Color::rgba(0.9f, 0.25f, 0.25f, a);
        dl.addRectFilled({72.f, 104.f, 160.f, 26.f}, Color::rgba(stCol.r, stCol.g, stCol.b, 0.25f * a));
        text.draw(dl, 84.f, 110.f, st, sty(stCol, 1.05f, true));

        struct Row {
            const char* label;
            std::string value;
            bool on;
        };
        const char* compoundName =
            m_req.compound == TyreCompound::Soft ? "SOFT" :
            m_req.compound == TyreCompound::Medium ? "MEDIUM" : "HARD";

        char fuelStr[32];
        std::snprintf(fuelStr, sizeof(fuelStr), "%+.0f L", m_req.fuelLitres);

        Row rows[6] = {
            {"TYRE CHANGE", std::string(compoundName), m_req.changeTyres},
            {"REFUEL", fuelStr, m_req.refuel},
            {"FRONT WING", m_req.frontWing ? "REPAIR" : "—", m_req.frontWing},
            {"REAR WING", m_req.rearWing ? "REPAIR" : "—", m_req.rearWing},
            {"DAMAGE CHECK", m_req.damageCheck ? "ON" : "OFF", m_req.damageCheck},
            {"CONFIRM", m_state == PitState::Servicing ? "WORKING…" : "START SERVICE", false},
        };

        const float listLeft = 72.f;
        const float listTop = 150.f;
        const float listW = (float)w * 0.44f;
        const float rowH = 42.f;
        float iy = listTop;

        for (int i = 0; i < 6; ++i) {
            const bool sel = (i == m_selected);
            Rect rr{listLeft - 8.f, iy - 2.f, listW, rowH - 4.f};

            if (input && input->isHovering(rr) && input->state().leftReleased()) {
                m_selected = i;
                if (i == 5)
                    confirmAndStart();
                else
                    toggleSelected();
            }

            if (sel) {
                dl.addRectFilled(rr, Color::rgba(1, 1, 1, 0.07f * a));
                dl.addRectFilled({listLeft - 8.f, iy - 2.f, 4.f, rowH - 4.f},
                                 Color::rgba(0.9f, 0.15f, 0.15f, a));
            }

            // checkbox
            Rect box{listLeft + 8.f, iy + 10.f, 16.f, 16.f};
            dl.addRect(box, Color::rgba(0.8f, 0.8f, 0.85f, a), 1.5f);
            if (i < 5 && rows[i].on)
                dl.addRectFilled({box.x + 3, box.y + 3, 10.f, 10.f},
                                 Color::rgba(0.9f, 0.2f, 0.2f, a));

            text.draw(dl, listLeft + 36.f, iy + 10.f, rows[i].label,
                      sty(sel ? Color::rgba(1, 1, 1, a) : Color::rgba(0.72f, 0.72f, 0.75f, a),
                          sel ? 1.2f : 1.1f, sel));
            text.draw(dl, listLeft + listW - 140.f, iy + 10.f, rows[i].value,
                      sty(Color::rgba(0.9f, 0.9f, 0.92f, a), 1.1f));
            iy += rowH;
        }

        // Summary pane
        const float px = listLeft + listW + 36.f;
        const float py = listTop;
        const float pw = (float)w - px - 48.f;
        if (pw > 120.f) {
            dl.addRectFilled({px, py, pw, 200.f}, Color::rgba(0.05f, 0.05f, 0.07f, 0.9f * a));
            dl.addRectFilled({px, py, 3.f, 200.f}, Color::rgba(0.9f, 0.15f, 0.15f, 0.75f * a));

            text.draw(dl, px + 20.f, py + 20.f, "ESTIMATED TIME",
                      sty(Color::rgba(0.9f, 0.25f, 0.25f, a), 1.f, true));

            char tbuf[32];
            std::snprintf(tbuf, sizeof(tbuf), "%.1f s", estimatedSeconds());
            text.draw(dl, px + 20.f, py + 52.f, tbuf,
                      sty(Color::rgba(1, 1, 1, a), 1.8f, true));

            const float barV = (m_state == PitState::Servicing || m_state == PitState::Ready)
                ? m_progress
                : 0.f;
            dl.addProgressBar({px + 20.f, py + 100.f, pw - 40.f, 16.f}, barV,
                              Color::rgba(0.9f, 0.2f, 0.2f, a),
                              Color::rgba(0.15f, 0.15f, 0.18f, a));

            const char* tip =
                m_state == PitState::Ready ? "CLEAR TO LEAVE" :
                m_state == PitState::Servicing ? "CREW WORKING" :
                m_state == PitState::InBox ? "STOP ON MARKS · CONFIRM" :
                "PIT LIMITER · APPROACH BOX";
            text.draw(dl, px + 20.f, py + 140.f, tip,
                      sty(Color::rgba(0.7f, 0.7f, 0.74f, a), 1.05f));

            if (!m_carName.empty())
                text.draw(dl, px + 20.f, py + 168.f, m_carName,
                          sty(Color::rgba(0.55f, 0.55f, 0.6f, a), 1.f));
        }

        text.draw(dl, 72.f, (float)h - 40.f,
                  "UP/DOWN  select    SPACE  toggle    LEFT/RIGHT  adjust    ENTER  confirm    ESC  close",
                  sty(Color::rgba(0.5f, 0.5f, 0.55f, a), 0.9f));
    }

    std::function<void(const PitRequest&)> onServiceStarted;
    std::function<void(const PitRequest&)> onServiceComplete;

private:
    static float computeEstimate(const PitRequest& r) {
        const float base = 2.0f;
        float tyres = r.changeTyres ? 2.8f : 0.f;
        float fuel = r.refuel ? r.fuelLitres * 0.12f : 0.f;
        float wf = r.frontWing ? 3.5f : 0.f;
        float wr = r.rearWing ? 3.0f : 0.f;
        float insp = r.damageCheck ? 1.5f : 0.f;
        // tyre crew vs body/fuel crew in parallel
        return base + std::max(tyres, fuel + wf + wr + insp);
    }

    void toggleSelected() {
        switch (m_selected) {
        case 0: m_req.changeTyres = !m_req.changeTyres; break;
        case 1: m_req.refuel = !m_req.refuel; break;
        case 2: m_req.frontWing = !m_req.frontWing; break;
        case 3: m_req.rearWing = !m_req.rearWing; break;
        case 4: m_req.damageCheck = !m_req.damageCheck; break;
        default: break;
        }
    }

    void adjustSelected(int dir) {
        if (m_selected == 0) {
            int c = static_cast<int>(m_req.compound) + dir;
            c = (c % 3 + 3) % 3;
            m_req.compound = static_cast<TyreCompound>(c);
            m_req.changeTyres = true;
        } else if (m_selected == 1) {
            m_req.fuelLitres = std::clamp(m_req.fuelLitres + dir * 5.f, 0.f, 80.f);
            m_req.refuel = m_req.fuelLitres > 0.f;
        }
    }

    void confirmAndStart() {
        if (m_state == PitState::Servicing || m_state == PitState::Ready)
            return;
        // Allow start from InBox; from Approach still arm request
        if (m_state == PitState::Approach)
            m_state = PitState::InBox;
        m_serviceTotal = computeEstimate(m_req);
        m_serviceElapsed = 0.f;
        m_progress = 0.f;
        m_state = PitState::Servicing;
        if (onServiceStarted)
            onServiceStarted(m_req);
    }

    PitState m_state = PitState::Closed;
    float m_fade = 0.f;
    int m_selected = 0;
    int m_lap = 1;
    int m_position = 1;
    std::string m_carName;
    PitRequest m_req;
    float m_serviceTotal = 1.f;
    float m_serviceElapsed = 0.f;
    float m_progress = 0.f;
};

} // namespace ui
} // namespace sim
} // namespace ks
