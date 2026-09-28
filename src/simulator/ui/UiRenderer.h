#pragma once
/**
 * Batches DrawList into a dynamic vertex buffer (dirty-flag rebuild).
 * Text uses FontAtlas glyph UVs; solid quads use the atlas white texel.
 */
#include "NativeUiTypes.h"
#include "FontAtlas.h"
#include <vector>
#include <cstdint>
#include <memory>

namespace ks {
namespace sim {
namespace ui {

class UiRenderer {
public:
    UiRenderer() : m_font(std::make_shared<FontAtlas>()) {}

    void setFont(std::shared_ptr<FontAtlas> font) {
        if (font) {
            m_font = std::move(font);
            m_dirty = true;
        }
    }
    const FontAtlas& font() const { return *m_font; }
    FontAtlas& font() { return *m_font; }

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

    /** Upload this R8 atlas to GPU once (width/height/pixels). */
    const std::vector<uint8_t>& atlasPixels() const { return m_font->pixelsR8(); }
    int atlasWidth() const { return m_font->width(); }
    int atlasHeight() const { return m_font->height(); }

private:
    void pushQuadUV(float x0, float y0, float x1, float y1,
                    float u0, float v0, float u1, float v1, const Color& c) {
        const uint32_t base = static_cast<uint32_t>(m_vertices.size());
        m_vertices.push_back({x0, y0, c.r, c.g, c.b, c.a, u0, v0});
        m_vertices.push_back({x1, y0, c.r, c.g, c.b, c.a, u1, v0});
        m_vertices.push_back({x1, y1, c.r, c.g, c.b, c.a, u1, v1});
        m_vertices.push_back({x0, y1, c.r, c.g, c.b, c.a, u0, v1});
        m_indices.push_back(base + 0);
        m_indices.push_back(base + 1);
        m_indices.push_back(base + 2);
        m_indices.push_back(base + 0);
        m_indices.push_back(base + 2);
        m_indices.push_back(base + 3);
    }

    void pushSolid(float x0, float y0, float x1, float y1, const Color& c) {
        float u0, v0, u1, v1;
        m_font->whiteUV(u0, v0, u1, v1);
        pushQuadUV(x0, y0, x1, y1, u0, v0, u1, v1, c);
    }

    void rebuildVertices() {
        m_vertices.clear();
        m_indices.clear();
        for (const auto& cmd : m_list.commands()) {
            switch (cmd.type) {
            case DrawCmdType::RectFilled:
            case DrawCmdType::ProgressBar:
                pushSolid(cmd.x0, cmd.y0, cmd.x1, cmd.y1, cmd.color);
                break;
            case DrawCmdType::RectOutline: {
                const float t = cmd.thickness;
                pushSolid(cmd.x0, cmd.y0, cmd.x1, cmd.y0 + t, cmd.color);
                pushSolid(cmd.x0, cmd.y1 - t, cmd.x1, cmd.y1, cmd.color);
                pushSolid(cmd.x0, cmd.y0, cmd.x0 + t, cmd.y1, cmd.color);
                pushSolid(cmd.x1 - t, cmd.y0, cmd.x1, cmd.y1, cmd.color);
                break;
            }
            case DrawCmdType::Line: {
                float dx = cmd.x1 - cmd.x0;
                float dy = cmd.y1 - cmd.y0;
                float len = std::sqrt(dx * dx + dy * dy);
                if (len < 1e-3f) break;
                float nx = -dy / len * (cmd.thickness * 0.5f);
                float ny = dx / len * (cmd.thickness * 0.5f);
                float u0, v0, u1, v1;
                m_font->whiteUV(u0, v0, u1, v1);
                const uint32_t base = static_cast<uint32_t>(m_vertices.size());
                m_vertices.push_back({cmd.x0 + nx, cmd.y0 + ny, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a, u0, v0});
                m_vertices.push_back({cmd.x0 - nx, cmd.y0 - ny, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a, u1, v0});
                m_vertices.push_back({cmd.x1 - nx, cmd.y1 - ny, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a, u1, v1});
                m_vertices.push_back({cmd.x1 + nx, cmd.y1 + ny, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a, u0, v1});
                m_indices.push_back(base + 0);
                m_indices.push_back(base + 1);
                m_indices.push_back(base + 2);
                m_indices.push_back(base + 0);
                m_indices.push_back(base + 2);
                m_indices.push_back(base + 3);
                break;
            }
            case DrawCmdType::Text: {
                float cx = cmd.x0;
                float cy = cmd.y0;
                const float scale = cmd.fontScale;
                for (unsigned char ch : cmd.text) {
                    if (ch == '\n') {
                        cx = cmd.x0;
                        cy += m_font->lineHeight(scale);
                        continue;
                    }
                    const GlyphInfo g = m_font->glyph(ch);
                    const float x0 = cx + g.xoff * scale;
                    const float y0 = cy + g.yoff * scale;
                    const float x1 = x0 + g.width * scale;
                    const float y1 = y0 + g.height * scale;
                    if (ch != ' ')
                        pushQuadUV(x0, y0, x1, y1, g.u0, g.v0, g.u1, g.v1, cmd.color);
                    cx += g.advance * scale;
                }
                break;
            }
            }
        }
    }

    std::shared_ptr<FontAtlas> m_font;
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
