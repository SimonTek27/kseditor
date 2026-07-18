#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector3D>
#include <QVector2D>
#include <QColor>
#include <functional>

namespace ks {
namespace material {

// ─── Shader Graph Data Structures ──────────────────────────────────────────

enum class ShaderNodeType {
    // Inputs
    InputPosition,
    InputNormal,
    InputUV,
    InputColor,
    InputViewDir,
    InputWorldPos,
    InputTangent,
    InputBitangent,
    InputVertexColor,
    InputInstanceID,
    
    // Constants
    ConstantFloat,
    ConstantVector2,
    ConstantVector3,
    ConstantVector4,
    ConstantColor,
    ConstantInt,
    ConstantBool,
    
    // Math
    MathAdd,
    MathSubtract,
    MathMultiply,
    MathDivide,
    MathPower,
    MathSqrt,
    MathAbs,
    MathMin,
    MathMax,
    MathClamp,
    MathSaturate,
    MathLerp,
    MathStep,
    MathSmoothstep,
    MathSign,
    MathFloor,
    MathCeil,
    MathFract,
    MathSin,
    MathCos,
    MathTan,
    MathAsin,
    MathAcos,
    MathAtan,
    MathAtan2,
    MathExp,
    MathLog,
    MathDot,
    MathCross,
    MathLength,
    MathNormalize,
    MathDistance,
    MathReflect,
    MathRefract,
    MathFaceForward,
    MathMatrixMultiply,
    MathTransformVector,
    MathTransformPoint,
    
    // Vector ops
    VectorCombine2,
    VectorCombine3,
    VectorCombine4,
    VectorSplit2,
    VectorSplit3,
    VectorSplit4,
    VectorSwizzle,
    
    // Texture
    TextureSample,
    TextureSampleGrad,
    TextureSampleLevel,
    TextureSize,
    TextureCubeSample,
    TextureArraySample,
    NormalMapSample,
    
    // Color
    ColorGammaToLinear,
    ColorLinearToGamma,
    ColorHSVToRGB,
    ColorRGBToHSV,
    ColorMix,
    ColorBrightnessContrast,
    ColorHueSaturation,
    ColorInvert,
    ColorDesaturate,
    ColorChannelPack,
    ColorChannelUnpack,
    
    // PBR
    PBRMetallicRoughness,
    PBRSpecularGlossiness,
    PBRDisneyPrincipled,
    PBRClearcoat,
    PBRSheen,
    PBRAnisotropy,
    PBRSubsurface,
    PBRTransmission,
    PBRAmbientOcclusion,
    
    // Lighting
    LightDirectional,
    LightPoint,
    LightSpot,
    LightArea,
    LightIBL,
    LightImageBased,
    ShadowPCF,
    ShadowVSM,
    ShadowESM,
    
    // Utility
    UtilityTime,
    UtilityScreenPosition,
    UtilityCameraParams,
    UtilityRandom,
    UtilityNoise,
    UtilityVoronoi,
    UtilityFBM,
    UtilityRound,
    UtilityModulo,
    UtilityClamp,
    
    // Flow control
    FlowBranch,
    FlowSwitch,
    FlowLoop,
    
    // Custom
    CustomFunction,
    CustomCode,
    
    // Output
    OutputMaterial,
    OutputSurface,
    OutputDisplacement,
    OutputNormal,
    OutputEmissive,
    OutputAlpha,
    OutputCustom
};

enum class ShaderPortType {
    Float,
    Float2,
    Float3,
    Float4,
    Color,
    Int,
    Bool,
    Texture2D,
    Texture3D,
    TextureCube,
    Sampler,
    Matrix2,
    Matrix3,
    Matrix4,
    Generic
};

struct ShaderPort {
    QUuid id;
    QString name;
    ShaderPortType type = ShaderPortType::Float;
    bool isInput = true;
    bool isConnected = false;
    QVariant defaultValue;
    QString description;
    int sortOrder = 0;
};

struct ShaderNode {
    QUuid id;
    ShaderNodeType type;
    QString title;
    QString subtitle;
    QVector<ShaderPort> inputs;
    QVector<ShaderPort> outputs;
    QMap<QString, QVariant> properties;
    QPointF position;
    QSize size;
    bool selected = false;
    bool minimized = false;
    bool error = false;
    QString errorMessage;
    QString comment;
    QColor headerColor;
    bool isGroup = false;
    QUuid groupId;
    
    ShaderNode() = default;
    ShaderNode(ShaderNodeType t, const QPointF& pos = QPointF());
};

struct ShaderConnection {
    QUuid id;
    QUuid fromNodeId;
    QUuid fromPortId;
    QUuid toNodeId;
    QUuid toPortId;
    QColor color;
    bool isValid = true;
};

struct ShaderGraph {
    QUuid id;
    QString name;
    QString version = "1.0";
    QString description;
    QString author;
    QDateTime created;
    QDateTime modified;
    QVector<ShaderNode> nodes;
    QVector<ShaderConnection> connections;
    QMap<QString, QVariant> metadata;
    QRectF viewRect;
    float zoom = 1.0f;
    
    QJsonObject toJson() const;
    static ShaderGraph fromJson(const QJsonObject& obj);
};

struct ShaderPermutation {
    QString name;
    QMap<QString, bool> defines;
    QMap<QString, QVariant> properties;
    QString vsSource;
    QString fsSource;
    QString gsSource;
    QString csSource;
    bool isValid = false;
    QString error;
    QDateTime compileTime;
    uint32_t hash = 0;
};

struct ShaderExportOptions {
    enum class Target { HLSL, GLSL, SPIRV, Metal, MSL };
    Target target = Target::GLSL;
    int version = 450;  // GLSL version
    bool includeLineDirectives = false;
    bool optimize = true;
    bool debugInfo = false;
    QString entryPoint = "main";
    QString outputDirectory;
    bool separateFiles = true;
    QMap<QString, QString> customDefines;
};

// ─── Shader Graph Manager ───────────────────────────────────────────────────

class ShaderGraphManager : public QObject {
    Q_OBJECT

public:
    static ShaderGraphManager* instance();
    
    explicit ShaderGraphManager(QObject* parent = nullptr);
    ~ShaderGraphManager() override;

    // Graph management
    QUuid createGraph(const QString& name = "New Graph");
    bool deleteGraph(const QUuid& graphId);
    ShaderGraph* getGraph(const QUuid& graphId);
    const ShaderGraph* getGraph(const QUuid& graphId) const;
    QVector<QUuid> getAllGraphIds() const;
    QVector<ShaderGraph*> getAllGraphs() const;
    
    // Node operations
    ShaderNode* addNode(const QUuid& graphId, ShaderNodeType type, const QPointF& position = QPointF());
    bool removeNode(const QUuid& graphId, const QUuid& nodeId);
    ShaderNode* getNode(const QUuid& graphId, const QUuid& nodeId);
    const ShaderNode* getNode(const QUuid& graphId, const QUuid& nodeId) const;
    void moveNode(const QUuid& graphId, const QUuid& nodeId, const QPointF& position);
    void setNodeProperty(const QUuid& graphId, const QUuid& nodeId, const QString& property, const QVariant& value);
    
    // Connection operations
    bool connectPorts(const QUuid& graphId, const QUuid& fromNodeId, const QUuid& fromPortId,
                      const QUuid& toNodeId, const QUuid& toPortId);
    bool disconnectPorts(const QUuid& graphId, const QUuid& connectionId);
    ShaderConnection* getConnection(const QUuid& graphId, const QUuid& connectionId);
    QVector<ShaderConnection*> getNodeConnections(const QUuid& graphId, const QUuid& nodeId, bool inputs);
    
    // Validation
    struct ValidationResult {
        bool valid = true;
        QVector<QUuid> errorNodes;
        QVector<QUuid> warningNodes;
        QMap<QUuid, QString> errors;
        QMap<QUuid, QString> warnings;
    };
    ValidationResult validateGraph(const QUuid& graphId) const;
    
    // Compilation
    QString generateGLSL(const QUuid& graphId, const QString& entryPoint = "main") const;
    QString generateHLSL(const QUuid& graphId, const QString& entryPoint = "main") const;
    QString generateSPIRV(const QUuid& graphId) const;
    QString generateMetal(const QUuid& graphId) const;
    
    // Permutations
    ShaderPermutation compilePermutation(const QUuid& graphId, const QMap<QString, bool>& defines);
    QVector<ShaderPermutation> compileAllPermutations(const QUuid& graphId, 
                                                       const QVector<QMap<QString, bool>>& permutationDefines);
    
    // Export
    bool exportToFiles(const QUuid& graphId, const ShaderExportOptions& options);
    QString exportToString(const QUuid& graphId, ShaderExportOptions::Target target) const;
    
    // Templates
    ShaderGraph createTemplateGraph(const QString& templateName);
    QVector<QString> getAvailableTemplates() const;
    
    // Serialization
    bool saveGraph(const QUuid& graphId, const QString& filePath);
    QUuid loadGraph(const QString& filePath);
    
    // Node type registry
    struct NodeTypeInfo {
        ShaderNodeType type;
        QString name;
        QString category;
        QString description;
        QVector<ShaderPort> defaultInputs;
        QVector<ShaderPort> defaultOutputs;
        QMap<QString, QVariant> defaultProperties;
        QColor headerColor;
        bool isInput = false;
        bool isOutput = false;
        bool isMath = false;
        bool isTexture = false;
        bool isPBR = false;
        bool isLighting = false;
    };
    static const QMap<ShaderNodeType, NodeTypeInfo>& getNodeTypeInfo();
    static const NodeTypeInfo& getNodeTypeInfo(ShaderNodeType type);

signals:
    void graphCreated(const QUuid& graphId);
    void graphDeleted(const QUuid& graphId);
    void graphChanged(const QUuid& graphId);
    void nodeAdded(const QUuid& graphId, const QUuid& nodeId);
    void nodeRemoved(const QUuid& graphId, const QUuid& nodeId);
    void nodeMoved(const QUuid& graphId, const QUuid& nodeId, const QPointF& position);
    void nodePropertyChanged(const QUuid& graphId, const QUuid& nodeId, const QString& property, const QVariant& value);
    void connectionAdded(const QUuid& graphId, const QUuid& connectionId);
    void connectionRemoved(const QUuid& graphId, const QUuid& connectionId);
    void validationCompleted(const QUuid& graphId, const ValidationResult& result);
    void compilationCompleted(const QUuid& graphId, bool success, const QString& output);
    void permutationCompiled(const ShaderPermutation& permutation);

private:
    QMap<QUuid, ShaderGraph> m_graphs;
    QMap<QUuid, QUuid> m_nodeToGraph;
    
    void registerBuiltinNodeTypes();
    static QMap<ShaderNodeType, NodeTypeInfo> s_nodeTypeInfo;
    static bool s_nodeTypesRegistered;
};

// ─── Shader Permutation Manager ────────────────────────────────────────────

class ShaderPermutationManager : public QObject {
    Q_OBJECT

public:
    static ShaderPermutationManager* instance();
    
    explicit ShaderPermutationManager(QObject* parent = nullptr);
    ~ShaderPermutationManager() override;

    // Permutation definition
    struct PermutationAxis {
        QString name;
        QString defineName;
        QStringList options;  // e.g., ["OFF", "ON", "HIGH"]
        QString defaultOption;
        QString tooltip;
    };
    
    void registerPermutationAxis(const PermutationAxis& axis);
    void unregisterPermutationAxis(const QString& name);
    QVector<PermutationAxis> getPermutationAxes() const;
    
    // Generate all valid permutations
    QVector<QMap<QString, bool>> generatePermutations() const;
    QVector<QMap<QString, bool>> generatePermutations(const QVector<QString>& enabledAxes) const;
    
    // Filter permutations
    using FilterFunction = std::function<bool(const QMap<QString, bool>&)>;
    void setFilter(FilterFunction filter);
    
    // Compile permutations
    QVector<ShaderPermutation> compilePermutations(const QUuid& graphId,
                                                    const QVector<QMap<QString, bool>>& permutations);
    
    // Cache
    void cachePermutation(const ShaderPermutation& permutation);
    ShaderPermutation getCachedPermutation(uint32_t hash) const;
    void clearCache();
    void saveCache(const QString& filePath);
    void loadCache(const QString& filePath);
    
    // Statistics
    struct Stats {
        int totalPermutations = 0;
        int compiledPermutations = 0;
        int failedPermutations = 0;
        qint64 totalCompileTime = 0;
        int cacheHits = 0;
        int cacheMisses = 0;
    };
    Stats getStats() const;

signals:
    void permutationStarted(const QString& name);
    void permutationCompleted(const ShaderPermutation& permutation);
    void permutationFailed(const QString& name, const QString& error);
    void allPermutationsCompleted(int total, int succeeded, int failed);
    void progressChanged(int current, int total);

private:
    QVector<PermutationAxis> m_axes;
    FilterFunction m_filter;
    QMap<uint32_t, ShaderPermutation> m_cache;
    Stats m_stats;
    uint32_t computeHash(const QMap<QString, bool>& defines) const;
};

// ─── Shader Export Utilities ───────────────────────────────────────────────

class ShaderExporter : public QObject {
    Q_OBJECT

public:
    static ShaderExporter* instance();
    
    explicit ShaderExporter(QObject* parent = nullptr);
    ~ShaderExporter() override;

    // Export shader graph to target language
    bool exportGraph(const QUuid& graphId, const ShaderExportOptions& options);
    
    // Export specific permutation
    bool exportPermutation(const ShaderPermutation& permutation, const ShaderExportOptions& options);
    
    // Batch export all permutations
    bool exportAllPermutations(const QUuid& graphId, const ShaderExportOptions& options);
    
    // Generate shader templates
    QString generateTemplate(ShaderExportOptions::Target target, const QString& shaderType);
    
    // Validation
    bool validateExport(const QUuid& graphId, ShaderExportOptions::Target target, QString* error = nullptr);

signals:
    void exportStarted(const QString& targetName);
    void exportProgress(int current, int total, const QString& currentFile);
    void exportCompleted(const QString& outputDirectory);
    void exportFailed(const QString& error);

private:
    QString glslFromGraph(const ShaderGraph& graph) const;
    QString hlslFromGraph(const ShaderGraph& graph) const;
    QString spirvFromGraph(const ShaderGraph& graph) const;
    QString metalFromGraph(const ShaderGraph& graph) const;
    
    QString nodeToGLSL(const ShaderNode& node, const ShaderGraph& graph) const;
    QString portTypeToGLSL(ShaderPortType type) const;
    QString portTypeToHLSL(ShaderPortType type) const;
    QString portTypeToMSL(ShaderPortType type) const;
    
    QString generateStructDefinitions(const ShaderGraph& graph) const;
    QString generateFunctionBody(const ShaderGraph& graph) const;
    QString generateEntryPoint(const ShaderGraph& graph, ShaderExportOptions::Target target) const;
};

} // namespace material
} // namespace ks