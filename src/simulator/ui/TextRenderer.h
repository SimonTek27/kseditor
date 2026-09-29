#pragma once
/**
 * TextRenderer — high-level bitmap text API for simulator UI.
 * Wraps FontAtlas + BitmapText; draws into UiRenderer / DrawList.
 */
#include "BitmapText.h"
#include "UiRenderer.h"
#include <memory>
#include <string>
#include <cstdio>

namespace ks {
namespace sim {
namespace ui {

class TextRenderer {
public:
    TextRenderer() : m_font(std::make_shared<FontAtlas>()) {}
    explicit TextRenderer(std::shared_ptr<FontAtlas> font)
        : m_font(font ? std::move(font) : std::make_shared<FontAtlas>()) {}

    const FontAtlas& atlas() const { return *m_font; }
    std::shared_ptr<FontAtlas> atlasPtr() const { return m_font; }

    void setDefaultStyle(const TextStyle& s) { m_default = s; }
    const TextStyle& defaultStyle() const { return m_default; }

    // --- Measure ---
    TextMetrics measure(const std::string& text) const {
        return BitmapText(*m_font).measure(text, m_default);
    }
    TextMetrics measure(const std::string& text, const TextStyle& style) const {
        return BitmapText(*m_font).measure(text, style);
    }
    float measureWidth(const std::string& text, float scale = 1.f) const {
        TextStyle s = m_default;
        s.scale = scale;
        return measure(text, s).width;
    }

    // --- Draw into UiRenderer frame list ---
    void draw(UiRenderer& ui, float x, float y, const std::string& text) {
        ui.addTextStyled(x, y, text, m_default);
    }

    void draw(UiRenderer& ui, float x, float y, const std::string& text, const TextStyle& style) {
        ui.addTextStyled(x, y, text, style);
    }

    void draw(DrawList& dl, float x, float y, const std::string& text, const TextStyle& style) {
        BitmapText(*m_font).addStyledToDrawList(dl, x, y, text, style);
    }

    /** Center text horizontally around cx. */
    void drawCentered(UiRenderer& ui, float cx, float y, const std::string& text, const TextStyle& style) {
        TextStyle s = style;
        s.align = TextAlign::Center;
        ui.addTextStyled(cx, y, text, s);
    }

    /** printf-style helper (stack buffer). */
    template <typename... Args>
    void drawf(UiRenderer& ui, float x, float y, const TextStyle& style, const char* fmt, Args... args) {
        char buf[256];
        std::snprintf(buf, sizeof(buf), fmt, args...);
        draw(ui, x, y, buf, style);
    }

    // Preset styles for HUD
    static TextStyle hudLabel() {
        TextStyle s;
        s.color = Color::rgb(200, 210, 220);
        s.scale = 1.f;
        s.shadow = true;
        return s;
    }
    static TextStyle hudValue() {
        TextStyle s;
        s.color = Color::rgb(255, 255, 255);
        s.scale = 1.25f;
        s.shadow = true;
        s.outline = true;
        s.outlinePx = 1.f;
        return s;
    }
    static TextStyle menuTitle() {
        TextStyle s;
        s.color = Color::rgb(220, 235, 255);
        s.scale = 2.f;
        s.shadow = true;
        s.outline = true;
        return s;
    }
    static TextStyle menuItem() {
        TextStyle s;
        s.color = Color::rgb(230, 230, 240);
        s.scale = 1.2f;
        s.shadow = true;
        return s;
    }

private:
    std::shared_ptr<FontAtlas> m_font;
    TextStyle m_default = hudLabel();
};

} // namespace ui
} // namespace sim
} // namespace ks
