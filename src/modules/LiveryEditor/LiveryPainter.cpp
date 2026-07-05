#include "LiveryPainter.h"
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

LiveryPainter::LiveryPainter(QObject* parent)
    : QObject(parent)
{
}

LiveryPainter::~LiveryPainter()
{
}

bool LiveryPainter::startPainting(int textureWidth, int textureHeight)
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

void LiveryPainter::stopPainting()
{
    if (m_isPainting) {
        m_isPainting = false;
        emit paintingStopped();
    }
}

void LiveryPainter::setTexture(const QImage& texture)
{
    m_texture = texture.copy();
    m_originalTexture = m_texture;
    m_width = texture.width();
    m_height = texture.height();
}

bool LiveryPainter::saveTexture(const QString& path)
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

void LiveryPainter::reloadTexture()
{
    if (!m_originalTexture.isNull()) {
        m_texture = m_originalTexture.copy();
        emit textureUpdated(m_texture);
    }
}

void LiveryPainter::clearTexture()
{
    if (!m_texture.isNull()) {
        m_texture.fill(Qt::white);
        emit textureUpdated(m_texture);
    }
}

void LiveryPainter::setMask(const QImage& mask)
{
    m_mask = mask.copy();
}

void LiveryPainter::paintAt(const QPoint& screenPos, const LiveryPaintBrush& brush)
{
    if (m_texture.isNull()) return;
    
    if (screenPos.x() < 0 || screenPos.x() >= m_width ||
        screenPos.y() < 0 || screenPos.y() >= m_height) {
        return;
    }
    
    applyBrushAtPixel(screenPos, brush);
    emit textureUpdated(m_texture);
}

void LiveryPainter::paintLine(const QPoint& from, const QPoint& to, const LiveryPaintBrush& brush)
{
    QVector<QPoint> line = bresenhamLine(from, to);
    for (const QPoint& p : line) {
        applyBrushAtPixel(p, brush);
    }
    emit textureUpdated(m_texture);
}

void LiveryPainter::fillArea(const QPoint& screenPos, const QColor& color)
{
    if (m_texture.isNull()) return;
    floodFill(screenPos, color, 0);
    emit textureUpdated(m_texture);
}

QVector<QPoint> LiveryPainter::getNeighbors(const QPoint& p)
{
    QVector<QPoint> neighbors;
    neighbors.append(QPoint(p.x() - 1, p.y()));
    neighbors.append(QPoint(p.x() + 1, p.y()));
    neighbors.append(QPoint(p.x(), p.y() - 1));
    neighbors.append(QPoint(p.x(), p.y() + 1));
    return neighbors;
}

void LiveryPainter::applyBrushAtPixel(const QPoint& pixel, const LiveryPaintBrush& brush)
{
    int radius = static_cast<int>(brush.radius);
    int innerRadius = static_cast<int>(brush.radius * brush.hardness);
    
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int px = pixel.x() + dx;
            int py = pixel.y() + dy;
            
            if (px < 0 || px >= m_width || py < 0 || py >= m_height) {
                continue;
            }
            
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius) continue;
            
            float falloff = 1.0f;
            if (dist > innerRadius && innerRadius > 0) {
                falloff = 1.0f - (dist - innerRadius) / (radius - innerRadius);
            }
            
            float alpha = brush.opacity * falloff;
            
            QRgb base = m_texture.pixel(px, py);
            QRgb brushColor = brush.color.rgb();
            
            QRgb blended = blendPixel(base, brushColor, alpha);
            m_texture.setPixel(px, py, blended);
        }
    }
}

QRgb LiveryPainter::blendPixel(QRgb base, QRgb brush, float alpha)
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

void LiveryPainter::floodFill(const QPoint& start, const QColor& fillColor, float tolerance)
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

QJsonObject LiveryPreset::toJson() const
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

LiveryPreset LiveryPreset::fromJson(const QJsonObject& json)
{
    LiveryPreset preset;
    preset.name = json["name"].toString();
    
    QJsonArray colorsArray = json["colors"].toArray();
    for (const QJsonValue& val : colorsArray) {
        preset.colors.append(QColor(val.toString()));
    }
    
    return preset;
}

LiveryPainterWidget::LiveryPainterWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(512, 512);
    setMouseTracking(true);
}

LiveryPainterWidget::~LiveryPainterWidget()
{
}

void LiveryPainterWidget::setTexture(const QImage& texture)
{
    m_painter.setTexture(texture);
    update();
}

void LiveryPainterWidget::setBrush(const LiveryPaintBrush& brush)
{
    m_currentBrush = brush;
    updateBrushPreview();
}

void LiveryPainterWidget::setMask(const QImage& mask)
{
    m_painter.setMask(mask);
}

void LiveryPainterWidget::loadPreset(const LiveryPreset& preset)
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

LiveryPreset LiveryPainterWidget::savePreset(const QString& name) const
{
    LiveryPreset preset;
    preset.name = name;
    return preset;
}

void LiveryPainterWidget::onColorSelected(const QColor& color)
{
    m_currentBrush.color = color;
    emit brushChanged(m_currentBrush);
}

void LiveryPainterWidget::onBrushSizeChanged(int size)
{
    m_currentBrush.radius = size;
    emit brushChanged(m_currentBrush);
}

void LiveryPainterWidget::onBrushHardnessChanged(double hardness)
{
    m_currentBrush.hardness = hardness;
    emit brushChanged(m_currentBrush);
}

void LiveryPainterWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDrawing = true;
        m_lastPos = event->pos();
        m_painter.paintAt(event->pos(), m_currentBrush);
        update();
    }
}

void LiveryPainterWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDrawing && (event->buttons() & Qt::LeftButton)) {
        m_painter.paintLine(m_lastPos, event->pos(), m_currentBrush);
        m_lastPos = event->pos();
        update();
    }
}

void LiveryPainterWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDrawing = false;
        emit textureChanged(m_painter.getCurrentTexture());
    }
}

void LiveryPainterWidget::paintEvent(QPaintEvent* event)
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

void LiveryPainterWidget::updateBrushPreview()
{
    update();
}

} // namespace ks