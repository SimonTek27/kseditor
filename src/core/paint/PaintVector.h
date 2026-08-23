#pragma once
#include <QObject>
#include <QImage>
#include <QPainterPath>
#include <QTransform>
#include <QColor>
#include <QGradient>
#include <QXmlStreamReader>
#include <QSvgRenderer>
#include <QVector>
#include <QMap>
#include <QVariantMap>
namespace ks { namespace paint {
enum class VectorObjectType { Path=0, Rect, Ellipse, Star, Polygon, Spiral, Box3D, Text, Image, Group, Clone, Offset };
enum class VectorBooleanOp { Union=0, Difference, Intersection, Exclusion, Division, CutPath };
enum class VectorPathEffect { None=0, Simplify, Inset, Outset, DynamicOffset, LinkedOffset, Bend, PatternAlongPath, Envelope };
struct VectorStyle {
    QColor fill = QColor(0,0,0);
    QColor stroke = QColor(Qt::transparent);
    double strokeWidth = 1.0;
    Qt::PenStyle penStyle = Qt::SolidLine;
    Qt::PenCapStyle capStyle = Qt::SquareCap;
    Qt::PenJoinStyle joinStyle = Qt::MiterJoin;
    QVector<qreal> dashPattern;
    double dashOffset = 0;
    double opacity = 1.0;
    QString markerStart, markerMid, markerEnd;
    QGradient* fillGradient = nullptr;
    QGradient* strokeGradient = nullptr;
    QString patternId;
    int blendMode = 0;
    bool fillNone = false;
    bool strokeNone = true;
};
struct VectorNode {
    QPointF pos;
    QPointF handleIn;
    QPointF handleOut;
    bool smooth = false;
    bool cusp = true;
};
class VectorObject {
public:
    VectorObjectType type = VectorObjectType::Path;
    QString id;
    QString label;
    QPainterPath path;
    QVector<VectorNode> nodes;
    VectorStyle style;
    QTransform transform;
    double rotation = 0;
    bool visible = true;
    bool locked = false;
    QString cloneSourceId;
    QString textContent;
    QFont textFont = QFont("Arial", 12);
    QPainterPath textOnPathRef;
    VectorPathEffect lpe = VectorPathEffect::None;
    QVariantMap lpeParams;
    QMap<QString,QString> svgAttrs;
    void rebuildPathFromNodes();
    void rebuildNodesFromPath();
    QRectF bounds() const { return transform.map(path).boundingRect(); }
};
class PaintVectorDocument : public QObject {
    Q_OBJECT
public:
    explicit PaintVectorDocument(QObject* parent=nullptr);
    int objectCount() const { return m_objects.size(); }
    VectorObject& objectAt(int i) { return m_objects[i]; }
    const VectorObject& objectAt(int i) const { return m_objects[i]; }
    int addObject(const VectorObject& o);
    bool removeObject(int idx);
    bool removeObjectById(const QString& id);
    int indexOf(const QString& id) const;
    void clear() { m_objects.clear(); emit changed(); }
    QPainterPath combinedPath() const;
    QImage rasterize(const QSize& size, double dpr=1.0) const;
    bool importSvg(const QString& svgText);
    bool importSvgFile(const QString& path);
    QString exportSvg() const;
    bool exportSvgFile(const QString& path) const;
    bool importPdf(const QString& path);
    bool importEps(const QString& path) { Q_UNUSED(path); return false; }
    VectorObject createRect(const QRectF& r, double rx=0, double ry=0, const VectorStyle& s={});
    VectorObject createEllipse(const QRectF& r, const VectorStyle& s={});
    VectorObject createStar(const QPointF& c, double r1, double r2, int points, double phase=0, const VectorStyle& s={});
    VectorObject createPolygon(const QPointF& c, double r, int sides, double phase=0, const VectorStyle& s={});
    VectorObject createSpiral(const QPointF& c, double t0, double r0, double t1, double r1, double turns, const VectorStyle& s={});
    VectorObject createBox3D(const QRectF& r, double depth, const VectorStyle& s={});
    VectorObject createText(const QPointF& p, const QString& txt, const QFont& f, const VectorStyle& s={});
    bool textOnPath(int textIdx, int pathIdx);
    bool textInShape(int textIdx, int shapeIdx);
    QPainterPath booleanOp(int aIdx, int bIdx, VectorBooleanOp op) const;
    bool applyBoolean(int aIdx, int bIdx, VectorBooleanOp op);
    bool simplify(int idx, double threshold=2.0);
    bool inset(int idx, double d);
    bool outset(int idx, double d);
    bool dynamicOffset(int idx, double d);
    bool strokeToPath(int idx);
    bool objectToPath(int idx);
    bool breakApart(int idx);
    bool combine(const QVector<int>& indices);
    QPainterPath traceBitmap(const QImage& img, double threshold=128, bool color=false, int colors=2);
    int addClone(const QString& sourceId, const QTransform& t={});
    bool tiledClones(int sourceIdx, int rows, int cols, double dx, double dy);
    bool sprayClones(int sourceIdx, int count, const QRectF& area, double scaleMin=0.5, double scaleMax=1.5);
    void setFill(int idx, const QColor& c) { if(idx>=0&&idx<m_objects.size()){ m_objects[idx].style.fill=c; m_objects[idx].style.fillNone=false; emit changed(); } }
    void setStroke(int idx, const QColor& c, double w=1.0){ if(idx>=0&&idx<m_objects.size()){ m_objects[idx].style.stroke=c; m_objects[idx].style.strokeWidth=w; m_objects[idx].style.strokeNone=false; emit changed(); } }
    void setGradient(int idx, QGradient* g, bool isFill=true);
    void setPattern(int idx, const QString& patId){ if(idx>=0&&idx<m_objects.size()) m_objects[idx].style.patternId=patId; emit changed(); }
    void setDash(int idx, const QVector<qreal>& d, double off=0){ if(idx>=0&&idx<m_objects.size()){ m_objects[idx].style.dashPattern=d; m_objects[idx].style.dashOffset=off; emit changed(); } }
    void setMarker(int idx, const QString& s, const QString& m, const QString& e){ if(idx>=0&&idx<m_objects.size()){ m_objects[idx].style.markerStart=s; m_objects[idx].style.markerMid=m; m_objects[idx].style.markerEnd=e; emit changed(); } }
    bool setLpe(int idx, VectorPathEffect e, const QVariantMap& p={}){ if(idx<0||idx>=m_objects.size()) return false; m_objects[idx].lpe=e; m_objects[idx].lpeParams=p; applyLpe(idx); emit changed(); return true; }
    void applyLpe(int idx);
    void applyFilters(QImage& img, const QString& filterId) const;
    QSize docSize() const { return m_docSize; }
    void setDocSize(const QSize& s){ m_docSize=s; emit changed(); }
    int pageCount() const { return m_pages.size(); }
    void addPage(const QSize& s){ m_pages.append(s); emit changed(); }
    QRectF pageRect(int i) const { if(i<0||i>=m_pages.size()) return QRectF(QPointF(), m_docSize); return QRectF(QPointF(i*m_docSize.width(),0), m_pages[i]); }
    QVariantMap documentProperties() const;
    void setDocumentProperty(const QString& k, const QVariant& v){ m_props[k]=v; emit changed(); }
    QString xmlEditorText() const { return exportSvg(); }
    bool setXmlEditorText(const QString& t){ return importSvg(t); }
    QVariantMap objectProperties(int idx) const;
    bool setObjectProperty(int idx, const QString& k, const QVariant& v);
    bool alignObjects(const QVector<int>& ids, int alignType);
    bool distributeObjects(const QVector<int>& ids, int dir, double gap);
    bool arrangeObjects(const QVector<int>& ids, int cols, double padX, double padY);
    void setSnapEnabled(bool e){ m_snapEnabled=e; }
    bool snapEnabled() const { return m_snapEnabled; }
    QPointF snapPoint(const QPointF& p) const;
    QImage renderThumbnail(int w=256,int h=256) const { return rasterize(QSize(w,h)); }
signals:
    void changed();
private:
    QVector<VectorObject> m_objects;
    QSize m_docSize = QSize(800,600);
    QVector<QSize> m_pages;
    QMap<QString,QVariant> m_props;
    QMap<QString,QImage> m_patterns;
    QMap<QString,QString> m_filters;
    bool m_snapEnabled = true;
    double m_gridSize = 10.0;
};
} }
