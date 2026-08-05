#include "PaintPainter.h"
#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFileInfo>
#include <cmath>

static QVector<QPoint> bresenhamLine(const QPoint& from, const QPoint& to) {
    QVector<QPoint> points;
    int x0 = from.x(), y0 = from.y();
    int x1 = to.x(), y1 = to.y();
    int dx = qAbs(x1 - x0), dy = qAbs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    while (true) {
        points.append(QPoint(x0, y0));
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
    return points;
}

namespace ks {

PaintPainter::PaintPainter(QObject* parent)
    : QObject(parent)
{
}

PaintPainter::~PaintPainter()
{
}

bool PaintPainter::startPainting(int textureWidth, int textureHeight)
{
    if (textureWidth <= 0 || textureHeight <= 0) {
        return false;
    }
    
    m_width = textureWidth;
    m_height = textureHeight;
    m_texture = QImage(textureWidth, textureHeight, QImage::Format_ARGB32);
    m_texture.fill(Qt::white);
    m_originalTexture = m_texture;
    m_isPainting = true;
    
    emit paintingStarted();
    return true;
}

void PaintPainter::stopPainting()
{
    if (m_isPainting) {
        m_isPainting = false;
        emit paintingStopped();
    }
}

void PaintPainter::setTexture(const QImage& texture)
{
    m_texture = texture.copy();
    m_originalTexture = m_texture;
    m_width = texture.width();
    m_height = texture.height();
}

bool PaintPainter::saveTexture(const QString& path)
{
    if (m_texture.isNull()) {
        return false;
    }
    
    QString format = QFileInfo(path).suffix().toUpper();
    if (format.isEmpty()) {
        format = "PNG";
    }
    
    return m_texture.save(path, format.toLocal8Bit().constData());
}

void PaintPainter::reloadTexture()
{
    if (!m_originalTexture.isNull()) {
        m_texture = m_originalTexture.copy();
        emit textureUpdated(m_texture);
    }
}

void PaintPainter::clearTexture()
{
    if (!m_texture.isNull()) {
        m_texture.fill(Qt::white);
        emit textureUpdated(m_texture);
    }
}

void PaintPainter::setMask(const QImage& mask)
{
    m_mask = mask.copy();
}

void PaintPainter::paintAt(const QPoint& screenPos, const PaintBrush& brush)
{
    if (m_texture.isNull()) return;
    
    if (screenPos.x() < 0 || screenPos.x() >= m_width ||
        screenPos.y() < 0 || screenPos.y() >= m_height) {
        return;
    }
    
    switch (brush.type) {
    case PaintBrush::Brush:
    case PaintBrush::SquareBrush:
        applyBrushAtPixel(screenPos, brush);
        break;
    case PaintBrush::Airbrush:
        applyAirbrushAtPixel(screenPos, brush);
        break;
    case PaintBrush::Eraser:
        applyBrushAtPixel(screenPos, brush);
        break;
    case PaintBrush::Smudge:
        break;
    case PaintBrush::Blur:
        applyBlurAtPixel(screenPos, brush);
        break;
    case PaintBrush::Sharpen:
        applySharpenAtPixel(screenPos, brush);
        break;
    case PaintBrush::Clone:
    case PaintBrush::Healing:
        applyCloneAtPixel(screenPos, brush);
        break;
    case PaintBrush::Dodge:
        applyDodgeAtPixel(screenPos, brush);
        break;
    case PaintBrush::Burn:
        applyBurnAtPixel(screenPos, brush);
        break;
    case PaintBrush::Fill:
        floodFill(screenPos, brush.color, 0);
        break;
    case PaintBrush::Gradient:
        break;
    case PaintBrush::Stamp:
        if (!brush.stampTexture.isNull()) {
            applyStamp(screenPos, brush.stampTexture, brush.opacity);
        }
        break;
    }
    
    emit textureUpdated(m_texture);
}

void PaintPainter::paintLine(const QPoint& from, const QPoint& to, const PaintBrush& brush)
{
    QVector<QPoint> line = bresenhamLine(from, to);
    for (const QPoint& p : line) {
        switch (brush.type) {
        case PaintBrush::Brush:
        case PaintBrush::SquareBrush:
        case PaintBrush::Eraser:
            applyBrushAtPixel(p, brush);
            break;
        case PaintBrush::Airbrush:
            applyAirbrushAtPixel(p, brush);
            break;
        case PaintBrush::Smudge:
            applySmudgeAtPixel(from, to, brush);
            break;
        case PaintBrush::Blur:
            applyBlurAtPixel(p, brush);
            break;
        case PaintBrush::Sharpen:
            applySharpenAtPixel(p, brush);
            break;
        case PaintBrush::Clone:
        case PaintBrush::Healing:
            applyCloneAtPixel(p, brush);
            break;
        case PaintBrush::Dodge:
            applyDodgeAtPixel(p, brush);
            break;
        case PaintBrush::Burn:
            applyBurnAtPixel(p, brush);
            break;
        case PaintBrush::Fill:
        case PaintBrush::Gradient:
        case PaintBrush::Stamp:
            break;
        }
    }
    emit textureUpdated(m_texture);
}

void PaintPainter::fillArea(const QPoint& screenPos, const QColor& color)
{
    if (m_texture.isNull()) return;
    floodFill(screenPos, color, 0);
    emit textureUpdated(m_texture);
}

QVector<QPoint> PaintPainter::getNeighbors(const QPoint& p)
{
    QVector<QPoint> neighbors;
    neighbors.append(QPoint(p.x() - 1, p.y()));
    neighbors.append(QPoint(p.x() + 1, p.y()));
    neighbors.append(QPoint(p.x(), p.y() - 1));
    neighbors.append(QPoint(p.x(), p.y() + 1));
    return neighbors;
}

void PaintPainter::applyBrushAtPixel(const QPoint& pixel, const PaintBrush& brush)
{
    int radius = static_cast<int>(brush.radius);
    int innerRadius = static_cast<int>(brush.radius * brush.hardness);
    bool isEraser = (brush.type == PaintBrush::Eraser);
    bool isSquare = (brush.type == PaintBrush::SquareBrush);
    
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int px = pixel.x() + dx;
            int py = pixel.y() + dy;
            
            if (px < 0 || px >= m_width || py < 0 || py >= m_height) {
                continue;
            }
            
            float dist;
            if (isSquare) {
                dist = static_cast<float>(qMax(qAbs(dx), qAbs(dy)));
            } else {
                dist = std::sqrt(dx * dx + dy * dy);
            }
            if (dist > radius) continue;
            
            float falloff = 1.0f;
            if (dist > innerRadius && innerRadius > 0) {
                falloff = 1.0f - (dist - innerRadius) / (radius - innerRadius);
            }
            
            float alpha = brush.opacity * falloff;
            
            QRgb base = m_texture.pixel(px, py);

            if (isEraser) {
                int ba = qAlpha(base);
                int na = static_cast<int>(ba * (1.0f - alpha));
                m_texture.setPixel(px, py, qRgba(qRed(base), qGreen(base), qBlue(base), na));
            } else {
                QRgb brushColor = brush.color.rgb();
                QRgb blended = blendPixel(base, brushColor, alpha);
                m_texture.setPixel(px, py, blended);
            }
        }
    }
}

QRgb PaintPainter::blendPixel(QRgb base, QRgb brush, float alpha)
{
    int br = qRed(base);
    int bg = qGreen(base);
    int bb = qBlue(base);
    int ba = qAlpha(base);
    
    int cr = qRed(brush);
    int cg = qGreen(brush);
    int cb = qBlue(brush);
    
    int nr = static_cast<int>(br + (cr - br) * alpha);
    int ng = static_cast<int>(bg + (cg - bg) * alpha);
    int nb = static_cast<int>(bb + (cb - bb) * alpha);
    
    return qRgba(nr, ng, nb, ba);
}

QRgb PaintPainter::colorDodge(QRgb base, float strength)
{
    int br = qRed(base);
    int bg = qGreen(base);
    int bb = qBlue(base);
    int r = (br == 255) ? 255 : qMin(255, static_cast<int>(br / (1.0f - strength * (br / 255.0f))));
    int g = (bg == 255) ? 255 : qMin(255, static_cast<int>(bg / (1.0f - strength * (bg / 255.0f))));
    int b = (bb == 255) ? 255 : qMin(255, static_cast<int>(bb / (1.0f - strength * (bb / 255.0f))));
    return qRgba(r, g, b, qAlpha(base));
}

QRgb PaintPainter::colorBurn(QRgb base, float strength)
{
    int br = qRed(base);
    int bg = qGreen(base);
    int bb = qBlue(base);
    int r = (br == 0) ? 0 : qMax(0, static_cast<int>(255 - (255 - br) / (1.0f + strength * (br / 255.0f) - strength)));
    int g = (bg == 0) ? 0 : qMax(0, static_cast<int>(255 - (255 - bg) / (1.0f + strength * (bg / 255.0f) - strength)));
    int b = (bb == 0) ? 0 : qMax(0, static_cast<int>(255 - (255 - bb) / (1.0f + strength * (bb / 255.0f) - strength)));
    return qRgba(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255), qAlpha(base));
}

void PaintPainter::applyAirbrushAtPixel(const QPoint& pixel, const PaintBrush& brush)
{
    int radius = static_cast<int>(brush.radius);
    float flowRate = brush.flow * brush.opacity;

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int px = pixel.x() + dx;
            int py = pixel.y() + dy;
            if (px < 0 || px >= m_width || py < 0 || py >= m_height) continue;

            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius) continue;

            float falloff = 1.0f - (dist / radius);
            float alpha = flowRate * falloff * falloff;

            QRgb base = m_texture.pixel(px, py);
            QRgb brushColor = brush.color.rgb();
            m_texture.setPixel(px, py, blendPixel(base, brushColor, alpha));
        }
    }
}

void PaintPainter::applySmudgeAtPixel(const QPoint& from, const QPoint& to, const PaintBrush& brush)
{
    int radius = static_cast<int>(brush.radius * 0.5f);
    float strength = brush.strength;

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int px = to.x() + dx;
            int py = to.y() + dy;
            if (px < 0 || px >= m_width || py < 0 || py >= m_height) continue;

            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius) continue;

            float falloff = 1.0f - (dist / radius);
            float alpha = strength * falloff * brush.opacity;

            int sx = from.x() + dx;
            int sy = from.y() + dy;
            if (sx < 0 || sx >= m_width || sy < 0 || sy >= m_height) continue;

            QRgb srcColor = m_texture.pixel(sx, sy);
            QRgb dstColor = m_texture.pixel(px, py);
            m_texture.setPixel(px, py, blendPixel(dstColor, srcColor, alpha));
        }
    }
}

void PaintPainter::applyBlurAtPixel(const QPoint& pixel, const PaintBrush& brush)
{
    int radius = static_cast<int>(brush.radius * 0.5f);
    if (radius < 1) radius = 1;

    float sumR = 0, sumG = 0, sumB = 0;
    int count = 0;

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int px = pixel.x() + dx;
            int py = pixel.y() + dy;
            if (px < 0 || px >= m_width || py < 0 || py >= m_height) continue;

            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius) continue;

            QRgb p = m_texture.pixel(px, py);
            sumR += qRed(p);
            sumG += qGreen(p);
            sumB += qBlue(p);
            count++;
        }
    }

    if (count == 0) return;

    int avgR = static_cast<int>(sumR / count);
    int avgG = static_cast<int>(sumG / count);
    int avgB = static_cast<int>(sumB / count);

    QRgb center = m_texture.pixel(pixel.x(), pixel.y());
    float alpha = brush.strength * brush.opacity;
    int r = static_cast<int>(qRed(center) + (avgR - qRed(center)) * alpha);
    int g = static_cast<int>(qGreen(center) + (avgG - qGreen(center)) * alpha);
    int b = static_cast<int>(qBlue(center) + (avgB - qBlue(center)) * alpha);
    m_texture.setPixel(pixel.x(), pixel.y(), qRgba(r, g, b, qAlpha(center)));
}

void PaintPainter::applySharpenAtPixel(const QPoint& pixel, const PaintBrush& brush)
{
    int x = pixel.x(), y = pixel.y();
    if (x < 1 || x >= m_width - 1 || y < 1 || y >= m_height - 1) return;

    float kernel[3][3] = {
        { 0, -1,  0},
        {-1,  5, -1},
        { 0, -1,  0}
    };

    float sumR = 0, sumG = 0, sumB = 0;
    for (int ky = -1; ky <= 1; ++ky) {
        for (int kx = -1; kx <= 1; ++kx) {
            QRgb p = m_texture.pixel(x + kx, y + ky);
            sumR += qRed(p) * kernel[ky + 1][kx + 1];
            sumG += qGreen(p) * kernel[ky + 1][kx + 1];
            sumB += qBlue(p) * kernel[ky + 1][kx + 1];
        }
    }

    QRgb orig = m_texture.pixel(x, y);
    float amount = brush.strength * brush.opacity;
    int r = qBound(0, static_cast<int>(qRed(orig) + (sumR - qRed(orig)) * amount), 255);
    int g = qBound(0, static_cast<int>(qGreen(orig) + (sumG - qGreen(orig)) * amount), 255);
    int b = qBound(0, static_cast<int>(qBlue(orig) + (sumB - qBlue(orig)) * amount), 255);
    m_texture.setPixel(x, y, qRgba(r, g, b, qAlpha(orig)));
}

void PaintPainter::applyDodgeAtPixel(const QPoint& pixel, const PaintBrush& brush)
{
    int radius = static_cast<int>(brush.radius * 0.5f);
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int px = pixel.x() + dx;
            int py = pixel.y() + dy;
            if (px < 0 || px >= m_width || py < 0 || py >= m_height) continue;

            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius) continue;

            float falloff = 1.0f - (dist / radius);
            float alpha = brush.strength * falloff * brush.opacity;

            QRgb base = m_texture.pixel(px, py);
            m_texture.setPixel(px, py, colorDodge(base, alpha));
        }
    }
}

void PaintPainter::applyBurnAtPixel(const QPoint& pixel, const PaintBrush& brush)
{
    int radius = static_cast<int>(brush.radius * 0.5f);
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int px = pixel.x() + dx;
            int py = pixel.y() + dy;
            if (px < 0 || px >= m_width || py < 0 || py >= m_height) continue;

            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius) continue;

            float falloff = 1.0f - (dist / radius);
            float alpha = brush.strength * falloff * brush.opacity;

            QRgb base = m_texture.pixel(px, py);
            m_texture.setPixel(px, py, colorBurn(base, alpha));
        }
    }
}

void PaintPainter::applyCloneAtPixel(const QPoint& pixel, const PaintBrush& brush)
{
    if (m_cloneImage.isNull()) return;

    int radius = static_cast<int>(brush.radius * 0.5f);
    QPoint src = brush.cloneSource;

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int px = pixel.x() + dx;
            int py = pixel.y() + dy;
            if (px < 0 || px >= m_width || py < 0 || py >= m_height) continue;

            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius) continue;

            float falloff = 1.0f - (dist / radius);
            float alpha = brush.opacity * falloff;

            int sx = src.x() + dx;
            int sy = src.y() + dy;
            if (sx < 0 || sx >= m_cloneImage.width() || sy < 0 || sy >= m_cloneImage.height()) continue;

            QRgb srcColor = m_cloneImage.pixel(sx, sy);
            QRgb dstColor = m_texture.pixel(px, py);
            m_texture.setPixel(px, py, blendPixel(dstColor, srcColor, alpha));
        }
    }
}

void PaintPainter::applyHealingAtPixel(const QPoint& pixel, const QPoint& source, const PaintBrush& brush)
{
    if (m_cloneImage.isNull()) return;

    int radius = static_cast<int>(brush.radius * 0.5f);

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int px = pixel.x() + dx;
            int py = pixel.y() + dy;
            if (px < 0 || px >= m_width || py < 0 || py >= m_height) continue;

            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius) continue;

            float falloff = 1.0f - (dist / radius);
            float alpha = brush.opacity * falloff;

            int sx = source.x() + dx;
            int sy = source.y() + dy;
            if (sx < 0 || sx >= m_cloneImage.width() || sy < 0 || sy >= m_cloneImage.height()) continue;

            QRgb srcColor = m_cloneImage.pixel(sx, sy);
            QRgb dstColor = m_texture.pixel(px, py);

            int dr = qRed(dstColor) + (qRed(srcColor) - qRed(dstColor)) * alpha;
            int dg = qGreen(dstColor) + (qGreen(srcColor) - qGreen(dstColor)) * alpha;
            int db = qBlue(dstColor) + (qBlue(srcColor) - qBlue(dstColor)) * alpha;
            m_texture.setPixel(px, py, qRgba(qBound(0, dr, 255), qBound(0, dg, 255), qBound(0, db, 255), qAlpha(dstColor)));
        }
    }
}

void PaintPainter::applyStamp(const QPoint& pos, const QImage& stamp, float opacity)
{
    if (m_texture.isNull() || stamp.isNull()) return;

    QPainter painter(&m_texture);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setOpacity(opacity);
    painter.drawImage(pos - QPoint(stamp.width() / 2, stamp.height() / 2), stamp);
    painter.end();
}

void PaintPainter::setCloneImage(const QImage& image)
{
    m_cloneImage = image;
}

void PaintPainter::gradientFill(const QPoint& from, const QPoint& to,
                                  const QColor& startColor, const QColor& endColor, bool radial)
{
    if (m_texture.isNull()) return;

    QPainter painter(&m_texture);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);

    if (radial) {
        float radius = QLineF(from, to).length();
        QRadialGradient grad(from, radius);
        grad.setColorAt(0.0, startColor);
        grad.setColorAt(1.0, endColor);
        painter.setBrush(grad);
        painter.drawRect(m_texture.rect());
    } else {
        QLinearGradient grad(from, to);
        grad.setColorAt(0.0, startColor);
        grad.setColorAt(1.0, endColor);
        painter.setBrush(grad);
        painter.drawRect(m_texture.rect());
    }

    painter.end();
    emit textureUpdated(m_texture);
}

void PaintPainter::applyBlurAt(const QPoint& center, int radius, float strength)
{
    PaintBrush brush;
    brush.type = PaintBrush::Blur;
    brush.radius = radius;
    brush.strength = strength;
    brush.opacity = 1.0f;
    applyBlurAtPixel(center, brush);
}

void PaintPainter::applySharpenAt(const QPoint& center, int radius, float strength)
{
    PaintBrush brush;
    brush.type = PaintBrush::Sharpen;
    brush.radius = radius;
    brush.strength = strength;
    brush.opacity = 1.0f;
    applySharpenAtPixel(center, brush);
}

void PaintPainter::applyDodgeAt(const QPoint& center, int radius, float strength)
{
    PaintBrush brush;
    brush.type = PaintBrush::Dodge;
    brush.radius = radius;
    brush.strength = strength;
    brush.opacity = 1.0f;
    applyDodgeAtPixel(center, brush);
}

void PaintPainter::applyBurnAt(const QPoint& center, int radius, float strength)
{
    PaintBrush brush;
    brush.type = PaintBrush::Burn;
    brush.radius = radius;
    brush.strength = strength;
    brush.opacity = 1.0f;
    applyBurnAtPixel(center, brush);
}

void PaintPainter::smudgeAt(const QPoint& from, const QPoint& to, int radius, float strength)
{
    PaintBrush brush;
    brush.type = PaintBrush::Smudge;
    brush.radius = radius;
    brush.strength = strength;
    brush.opacity = 1.0f;
    applySmudgeAtPixel(from, to, brush);
}

void PaintPainter::applyHealingAt(const QPoint& target, const QPoint& source, int radius)
{
    PaintBrush brush;
    brush.type = PaintBrush::Healing;
    brush.radius = radius;
    brush.strength = 1.0f;
    brush.opacity = 1.0f;
    applyHealingAtPixel(target, source, brush);
}

void PaintPainter::floodFill(const QPoint& start, const QColor& fillColor, float tolerance)
{
    if (m_texture.isNull()) return;
    if (start.x() < 0 || start.x() >= m_width || start.y() < 0 || start.y() >= m_height) return;
    
    QRgb targetColor = m_texture.pixel(start.x(), start.y());
    QRgb fillRgb = fillColor.rgb();
    
    if (targetColor == fillRgb) return;
    
    QVector<QPoint> queue;
    queue.append(start);
    
    QVector<QVector<bool>> visited(m_height, QVector<bool>(m_width, false));
    
    while (!queue.isEmpty()) {
        QPoint p = queue.takeLast();
        
        if (p.x() < 0 || p.x() >= m_width || p.y() < 0 || p.y() >= m_height) continue;
        if (visited[p.y()][p.x()]) continue;
        
        QRgb current = m_texture.pixel(p.x(), p.y());
        
        int dr = abs(qRed(current) - qRed(targetColor));
        int dg = abs(qGreen(current) - qGreen(targetColor));
        int db = abs(qBlue(current) - qBlue(targetColor));
        
        if (dr > tolerance * 255 || dg > tolerance * 255 || db > tolerance * 255) continue;
        
        visited[p.y()][p.x()] = true;
        m_texture.setPixel(p.x(), p.y(), fillRgb);
        
        queue.append(QPoint(p.x() - 1, p.y()));
        queue.append(QPoint(p.x() + 1, p.y()));
        queue.append(QPoint(p.x(), p.y() - 1));
        queue.append(QPoint(p.x(), p.y() + 1));
    }
}

QJsonObject PaintPreset::toJson() const
{
    QJsonObject json;
    json["name"] = name;
    
    QJsonArray colorsArray;
    for (const QColor& color : colors) {
        colorsArray.append(color.name());
    }
    json["colors"] = colorsArray;
    
    return json;
}

PaintPreset PaintPreset::fromJson(const QJsonObject& json)
{
    PaintPreset preset;
    preset.name = json["name"].toString();
    
    QJsonArray colorsArray = json["colors"].toArray();
    for (const QJsonValue& val : colorsArray) {
        preset.colors.append(QColor(val.toString()));
    }
    
    return preset;
}

PaintPainterWidget::PaintPainterWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(512, 512);
    setMouseTracking(true);
}

PaintPainterWidget::~PaintPainterWidget()
{
}

void PaintPainterWidget::setTexture(const QImage& texture)
{
    m_painter.setTexture(texture);
    update();
}

void PaintPainterWidget::setBrush(const PaintBrush& brush)
{
    m_currentBrush = brush;
    updateBrushPreview();
}

void PaintPainterWidget::setMask(const QImage& mask)
{
    m_painter.setMask(mask);
}

void PaintPainterWidget::loadPreset(const PaintPreset& preset)
{
    QImage texture = m_painter.getCurrentTexture();
    if (texture.isNull() || preset.colors.isEmpty()) return;

    int colorsPerRow = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(preset.colors.size()))));
    if (colorsPerRow == 0) return;
    int swatchSize = texture.width() / colorsPerRow;
    if (swatchSize == 0) return;

    QPainter p(&texture);
    for (int i = 0; i < preset.colors.size(); ++i) {
        int row = i / colorsPerRow;
        int col = i % colorsPerRow;
        int x = col * swatchSize;
        int y = row * swatchSize;
        p.fillRect(QRect(x, y, swatchSize, swatchSize), preset.colors[i]);
    }
    p.end();

    m_painter.setTexture(texture);
    update();
}

PaintPreset PaintPainterWidget::savePreset(const QString& name) const
{
    PaintPreset preset;
    preset.name = name;
    return preset;
}

void PaintPainterWidget::onColorSelected(const QColor& color)
{
    m_currentBrush.color = color;
    emit brushChanged(m_currentBrush);
}

void PaintPainterWidget::onBrushSizeChanged(int size)
{
    m_currentBrush.radius = size;
    emit brushChanged(m_currentBrush);
}

void PaintPainterWidget::onBrushHardnessChanged(double hardness)
{
    m_currentBrush.hardness = hardness;
    emit brushChanged(m_currentBrush);
}

void PaintPainterWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDrawing = true;
        m_lastPos = event->pos();
        m_painter.paintAt(event->pos(), m_currentBrush);
        update();
    }
}

void PaintPainterWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDrawing && (event->buttons() & Qt::LeftButton)) {
        m_painter.paintLine(m_lastPos, event->pos(), m_currentBrush);
        m_lastPos = event->pos();
        update();
    }
}

void PaintPainterWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDrawing = false;
        emit textureChanged(m_painter.getCurrentTexture());
    }
}

void PaintPainterWidget::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    
    QImage texture = m_painter.getCurrentTexture();
    if (!texture.isNull()) {
        p.drawImage(rect(), texture);
    } else {
        p.fillRect(rect(), Qt::darkGray);
        p.drawText(rect(), Qt::AlignCenter, "Load a texture to start painting");
    }
    
    if (m_isDrawing) {
        p.setPen(Qt::white);
        p.drawEllipse(m_lastPos, static_cast<int>(m_currentBrush.radius),
                      static_cast<int>(m_currentBrush.radius));
    }
}

void PaintPainterWidget::updateBrushPreview()
{
    update();
}

} // namespace ks