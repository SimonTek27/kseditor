#include "TrackTerrainEditor.h"
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QTextStream>
#include <cmath>

namespace ks {

TrackTerrainEditor* TrackTerrainEditor::s_instance = nullptr;

TrackTerrainEditor* TrackTerrainEditor::instance() {
    if (!s_instance) {
        s_instance = new TrackTerrainEditor();
    }
    return s_instance;
}

TrackTerrainEditor::TrackTerrainEditor(QObject* parent)
    : QObject(parent), m_width(256), m_height(256)
{
    m_heights.resize(m_width * m_height, 0.0f);
    saveUndoState();
}

QString TrackTerrainEditor::brushType() const {
    switch (m_brush.type) {
        case TerrainBrushType::Raise: return "raise";
        case TerrainBrushType::Lower: return "lower";
        case TerrainBrushType::Smooth: return "smooth";
        case TerrainBrushType::Flatten: return "flatten";
        case TerrainBrushType::Noise: return "noise";
        case TerrainBrushType::Paint: return "paint";
        default: return "raise";
    }
}

void TrackTerrainEditor::setBrushType(const QString& type) {
    if (type == "raise") m_brush.type = TerrainBrushType::Raise;
    else if (type == "lower") m_brush.type = TerrainBrushType::Lower;
    else if (type == "smooth") m_brush.type = TerrainBrushType::Smooth;
    else if (type == "flatten") m_brush.type = TerrainBrushType::Flatten;
    else if (type == "noise") m_brush.type = TerrainBrushType::Noise;
    else if (type == "paint") m_brush.type = TerrainBrushType::Paint;
    emit brushSettingsChanged();
}

bool TrackTerrainEditor::loadTerrain(const QString& heightmapPath) {
    QImage img(heightmapPath);
    if (img.isNull()) return false;

    m_width = img.width();
    m_height = img.height();
    m_heights.resize(m_width * m_height);

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            QRgb pixel = img.pixel(x, y);
            float height = (qRed(pixel) + qGreen(pixel) + qBlue(pixel)) / (3.0f * 255.0f);
            m_heights[y * m_width + x] = height;
        }
    }

    m_currentTerrain = heightmapPath;
    saveUndoState();
    emit currentTerrainChanged();
    emit terrainSizeChanged();
    return true;
}

bool TrackTerrainEditor::saveTerrain(const QString& heightmapPath) {
    QImage img(m_width, m_height, QImage::Format_Grayscale8);

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int value = qBound(0, static_cast<int>(m_heights[y * m_width + x] * 255.0f), 255);
            img.setPixel(x, y, qRgb(value, value, value));
        }
    }

    return img.save(heightmapPath);
}

bool TrackTerrainEditor::createNewTerrain(int width, int height) {
    m_width = width;
    m_height = height;
    m_heights.resize(m_width * m_height, 0.0f);
    m_currentTerrain.clear();
    saveUndoState();
    emit terrainSizeChanged();
    emit currentTerrainChanged();
    return true;
}

void TrackTerrainEditor::applyBrush(float x, float y, float delta) {
    int cx = qBound(0, static_cast<int>(x), m_width - 1);
    int cy = qBound(0, static_cast<int>(y), m_height - 1);
    int radius = static_cast<int>(m_brush.radius);

    saveUndoState();

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int px = cx + dx;
            int py = cy + dy;

            if (px < 0 || px >= m_width || py < 0 || py >= m_height) continue;

            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > m_brush.radius) continue;

            float factor = 1.0f - (dist / m_brush.radius);
            if (m_brush.hardness < 1.0f) {
                factor = std::pow(factor, 1.0f / (1.0f - m_brush.hardness + 0.01f));
            }

            int idx = py * m_width + px;
            float appliedDelta = delta * m_brush.strength * factor;

            switch (m_brush.type) {
                case TerrainBrushType::Raise:
                    m_heights[idx] += appliedDelta;
                    break;
                case TerrainBrushType::Lower:
                    m_heights[idx] -= appliedDelta;
                    break;
                case TerrainBrushType::Smooth:
                    break;
                case TerrainBrushType::Flatten:
                    break;
                case TerrainBrushType::Noise:
                    m_heights[idx] += (static_cast<float>(rand()) / RAND_MAX - 0.5f) * appliedDelta;
                    break;
                case TerrainBrushType::Paint:
                    break;
            }

            m_heights[idx] = qBound(0.0f, m_heights[idx], 1.0f);
        }
    }

    emit terrainModified();
    emit heightChanged(x, y, m_heights[cy * m_width + cx]);
}

void TrackTerrainEditor::applyBrushStroke(const QVector<QPointF>& points, float totalDelta) {
    if (points.isEmpty()) return;

    saveUndoState();

    for (const QPointF& pt : points) {
        applyBrush(static_cast<float>(pt.x()), static_cast<float>(pt.y()), totalDelta / points.size());
    }
}

void TrackTerrainEditor::smoothTerrain(float x, float y, float radius) {
    int cx = qBound(0, static_cast<int>(x), m_width - 1);
    int cy = qBound(0, static_cast<int>(y), m_height - 1);
    int r = static_cast<int>(radius);

    saveUndoState();

    QVector<float> oldHeights = m_heights;

    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            int px = cx + dx;
            int py = cy + dy;

            if (px < 1 || px >= m_width - 1 || py < 1 || py >= m_height - 1) continue;

            int idx = py * m_width + px;
            float avg = (oldHeights[idx - 1] + oldHeights[idx + 1] +
                        oldHeights[idx - m_width] + oldHeights[idx + m_width] +
                        oldHeights[idx]) / 5.0f;

            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist <= radius) {
                float factor = 1.0f - (dist / radius);
                m_heights[idx] = oldHeights[idx] + (avg - oldHeights[idx]) * factor * m_brush.strength;
            }
        }
    }

    emit terrainModified();
}

void TrackTerrainEditor::flattenTerrain(float x, float y, float radius, float targetHeight) {
    int cx = qBound(0, static_cast<int>(x), m_width - 1);
    int cy = qBound(0, static_cast<int>(y), m_height - 1);
    int r = static_cast<int>(radius);

    saveUndoState();

    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            int px = cx + dx;
            int py = cy + dy;

            if (px < 0 || px >= m_width || py < 0 || py >= m_height) continue;

            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist <= radius) {
                int idx = py * m_width + px;
                float factor = 1.0f - (dist / radius);
                m_heights[idx] += (targetHeight - m_heights[idx]) * factor * m_brush.strength;
            }
        }
    }

    emit terrainModified();
}

void TrackTerrainEditor::addNoise(float x, float y, float radius, float intensity) {
    int cx = qBound(0, static_cast<int>(x), m_width - 1);
    int cy = qBound(0, static_cast<int>(y), m_height - 1);
    int r = static_cast<int>(radius);

    saveUndoState();

    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            int px = cx + dx;
            int py = cy + dy;

            if (px < 0 || px >= m_width || py < 0 || py >= m_height) continue;

            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist <= radius) {
                int idx = py * m_width + px;
                float factor = 1.0f - (dist / radius);
                float noise = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * intensity * factor;
                m_heights[idx] = qBound(0.0f, m_heights[idx] + noise, 1.0f);
            }
        }
    }

    emit terrainModified();
}

float TrackTerrainEditor::getHeight(float x, float y) const {
    int idx = heightToIndex(x, y);
    if (idx < 0 || idx >= m_heights.size()) return 0.0f;
    return m_heights[idx];
}

void TrackTerrainEditor::setHeight(float x, float y, float height) {
    int idx = heightToIndex(x, y);
    if (idx >= 0 && idx < m_heights.size()) {
        saveUndoState();
        m_heights[idx] = qBound(0.0f, height, 1.0f);
        emit terrainModified();
        emit heightChanged(x, y, m_heights[idx]);
    }
}

QVector3D TrackTerrainEditor::getNormal(float x, float y) const {
    int idx = heightToIndex(x, y);
    if (idx < 0 || idx >= m_heights.size()) return QVector3D(0, 1, 0);

    int px = idx % m_width;
    int py = idx / m_width;

    float hL = (px > 0) ? m_heights[py * m_width + (px - 1)] : m_heights[idx];
    float hR = (px < m_width - 1) ? m_heights[py * m_width + (px + 1)] : m_heights[idx];
    float hD = (py > 0) ? m_heights[(py - 1) * m_width + px] : m_heights[idx];
    float hU = (py < m_height - 1) ? m_heights[(py + 1) * m_width + px] : m_heights[idx];

    QVector3D normal(hL - hR, 2.0f, hD - hU);
    return normal.normalized();
}

QImage TrackTerrainEditor::getHeightmapImage() const {
    QImage img(m_width, m_height, QImage::Format_Grayscale8);

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int value = qBound(0, static_cast<int>(m_heights[y * m_width + x] * 255.0f), 255);
            img.setPixel(x, y, qRgb(value, value, value));
        }
    }

    return img;
}

QPixmap TrackTerrainEditor::getBrushPreview() const {
    return generateBrushPreview();
}

QVariantMap TrackTerrainEditor::getTerrainStats() const {
    QVariantMap stats;
    stats["width"] = m_width;
    stats["height"] = m_height;
    stats["vertexCount"] = m_width * m_height;
    stats["triangleCount"] = (m_width - 1) * (m_height - 1) * 2;

    float minH = 1.0f, maxH = 0.0f, avgH = 0.0f;
    for (float h : m_heights) {
        if (h < minH) minH = h;
        if (h > maxH) maxH = h;
        avgH += h;
    }
    avgH /= m_heights.size();

    stats["minHeight"] = minH;
    stats["maxHeight"] = maxH;
    stats["avgHeight"] = avgH;
    stats["canUndo"] = canUndo();
    stats["canRedo"] = canRedo();

    return stats;
}

void TrackTerrainEditor::undo() {
    if (canUndo()) {
        m_undoIndex--;
        m_heights = m_undoStack[m_undoIndex];
        emit terrainModified();
        emit undoAvailable(canUndo());
        emit redoAvailable(canRedo());
    }
}

void TrackTerrainEditor::redo() {
    if (canRedo()) {
        m_undoIndex++;
        m_heights = m_undoStack[m_undoIndex];
        emit terrainModified();
        emit undoAvailable(canUndo());
        emit redoAvailable(canRedo());
    }
}

void TrackTerrainEditor::exportToOBJ(const QString& path, float scale) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return;

    QTextStream out(&file);
    out << "# Terrain exported from ksEditor\n";
    out << QString("# Vertices: %1 x %2\n").arg(m_width).arg(m_height);

    float stepX = scale / (m_width - 1);
    float stepZ = scale / (m_height - 1);

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            float h = m_heights[y * m_width + x] * scale;
            out << QString("v %1 %2 %3\n").arg(x * stepX).arg(h).arg(y * stepZ);
        }
    }

    for (int y = 0; y < m_height - 1; ++y) {
        for (int x = 0; x < m_width - 1; ++x) {
            int i0 = y * m_width + x;
            int i1 = i0 + 1;
            int i2 = i0 + m_width;
            int i3 = i2 + 1;
            out << QString("f %1 %2 %3\n").arg(i0 + 1).arg(i2 + 1).arg(i1 + 1);
            out << QString("f %1 %2 %3\n").arg(i1 + 1).arg(i2 + 1).arg(i3 + 1);
        }
    }

    file.close();
}

void TrackTerrainEditor::exportToPNG(const QString& path) {
    saveTerrain(path);
}

void TrackTerrainEditor::importFromPNG(const QString& path) {
    loadTerrain(path);
}

void TrackTerrainEditor::saveUndoState() {
    pushUndoState(m_heights);
}

void TrackTerrainEditor::pushUndoState(const QVector<float>& heights) {
    while (m_undoStack.size() > m_undoIndex) {
        m_undoStack.pop_back();
    }

    m_undoStack.push_back(heights);

    while (m_undoStack.size() > MAX_UNDO_STATES) {
        m_undoStack.pop_front();
        m_undoIndex--;
    }

    emit undoAvailable(canUndo());
    emit redoAvailable(canRedo());
}

int TrackTerrainEditor::heightToIndex(float x, float y) const {
    int ix = qBound(0, static_cast<int>(x), m_width - 1);
    int iy = qBound(0, static_cast<int>(y), m_height - 1);
    return iy * m_width + ix;
}

void TrackTerrainEditor::indexToHeight(int index, float& x, float& y) const {
    x = index % m_width;
    y = index / m_width;
}

QPixmap TrackTerrainEditor::generateBrushPreview() const {
    int size = 128;
    QPixmap preview(size, size);
    preview.fill(QColor("#1a1a1a"));

    QPainter painter(&preview);
    painter.setRenderHint(QPainter::Antialiasing);

    int center = size / 2;
    int radius = static_cast<int>((m_brush.radius / 50.0f) * (size / 2));
    radius = qBound(5, radius, size / 2 - 5);

    QColor brushColor;
    switch (m_brush.type) {
        case TerrainBrushType::Raise: brushColor = QColor(0, 255, 0, 128); break;
        case TerrainBrushType::Lower: brushColor = QColor(255, 0, 0, 128); break;
        case TerrainBrushType::Smooth: brushColor = QColor(0, 128, 255, 128); break;
        case TerrainBrushType::Flatten: brushColor = QColor(255, 255, 0, 128); break;
        case TerrainBrushType::Noise: brushColor = QColor(255, 0, 255, 128); break;
        case TerrainBrushType::Paint: brushColor = QColor(0, 255, 255, 128); break;
    }

    painter.setBrush(brushColor);
    painter.setPen(QPen(Qt::white, 2));

    if (m_brush.shape == TerrainBrushShape::Circle) {
        painter.drawEllipse(center - radius, center - radius, radius * 2, radius * 2);
    } else if (m_brush.shape == TerrainBrushShape::Square) {
        painter.drawRect(center - radius, center - radius, radius * 2, radius * 2);
    }

    painter.setPen(Qt::white);
    painter.drawText(preview.rect(), Qt::AlignBottom | Qt::AlignHCenter,
                     QString("%1 (r:%2 s:%3)").arg(brushType()).arg(m_brush.radius).arg(m_brush.strength));

    return preview;
}

} // namespace ks
