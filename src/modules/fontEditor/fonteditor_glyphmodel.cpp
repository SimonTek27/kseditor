#include "fonteditor_glyphmodel.h"

#include <QFontMetricsF>
#include <QPainter>
#include <algorithm>

GlyphModel::GlyphModel(const QString &value, int charIndex, QObject *parent)
    : QObject(parent), m_value(value), m_charIndex(charIndex)
{
}

void GlyphModel::setPixelWidth(int w) { m_pixelWidth = w; }
void GlyphModel::setPixelHeight(int h) { m_pixelHeight = h; }
void GlyphModel::setHPadding(int p) { m_hPadding = p; }
void GlyphModel::setVPadding(int p) { m_vPadding = p; }

void GlyphModel::render(const QFont &font)
{
    // 1) Measure, the same way the original used Graphics.MeasureString():
    //    a layout-box size (advance width x full line height), not a tight
    //    ink bounding box. This only ever picks a *default* box size the
    //    first time a glyph is rendered - once PixelWidth/PixelHeight are
    //    non-zero (typically loaded from a preset, or already measured
    //    earlier in this session) they are left untouched, exactly like
    //    the original DrawText().
    if (m_pixelWidth <= 0 || m_pixelHeight <= 0) {
        const QFontMetricsF fm(font);
        const QSizeF measured = fm.size(Qt::TextSingleLine, m_value);
        if (m_pixelWidth <= 0)
            m_pixelWidth = std::max(1, qRound(measured.width()));
        if (m_pixelHeight <= 0)
            m_pixelHeight = std::max(1, qRound(measured.height()));
    }

    // 2) Rasterise: black background, white glyph, drawn into a rectangle
    //    that spans the whole cell but is offset by (HPadding, VPadding)
    //    and centred within that shifted rectangle - equivalent to the
    //    original's centred DrawString() into a
    //    RectangleF(HPadding, VPadding, PixelWidth, PixelHeight).
    QImage img(m_pixelWidth, m_pixelHeight, QImage::Format_ARGB32_Premultiplied);
    // Pin the DPI to 96 so point sizes map to pixels identically on every
    // machine, regardless of the host display's own scale factor.
    img.setDotsPerMeterX(3780); // 96 dpi
    img.setDotsPerMeterY(3780);
    img.fill(Qt::black);

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.setFont(font);
    p.setPen(Qt::white);
    const QRectF rect(m_hPadding, m_vPadding, m_pixelWidth, m_pixelHeight);
    p.drawText(rect, Qt::AlignCenter, m_value);
    p.end();

    m_image = img;
}
