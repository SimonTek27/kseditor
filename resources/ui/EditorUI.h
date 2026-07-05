#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QWidget>
#include <QVector3D>

namespace ks {

namespace ui {

class EditorWindow : public QObject
{
    Q_OBJECT
public:
    explicit EditorWindow(QObject* parent = nullptr) : QObject(parent) {}
    ~EditorWindow() {}

    void setTitle(const QString& title) { m_title = title; }
    QString title() const { return m_title; }

    void setGeometry(int x, int y, int width, int height);
    QRect geometry() const { return QRect(m_x, m_y, m_width, m_height); }

    void show();
    void hide();
    void close();
    bool isVisible() const { return m_visible; }

    void setFullscreen(bool fs) { m_fullscreen = fs; }
    bool isFullscreen() const { return m_fullscreen; }

signals:
    void windowShown();
    void windowHidden();
    void windowClosed();

private:
    QString m_title = "ksEditor";
    int m_x = 100, m_y = 100, m_width = 1280, m_height = 720;
    bool m_visible = false;
    bool m_fullscreen = false;
};

class DockPanel : public QObject
{
    Q_OBJECT
public:
    explicit DockPanel(QObject* parent = nullptr) : QObject(parent) {}
    ~DockPanel() {}

    enum DockArea { Left, Right, Top, Bottom, Floating };

    void setArea(DockArea area) { m_area = area; }
    DockArea area() const { return m_area; }

    void setWidget(QWidget* widget) { m_widget = widget; }
    QWidget* widget() const { return m_widget; }

    void setTitle(const QString& title) { m_title = title; }
    QString title() const { return m_title; }

    void setFloating(bool floating) { m_floating = floating; }
    bool isFloating() const { return m_floating; }

    void setSize(int width, int height) { m_width = width; m_height = height; }
    QSize size() const { return QSize(m_width, m_height); }

signals:
    void visibilityChanged(bool visible);

private:
    DockArea m_area = Left;
    QWidget* m_widget = nullptr;
    QString m_title;
    bool m_floating = false;
    int m_width = 250, m_height = 400;
};

class MenuBar : public QObject
{
    Q_OBJECT
public:
    explicit MenuBar(QObject* parent = nullptr) : QObject(parent) {}
    ~MenuBar() {}

    void addMenu(const QString& name);
    void addAction(const QString& menu, const QString& action, const QString& shortcut = QString());
    void addSeparator(const QString& menu);

    void addSubmenu(const QString& menu, const QString& submenu);

signals:
    void actionTriggered(const QString& action);

private:
    QMap<QString, QStringList> m_menus;
    QMap<QString, QString> m_shortcuts;
};

class Toolbar : public QObject
{
    Q_OBJECT
public:
    explicit Toolbar(QObject* parent = nullptr) : QObject(parent) {}
    ~Toolbar() {}

    struct Button {
        QString id;
        QString icon;
        QString tooltip;
    };

    void setName(const QString& name) { m_name = name; }
    QString name() const { return m_name; }

    void addButton(const QString& id, const QString& icon, const QString& tooltip);
    void addSeparator();
    void addDropdown(const QString& id, const QStringList& options);

    void setOrientation(Qt::Orientation orientation) { m_orientation = orientation; }
    Qt::Orientation orientation() const { return m_orientation; }

    const QVector<Button>& buttons() const { return m_buttons; }
    const QMap<QString, QStringList>& dropdowns() const { return m_dropdowns; }

signals:
    void buttonClicked(const QString& id);
    void dropdownChanged(const QString& id, int index);

private:
    QString m_name;
    Qt::Orientation m_orientation = Qt::Horizontal;
    QVector<Button> m_buttons;
    QMap<QString, QStringList> m_dropdowns;
};

class PropertyPanel : public QObject
{
    Q_OBJECT
public:
    explicit PropertyPanel(QObject* parent = nullptr) : QObject(parent) {}
    ~PropertyPanel() {}

    void addProperty(const QString& name, const QString& type, const QVariant& defaultValue);
    void addSlider(const QString& name, float min, float max, float value);
    void addColorPicker(const QString& name, const QColor& color);
    void addVectorInput(const QString& name, const QVector3D& value);

    void setValue(const QString& property, const QVariant& value);
    QVariant getValue(const QString& property) const;

    void clear();
    void setObject(void* object);

signals:
    void propertyChanged(const QString& name, const QVariant& value);

private:
    struct Property {
        QString name;
        QString type;
        QVariant value;
    };
    QMap<QString, Property> m_properties;
    void* m_object = nullptr;
};

class Timeline : public QObject
{
    Q_OBJECT
public:
    explicit Timeline(QObject* parent = nullptr) : QObject(parent) {}
    ~Timeline() {}

    void setDuration(float seconds) { m_duration = seconds; }
    float duration() const { return m_duration; }

    void setCurrentTime(float time) { m_currentTime = time; }
    float currentTime() const { return m_currentTime; }

    void play();
    void pause();
    void stop();
    void setPlayhead(float time);

    enum State { Stopped, Playing, Paused };
    void setState(State state) { m_state = state; }
    State state() const { return m_state; }

    void addTrack(const QString& name, const QString& color);
    void addKeyframe(const QString& track, float time, const QVariant& value);

    void setLoop(bool loop) { m_loop = loop; }
    bool isLooping() const { return m_loop; }

    void setFps(int fps) { m_fps = fps; }
    int fps() const { return m_fps; }

signals:
    void timeChanged(float time);
    void stateChanged(State state);
    void keyframeAdded(const QString& track, float time);

private:
    float m_duration = 60.0f;
    float m_currentTime = 0.0f;
    State m_state = Stopped;
    bool m_loop = false;
    int m_fps = 30;
};

class Viewport : public QObject
{
    Q_OBJECT
public:
    explicit Viewport(QObject* parent = nullptr) : QObject(parent) {}
    ~Viewport() {}

    enum ViewMode { Perspective, Top, Bottom, Left, Right, Front, Back, Orthographic };

    void setMode(ViewMode mode) { m_mode = mode; }
    ViewMode mode() const { return m_mode; }

    void setCamera(void* camera) { m_camera = camera; }
    void* camera() const { return m_camera; }

    enum ShadingMode { Wireframe, Solid, Shaded, Textured };
    void setShading(ShadingMode mode) { m_shading = mode; }
    ShadingMode shading() const { return m_shading; }

    void setGrid(bool show) { m_showGrid = show; }
    bool showGrid() const { return m_showGrid; }

    void setAxes(bool show) { m_showAxes = show; }
    bool showAxes() const { return m_showAxes; }

    void setOverlay(bool show) { m_showOverlay = show; }
    bool showOverlay() const { return m_showOverlay; }

    void focusOn(const QVector3D& target, float distance);

signals:
    void viewModeChanged(ViewMode mode);

private:
    ViewMode m_mode = Perspective;
    void* m_camera = nullptr;
    ShadingMode m_shading = Shaded;
    bool m_showGrid = true;
    bool m_showAxes = true;
    bool m_showOverlay = true;
    QVector3D m_focusTarget;
    float m_focusDistance = 10.0f;
};

class NodeGraph : public QObject
{
    Q_OBJECT
public:
    explicit NodeGraph(QObject* parent = nullptr) : QObject(parent) {}
    ~NodeGraph() {}

    struct Node {
        QString id;
        QString type;
        QString title;
        QPointF position;
        QMap<QString, QVariant> inputs;
        QMap<QString, QVariant> outputs;
        bool selected = false;
    };

    struct Connection {
        QString fromNode;
        QString fromSocket;
        QString toNode;
        QString toSocket;
    };

    void addNode(const QString& type, const QString& title, const QPointF& pos);
    void removeNode(const QString& nodeId);

    void connect(const QString& fromNode, const QString& fromSocket,
                 const QString& toNode, const QString& toSocket);
    void disconnect(const QString& fromNode, const QString& fromSocket,
                    const QString& toNode, const QString& toSocket);

    QVector<Node> nodes() const { return m_nodes.values(); }
    QVector<Connection> connections() const { return m_connections; }

    void selectNode(const QString& nodeId, bool addToSelection);
    void deleteSelected();

signals:
    void nodeAdded(const QString& nodeId);
    void nodeRemoved(const QString& nodeId);
    void connectionAdded();
    void selectionChanged();

private:
    QMap<QString, Node> m_nodes;
    QVector<Connection> m_connections;
};

} // namespace ui
} // namespace ks