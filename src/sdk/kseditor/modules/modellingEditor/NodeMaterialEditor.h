#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include <QPointF>
#include <QColor>
#include <QVariant>
#include <QObject>
#include <QSharedPointer>
#include <QSize>
#include <QVector3D>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <functional>

namespace ks {

class MaterialNode;

enum class MaterialNodeType {
    Input,
    Output,
    Shader,
    Utility,
    Texture,
    Color,
    Math,
    Vector,
    Group
};

enum class NodeInputType {
    Float,
    Float2,
    Float3,
    Float4,
    Color,
    Image,
    Shader,
    Int
};

struct NodeSocket {
    QString id;
    QString name;
    NodeInputType type;
    QVariant defaultValue;
    QVariant value;
    bool isOutput;
    bool isConnected;
    QString connectedSocketId;
};

class MaterialNode {
public:
    MaterialNode(const QString& name, MaterialNodeType type);
    virtual ~MaterialNode();

    QString id;
    QString name;
    QString factoryType;
    MaterialNodeType type;
    QPointF position;
    QSize size;
    bool isSelected;
    bool isMuted;

    QVector<NodeSocket> inputs;
    QVector<NodeSocket> outputs;

    virtual QString getExpression() const;
    virtual QMap<QString, QString> getGeneratedCode() const;

    void addInput(const QString& name, NodeInputType type, const QVariant& defaultValue = QVariant());
    void addOutput(const QString& name, NodeInputType type);
    NodeSocket* findSocket(const QString& socketId);

    bool processInputs();
    QVariant evaluateOutput(const QString& socketId);

    QMap<QString, MaterialNode*> linkedNodes;

    static QString generateNodeId();
    static QString generateSocketId();
};

class OutputNode : public MaterialNode {
public:
    OutputNode();

    QString getExpression() const override;
};

class InputNode : public MaterialNode {
public:
    InputNode(const QString& name, NodeInputType type);

    void setValue(const QVariant& value);
    QVariant getValue() const;

    QString getExpression() const override;
};

class ImageNode : public MaterialNode {
public:
    ImageNode();
    QString getExpression() const override;
    QString texturePath;
    int textureId = -1;
};

class CubeMapNode : public MaterialNode {
public:
    CubeMapNode();
    QString getExpression() const override;
    QString texturePath;
    int textureId = -1;
};

class NormalMapNode : public MaterialNode {
public:
    NormalMapNode();
    QString getExpression() const override;
    QString texturePath;
    int textureId = -1;
};

class TextureNode : public MaterialNode {
public:
    TextureNode();
    QString getExpression() const override;
    QString texturePath;
    QString colorSpace;
    int textureId = -1;
};

class ColorRampNode : public MaterialNode {
public:
    ColorRampNode();

    struct ColorStop {
        float position;
        QColor color;
    };

    QVector<ColorStop> stops;
    int interpolation;

    QString getExpression() const override;
};

class MixNode : public MaterialNode {
public:
    MixNode();

    enum class MixType { Mix, Add, Subtract, Multiply, Divide, Power, Log, SQRT, Min, Max, Mod, Atan2, Compare };
    MixType mixType;

    QString getExpression() const override;
};

class MathNode : public MaterialNode {
public:
    MathNode();

    enum class MathType { Add, Subtract, Multiply, Divide, Power, Log, Abs, Clamp, Floor, Ceil, Fract, Mod, Min, Max, Round, Sin, Cos, Tan, Asin, Acos, Atan, Atan2 };
    MathType mathType;

    QString getExpression() const override;
};

class VectorMathNode : public MaterialNode {
public:
    VectorMathNode();

    enum class VectorType { Add, Subtract, Multiply, Divide, Cross, Dot, Normalize, Length, Distance };
    VectorType vectorType;

    QString getExpression() const override;
};

class SeparateXYZNode : public MaterialNode {
public:
    SeparateXYZNode();
    QString getExpression() const override;
};

class CombineXYZNode : public MaterialNode {
public:
    CombineXYZNode();
    QString getExpression() const override;
};

class RGBToBWNode : public MaterialNode {
public:
    RGBToBWNode();
    QString getExpression() const override;
};

class FresnelNode : public MaterialNode {
public:
    FresnelNode();

    float ior;

    QString getExpression() const override;
};

class AmbientOcclusionNode : public MaterialNode {
public:
    AmbientOcclusionNode();

    float samples;
    float distance;
    float strength;

    QString getExpression() const override;
};

class BevelNode : public MaterialNode {
public:
    BevelNode();

    float radius;

    QString getExpression() const override;
};

class EmissionNode : public MaterialNode {
public:
    EmissionNode();

    float strength;

    QString getExpression() const override;
};

class BSDFPrincipledNode : public MaterialNode {
public:
    BSDFPrincipledNode();

    float subsurface;
    float subsurfaceScale;
    QColor subsurfaceColor;
    float metallic;
    float specular;
    float specularTint;
    float roughness;
    float anisotropic;
    float anisotropicRotation;
    float clearcoat;
    float clearcoatRoughness;
    float ior;
    float transmission;
    float thickness;
    QColor emission;
    float emissionStrength;

    QString getExpression() const override;
};

class ShaderToRGBNode : public MaterialNode {
public:
    ShaderToRGBNode();
    QString getExpression() const override;
};

class RGBToShaderNode : public MaterialNode {
public:
    RGBToShaderNode();
    QString getExpression() const override;
};

class NoiseTextureNode : public MaterialNode {
public:
    NoiseTextureNode();

    float scale;
    float detail;
    float distortion;

    QString getExpression() const override;
};

class VoronoiNode : public MaterialNode {
public:
    VoronoiNode();

    enum class Metric { Euclidean, Manhattan, Chebyshev };
    enum class Feature { F1, F2, SmoothF1, F4 };

    float scale;
    int detail;
    Metric metric;
    Feature feature;

    QString getExpression() const override;
};

class WaveTextureNode : public MaterialNode {
public:
    WaveTextureNode();

    enum class WaveType { Sine, Saw, Square, Triangle };
    enum class WaveDirection { X, Y, Z };

    WaveType waveType;
    WaveDirection direction;
    float scale;
    float distortion;
    float detail;

    QString getExpression() const override;
};

class GradientTextureNode : public MaterialNode {
public:
    GradientTextureNode();

    enum class GradientType { Linear, Quadratic, Easing, Diagonal, Radial, QuadraticSphere, Spherical };
    GradientType gradientType;

    QString getExpression() const override;
};

class MappingNode : public MaterialNode {
public:
    MappingNode();

    QVector3D location;
    QVector3D rotation;
    QVector3D scale;

    QString getExpression() const override;
};

class TextureCoordinateNode : public MaterialNode {
public:
    TextureCoordinateNode();

    enum class UVMap { Generated, Normal, UV, Object };
    UVMap uvMap;

    QString getExpression() const override;
};

class BrightContrastNode : public MaterialNode {
public:
    BrightContrastNode();

    float brightness;
    float contrast;

    QString getExpression() const override;
};

class HSVNode : public MaterialNode {
public:
    HSVNode();

    enum class Mode { Combine, Separate };
    Mode mode;

    QString getExpression() const override;
};

class MaterialGraph {
public:
    MaterialGraph();
    ~MaterialGraph();

    QString name;
    QVector<MaterialNode*> nodes;
    MaterialNode* outputNode;

    MaterialNode* createNode(const QString& type, const QPointF& position);
    void deleteNode(const QString& nodeId);
    void connectNodes(const QString& fromNode, const QString& fromSocket,
                     const QString& toNode, const QString& toSocket);
    void disconnectNodes(const QString& fromNode, const QString& fromSocket,
                        const QString& toNode, const QString& toSocket);

    MaterialNode* findNode(const QString& nodeId);
    QVector<MaterialNode*> getInputNodes();
    QVector<MaterialNode*> topologicalSort();

    QString generateGLSL();
    QString generateHLSL();

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);

    void clear();
    void copyFrom(const MaterialGraph& other);

private:
    void updateLinks(const QString& removedNodeId = "");
};

class NodeGroup {
public:
    QString id;
    QString name;
    QVector<MaterialNode*> nodes;
    QVector<NodeSocket> inputs;
    QVector<NodeSocket> outputs;
    QPointF position;

    MaterialGraph* toGraph() const;
    static NodeGroup* fromGraph(const MaterialGraph& graph);
};

class MaterialNodeEditor {
public:
    MaterialNodeEditor();
    ~MaterialNodeEditor();

    MaterialGraph* currentGraph;

    QVector<MaterialGraph*> graphs;
    QVector<NodeGroup*> groups;

    MaterialGraph* newGraph();
    void setCurrentGraph(MaterialGraph* graph);
    MaterialGraph* getCurrentGraph();

    void registerDefaultNodes();

    QMap<QString, std::function<MaterialNode*()>> nodeFactory;
    QStringList getAvailableNodeTypes();
};

}