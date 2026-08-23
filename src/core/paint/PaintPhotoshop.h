#pragma once
#include <QObject>
#include <QImage>
#include <QColor>
#include <QVariantMap>
#include <QVector>
#include <QMap>
#include <QTransform>
namespace ks { namespace paint {
enum class AdjustmentType { Levels=0, Curves, BrightnessContrast, HueSaturation, ColorBalance, BlackWhite, PhotoFilter, Exposure, Vibrance, ChannelMixer, SelectiveColor, GradientMap, Invert, Posterize, Threshold, HDRToning };
struct AdjustmentLayer { AdjustmentType type; QVariantMap params; double opacity=1.0; int blendMode=0; QImage mask; bool enabled=true; QString name; };
enum class LayerEffectType { DropShadow=0, InnerShadow, OuterGlow, InnerGlow, BevelEmboss, Stroke, ColorOverlay, GradientOverlay, PatternOverlay, Satin };
struct LayerEffect { LayerEffectType type; QVariantMap params; bool enabled=true; };
struct LayerStyle { QVector<LayerEffect> effects; double globalLightAngle=120; bool visible=true; };
struct SmartObject { QString id; QImage source; QTransform transform; QVector<QImage> smartFilters; bool linked=true; QString filePath; };
struct HistoryState { QImage image; QString name; qint64 timestamp; };
enum class CameraRawParamsType { Exposure=0, Highlights, Shadows, Whites, Blacks, Clarity, Vibrance, Saturation, Temp, Tint, Dehaze };
struct CameraRawSettings { double exposure=0, highlights=0, shadows=0, whites=0, blacks=0, clarity=0, vibrance=0, saturation=0, temp=0, tint=0, dehaze=0; QVariantMap toMap() const; void fromMap(const QVariantMap& m); };
class PaintPhotoshopEngine : public QObject {
    Q_OBJECT
public:
    explicit PaintPhotoshopEngine(QObject* p=nullptr):QObject(p){}
    int addAdjustment(AdjustmentType t, const QVariantMap& pr={});
    bool removeAdjustment(int idx);
    bool setAdjustmentParams(int idx, const QVariantMap& pr);
    QVariantMap adjustmentParams(int idx) const;
    QImage applyAdjustments(const QImage& src) const;
    QVector<AdjustmentLayer> adjustments() const { return m_adjustments; }
    int adjustmentCount() const { return m_adjustments.size(); }
    LayerStyle layerStyle(int layerIdx) const { return m_styles.value(layerIdx); }
    void setLayerStyle(int layerIdx, const LayerStyle& s){ m_styles[layerIdx]=s; emit changed(); }
    bool addLayerEffect(int layerIdx, const LayerEffect& e){ m_styles[layerIdx].effects.append(e); emit changed(); return true; }
    bool removeLayerEffect(int layerIdx, int effIdx);
    QImage applyLayerStyle(const QImage& src, int layerIdx, const QSize& docSize) const;
    QString addSmartObject(const QImage& img, const QString& path="");
    bool updateSmartObject(const QString& id, const QImage& img);
    bool rasterizeSmartObject(const QString& id);
    SmartObject smartObject(const QString& id) const { return m_smartObjects.value(id); }
    QStringList smartObjectIds() const { return m_smartObjects.keys(); }
    bool addSmartFilter(const QString& id, const QImage& filterResult){ if(!m_smartObjects.contains(id)) return false; m_smartObjects[id].smartFilters.append(filterResult); emit changed(); return true; }
    QImage contentAwareFill(const QImage& img, const QImage& mask) const;
    QImage contentAwareMove(const QImage& img, const QRect& src, const QPoint& dst) const;
    QImage contentAwarePatch(const QImage& img, const QRect& src, const QRect& dst) const;
    QImage puppetWarp(const QImage& img, const QVector<QPointF>& srcPts, const QVector<QPointF>& dstPts) const;
    QImage liquify(const QImage& img, const QPoint& center, float radius, float strength, int mode=0) const;
    QImage vanishingPoint(const QImage& img, const QVector<QPointF>& plane) const;
    QImage perspectiveWarp(const QImage& img, const QVector<QPointF>& srcQuad, const QVector<QPointF>& dstQuad) const;
    QImage cameraRawFilter(const QImage& img, const CameraRawSettings& s) const;
    QImage neuralFilter(const QImage& img, const QString& filterId, const QVariantMap& params={}) const;
    QImage selectSubject(const QImage& img) const;
    QImage quickSelection(const QImage& img, const QPoint& seed, float tolerance=20) const;
    QImage objectSelection(const QImage& img, const QRect& roi) const;
    QImage skySelection(const QImage& img) const;
    void pushHistory(const QImage& img, const QString& name="State");
    bool canUndoHistory() const { return m_historyIdx>0; }
    bool canRedoHistory() const { return m_historyIdx+1 < m_history.size(); }
    QImage historyUndo();
    QImage historyRedo();
    void createSnapshot(const QString& name);
    QImage snapshot(const QString& name) const { return m_snapshots.value(name); }
    QStringList snapshots() const { return m_snapshots.keys(); }
    int historyCount() const { return m_history.size(); }
    struct BrushDynamics { double spacing=0.25, jitter=0, scattering=0, textureScale=1, dualBlend=0; bool penPressure=false, penTilt=false; };
    void setBrushDynamics(const BrushDynamics& d){ m_brushDyn=d; emit changed(); }
    BrushDynamics brushDynamics() const { return m_brushDyn; }
    QImage applyBrushDynamics(const QImage& dab, float pressure, float tiltX, float tiltY) const;
    QString addAction(const QString& name, const QVariantList& steps);
    bool playAction(const QString& name);
    bool removeAction(const QString& name){ return m_actions.remove(name)>0; }
    QStringList actionNames() const { return m_actions.keys(); }
    QVariantList actionSteps(const QString& name) const { return m_actions.value(name); }
    bool batchPlay(const QString& actionName, const QStringList& files, const QString& outDir);
    QStringList layerComps() const { return m_layerComps.keys(); }
    bool saveLayerComp(const QString& name, const QVariantMap& state);
    bool loadLayerComp(const QString& name);
    bool removeLayerComp(const QString& name){ return m_layerComps.remove(name)>0; }
    QVariantMap artboardProperties(int idx) const;
    int addArtboard(const QRect& r, const QString& name="Artboard");
    bool removeArtboard(int idx);
signals: void changed();
private:
    QVector<AdjustmentLayer> m_adjustments;
    QMap<int, LayerStyle> m_styles;
    QMap<QString, SmartObject> m_smartObjects;
    QVector<HistoryState> m_history; int m_historyIdx=-1;
    QMap<QString, QImage> m_snapshots;
    QMap<QString, QVariantList> m_actions;
    QMap<QString, QVariantMap> m_layerComps;
    QVector<QRect> m_artboards;
    BrushDynamics m_brushDyn;
    CameraRawSettings m_cameraRaw;
};
} }
