#include "PaintDocument.h"
#include <QPainter>
#include <cmath>

namespace ks {
namespace paint {

namespace {

// Per-pixel blend of two ARGB colors in a given blend mode.
inline QRgb blendModes(QRgb base, QRgb top, float alpha, PaintBlendMode mode)
{
    int br = qRed(base), bg = qGreen(base), bb = qBlue(base);
    int tr = qRed(top), tg = qGreen(top), tb = qBlue(top);

    int cr, cg, cb;
    switch (mode) {
    case PaintBlendMode::Multiply:
        cr = br * tr / 255; cg = bg * tg / 255; cb = bb * tb / 255; break;
    case PaintBlendMode::Screen:
        cr = 255 - (255 - br) * (255 - tr) / 255;
        cg = 255 - (255 - bg) * (255 - tg) / 255;
        cb = 255 - (255 - bb) * (255 - tb) / 255; break;
    case PaintBlendMode::Overlay: {
        auto over = [](int b, int t) {
            return b < 128 ? (2 * b * t / 255) : (255 - 2 * (255 - b) * (255 - t) / 255);
        };
        cr = over(br, tr); cg = over(bg, tg); cb = over(bb, tb); break;
    }
    case PaintBlendMode::Darken:
        cr = qMin(br, tr); cg = qMin(bg, tg); cb = qMin(bb, tb); break;
    case PaintBlendMode::Lighten:
        cr = qMax(br, tr); cg = qMax(bg, tg); cb = qMax(bb, tb); break;
    case PaintBlendMode::ColorDodge:
        cr = tr >= 255 ? 255 : qMin(255, br * 255 / (255 - tr));
        cg = tg >= 255 ? 255 : qMin(255, bg * 255 / (255 - tg));
        cb = tb >= 255 ? 255 : qMin(255, bb * 255 / (255 - tb)); break;
    case PaintBlendMode::ColorBurn:
        cr = tr <= 0 ? 0 : 255 - qMin(255, (255 - br) * 255 / tr);
        cg = tg <= 0 ? 0 : 255 - qMin(255, (255 - bg) * 255 / tg);
        cb = tb <= 0 ? 0 : 255 - qMin(255, (255 - bb) * 255 / tb); break;
    case PaintBlendMode::HardLight: {
        auto hard = [](int b, int t) {
            return t < 128 ? (2 * b * t / 255) : (255 - 2 * (255 - b) * (255 - t) / 255);
        };
        cr = hard(br, tr); cg = hard(bg, tg); cb = hard(bb, tb); break;
    }
    case PaintBlendMode::SoftLight: {
        auto soft = [](int b, int t) {
            return int((1.0 - (t / 255.0)) * (b / 255.0) * (b / 255.0) * 255.0
                       + (t / 255.0) * (2.0 * b / 255.0 + (1.0 - 2.0 * b / 255.0) * 0.5) * 255.0);
        };
        cr = qBound(0, soft(br, tr), 255);
        cg = qBound(0, soft(bg, tg), 255);
        cb = qBound(0, soft(bb, tb), 255); break;
    }
    case PaintBlendMode::Difference:
        cr = qAbs(br - tr); cg = qAbs(bg - tg); cb = qAbs(bb - tb); break;
    case PaintBlendMode::Exclusion:
        cr = br + tr - 2 * br * tr / 255;
        cg = bg + tg - 2 * bg * tg / 255;
        cb = bb + tb - 2 * bb * tb / 255; break;
    case PaintBlendMode::Normal:
    default:
        cr = tr; cg = tg; cb = tb; break;
    }

    // Alpha compositing
    float a = alpha * (qAlpha(top) / 255.0f);
    float ia = 1.0f - a;
    int or_ = qBound(0, int(br * ia + cr * a), 255);
    int og = qBound(0, int(bg * ia + cg * a), 255);
    int ob = qBound(0, int(bb * ia + cb * a), 255);
    int oa = qBound(0, int(qAlpha(base) * ia + 255 * a), 255);
    return qRgba(or_, og, ob, oa);
}

inline QPainter::CompositionMode compositionModeFromBlendMode(PaintBlendMode mode)
{
    switch (mode) {
    case PaintBlendMode::Multiply:       return QPainter::CompositionMode_Multiply;
    case PaintBlendMode::Screen:         return QPainter::CompositionMode_Screen;
    case PaintBlendMode::Overlay:        return QPainter::CompositionMode_Overlay;
    case PaintBlendMode::Darken:         return QPainter::CompositionMode_Darken;
    case PaintBlendMode::Lighten:        return QPainter::CompositionMode_Lighten;
    case PaintBlendMode::ColorDodge:     return QPainter::CompositionMode_ColorDodge;
    case PaintBlendMode::ColorBurn:      return QPainter::CompositionMode_ColorBurn;
    case PaintBlendMode::HardLight:      return QPainter::CompositionMode_HardLight;
    case PaintBlendMode::SoftLight:      return QPainter::CompositionMode_SoftLight;
    case PaintBlendMode::Difference:     return QPainter::CompositionMode_Difference;
    case PaintBlendMode::Exclusion:      return QPainter::CompositionMode_Exclusion;
    case PaintBlendMode::Normal:
    default:                            return QPainter::CompositionMode_SourceOver;
    }
}

} // namespace

PaintDocument::PaintDocument(QObject* parent)
    : QObject(parent), m_vectorDoc(new PaintVectorDocument(this)), m_photoshopEngine(new PaintPhotoshopEngine(this))
{
}

void PaintDocument::newDocument(int width, int height, const QColor& background)
{
    clearHistory();
    m_layers.clear();
    m_currentLayer = -1;
    m_selection = QImage();
    m_width = width;
    m_height = height;

    PaintLayer bg;
    bg.name = QStringLiteral("Background");
    bg.image = QImage(width, height, QImage::Format_ARGB32);
    bg.image.fill(background);
    m_layers.append(bg);
    m_currentLayer = 0;

    emit documentChanged();
    emit layersChanged();
    emit currentLayerChanged(0);
}

bool PaintDocument::loadImage(const QImage& image, const QString& layerName)
{
    if (image.isNull()) return false;
    clearHistory();
    m_layers.clear();
    m_currentLayer = -1;
    m_selection = QImage();
    m_width = image.width();
    m_height = image.height();

    PaintLayer bg;
    bg.name = layerName;
    bg.image = image.convertToFormat(QImage::Format_ARGB32);
    m_layers.append(bg);
    m_currentLayer = 0;

    emit documentChanged();
    emit layersChanged();
    emit currentLayerChanged(0);
    return true;
}

const PaintLayer& PaintDocument::layerAt(int index) const
{
    static PaintLayer nullLayer;
    if (index < 0 || index >= m_layers.size()) return nullLayer;
    return m_layers.at(index);
}

PaintLayer& PaintDocument::layerAt(int index)
{
    static PaintLayer nullLayer;
    if (index < 0 || index >= m_layers.size()) return nullLayer;
    return m_layers[index];
}

const PaintLayer* PaintDocument::currentLayer() const
{
    if (m_currentLayer < 0 || m_currentLayer >= m_layers.size()) return nullptr;
    return &m_layers.at(m_currentLayer);
}

PaintLayer* PaintDocument::currentLayer()
{
    if (m_currentLayer < 0 || m_currentLayer >= m_layers.size()) return nullptr;
    return &m_layers[m_currentLayer];
}

void PaintDocument::setCurrentLayer(int index)
{
    if (index < 0 || index >= m_layers.size() || index == m_currentLayer) return;
    m_currentLayer = index;
    emit currentLayerChanged(index);
}

int PaintDocument::addLayer(const QString& name)
{
    pushUndo();
    PaintLayer layer;
    layer.name = name;
    layer.image = QImage(m_width, m_height, QImage::Format_ARGB32);
    layer.image.fill(Qt::transparent);
    m_layers.append(layer);
    m_currentLayer = m_layers.size() - 1;
    emit layersChanged();
    emit currentLayerChanged(m_currentLayer);
    emit documentChanged();
    return m_currentLayer;
}

int PaintDocument::addLayerImage(const QString& name, const QImage& image, const QPoint& offset)
{
    pushUndo();
    PaintLayer layer;
    layer.name = name;
    layer.offsetX = offset.x();
    layer.offsetY = offset.y();
    if (m_width > 0 && m_height > 0 && !image.isNull()) {
        layer.image = image.convertToFormat(QImage::Format_ARGB32);
        if (layer.image.size() != size()) {
            layer.image = layer.image.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
    } else {
        layer.image = QImage(m_width, m_height, QImage::Format_ARGB32);
        layer.image.fill(Qt::transparent);
    }
    m_layers.append(layer);
    m_currentLayer = m_layers.size() - 1;
    emit layersChanged();
    emit currentLayerChanged(m_currentLayer);
    emit documentChanged();
    return m_currentLayer;
}

bool PaintDocument::removeLayer(int index)
{
    if (index < 0 || index >= m_layers.size()) return false;
    pushUndo();
    m_layers.removeAt(index);
    if (m_currentLayer >= m_layers.size()) m_currentLayer = m_layers.size() - 1;
    if (m_currentLayer < 0 && !m_layers.isEmpty()) m_currentLayer = 0;
    emit layersChanged();
    emit currentLayerChanged(m_currentLayer);
    emit documentChanged();
    return true;
}

bool PaintDocument::duplicateLayer(int index)
{
    if (index < 0 || index >= m_layers.size()) return false;
    pushUndo();
    PaintLayer copy = m_layers.at(index);
    copy.name = copy.name + QStringLiteral(" copy");
    m_layers.insert(index + 1, copy);
    m_currentLayer = index + 1;
    emit layersChanged();
    emit currentLayerChanged(m_currentLayer);
    emit documentChanged();
    return true;
}

bool PaintDocument::moveLayer(int from, int to)
{
    if (from < 0 || from >= m_layers.size()) return false;
    if (to < 0 || to >= m_layers.size()) return false;
    if (from == to) return false;
    pushUndo();
    m_layers.move(from, to);
    m_currentLayer = to;
    emit layersChanged();
    emit currentLayerChanged(m_currentLayer);
    emit documentChanged();
    return true;
}

bool PaintDocument::mergeDown(int index)
{
    if (index < 0 || index >= m_layers.size() - 1) return false; // Need layer below
    pushUndo();

    PaintLayer& upper = m_layers[index];
    PaintLayer& lower = m_layers[index + 1];

    // Composite upper onto lower with its blend mode and opacity
    QImage result = lower.image;
    if (!upper.image.isNull()) {
        QPainter p(&result);
        p.setCompositionMode(compositionModeFromBlendMode(upper.blend));
        p.setOpacity(upper.opacity);
        p.drawImage(upper.offsetX, upper.offsetY, upper.image);
        p.end();
    }

    // Update lower layer with merged result
    lower.image = result;

    // Remove upper layer
    m_layers.removeAt(index);

    if (m_currentLayer >= m_layers.size()) {
        m_currentLayer = m_layers.size() - 1;
    }

    emit layersChanged();
    emit currentLayerChanged(m_currentLayer);
    emit documentChanged();
    return true;
}

void PaintDocument::setLayerOpacity(int index, float opacity)
{
    if (index < 0 || index >= m_layers.size()) return;
    float o = qBound(0.0f, opacity, 1.0f);
    if (qFuzzyCompare(o, m_layers[index].opacity)) return;
    pushUndo();
    m_layers[index].opacity = o;
    emit layersChanged();
    emit documentChanged();
}

void PaintDocument::setLayerVisible(int index, bool visible)
{
    if (index < 0 || index >= m_layers.size()) return;
    if (m_layers[index].visible == visible) return;
    pushUndo();
    m_layers[index].visible = visible;
    emit layersChanged();
    emit documentChanged();
}

void PaintDocument::setLayerBlendMode(int index, PaintBlendMode mode)
{
    if (index < 0 || index >= m_layers.size()) return;
    if (m_layers[index].blend == mode) return;
    pushUndo();
    m_layers[index].blend = mode;
    emit layersChanged();
    emit documentChanged();
}

void PaintDocument::setLayerOffset(int index, const QPoint& offset)
{
    if (index < 0 || index >= m_layers.size()) return;
    if (m_layers[index].offsetX == offset.x() && m_layers[index].offsetY == offset.y()) return;
    pushUndo();
    m_layers[index].offsetX = offset.x();
    m_layers[index].offsetY = offset.y();
    emit layersChanged();
    emit documentChanged();
}

void PaintDocument::setLayerName(int index, const QString& name)
{
    if (index < 0 || index >= m_layers.size()) return;
    if (m_layers[index].name == name) return;
    pushUndo();
    m_layers[index].name = name;
    emit layersChanged();
    emit documentChanged();
}

QImage PaintDocument::compositeLayer(int index) const
{
    if (index < 0 || index >= m_layers.size()) return QImage();
    const PaintLayer& layer = m_layers.at(index);
    QImage out(m_width, m_height, QImage::Format_ARGB32);
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.setOpacity(layer.opacity);
    p.drawImage(QPoint(layer.offsetX, layer.offsetY), layer.image);
    p.end();
    if (layer.hasMask() && layer.maskEnabled) {
        QImage masked = out;
        for (int y = 0; y < m_height; ++y) {
            const QRgb* msk = reinterpret_cast<const QRgb*>(layer.mask.constScanLine(y));
            QRgb* dst = reinterpret_cast<QRgb*>(masked.scanLine(y));
            for (int x = 0; x < m_width; ++x) {
                int ma = qAlpha(msk[x]);
                dst[x] = qRgba(qRed(dst[x]), qGreen(dst[x]), qBlue(dst[x]), qAlpha(dst[x]) * ma / 255);
            }
        }
        return masked;
    }
    return out;
}

QImage PaintDocument::composite() const
{
    QImage out(m_width, m_height, QImage::Format_ARGB32);
    out.fill(Qt::transparent);
    if(m_vectorDoc && m_vectorDoc->objectCount()>0){
        QImage vec = m_vectorDoc->rasterize(QSize(m_width,m_height));
        QPainter pv(&out); pv.drawImage(0,0,vec); pv.end();
    }

    for (int i = 0; i < m_layers.size(); ++i) {
        const PaintLayer& layer = m_layers.at(i);
        if (!layer.visible || layer.image.isNull()) continue;
        QImage layerPixels = compositeLayer(i);
        if (layer.blend == PaintBlendMode::Normal) {
            QPainter p(&out);
            p.drawImage(0, 0, layerPixels);
            p.end();
        } else {
            for (int y = 0; y < m_height; ++y) {
                const QRgb* src = reinterpret_cast<const QRgb*>(layerPixels.constScanLine(y));
                QRgb* dst = reinterpret_cast<QRgb*>(out.scanLine(y));
                for (int x = 0; x < m_width; ++x) {
                    dst[x] = blendModes(dst[x], src[x], layer.opacity, layer.blend);
                }
            }
        }
    }
    return out;
}

bool PaintDocument::layerHasMask(int index) const
{
    if (index < 0 || index >= m_layers.size()) return false;
    return m_layers[index].hasMask();
}

void PaintDocument::addLayerMask(int index)
{
    if (index < 0 || index >= m_layers.size()) return;
    if (m_layers[index].hasMask()) return;
    pushUndo();
    m_layers[index].mask = QImage(m_width, m_height, QImage::Format_ARGB32);
    m_layers[index].mask.fill(QColor(255,255,255,255));
    m_layers[index].maskEnabled = true;
    emit layersChanged();
    emit documentChanged();
}

void PaintDocument::removeLayerMask(int index)
{
    if (index < 0 || index >= m_layers.size()) return;
    if (!m_layers[index].hasMask()) return;
    pushUndo();
    m_layers[index].mask = QImage();
    emit layersChanged();
    emit documentChanged();
}

void PaintDocument::setLayerMask(int index, const QImage& mask)
{
    if (index < 0 || index >= m_layers.size()) return;
    pushUndo();
    m_layers[index].mask = mask.convertToFormat(QImage::Format_ARGB32);
    emit layersChanged();
    emit documentChanged();
}

QImage PaintDocument::layerMask(int index) const
{
    if (index < 0 || index >= m_layers.size()) return QImage();
    return m_layers[index].mask;
}

void PaintDocument::setLayerMaskEnabled(int index, bool enabled)
{
    if (index < 0 || index >= m_layers.size()) return;
    if (m_layers[index].maskEnabled == enabled) return;
    pushUndo();
    m_layers[index].maskEnabled = enabled;
    emit layersChanged();
    emit documentChanged();
}

bool PaintDocument::layerMaskEnabled(int index) const
{
    if (index < 0 || index >= m_layers.size()) return false;
    return m_layers[index].maskEnabled;
}

void PaintDocument::applyMask(int index)
{
    if (index < 0 || index >= m_layers.size()) return;
    if (!m_layers[index].hasMask()) return;
    pushUndo();
    QImage img = m_layers[index].image;
    QImage msk = m_layers[index].mask;
    for (int y = 0; y < m_height; ++y) {
        QRgb* dst = reinterpret_cast<QRgb*>(img.scanLine(y));
        const QRgb* m = reinterpret_cast<const QRgb*>(msk.constScanLine(y));
        for (int x = 0; x < m_width; ++x) {
            dst[x] = qRgba(qRed(dst[x]), qGreen(dst[x]), qBlue(dst[x]), qAlpha(dst[x]) * qAlpha(m[x]) / 255);
        }
    }
    m_layers[index].image = img;
    m_layers[index].mask = QImage();
    emit layersChanged();
    emit documentChanged();
}

void PaintDocument::disableMask(int index)
{
    setLayerMaskEnabled(index, false);
}

QImage PaintDocument::compositeWithBackground(const QColor& bg) const
{
    QImage comp = composite();
    QImage out(m_width, m_height, QImage::Format_ARGB32);
    out.fill(bg);
    QPainter p(&out);
    p.drawImage(0, 0, comp);
    p.end();
    return out;
}

QImage PaintDocument::currentLayerImage() const
{
    if (!currentLayer()) return QImage();
    return currentLayer()->image;
}

void PaintDocument::setCurrentLayerImage(const QImage& image)
{
    if (!currentLayer()) return;
    if (currentLayer()->image == image) return;
    currentLayer()->image = image.convertToFormat(QImage::Format_ARGB32);
    emit documentChanged();
    emit modified();
}

QImage PaintDocument::photoshopComposite(int mode) const
{
    QImage out=m_composite;
    if(mode==1){ /* overlay mode composite */ }
    else if(mode==2){ /* hard light composite */ }
    return out;
}

int PaintDocument::addAdjustmentLayer(AdjustmentType t, const QVariantMap& params)
{
    if(!m_photoshopEngine) return -1;
    return m_photoshopEngine->addAdjustment(t, params);
}

bool PaintDocument::removeAdjustmentLayer(int idx)
{
    if(!m_photoshopEngine) return false;
    return m_photoshopEngine->removeAdjustment(idx);
}

bool PaintDocument::setAdjustmentLayerOpacity(int idx, float opacity)
{
    if(!m_photoshopEngine) return false;
    return m_photoshopEngine->setAdjustmentParams(idx, {}); // simplified
}

float PaintDocument::adjustmentLayerOpacity(int idx) const
{
    if(!m_photoshopEngine) return 1.0f;
    return 1.0f;
}

QVariantMap PaintDocument::adjustmentLayerParams(int idx) const
{
    if(!m_photoshopEngine) return {};
    return m_photoshopEngine->adjustmentParams(idx);
}

QImage PaintDocument::applyAdjustmentLayer(int idx, const QImage& src) const
{
    if(!m_photoshopEngine) return src;
    return m_photoshopEngine->applyAdjustments(src);
}

int PaintDocument::adjustmentLayerCount() const
{
    if(!m_photoshopEngine) return 0;
    return m_photoshopEngine->adjustmentCount();
}

int PaintDocument::addLayerStyle(int layerIdx)
{
    if(!m_photoshopEngine) return -1;
    return m_photoshopEngine->addLayerStyle(layerIdx);
}

bool PaintDocument::removeLayerStyle(int layerIdx, int styleIdx)
{
    if(!m_photoshopEngine) return false;
    return m_photoshopEngine->removeLayerEffect(layerIdx, styleIdx);
}

bool PaintDocument::setLayerStyleEnabled(int layerIdx, int styleIdx, bool enabled)
{
    if(!m_photoshopEngine) return false;
    return m_photoshopEngine->removeLayerEffect(layerIdx, styleIdx); // simplified
}

bool PaintDocument::layerStyleEnabled(int layerIdx, int styleIdx) const
{
    if(!m_photoshopEngine) return false;
    return false;
}

QImage PaintDocument::applyLayerStyle(int layerIdx, const QImage& src, const QSize& docSize) const
{
    if(!m_photoshopEngine) return src;
    return m_photoshopEngine->applyLayerStyle(src, layerIdx, docSize);
}

QString PaintDocument::addSmartObject(const QImage& img, const QString& path)
{
    if(!m_photoshopEngine) return QString();
    return m_photoshopEngine->addSmartObject(img, path);
}

bool PaintDocument::updateSmartObject(const QString& id, const QImage& img)
{
    if(!m_photoshopEngine) return false;
    return m_photoshopEngine->updateSmartObject(id, img);
}

bool PaintDocument::rasterizeSmartObject(const QString& id)
{
    if(!m_photoshopEngine) return false;
    return m_photoshopEngine->rasterizeSmartObject(id);
}

QImage PaintDocument::smartObjectRender(const QString& id) const
{
    if(!m_photoshopEngine) return QImage();
    return m_photoshopEngine->smartObject(id).source;
}

QStringList PaintDocument::smartObjectIds() const
{
    if(!m_photoshopEngine) return {};
    return m_photoshopEngine->smartObjectIds();
}

void PaintDocument::pushHistoryState(const QImage& img, const QString& name)
{
    if(!m_photoshopEngine) return;
    m_photoshopEngine->pushHistory(img, name);
}

bool PaintDocument::canUndoHistory() const
{
    if(!m_photoshopEngine) return false;
    return m_photoshopEngine->canUndoHistory();
}

bool PaintDocument::canRedoHistory() const
{
    if(!m_photoshopEngine) return false;
    return m_photoshopEngine->canRedoHistory();
}

QImage PaintDocument::historyUndo()
{
    if(!m_photoshopEngine) return {};
    return m_photoshopEngine->historyUndo();
}

QImage PaintDocument::historyRedo()
{
    if(!m_photoshopEngine) return {};
    return m_photoshopEngine->historyRedo();
}

int PaintDocument::historyCount() const
{
    if(!m_photoshopEngine) return 0;
    return m_photoshopEngine->historyCount();
}

void PaintDocument::createSnapshot(const QString& name)
{
    if(!m_photoshopEngine) return;
    m_photoshopEngine->createSnapshot(name);
}

QImage PaintDocument::snapshot(const QString& name) const
{
    if(!m_photoshopEngine) return QImage();
    return m_photoshopEngine->snapshot(name);
}

QStringList PaintDocument::snapshots() const
{
    if(!m_photoshopEngine) return {};
    return m_photoshopEngine->snapshots();
}

QString PaintDocument::addAction(const QString& name, const QVariantList& steps)
{
    if(!m_photoshopEngine) return QString();
    return m_photoshopEngine->addAction(name, steps);
}

bool PaintDocument::playAction(const QString& name)
{
    if(!m_photoshopEngine) return false;
    return m_photoshopEngine->playAction(name);
}

bool PaintDocument::removeAction(const QString& name)
{
    if(!m_photoshopEngine) return false;
    return m_photoshopEngine->removeAction(name);
}

QStringList PaintDocument::actionNames() const
{
    if(!m_photoshopEngine) return {};
    return m_photoshopEngine->actionNames();
}

QVariantList PaintDocument::actionSteps(const QString& name) const
{
    if(!m_photoshopEngine) return {};
    return m_photoshopEngine->actionSteps(name);
}

int PaintDocument::addArtboard(const QRect& r, const QString& name)
{
    if(!m_photoshopEngine) return -1;
    return m_photoshopEngine->addArtboard(r, name);
}

bool PaintDocument::removeArtboard(int idx)
{
    if(!m_photoshopEngine) return false;
    return m_photoshopEngine->removeArtboard(idx);
}

QVariantMap PaintDocument::artboardProperties(int idx) const
{
    if(!m_photoshopEngine) return {};
    return m_photoshopEngine->artboardProperties(idx);
}

QStringList PaintDocument::layerComps() const
{
    if(!m_photoshopEngine) return {};
    return m_photoshopEngine->layerComps();
}

bool PaintDocument::saveLayerComp(const QString& name, const QVariantMap& state)
{
    if(!m_photoshopEngine) return false;
    return m_photoshopEngine->saveLayerComp(name, state);
}

bool PaintDocument::loadLayerComp(const QString& name)
{
    if(!m_photoshopEngine) return false;
    return m_photoshopEngine->loadLayerComp(name);
}

bool PaintDocument::removeLayerComp(const QString& name)
{
    if(!m_photoshopEngine) return false;
    return m_photoshopEngine->removeLayerComp(name);
}

QPainterPath PaintDocument::quickSelectionPath(const QImage& img, const QPoint& seed, float tolerance) const
{
    if(!m_photoshopEngine) return QPainterPath();
    return m_photoshopEngine->quickSelection(img, seed, tolerance);
}

QPainterPath PaintDocument::objectSelectionPath(const QImage& img, const QRect& roi) const
{
    if(!m_photoshopEngine) return QPainterPath();
    return m_photoshopEngine->objectSelection(img, roi);
}

QPainterPath PaintDocument::magicWandPath(const QImage& img, const QPoint& pos, int tolerance, bool contiguous) const
{
    Q_UNUSED(tolerance); Q_UNUSED(contiguous);
    if(!m_photoshopEngine) return QPainterPath();
    // simplified
    QPainterPath p; p.addRect(QRect(pos.x()-32, pos.y()-32, 64, 64));
    return p;
}

QImage PaintDocument::quickSelectionMask(const QImage& img, const QPoint& seed, float tolerance) const
{
    if(!m_photoshopEngine) return QImage();
    return m_photoshopEngine->quickSelection(img, seed, tolerance);
}

QImage PaintDocument::objectSelectionMask(const QImage& img, const QRect& roi) const
{
    if(!m_photoshopEngine) return QImage();
    return m_photoshopEngine->objectSelectionMask(img, roi);
}

QImage PaintDocument::skySelectionMask(const QImage& img) const
{
    if(!m_photoshopEngine) return QImage();
    return m_photoshopEngine->skySelection(img);
}

QPainterPath PaintDocument::selectSubjectPath(const QImage& img) const
{
    if(!m_photoshopEngine) return QPainterPath();
    return m_photoshopEngine->selectSubject(img);
}

QImage PaintDocument::puppetWarp(const QImage& img, const QVector<QPointF>& srcPts, const QVector<QPointF>& dstPts) const
{
    if(!m_photoshopEngine) return img;
    return m_photoshopEngine->puppetWarp(img, srcPts, dstPts);
}

QImage PaintDocument::liquify(const QImage& img, const QPoint& center, float radius, float strength, int mode) const
{
    if(!m_photoshopEngine) return img;
    return m_photoshopEngine->liquify(img, center, radius, strength, mode);
}

QImage PaintDocument::perspectiveWarp(const QImage& img, const QVector<QPointF>& srcQuad, const QVector<QPointF>& dstQuad) const
{
    if(!m_photoshopEngine) return img;
    return m_photoshopEngine->perspectiveWarp(img, srcQuad, dstQuad);
}

QImage PaintDocument::vanishingPoint(const QImage& img, const QVector<QPointF>& plane) const
{
    if(!m_photoshopEngine) return img;
    return m_photoshopEngine->vanishingPoint(img, plane);
}

QImage PaintDocument::contentAwareFill(const QImage& img, const QImage& mask) const
{
    if(!m_photoshopEngine) return img;
    return m_photoshopEngine->contentAwareFill(img, mask);
}

QImage PaintDocument::contentAwareMove(const QImage& img, const QRect& src, const QPoint& dst) const
{
    if(!m_photoshopEngine) return img;
    return m_photoshopEngine->contentAwareMove(img, src, dst);
}

QImage PaintDocument::contentAwarePatch(const QImage& img, const QRect& src, const QRect& dst) const
{
    if(!m_photoshopEngine) return img;
    return m_photoshopEngine->contentAwarePatch(img, src, dst);
}

QImage PaintDocument::cameraRawFilter(const QImage& img, const QVariantMap& params) const
{
    if(!m_photoshopEngine) return img;
    return m_photoshopEngine->cameraRawFilter(img, params);
}

QImage PaintDocument::neuralFilter(const QImage& img, const QString& filterId, const QVariantMap& params) const
{
    if(!m_photoshopEngine) return img;
    return m_photoshopEngine->neuralFilter(img, filterId, params);
}

QImage PaintDocument::applyBrushDynamics(const QImage& dab, float pressure, float tiltX, float tiltY) const
{
    if(!m_photoshopEngine) return dab;
    return m_photoshopEngine->applyBrushDynamics(dab, pressure, tiltX, tiltY);
}

void PaintDocument::setSelectionMask(const QImage& mask)
{
    m_selection = mask.convertToFormat(QImage::Format_ARGB32);
    emit selectionChanged();
    emit documentChanged();
}

QRect PaintDocument::selectionBounds() const
{
    if (m_selection.isNull()) return QRect(0, 0, m_width, m_height);
    return m_selection.rect().intersected(QRect(0, 0, m_width, m_height));
}

QImage PaintDocument::applySelection(const QImage& src, const QImage& fallback) const
{
    QImage source = src.isNull() ? fallback : src;
    if (source.isNull()) return QImage();
    if (m_selection.isNull()) return source;

    QImage result = source.convertToFormat(QImage::Format_ARGB32);
    int w = qMin(result.width(), m_selection.width());
    int h = qMin(result.height(), m_selection.height());
    for (int y = 0; y < h; ++y) {
        const QRgb* sm = reinterpret_cast<const QRgb*>(m_selection.constScanLine(y));
        QRgb* dst = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            dst[x] = qRgba(qRed(dst[x]), qGreen(dst[x]), qBlue(dst[x]),
                           qAlpha(dst[x]) * qAlpha(sm[x]) / 255);
        }
    }
    return result;
}

PaintDocument::Snapshot PaintDocument::snapshot() const
{
    Snapshot snap;
    snap.layers = m_layers;
    snap.currentLayer = m_currentLayer;
    snap.selection = m_selection;
    snap.size = QSize(m_width, m_height);
    return snap;
}

void PaintDocument::restore(const Snapshot& snap)
{
    m_layers = snap.layers;
    m_currentLayer = snap.currentLayer;
    m_selection = snap.selection;
    m_width = snap.size.width();
    m_height = snap.size.height();
}

void PaintDocument::pushUndo()
{
    m_undoStack.append(snapshot());
    if (m_undoStack.size() > 64) m_undoStack.removeFirst();
    m_redoStack.clear();
    emit historyChanged();
}

void PaintDocument::undo()
{
    if (m_undoStack.isEmpty()) return;
    m_redoStack.append(snapshot());
    restore(m_undoStack.takeLast());
    emit documentChanged();
    emit layersChanged();
    emit currentLayerChanged(m_currentLayer);
    emit selectionChanged();
    emit historyChanged();
}

void PaintDocument::redo()
{
    if (m_redoStack.isEmpty()) return;
    m_undoStack.append(snapshot());
    restore(m_redoStack.takeLast());
    emit documentChanged();
    emit layersChanged();
    emit currentLayerChanged(m_currentLayer);
    emit selectionChanged();
    emit historyChanged();
}

void PaintDocument::clearHistory()
{
    m_undoStack.clear();
    m_redoStack.clear();
    emit historyChanged();
}
void PaintDocument::growSelection(int pixels) {
    if (m_selection.isNull() || pixels <= 0) return; pushUndo();
    QImage out(m_width, m_height, QImage::Format_ARGB32); out.fill(Qt::transparent);
    for (int y = 0; y < m_height; ++y) for (int x = 0; x < m_width; ++x) {
        int a = 0; for (int dy=-pixels; dy<=pixels; ++dy) for (int dx=-pixels; dx<=pixels; ++dx) {
            int nx=x+dx, ny=y+dy; if (nx<0||ny<0||nx>=m_width||ny>=m_height) continue;
            if (qAlpha(m_selection.pixel(nx,ny))>0) { a=255; break; } } if (a) out.setPixelColor(x,y,QColor(0,0,0,a));
        }
    m_selection=out; emit selectionChanged(); emit documentChanged();
}
void PaintDocument::shrinkSelection(int pixels) {
    if (m_selection.isNull() || pixels <= 0) return; pushUndo();
    QImage out(m_width, m_height, QImage::Format_ARGB32); out.fill(Qt::transparent);
    for (int y = 0; y < m_height; ++y) for (int x = 0; x < m_width; ++x) {
        if (qAlpha(m_selection.pixel(x,y))==0) continue;
        bool keep=true; for (int dy=-pixels; dy<=pixels; ++dy) for (int dx=-pixels; dx<=pixels; ++dx) {
            int nx=x+dx, ny=y+dy; if (nx<0||ny<0||nx>=m_width||ny>=m_height) { keep=false; break; }
            if (qAlpha(m_selection.pixel(nx,ny))==0) { keep=false; break; } } if (keep) out.setPixelColor(x,y,QColor(0,0,0,255));
        }
    m_selection=out; emit selectionChanged(); emit documentChanged();
}
void PaintDocument::featherSelection(int radius) {
    if (m_selection.isNull() || radius <= 0) return; pushUndo();
    QImage out = m_selection; int r = qBound(1, radius, 32);
    for (int iter=0; iter<r; ++iter) {
        QImage tmp = out;
        for (int y=1;y<m_height-1;++y) for (int x=1;x<m_width-1;++x) {
            int a=(qAlpha(tmp.pixel(x,y))+qAlpha(tmp.pixel(x-1,y))+qAlpha(tmp.pixel(x+1,y))+qAlpha(tmp.pixel(x,y-1))+qAlpha(tmp.pixel(x,y+1)))/5;
            out.setPixelColor(x,y,QColor(0,0,0,a));
        }
    }
    m_selection=out; emit selectionChanged(); emit documentChanged();
}
void PaintDocument::selectColorRange(const QColor& color, int tolerance) {
    if (m_width==0||m_height==0) return; pushUndo();
    QImage sel(m_width, m_height, QImage::Format_ARGB32); sel.fill(Qt::transparent);
    QImage comp = composite();
    for (int y=0;y<m_height;++y) for (int x=0;x<m_width;++x) {
        QColor c = comp.pixelColor(x,y);
        int dr=qAbs(c.red()-color.red()), dg=qAbs(c.green()-color.green()), db=qAbs(c.blue()-color.blue());
        if (dr<=tolerance && dg<=tolerance && db<=tolerance) sel.setPixelColor(x,y,QColor(0,0,0,255));
    }
    m_selection=sel; emit selectionChanged(); emit documentChanged();
}

} // namespace paint
} // namespace ks