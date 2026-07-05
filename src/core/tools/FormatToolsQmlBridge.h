#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QMap>
#include "../editor/EditorModule.h"

namespace ks {

struct AiLineDataPoint {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float distance = 0.0f;
    int id = 0;
};

struct AiLineDetailData {
    float unknown0 = 0.0f;
    float speed = 0.0f;
    float gas = 0.0f;
    float brake = 0.0f;
    float obsoleteLatG = 0.0f;
    float radius = 0.0f;
    float wallLeft = 0.0f;
    float wallRight = 0.0f;
    float camber = 0.0f;
    float direction = 0.0f;
    float normalX = 0.0f;
    float normalY = 0.0f;
    float normalZ = 0.0f;
    float length = 0.0f;
    float forwardVectorX = 0.0f;
    float forwardVectorY = 0.0f;
    float forwardVectorZ = 0.0f;
    float tag = 0.0f;
};

struct AiLineFileData {
    int header = 0;
    int pointCount = 0;
    int unknown1 = 0;
    int unknown2 = 0;
    QVector<AiLineDataPoint> idealLine;
    QVector<AiLineDetailData> detailData;
    QByteArray restData;
};

struct CameraData {
    QString name;
    float positionX = 0.0f;
    float positionY = 0.0f;
    float positionZ = 0.0f;
    float targetX = 0.0f;
    float targetY = 0.0f;
    float targetZ = 0.0f;
    float fov = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float tilt = 0.0f;
};

struct OverlayData {
    QString name;
    int type = 0;
    float posX = 0.0f;
    float posY = 0.0f;
    float sizeX = 1.0f;
    float sizeY = 1.0f;
    bool visible = true;
    QString texture;
};

struct AiLineVertex {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct AiLineData {
    QString name;
    QVector<AiLineVertex> vertices;
    QVector<QPair<int,int>> edges;
};

class FormatToolsQmlBridge : public QObject {
    Q_OBJECT
public:
    static FormatToolsQmlBridge* instance();

    // AI Line operations
    Q_INVOKABLE QVariantMap importAiLine(const QString& filePath, float scaling = 1.0f, bool importExtraData = true);
    Q_INVOKABLE bool exportAiLine(const QString& filePath, const QVariantList& vertices, float scaling = 1.0f,
                                   int shiftCount = 0, bool reverse = false, bool fixedBorders = false,
                                   float fixedLeft = 2.0f, float fixedRight = 2.0f);
    Q_INVOKABLE QVariantMap importAiLineBorders(const QString& filePath, float scaling = 1.0f);

    // CSV border operations
    Q_INVOKABLE QVariantList importCsv(const QString& filePath, float scaling = 0.01f);
    Q_INVOKABLE bool exportCsv(const QString& filePath, const QVariantList& vertices, float scaling = 1.0f,
                                int shiftCount = 0, bool reverse = false, bool skipPoT = false);

    // Camera.ini operations
    Q_INVOKABLE QVariantList importCameraIni(const QString& filePath);
    Q_INVOKABLE bool exportCameraIni(const QString& filePath, const QVariantList& cameras);

    // Overlay.ini operations
    Q_INVOKABLE QVariantList importOverlayIni(const QString& filePath);
    Q_INVOKABLE bool exportOverlayIni(const QString& filePath, const QVariantList& overlays);

    // Material fix tools
    Q_INVOKABLE QVariantMap fixAlphaBlendToOpaque(const QVariantList& materials);
    Q_INVOKABLE QVariantMap resetSpecularMetallic(const QVariantList& materials);

    // Mesh cleanup tools
    Q_INVOKABLE QVariantMap mergeByDistance(const QVariantList& vertices, float threshold = 0.001f);

    // Name cleanup for export
    Q_INVOKABLE QStringList cleanNames(const QStringList& names);

    // AC object creation helpers
    Q_INVOKABLE QVariantMap createAcObject(const QString& objectType, const QVariantMap& params = QVariantMap());
    Q_INVOKABLE QVariantMap createStartPosition(float x = 0.0f, float z = 0.0f, float rotation = 0.0f, bool isRightSide = false);
    Q_INVOKABLE QVariantMap createTimingPosition(float x = 0.0f, float z = 0.0f, bool isRightSide = false);

    // Mass/batch operations
    Q_INVOKABLE QVariantMap batchImportAiLines(const QStringList& filePaths, float scaling = 1.0f);
    Q_INVOKABLE QVariantMap batchExportAiLines(const QString& directory, const QVariantList& lineData, float scaling = 1.0f);

    // 3D Replay visualization
    Q_INVOKABLE QVariantList importReplayPath(const QString& replayPath, int carId = 0);
    Q_INVOKABLE QVariantMap getReplayInfo(const QString& replayPath);

    // Naming convention (modder_year_manufacturer_carname)
    Q_INVOKABLE QString generateCarName(const QString& modder, int year, const QString& manufacturer, const QString& carName);
    Q_INVOKABLE QString generatePrefix(const QString& manufacturer);
    Q_INVOKABLE QVariantMap getComponentTree();
    Q_INVOKABLE QVariantList generateComponentNames(const QString& carName, const QString& manufacturer);
    Q_INVOKABLE QVariantMap validateCarNaming(const QString& carName, const QVariantMap& componentTree);
    Q_INVOKABLE QVariantMap autoFixNaming(const QString& carName, const QVariantMap& componentTree);
    Q_INVOKABLE QVariantMap parseNodeHierarchy(const QString& nodeList);
    Q_INVOKABLE QString buildNodeHierarchy(const QVariantMap& tree, int indent = 0);

    // JSON component tree
    Q_INVOKABLE bool loadComponentTree(const QString& jsonPath);
    Q_INVOKABLE bool saveComponentTree(const QString& jsonPath);
    Q_INVOKABLE QStringList getManufacturers();
    Q_INVOKABLE QVariantMap getCategories();

    // Hierarchy import/export
    Q_INVOKABLE QVariantMap importHierarchyIni(const QString& filePath);
    Q_INVOKABLE bool exportHierarchyIni(const QString& filePath, const QVariantMap& tree);
    Q_INVOKABLE QVariantMap importHierarchyJson(const QString& filePath);
    Q_INVOKABLE bool exportHierarchyJson(const QString& filePath, const QVariantMap& tree);

    // Tree manipulation
    Q_INVOKABLE QVariantMap addNode(const QVariantMap& tree, const QString& path, const QString& nodeName);
    Q_INVOKABLE QVariantMap removeNode(const QVariantMap& tree, const QString& path);
    Q_INVOKABLE QVariantMap renameNode(const QVariantMap& tree, const QString& path, const QString& newName);
    Q_INVOKABLE int countNodes(const QVariantMap& tree);
    Q_INVOKABLE QStringList flattenTree(const QVariantMap& tree, const QString& prefix = "");

    // Project scaffolding
    Q_INVOKABLE bool createCarProject(const QString& basePath, const QString& carName, const QVariantMap& metadata);
    Q_INVOKABLE bool createTrackProject(const QString& basePath, const QString& trackName, const QVariantMap& metadata);
    Q_INVOKABLE QStringList getRequiredFiles(const QString& projectType);

signals:
    void statusMessage(const QString& msg);
    void errorMessage(const QString& msg);
    void importComplete(const QString& filePath, int pointCount);
    void exportComplete(const QString& filePath);
    void batchProgress(int current, int total);

private:
    FormatToolsQmlBridge(QObject* parent = nullptr);
    static FormatToolsQmlBridge* s_instance;

    bool parseAiBinaryLine(QDataStream& stream, AiLineFileData& data);
    bool writeAiBinaryLine(QDataStream& stream, const AiLineFileData& data);
    float calculateDistance(const AiLineDataPoint& a, const AiLineDataPoint& b);

    QVariantMap m_componentTree;
    QVariantMap m_manufacturers;
    QVariantMap m_categories;
    QStringList m_requiredNodes;

    QVariantList getComponentList();
};

class FormatToolsModule : public EditorModule {
    Q_OBJECT
public:
    explicit FormatToolsModule(QWidget* parent = nullptr);
    ~FormatToolsModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Format Tools"; }
    QString moduleId() const override { return "formatTools"; }
    int getModulePriority() const override { return 25; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;
};

}
