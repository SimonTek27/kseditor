#pragma once

#include <QObject>
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsProxyWidget>
#include <QPainter>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QMenu>
#include <QAction>
#include <QSplitter>
#include <QToolBar>
#include <QAbstractItemModel>
#include <QAbstractListModel>
#include <QVBoxLayout>
#include <QScrollBar>

namespace ks {
namespace ui {

// ─── Node Graph Data Structures ────────────────────────────────────────────

struct NodePort {
    QUuid id;
    QString name;
    QString type;           // "float", "vec3", "texture", "execution", etc.
    bool isInput = true;
    bool isMultiConnection = false;
    QVariant defaultValue;
    QString description;
};

struct NodeInfo {
    QString typeName;
    QString displayName;
    QString category;
    QString description;
    QString icon;
    QVector<NodePort> inputs;
    QVector<NodePort> outputs;
    QMap<QString, QVariant> defaultProperties;
    bool isExecNode = false;
    bool isPureNode = false;
};

struct GraphConnection {
    QUuid id;
    QUuid fromNodeId;
    QUuid fromPortId;
    QUuid toNodeId;
    QUuid toPortId;
    QVector<QPointF> customPath;  // For curved connections
};

struct GraphNode {
    QUuid id;
    QString typeName;
    QString title;
    QPointF position;
    QSizeF size;
    QMap<QString, QVariant> properties;
    QVector<QUuid> inputPortIds;
    QVector<QUuid> outputPortIds;
    bool selected = false;
    bool error = false;
    QString errorMessage;
    QColor headerColor;
    QString comment;
    bool minimized = false;
    int zOrder = 0;
};

struct GraphData {
    QString name;
    QString version = "1.0";
    QUuid id;
    QVector<GraphNode> nodes;
    QVector<GraphConnection> connections;
    QMap<QString, QVariant> metadata;
    QRectF viewportRect;
    float zoom = 1.0f;
};

// Forward declaration of NodeGraphScene (defined in NodeGraphScene.h)
class NodeGraphScene;

// ─── Node Graph View ───────────────────────────────────────────────────────

class NodeGraphView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit NodeGraphView(NodeGraphScene* scene, QWidget* parent = nullptr);
    ~NodeGraphView() override;

    // Navigation
    void fitInView(const QRectF& rect = QRectF(), bool animate = true);
    void centerOn(const QPointF& pos);
    void setZoom(float zoom, bool animate = true);
    float zoom() const { return m_zoom; }
    
    // Grid
    void setGridVisible(bool visible);
    bool isGridVisible() const { return m_gridVisible; }
    void setGridSize(float size);
    float gridSize() const { return m_gridSize; }
    void setSnapToGrid(bool enabled);
    bool snapToGrid() const { return m_snapToGrid; }
    
    // Mini-map
    void setMiniMapVisible(bool visible);
    bool isMiniMapVisible() const { return m_miniMapVisible; }
    
    // Interaction modes
    enum class InteractionMode {
        Select,
        Pan,
        Connect,
        BoxSelect,
        Navigate
    };
    void setInteractionMode(InteractionMode mode);
    InteractionMode interactionMode() const { return m_mode; }

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

signals:
    void zoomChanged(float zoom);
    void viewCentered(const QPointF& center);
    void nodeContextMenuRequested(const QUuid& nodeId, const QPoint& globalPos);
    void connectionContextMenuRequested(const QUuid& connectionId, const QPoint& globalPos);
    void backgroundContextMenuRequested(const QPoint& globalPos);
    void nodesDropped(const QVector<QString>& types, const QPointF& position);

private:
    NodeGraphScene* m_scene;
    class GraphMiniMap* m_miniMap = nullptr;
    float m_zoom = 1.0f;
    float m_gridSize = 20.0f;
    bool m_gridVisible = true;
    bool m_snapToGrid = true;
    bool m_miniMapVisible = false;
    InteractionMode m_mode = InteractionMode::Select;
    
    QPointF m_lastPanPos;
    bool m_panning = false;
    QUuid m_connectingFromNode;
    QUuid m_connectingFromPort;
    QPointF m_connectingToPos;
    bool m_connecting = false;
    QRectF m_boxSelectRect;
    bool m_boxSelecting = false;
    
    void handlePan(const QPointF& delta);
    void handleBoxSelect(const QRectF& rect);
    void handleConnection(const QPointF& pos);
    void completeConnection();
    void cancelConnection();
    void updateMiniMap();
};

// ─── Node Graphics Item ────────────────────────────────────────────────────

class GraphNodeItem : public QGraphicsItem
{
public:
    GraphNodeItem(const QUuid& nodeId, NodeGraphScene* scene);
    ~GraphNodeItem() override = default;

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    void updateGeometry();
    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }
    void setError(bool error, const QString& message = QString());
    void setMinimized(bool minimized);
    
    QUuid nodeId() const { return m_nodeId; }
    
    // Port hit testing
    struct PortHit {
        QUuid portId;
        bool isInput;
        QPointF scenePos;
    };
    PortHit hitTestPort(const QPointF& scenePos) const;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
    QUuid m_nodeId;
    NodeGraphScene* m_scene;
    GraphNode m_nodeData;
    bool m_selected = false;
    bool m_hovered = false;
    bool m_dragging = false;
    bool m_error = false;
    QPointF m_dragStartPos;
    QPointF m_hoverPortPos;
    QUuid m_hoverPortId;
    bool m_hoverPortIsInput = false;
    
    static constexpr float PORT_RADIUS = 8.0f;
    static constexpr float PORT_SPACING = 24.0f;
    static constexpr float HEADER_HEIGHT = 30.0f;
    static constexpr float MIN_WIDTH = 140.0f;
    static constexpr float MIN_HEIGHT = 60.0f;
    
    void drawPort(QPainter* painter, const QString& name, const QPointF& center, 
                  bool isInput, bool isConnected, bool isHovered, const QColor& typeColor);
    QColor portTypeColor(const QString& type) const;
};

// ─── Connection Graphics Item ──────────────────────────────────────────────

class GraphConnectionItem : public QGraphicsPathItem
{
public:
    GraphConnectionItem(const QUuid& connectionId, NodeGraphScene* scene);
    ~GraphConnectionItem() override = default;

    void updatePath();
    void setSelected(bool selected);
    void setHovered(bool hovered);
    
    QUuid connectionId() const { return m_connectionId; }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
    QUuid m_connectionId;
    NodeGraphScene* m_scene;
    GraphConnection m_connectionData;
    bool m_selected = false;
    bool m_hovered = false;
    
    void rebuildPath();
    QPointF portScenePos(const QUuid& nodeId, const QUuid& portId, bool isInput) const;
};

// ─── Mini Map Widget ───────────────────────────────────────────────────────

class GraphMiniMap : public QWidget
{
    Q_OBJECT

public:
    explicit GraphMiniMap(NodeGraphScene* scene, NodeGraphView* view, QWidget* parent = nullptr);
    ~GraphMiniMap() override = default;

    void setScene(NodeGraphScene* scene);
    void setView(NodeGraphView* view);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    NodeGraphScene* m_scene = nullptr;
    NodeGraphView* m_view = nullptr;
    QRectF m_viewRect;
    bool m_dragging = false;
    QPointF m_dragStart;
    
    QRectF sceneToWidget(const QRectF& rect) const;
    QRectF widgetToScene(const QRectF& rect) const;
};

// ─── Property Editor Widgets ──────────────────────────────────────────────

class PropertyEditor : public QWidget
{
    Q_OBJECT

public:
    explicit PropertyEditor(QWidget* parent = nullptr);
    ~PropertyEditor() override = default;

    void setObject(QObject* object);
    void setProperties(const QMap<QString, QVariant>& properties);
    void clear();

    void addProperty(const QString& name, const QVariant& value, 
                     const QString& description = QString(),
                     const QString& category = QString());
    void removeProperty(const QString& name);
    void setPropertyValue(const QString& name, const QVariant& value);
    QVariant propertyValue(const QString& name) const;

    void setPropertyReadOnly(const QString& name, bool readOnly);
    void setPropertyVisible(const QString& name, bool visible);
    void setPropertyRange(const QString& name, double min, double max, double step = 0.01);
    void setPropertyOptions(const QString& name, const QStringList& options);
    void setPropertyColor(const QString& name, bool alpha = true);

signals:
    void propertyChanged(const QString& name, const QVariant& value);

private:
    void rebuildUI();
    QWidget* createEditorForType(const QString& name, const QVariant& value);

    struct PropertyInfo {
        QString name;
        QVariant value;
        QString description;
        QString category;
        bool readOnly = false;
        bool visible = true;
        double min = -std::numeric_limits<double>::infinity();
        double max = std::numeric_limits<double>::infinity();
        double step = 0.01;
        QStringList options;
        bool isColor = false;
        bool alpha = true;
    };

    QMap<QString, PropertyInfo> m_properties;
    QMap<QString, QWidget*> m_editors;
    QVBoxLayout* m_layout = nullptr;
};

// ─── Virtualized List Model ──────────────────────────────────────────────

template<typename T>
class VirtualListModel : public QAbstractListModel
{

public:
    explicit VirtualListModel(QObject* parent = nullptr)
        : QAbstractListModel(parent), m_batchSize(100) {}

    void setData(const QVector<T>& data) {
        beginResetModel();
        m_data = data;
        m_visibleCount = qMin(m_batchSize, m_data.size());
        endResetModel();
    }

    void appendData(const QVector<T>& data) {
        int start = m_data.size();
        int count = data.size();
        beginInsertRows(QModelIndex(), start, start + count - 1);
        m_data.append(data);
        endInsertRows();
    }

    void setBatchSize(int size) { m_batchSize = size; }
    int batchSize() const { return m_batchSize; }

    // Load more items (for infinite scroll)
    void loadMore(int count) {
        int newCount = qMin(m_visibleCount + count, m_data.size());
        if (newCount > m_visibleCount) {
            beginInsertRows(QModelIndex(), m_visibleCount, newCount - 1);
            m_visibleCount = newCount;
            endInsertRows();
        }
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        return parent.isValid() ? 0 : m_visibleCount;
    }

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || index.row() >= m_visibleCount) return {};
        return itemData(m_data[index.row()], role);
    }

    virtual QVariant itemData(const T& item, int role) const = 0;

    const T& at(int index) const { return m_data[index]; }
    int totalCount() const { return m_data.size(); }
    int visibleCount() const { return m_visibleCount; }

protected:
    QVector<T> m_data;
    int m_visibleCount = 0;
    int m_batchSize;
};

// ─── Virtualized Tree Model ──────────────────────────────────────────────

class VirtualTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    struct TreeItem {
        QString id;
        QString name;
        QVariant data;
        QVector<TreeItem> children;
        bool expanded = true;
        bool selectable = true;
        QIcon icon;
        QString toolTip;
        QColor foreground;
        QColor background;
    };

    explicit VirtualTreeModel(QObject* parent = nullptr);
    ~VirtualTreeModel() override = default;

    void setRootItems(const QVector<TreeItem>& items);
    void appendItems(const QVector<TreeItem>& items, const QModelIndex& parent = QModelIndex());
    void removeItems(const QModelIndex& parent, int row, int count);
    
    void setItemExpanded(const QModelIndex& index, bool expanded);
    bool isItemExpanded(const QModelIndex& index) const;
    
    // Virtual loading
    void setLazyLoadCallback(std::function<void(const TreeItem&)> callback);
    void loadChildren(const QModelIndex& parent);

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QVector<int> expandedRows() const;

signals:
    void itemExpanded(const QModelIndex& index);
    void itemCollapsed(const QModelIndex& index);
    void lazyLoadRequested(const QModelIndex& index);

private:
    struct Node {
        TreeItem item;
        QVector<Node*> children;
        Node* parent = nullptr;
        bool loaded = false;
        bool loading = false;
    };

    Node* m_root = nullptr;
    QVector<Node*> m_rootChildren;
    std::function<void(const TreeItem&)> m_lazyLoadCallback;
    QSet<QString> m_expandedIds;
    
    Node* findNode(const QModelIndex& index) const;
    Node* findNodeById(Node* node, const QString& id) const;
    void deleteNode(Node* node);
    void updateExpandedState(Node* node, bool expanded);
};

// ─── Color Editor Widget ──────────────────────────────────────────────────

class ColorEditorWidget : public QWidget
{
    Q_OBJECT

public:
    enum Mode { RGB, HSV, HEX, PRESET };
    
    explicit ColorEditorWidget(QWidget* parent = nullptr);
    ~ColorEditorWidget() override = default;

    void setColor(const QColor& color);
    QColor color() const { return m_color; }
    
    void setMode(Mode mode);
    Mode mode() const { return m_mode; }
    
    void setAlphaVisible(bool visible);
    void setPresets(const QVector<QColor>& presets);
    void addPreset(const QColor& color);

signals:
    void colorChanged(const QColor& color);
    void colorChanging(const QColor& color);  // During drag

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QColor m_color = Qt::white;
    Mode m_mode = Mode::RGB;
    bool m_alphaVisible = true;
    QVector<QColor> m_presets;
    bool m_dragging = false;
    int m_dragHandle = 0;  // 0=hue, 1=saturation, 2=value, 3=alpha
    
    void drawColorWheel(QPainter* painter, const QRect& rect);
    void drawAlphaBar(QPainter* painter, const QRect& rect);
    void drawPresets(QPainter* painter, const QRect& rect);
    QColor colorFromWheelPos(const QPointF& pos) const;
};

// ─── Curve Editor Widget ──────────────────────────────────────────────────

class CurveEditorWidget : public QWidget
{
    Q_OBJECT

public:
    struct CurvePoint {
        float x, y;
        float inTangentX = 0, inTangentY = 0;
        float outTangentX = 0, outTangentY = 0;
        QString interpolation = "cubic";  // linear, cubic, constant, bezier
        bool autoTangents = true;
        bool selected = false;
    };

    explicit CurveEditorWidget(QWidget* parent = nullptr);
    ~CurveEditorWidget() override = default;

    void setCurve(const QVector<CurvePoint>& points);
    QVector<CurvePoint> curve() const { return m_points; }
    
    void setRange(float xMin, float xMax, float yMin, float yMax);
    void setGridVisible(bool visible) { m_gridVisible = visible; update(); }
    void setGridColor(const QColor& color) { m_gridColor = color; update(); }

    float evaluate(float x) const;

signals:
    void curveChanged(const QVector<CurvePoint>& points);
    void pointSelected(int index);
    void pointMoved(int index, const CurvePoint& point);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QVector<CurvePoint> m_points;
    float m_xMin = 0, m_xMax = 1, m_yMin = 0, m_yMax = 1;
    bool m_gridVisible = true;
    QColor m_gridColor = QColor(60, 60, 70);
    QColor m_curveColor = QColor(100, 180, 255);
    QColor m_selectedPointColor = QColor(255, 200, 50);
    
    int m_selectedPoint = -1;
    int m_hoveredPoint = -1;
    bool m_draggingPoint = false;
    bool m_draggingTangent = false;
    int m_dragTangentType = 0;  // 0=in, 1=out
    QPointF m_dragStartPos;
    CurvePoint m_dragStartPoint;
    float m_zoomX = 1.0f, m_zoomY = 1.0f;
    float m_panX = 0.0f, m_panY = 0.0f;
    
    QPointF dataToWidget(float x, float y) const;
    QPointF widgetToData(const QPointF& pos) const;
    int hitTestPoint(const QPointF& pos) const;
    int hitTestTangent(const QPointF& pos) const;
    void addPoint(float x, float y);
    void removePoint(int index);
    void updateTangents(int index);
    float cubicInterpolate(float t, const CurvePoint& p0, const CurvePoint& p1) const;
};

// ─── Gradient Editor Widget ────────────────────────────────────────────────

class GradientEditorWidget : public QWidget
{
    Q_OBJECT

public:
    struct GradientStop {
        float position;  // 0.0 - 1.0
        QColor color;
        float alpha = 1.0f;
    };

    explicit GradientEditorWidget(QWidget* parent = nullptr);
    ~GradientEditorWidget() override = default;

    void setGradient(const QVector<GradientStop>& stops);
    QVector<GradientStop> gradient() const { return m_stops; }
    QGradientStops toQGradientStops() const;

    void setOrientation(Qt::Orientation orientation) { m_orientation = orientation; update(); }

signals:
    void gradientChanged(const QVector<GradientStop>& stops);
    void stopSelected(int index);
    void stopMoved(int index, float position);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QVector<GradientStop> m_stops;
    Qt::Orientation m_orientation = Qt::Horizontal;
    int m_selectedStop = -1;
    int m_hoveredStop = -1;
    bool m_dragging = false;
    QPoint m_dragStartPos;
    
    int hitTestStop(const QPoint& pos) const;
    void addStop(float position);
    void removeStop(int index);
    void updateStopPosition(int index, float position);
};

// ─── Node Graph Widget (Complete Integration) ──────────────────────────────

class NodeGraphWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NodeGraphWidget(QWidget* parent = nullptr);
    ~NodeGraphWidget() override = default;

    NodeGraphScene* scene() const { return m_scene; }
    NodeGraphView* view() const { return m_view; }
    GraphMiniMap* miniMap() const { return m_miniMap; }

    // Graph operations
    QUuid addNode(const QString& typeName, const QPointF& position = QPointF());
    void removeSelectedNodes();
    void duplicateSelectedNodes();
    
    // Serialization
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& json);
    bool loadFromFile(const QString& filePath);
    bool saveToFile(const QString& filePath);

    // Search
    void findAndSelect(const QString& query);
    void centerOnNode(const QUuid& nodeId);

signals:
    void graphChanged();
    void nodeSelected(const QUuid& nodeId);
    void nodeDeselected(const QUuid& nodeId);
    void statusMessage(const QString& message);

private:
    void setupUI();
    void setupConnections();
    void setupContextMenus();

    NodeGraphScene* m_scene = nullptr;
    NodeGraphView* m_view = nullptr;
    GraphMiniMap* m_miniMap = nullptr;
    QSplitter* m_splitter = nullptr;
    QToolBar* m_toolbar = nullptr;
    QMenu* m_nodeContextMenu = nullptr;
    QMenu* m_connectionContextMenu = nullptr;
    QMenu* m_backgroundContextMenu = nullptr;
};

} // namespace ui
} // namespace ks