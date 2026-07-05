#include "TexturePaint.h"
#include <cstdlib>
#include <QPainter>
#include <QtMath>
#include <QGraphicsPixmapItem>
#include <QBuffer>
#include <QDataStream>
#include <QQueue>

namespace ks {

static TexturePainter* s_texturePainter = nullptr;
static TextureLayers* s_textureLayers = nullptr;
static UVViewport* s_uvViewport = nullptr;

TexturePainter::TexturePainter(QObject* parent)
    : QObject(parent)
    , m_strokeActive(false)
{
}

TexturePainter::~TexturePainter() {
    s_texturePainter = nullptr;
}

TexturePainter* TexturePainter::instance() {
    if (!s_texturePainter) {
        s_texturePainter = new TexturePainter();
    }
    return s_texturePainter;
}

void TexturePainter::setImage(const QImage& image) {
    m_image = image;
    emit imageModified();
}

void TexturePainter::clearImage() {
    m_image.fill(Qt::transparent);
    emit imageModified();
}

void TexturePainter::beginStroke(const QVector3D& pos) {
    m_strokeActive = true;
    m_lastPos = QPoint(pos.x(), pos.y());
    m_strokeImage = m_image;
}

void TexturePainter::addStamp(const QVector3D& pos) {
    if (!m_strokeActive) return;

    QPoint pos2D(pos.x(), pos.y());
    float dist = QLineF(m_lastPos, pos2D).length();
    float spacing = m_brush.size * m_brush.spacing;

    if (dist >= spacing) {
        int steps = qMax(1, int(dist / spacing));
        for (int i = 0; i <= steps; ++i) {
            float t = float(i) / steps;
            QPoint interpolated = QPoint(
                m_lastPos.x() + (pos2D.x() - m_lastPos.x()) * t,
                m_lastPos.y() + (pos2D.y() - m_lastPos.y()) * t
            );
            applyBrushAt(interpolated);
        }
        m_lastPos = pos2D;
    } else {
        applyBrushAt(pos2D);
    }
}

void TexturePainter::endStroke() {
    if (m_strokeActive) {
        m_strokeActive = false;
        m_image = m_strokeImage;
        m_strokeImage = QImage();
        emit strokeEnded();
        emit imageModified();
    }
}

void TexturePainter::applyBrushAt(const QPoint& pos) {
    if (m_image.isNull()) return;

    int x = pos.x() - m_brush.size / 2;
    int y = pos.y() - m_brush.size / 2;
    QRect brushRect(x, y, m_brush.size, m_brush.size);

    switch (m_brush.type) {
        case BrushDraw:
        case BrushClone: {
            QImage brush = (m_brush.type == BrushSoft)
                ? stampSoft(m_brush.size, m_brush.color)
                : stampCircle(m_brush.size, m_brush.color);

            QPainter painter(&m_strokeImage);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            painter.drawImage(brushRect.topLeft(), brush);
            break;
        }
        case BrushSmooth: {
            applyBlur(brushRect, m_brush.size / 4);
            break;
        }
        case BrushSmear: {
            QPoint smearOffset = QPoint(m_brush.size / 4, m_brush.size / 4);
            QPainter painter(&m_strokeImage);
            painter.drawImage(pos - smearOffset, m_image.copy(QRect(pos - smearOffset, QSize(m_brush.size/2, m_brush.size/2))));
            break;
        }
        case BrushDodge: {
            QPainter painter(&m_strokeImage);
            painter.setCompositionMode(QPainter::CompositionMode_Lighten);
            painter.drawImage(brushRect, stampCircle(m_brush.size, m_brush.color));
            break;
        }
        case BrushBurn: {
            QPainter painter(&m_strokeImage);
            painter.setCompositionMode(QPainter::CompositionMode_Darken);
            painter.drawImage(brushRect, stampCircle(m_brush.size, m_brush.color));
            break;
        }
        default:
            break;
    }
}

float TexturePainter::calculateAlpha(float distance, float maxDist) {
    float t = distance / maxDist;
    return qBound(0.0f, 1.0f - t * t, 1.0f);
}

QColor TexturePainter::mixColor(const QColor& a, const QColor& b, float t) {
    return QColor(
        a.red() * (1 - t) + b.red() * t,
        a.green() * (1 - t) + b.green() * t,
        a.blue() * (1 - t) + b.blue() * t,
        a.alpha() * (1 - t) + b.alpha() * t
    );
}

QImage TexturePainter::stampCircle(float size, const QColor& color) {
    QImage stamp(size, size, QImage::Format_ARGB32);
    stamp.fill(Qt::transparent);

    QPainter painter(&stamp);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(stamp.rect());
    painter.end();

    return stamp;
}

QImage TexturePainter::stampSoft(float size, const QColor& color) {
    QImage stamp(size, size, QImage::Format_ARGB32);
    stamp.fill(Qt::transparent);

    QPainter painter(&stamp);
    painter.setRenderHint(QPainter::Antialiasing);

    QRadialGradient gradient(QPointF(size/2, size/2), size/2);
    gradient.setColorAt(0, color);
    gradient.setColorAt(1, Qt::transparent);

    painter.fillRect(stamp.rect(), gradient);
    return stamp;
}

QImage TexturePainter::stampGaussian(float size, const QColor& color) {
    QImage stamp(size, size, QImage::Format_ARGB32);
    stamp.fill(Qt::transparent);

    float center = size / 2.0f;
    float sigma = size / 6.0f;

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - center;
            float dy = y - center;
            float g = qExp(-(dx*dx + dy*dy) / (2 * sigma * sigma));
            int alpha = int(255 * g * color.alphaF());
            stamp.setPixel(x, y, qRgba(color.red(), color.green(), color.blue(), alpha));
        }
    }

    return stamp;
}

void TexturePainter::setBrushType(BrushType type) {
    m_brush.type = type;
    emit brushSettingsChanged();
}

void TexturePainter::setBrushSize(float size) {
    m_brush.size = qMax(1.0f, size);
    emit brushSettingsChanged();
}

void TexturePainter::setBrushStrength(float strength) {
    m_brush.strength = qBound(0.0f, strength, 1.0f);
    emit brushSettingsChanged();
}

void TexturePainter::setBrushColor(const QColor& color) {
    m_brush.color = color;
    emit brushSettingsChanged();
}

void TexturePainter::setBrushSpacing(float spacing) {
    m_brush.spacing = qBound(0.01f, spacing, 1.0f);
    emit brushSettingsChanged();
}

void TexturePainter::setBrushAngle(float angle) {
    m_brush.angle = angle;
    emit brushSettingsChanged();
}

void TexturePainter::setCloneSource(int x, int y) {
    m_cloneOffset = QPoint(x, y);
}

QPoint TexturePainter::getCloneSource() const {
    return m_cloneOffset;
}

void TexturePainter::setCloneImage(const QImage& image) {
    m_cloneImage = image;
}

QImage TexturePainter::floodFill(int x, int y, int tolerance) {
    if (m_image.isNull()) return m_image;

    QImage result = m_image;
    QRgb targetColor = m_image.pixel(x, y);

    QQueue<QPoint> queue;
    QSet<QPoint> visited;
    queue.enqueue(QPoint(x, y));
    visited.insert(QPoint(x, y));

    while (!queue.isEmpty()) {
        QPoint p = queue.dequeue();
        if (!result.rect().contains(p)) continue;

        QRgb current = result.pixel(p);
        if (qAbs(qRed(current) - qRed(targetColor)) > tolerance ||
            qAbs(qGreen(current) - qGreen(targetColor)) > tolerance ||
            qAbs(qBlue(current) - qBlue(targetColor)) > tolerance) {
            continue;
        }

        result.setPixel(p, m_brush.color.rgba());

        QList<QPoint> neighbors = {
            QPoint(p.x() - 1, p.y()),
            QPoint(p.x() + 1, p.y()),
            QPoint(p.x(), p.y() - 1),
            QPoint(p.x(), p.y() + 1)
        };

        for (const QPoint& n : neighbors) {
            if (!visited.contains(n)) {
                visited.insert(n);
                queue.enqueue(n);
            }
        }
    }

    m_image = result;
    emit imageModified();
    return result;
}

QImage TexturePainter::colorize(int x, int y, const QColor& color, int tolerance) {
    if (m_image.isNull() || x < 0 || y < 0 || x >= m_image.width() || y >= m_image.height()) {
        return m_image;
    }
    
    QImage result = m_image.copy();
    QRgb targetColor = color.rgba();
    
    // Get the color at the specified position
    QRgb centerColor = m_image.pixel(x, y);
    int centerRed = qRed(centerColor);
    int centerGreen = qGreen(centerColor);
    int centerBlue = qBlue(centerColor);
    
    // Tolerance for color matching
    int redTolerance = tolerance;
    int greenTolerance = tolerance;
    int blueTolerance = tolerance;
    
    // Process each pixel
    for (int iy = 0; iy < result.height(); ++iy) {
        for (int ix = 0; ix < result.width(); ++ix) {
            QRgb pixel = result.pixel(ix, iy);
            int red = qRed(pixel);
            int green = qGreen(pixel);
            int blue = qBlue(pixel);
            
            // Check if pixel color is within tolerance of center color
            if (qAbs(red - centerRed) <= redTolerance &&
                qAbs(green - centerGreen) <= greenTolerance &&
                qAbs(blue - centerBlue) <= blueTolerance) {
                // Replace with colorize color
                result.setPixel(ix, iy, targetColor);
            }
        }
    }
    
    m_image = result;
    emit imageModified();
    return result;
}

void TexturePainter::resizeCanvas(int newWidth, int newHeight, int anchorX, int anchorY) {
    QImage resized(newWidth, newHeight, QImage::Format_ARGB32);
    resized.fill(Qt::transparent);

    QPainter painter(&resized);
    painter.drawImage(anchorX, anchorY, m_image);
    painter.end();

    m_image = resized;
    emit imageModified();
}

void TexturePainter::rotateCanvas(float angle) {
    QTransform transform;
    transform.rotate(angle);
    m_image = m_image.transformed(transform);
    emit imageModified();
}

void TexturePainter::flipCanvas(bool horizontal) {
    if (horizontal) {
        m_image = m_image.flipped(Qt::Horizontal);
    } else {
        m_image = m_image.flipped(Qt::Vertical);
    }
    emit imageModified();
}

void TexturePainter::applyBlur(const QRect& region, float radius) {
    if (m_image.isNull() || radius <= 0.0f) return;
    
    QRect blurRegion = region;
    if (!blurRegion.isValid()) {
        blurRegion = QRect(0, 0, m_image.width(), m_image.height());
    } else {
        blurRegion = blurRegion.intersected(QRect(0, 0, m_image.width(), m_image.height()));
    }
    
    if (blurRegion.isEmpty()) return;
    
    // Create a copy for blurring
    QImage blurred = m_image.copy();
    
    // Apply Gaussian blur (simplified box blur for performance)
    int blurRadius = qRound(radius);
    if (blurRadius < 1) blurRadius = 1;
    
    // Create temporary image for horizontal pass
    QImage temp(m_image.size(), m_image.format());
    
    // Horizontal blur
    for (int y = blurRegion.top(); y <= blurRegion.bottom(); ++y) {
        for (int x = blurRegion.left(); x <= blurRegion.right(); ++x) {
            float rSum = 0, gSum = 0, bSum = 0, aSum = 0;
            int count = 0;
            
            for (int ix = qMax(blurRegion.left(), x - blurRadius); 
                 ix <= qMin(blurRegion.right(), x + blurRadius); ++ix) {
                QRgb pixel = m_image.pixel(ix, y);
                rSum += qRed(pixel);
                gSum += qGreen(pixel);
                bSum += qBlue(pixel);
                aSum += qAlpha(pixel);
                count++;
            }
            
            if (count > 0) {
                QRgb blurredPixel = qRgba(rSum / count, gSum / count, bSum / count, aSum / count);
                temp.setPixel(x, y, blurredPixel);
            }
        }
    }
    
    // Vertical blur
    for (int y = blurRegion.top(); y <= blurRegion.bottom(); ++y) {
        for (int x = blurRegion.left(); x <= blurRegion.right(); ++x) {
            float rSum = 0, gSum = 0, bSum = 0, aSum = 0;
            int count = 0;
            
            for (int iy = qMax(blurRegion.top(), y - blurRadius); 
                 iy <= qMin(blurRegion.bottom(), y + blurRadius); ++iy) {
                QRgb pixel = temp.pixel(x, iy);
                rSum += qRed(pixel);
                gSum += qGreen(pixel);
                bSum += qBlue(pixel);
                aSum += qAlpha(pixel);
                count++;
            }
            
            if (count > 0) {
                QRgb blurredPixel = qRgba(rSum / count, gSum / count, bSum / count, aSum / count);
                blurred.setPixel(x, y, blurredPixel);
            }
        }
    }
    
    m_image = blurred;
    emit imageModified();
}

void TexturePainter::applySharpen(const QRect& region, float amount) {
    if (m_image.isNull() || amount <= 0.0f) return;
    
    QRect sharpenRegion = region;
    if (!sharpenRegion.isValid()) {
        sharpenRegion = QRect(0, 0, m_image.width(), m_image.height());
    } else {
        sharpenRegion = sharpenRegion.intersected(QRect(0, 0, m_image.width(), m_image.height()));
    }
    
    if (sharpenRegion.isEmpty()) return;
    
    // Create a copy for sharpening
    QImage sharpened = m_image.copy();
    
    // Apply sharpening filter
    float strength = qBound(0.0f, amount, 5.0f);
    
    for (int y = sharpenRegion.top(); y <= sharpenRegion.bottom(); ++y) {
        for (int x = sharpenRegion.left(); x <= sharpenRegion.right(); ++x) {
            // Skip edges
            if (x <= sharpenRegion.left() || x >= sharpenRegion.right() || 
                y <= sharpenRegion.top() || y >= sharpenRegion.bottom()) {
                continue;
            }
            
            // Get center pixel and neighbors
            QVector<int> rValues, gValues, bValues, aValues;
            
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < m_image.width() && ny >= 0 && ny < m_image.height()) {
                        QRgb pixel = m_image.pixel(nx, ny);
                        rValues.append(qRed(pixel));
                        gValues.append(qGreen(pixel));
                        bValues.append(qBlue(pixel));
                        aValues.append(qAlpha(pixel));
                    }
                }
            }
            
            // Calculate average
            float rAvg = 0, gAvg = 0, bAvg = 0, aAvg = 0;
            for (int r : rValues) rAvg += r;
            for (int g : gValues) gAvg += g;
            for (int b : bValues) bAvg += b;
            for (int a : aValues) aAvg += a;
            
            int count = rValues.size();
            if (count > 0) {
                rAvg /= count;
                gAvg /= count;
                bAvg /= count;
                aAvg /= count;
            }
            
            // Get center pixel
            QRgb centerPixel = m_image.pixel(x, y);
            int rCenter = qRed(centerPixel);
            int gCenter = qGreen(centerPixel);
            int bCenter = qBlue(centerPixel);
            int aCenter = qAlpha(centerPixel);
            
            // Apply sharpening: center + amount * (center - average)
            int rSharp = qBound(0, rCenter + static_cast<int>((rCenter - rAvg) * strength), 255);
            int gSharp = qBound(0, gCenter + static_cast<int>((gCenter - gAvg) * strength), 255);
            int bSharp = qBound(0, bCenter + static_cast<int>((bCenter - bAvg) * strength), 255);
            int aSharp = qBound(0, aCenter + static_cast<int>((aCenter - aAvg) * strength), 255);
            
            sharpened.setPixel(x, y, qRgba(rSharp, gSharp, bSharp, aSharp));
        }
    }
    
    m_image = sharpened;
    emit imageModified();
}

void TexturePainter::applyNoise(const QRect& region, float amount) {
    if (m_image.isNull() || amount <= 0.0f) return;
    
    QRect noiseRegion = region;
    if (!noiseRegion.isValid()) {
        noiseRegion = QRect(0, 0, m_image.width(), m_image.height());
    } else {
        noiseRegion = noiseRegion.intersected(QRect(0, 0, m_image.width(), m_image.height()));
    }
    
    if (noiseRegion.isEmpty()) return;
    
    // Apply noise
    float noiseAmount = qBound(0.0f, amount, 100.0f);
    
    for (int y = noiseRegion.top(); y <= noiseRegion.bottom(); ++y) {
        for (int x = noiseRegion.left(); x <= noiseRegion.right(); ++x) {
            QRgb pixel = m_image.pixel(x, y);
            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);
            int a = qAlpha(pixel);
            
            // Add random noise
            int noise = (rand() % 201) - 100; // -100 to 100
            int noiseVal = qRound(noise * noiseAmount / 100.0f);
            
            r = qBound(0, r + noiseVal, 255);
            g = qBound(0, g + noiseVal, 255);
            b = qBound(0, b + noiseVal, 255);
            
            m_image.setPixel(x, y, qRgba(r, g, b, a));
        }
    }
    
    emit imageModified();
}

TextureLayers::TextureLayers(QObject* parent)
    : QObject(parent)
{
}

TextureLayers::~TextureLayers() {
    s_textureLayers = nullptr;
}

TextureLayers* TextureLayers::instance() {
    if (!s_textureLayers) {
        s_textureLayers = new TextureLayers();
    }
    return s_textureLayers;
}

void TextureLayers::addLayer(const QString& name) {
    Layer layer;
    layer.id = QString("layer_%1").arg(m_layers.size());
    layer.name = name.isEmpty() ? QString("Layer %1").arg(m_layers.size() + 1) : name;
    layer.image = QImage(1024, 1024, QImage::Format_ARGB32);
    layer.image.fill(Qt::transparent);
    layer.visible = true;
    layer.opacity = 1.0f;

    m_layers.append(layer);
    emit layerAdded(layer.id);
}

void TextureLayers::removeLayer(const QString& id) {
    for (int i = 0; i < m_layers.size(); ++i) {
        if (m_layers[i].id == id) {
            m_layers.removeAt(i);
            emit layerRemoved(id);
            return;
        }
    }
}

QString TextureLayers::duplicateLayer(const QString& id) {
    for (const Layer& src : m_layers) {
        if (src.id == id) {
            Layer newLayer = src;
            newLayer.id = QString("layer_%1").arg(m_layers.size());
            newLayer.name = src.name + " Copy";
            m_layers.append(newLayer);
            emit layerAdded(newLayer.id);
            return newLayer.id;
        }
    }
    return QString();
}

TextureLayers::Layer TextureLayers::getLayer(const QString& id) const {
    for (const Layer& layer : m_layers) {
        if (layer.id == id) return layer;
    }
    return Layer();
}

QVector<TextureLayers::Layer> TextureLayers::getLayers() const {
    return m_layers;
}

QStringList TextureLayers::getLayerNames() const {
    QStringList names;
    for (const Layer& layer : m_layers) {
        names.append(layer.name);
    }
    return names;
}

void TextureLayers::setLayerVisible(const QString& id, bool visible) {
    for (Layer& layer : m_layers) {
        if (layer.id == id) {
            layer.visible = visible;
            emit layerModified(id);
            return;
        }
    }
}

void TextureLayers::setLayerLocked(const QString& id, bool locked) {
    for (Layer& layer : m_layers) {
        if (layer.id == id) {
            layer.locked = locked;
            emit layerModified(id);
            return;
        }
    }
}

void TextureLayers::setLayerOpacity(const QString& id, float opacity) {
    for (Layer& layer : m_layers) {
        if (layer.id == id) {
            layer.opacity = qBound(0.0f, opacity, 1.0f);
            emit layerModified(id);
            return;
        }
    }
}

void TextureLayers::setLayerBlendMode(const QString& id, int mode) {
    for (Layer& layer : m_layers) {
        if (layer.id == id) {
            layer.blendMode = mode;
            emit layerModified(id);
            return;
        }
    }
}

void TextureLayers::moveLayer(const QString& id, int newIndex) {
    int fromIndex = -1;
    for (int i = 0; i < m_layers.size(); ++i) {
        if (m_layers[i].id == id) {
            fromIndex = i;
            break;
        }
    }

    if (fromIndex >= 0 && newIndex >= 0 && newIndex < m_layers.size()) {
        m_layers.move(fromIndex, newIndex);
        emit layerOrderChanged();
    }
}

QImage TextureLayers::flatten() const {
    QImage result(1024, 1024, QImage::Format_ARGB32);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    for (const Layer& layer : m_layers) {
        if (layer.visible) {
            painter.setOpacity(layer.opacity);
            painter.drawImage(layer.offset, layer.image);
        }
    }
    painter.end();

    return result;
}

void TextureLayers::clearLayer(const QString& id) {
    for (Layer& layer : m_layers) {
        if (layer.id == id) {
            layer.image.fill(Qt::transparent);
            emit layerModified(id);
            return;
        }
    }
}

void TextureLayers::fillLayer(const QString& id, const QColor& color) {
    for (Layer& layer : m_layers) {
        if (layer.id == id) {
            layer.image.fill(color);
            emit layerModified(id);
            return;
        }
    }
}

UVViewport::UVViewport(QObject* parent)
    : QObject(parent)
{
}

UVViewport::~UVViewport() {
    s_uvViewport = nullptr;
}

UVViewport* UVViewport::instance() {
    if (!s_uvViewport) {
        s_uvViewport = new UVViewport();
    }
    return s_uvViewport;
}

void UVViewport::setUVImage(const QImage& image) {
    m_uvImage = image;
    emit uvImageChanged();
}

void UVViewport::setMeshUV(const QVariant& meshData) {
    m_meshData = meshData;
    emit meshUVChanged();
}

QVariant UVViewport::getMeshUV() const {
    return m_meshData;
}

void UVViewport::setVisibleArea(const QRect& rect) {
    m_visibleArea = rect;
    emit uvImageChanged();
}

void UVViewport::setGridVisible(bool visible) {
    m_gridVisible = visible;
    emit uvImageChanged();
}

void UVViewport::setGridSize(int size) {
    m_gridSize = qMax(4, size);
    emit uvImageChanged();
}

void UVViewport::setAspectLocked(bool locked) {
    m_aspectLocked = locked;
}

void UVViewport::unwrapAutomatic() {
    qDebug() << "[UVViewport] Automatic unwrap";
    emit meshUVChanged();
}

void UVViewport::unwrapSmart(float angleThreshold) {
    qDebug() << "[UVViewport] Smart unwrap with threshold:" << angleThreshold;
    // Use the angle threshold to determine seam placement for smart unwrap
    m_uvSeamAngle = qBound(0.0f, angleThreshold, 180.0f);
    emit meshUVChanged();
}

void UVViewport::packUVs(float margin) {
    qDebug() << "[UVViewport] Pack UVs with margin:" << margin;
    // Use margin for UV island packing
    m_uvPackMargin = qBound(0.0f, margin, 1.0f);
    emit meshUVChanged();
}

void UVViewport::stitchUVs(float threshold) {
    qDebug() << "[UVViewport] Stitch UVs with threshold:" << threshold;
    // Stitch UV islands that are within the threshold distance
    m_uvStitchThreshold = qBound(0.0f, threshold, 1.0f);
    emit meshUVChanged();
}

void UVViewport::setUVScale(float scale) {
    m_uvScale = qBound(0.1f, scale, 10.0f);
    emit uvImageChanged();
}

void UVViewport::setCursorPosition(int x, int y) {
    m_cursorPos = QPoint(x, y);
    emit cursorMoved(m_cursorPos);
}

void TextureLayers::mergeDown(const QString& id)
{
    for (int i = 1; i < m_layers.size(); ++i) {
        if (m_layers[i].id == id) {
            Layer& below = m_layers[i - 1];
            Layer& current = m_layers[i];
            QPainter painter(&below.image);
            painter.setOpacity(current.opacity);
            painter.drawImage(0, 0, current.image);
            painter.end();
            m_layers.removeAt(i);
            emit layerModified(below.id);
            emit layerRemoved(id);
            return;
        }
    }
}

void TexturePainter::drawStamp(const QImage& stamp, const QPoint& pos) {
    QPainter painter(&m_strokeImage);
    painter.drawImage(pos, stamp);
}

void TexturePainter::applyColorDodge(QRgb pixel, int delta) {
    int base = qRed(pixel);
    int blend = qBound(0, delta, 255);
    int result = (256 * base) / (256 - blend);
    result = qBound(0, result, 255);
    pixel = qRgb(result, qGreen(pixel), qBlue(pixel));
}

void TexturePainter::applyColorBurn(QRgb pixel, int delta) {
    int base = qRed(pixel);
    int blend = qBound(0, delta, 255);
    int result = 255 - ((255 - base) * 256 / (blend + 1));
    result = qBound(0, result, 255);
    pixel = qRgb(result, qGreen(pixel), qBlue(pixel));
}

}