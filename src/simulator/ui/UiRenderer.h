#pragma once
/**
 * Batches DrawList into a dynamic vertex buffer (dirty-flag rebuild).
 * GPU upload is left to NativeRenderer / platform; this produces CPU verts.
 */
#include "NativeUiTypes.h"
#include <vector>
#include <cstdint>

namespace ks {
namespace sim {
namespace ui {

class UiRenderer {
public:
    void setViewport(int width, int height) {
        if (width != m_w || height != m_h) {
            m_w = width;
            m_h = height;
            m_dirty = true;
        }
    }

    void beginFrame() {
        m_list.clear();
        m_dirty = true;
    }

    DrawList& list() { return m_list; }
    const DrawList& list() const { return m_list; }

    void endFrame() {
        if (m_dirty)
            rebuildVertices();
        m_dirty = false;
    }

    const std::vector<UiVertex>& vertices() const { return m_vertices; }
    const std::vector<uint32_t>& indices() const { return m_indices; }
    int viewportWidth() const { return m_w; }
    int viewportHeight() const { return m_h; }

private:
    void pushQuad(float x0, float y0, float x1, float y1, const Color& c) {
        const uint32_t base = static_cast<uint32_t>(m_vertices.size());
        auto vert = [&](float x, float y, float u, float v) {
            m_vertices.push_back({x, y, c.r, c.g, c.b, c.a, u, v});
        };
        vert(x0, y0, 0, 0);
        vert(x1, y0, 1, 0);
        vert(x1, y1, 1, 1);
        vert(x0, y1, 0, 1);
        m_indices.push_back(base + 0);
        m_indices.push_back(base + 1);
        m_indices.push_back(base + 2);
        m_indices.push_back(base + 0);
        m_indices.push_back(base + 2);
        m_indices.push_back(base + 3);
    }

    void rebuildVertices() {
        m_vertices.clear();
        m_indices.clear();
        for (const auto& cmd : m_list.commands()) {
            switch (cmd.type) {
            case DrawCmdType::RectFilled:
            case DrawCmdType::ProgressBar:
                pushQuad(cmd.x0, cmd.y0, cmd.x1, cmd.y1, cmd.color);
                break;
            case DrawCmdType::RectOutline: {
                const float t = cmd.thickness;
                pushQuad(cmd.x0, cmd.y0, cmd.x1, cmd.y0 + t, cmd.color);
                pushQuad(cmd.x0, cmd.y1 - t, cmd.x1, cmd.y1, cmd.color);
                pushQuad(cmd.x0, cmd.y0, cmd.x0 + t, cmd.y1, cmd.color);
                pushQuad(cmd.x1 - t, cmd.y0, cmd.x1, cmd.y1, cmd.color);
                break;
            }
            case DrawCmdType::Line: {
                // Axis-aligned approximation: thin quad along segment
                float dx = cmd.x1 - cmd.x0;
                float dy = cmd.y1 - cmd.y0;
                float len = std::sqrt(dx * dx + dy * dy);
                if (len < 1e-3f) break;
                float nx = -dy / len * (cmd.thickness * 0.5f);
                float ny = dx / len * (cmd.thickness * 0.5f);
                const uint32_t base = static_cast<uint32_t>(m_vertices.size());
                auto vert = [&](float x, float y) {
                    m_vertices.push_back({x, y, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a, 0, 0});
                };
                vert(cmd.x0 + nx, cmd.y0 + ny);
                vert(cmd.x0 - nx, cmd.y0 - ny);
                vert(cmd.x1 - nx, cmd.y1 - ny);
                vert(cmd.x1 + nx, cmd.y1 + ny);
                m_indices.push_back(base + 0);
                m_indices.push_back(base + 1);
                m_indices.push_back(base + 2);
                m_indices.push_back(base + 0);
                m_indices.push_back(base + 2);
                m_indices.push_back(base + 3);
                break;
            }
            case DrawCmdType::Text: {
                // Bitmap-less: each char ≈ 8x12 px block row (debug glyphs)
                float cx = cmd.x0;
                const float chW = 8.f * cmd.fontScale;
                const float chH = 12.f * cmd.fontScale;
                for (unsigned char ch : cmd.text) {
                    if (ch == ' ') { cx += chW * 0.6f; continue; }
                    pushQuad(cx, cmd.y0, cx + chW * 0.85f, cmd.y0 + chH, cmd.color);
                    cx += chW;
                }
                break;
            }
            }
        }
    }

    DrawList m_list;
    std::vector<UiVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    int m_w = 1280;
    int m_h = 720;
    bool m_dirty = true;
};

} // namespace ui
} // namespace sim
} // namespace ks
