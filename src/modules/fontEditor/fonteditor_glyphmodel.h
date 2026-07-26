#pragma once

#include <QObject>
#include <QString>
#include <QFont>
#include <QImage>

// GlyphModel holds the render state for a single character cell of the
// generated bitmap font: which character it represents, the pixel box it
// is rasterised into, the small per-character offset ("padding") used to
// nudge the glyph within that box, and the resulting glyph image itself.
//
// This mirrors the original WPF app's `FontModel` class field-for-field
// (Value / PixelWidth / PixelHeight / HPadding / VPadding), reproducing the
// same "measure once, then draw into a fixed box" pipeline via
// QFontMetrics/QPainter instead of GDI+ + WPF FormattedText.
class GlyphModel : public QObject
{
    Q_OBJECT

public:
    explicit GlyphModel(const QString &value, int charIndex, QObject *parent = nullptr);

    QString value() const { return m_value; }
    int charIndex() const { return m_charIndex; } // 0..94, i.e. (ASCII code - 32)

    int pixelWidth() const { return m_pixelWidth; }
    void setPixelWidth(int w);

    int pixelHeight() const { return m_pixelHeight; }
    void setPixelHeight(int h);

    int hPadding() const { return m_hPadding; }
    void setHPadding(int p);

    int vPadding() const { return m_vPadding; }
    void setVPadding(int p);

    // The rendered glyph: black background, white glyph, exactly
    // pixelWidth x pixelHeight pixels (until the next render() call).
    const QImage &image() const { return m_image; }

    // (Re)measures the glyph against `font` - only the very first time,
    // i.e. while pixelWidth/pixelHeight are still 0, exactly like the
    // original's DrawText(), which only auto-sizes a character once and
    // leaves user- or preset-supplied sizes alone afterwards - and then
    // (re)rasterises it into m_image. Safe to call as often as needed;
    // callers decide when a re-render is warranted (see MainWindow).
    void render(const QFont &font);

private:
    QString m_value;
    int m_charIndex;
    int m_pixelWidth = 0;
    int m_pixelHeight = 0;
    int m_hPadding = 0;
    int m_vPadding = 0;
    QImage m_image;
};
