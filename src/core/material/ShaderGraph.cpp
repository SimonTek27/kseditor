#include "ShaderGraph.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QQueue>
#include <QSet>
#include <QCryptographicHash>
#include <algorithm>

namespace ks {
namespace material {

// ─── Static Node Type Registry ──────────────────────────────────────────────

QMap<ShaderNodeType, ShaderGraphManager::NodeTypeInfo> ShaderGraphManager::s_nodeTypeInfo;
bool ShaderGraphManager::s_nodeTypesRegistered = false;

const QMap<ShaderNodeType, ShaderGraphManager::NodeTypeInfo>& ShaderGraphManager::getNodeTypeInfo() {
    if (!s_nodeTypesRegistered) {
        const_cast<ShaderGraphManager*>(nullptr)->registerBuiltinNodeTypes();
    }
    return s_nodeTypeInfo;
}

const ShaderGraphManager::NodeTypeInfo& ShaderGraphManager::getNodeTypeInfo(ShaderNodeType type) {
    static NodeTypeInfo unknown;
    if (!s_nodeTypesRegistered) {
        const_cast<ShaderGraphManager*>(nullptr)->registerBuiltinNodeTypes();
    }
    auto it = s_nodeTypeInfo.find(type);
    return it != s_nodeTypeInfo.end() ? it.value() : unknown;
}

void ShaderGraphManager::registerBuiltinNodeTypes() {
    if (s_nodeTypesRegistered) return;
    
    // Input nodes
    auto registerInput = [&](ShaderNodeType type, const QString& name, const QString& category,
                              ShaderPortType portType, const QString& portName, const QColor& color = Qt::cyan) {
        NodeTypeInfo info;
        info.type = type;
        info.name = name;
        info.category = category;
        info.description = name + " input";
        info.headerColor = color;
        info.isInput = true;
        info.defaultOutputs = QVector<ShaderPort>{
            ShaderPort{QUuid::createUuid(), portName, portType, false, false, QVariant(), "", 0}
        };
        s_nodeTypeInfo[type] = info;
    };
    
    registerInput(ShaderNodeType::InputPosition, "Position", "Input", ShaderPortType::Float3, "Position");
    registerInput(ShaderNodeType::InputNormal, "Normal", "Input", ShaderPortType::Float3, "Normal");
    registerInput(ShaderNodeType::InputUV, "UV", "Input", ShaderPortType::Float2, "UV");
    registerInput(ShaderNodeType::InputColor, "Vertex Color", "Input", ShaderPortType::Color, "Color");
    registerInput(ShaderNodeType::InputViewDir, "View Direction", "Input", ShaderPortType::Float3, "View Dir");
    registerInput(ShaderNodeType::InputWorldPos, "World Position", "Input", ShaderPortType::Float3, "World Pos");
    registerInput(ShaderNodeType::InputTangent, "Tangent", "Input", ShaderPortType::Float3, "Tangent");
    registerInput(ShaderNodeType::InputBitangent, "Bitangent", "Input", ShaderPortType::Float3, "Bitangent");
    registerInput(ShaderNodeType::InputVertexColor, "Vertex Color", "Input", ShaderPortType::Color, "Vert Color");
    registerInput(ShaderNodeType::InputInstanceID, "Instance ID", "Input", ShaderPortType::Int, "Instance ID");

    // Constant nodes
    auto registerConstant = [&](ShaderNodeType type, const QString& name, const QString& category,
                                 ShaderPortType portType, const QString& portName, const QVariant& defaultVal, const QColor& color = Qt::yellow) {
        NodeTypeInfo info;
        info.type = type;
        info.name = name;
        info.category = category;
        info.description = name + " constant";
        info.headerColor = color;
        info.defaultOutputs = QVector<ShaderPort>{
            ShaderPort{QUuid::createUuid(), portName, portType, false, false, defaultVal, "", 0}
        };
        s_nodeTypeInfo[type] = info;
    };
    
    registerConstant(ShaderNodeType::ConstantFloat, "Float", "Constant", ShaderPortType::Float, "Value", 0.0f);
    registerConstant(ShaderNodeType::ConstantVector2, "Vector2", "Constant", ShaderPortType::Float2, "Value", QVector2D(0,0));
    registerConstant(ShaderNodeType::ConstantVector3, "Vector3", "Constant", ShaderPortType::Float3, "Value", QVector3D(0,0,0));
    registerConstant(ShaderNodeType::ConstantVector4, "Vector4", "Constant", ShaderPortType::Float4, "Value", QVector4D(0,0,0,0));
    registerConstant(ShaderNodeType::ConstantColor, "Color", "Constant", ShaderPortType::Color, "Value", QColor(255,255,255));
    registerConstant(ShaderNodeType::ConstantInt, "Integer", "Constant", ShaderPortType::Int, "Value", 0);
    registerConstant(ShaderNodeType::ConstantBool, "Boolean", "Constant", ShaderPortType::Bool, "Value", false);

    // Math nodes
    auto registerMath = [&](ShaderNodeType type, const QString& name, int inputCount, const QString& category = "Math", const QColor& color = Qt::green) {
        NodeTypeInfo info;
        info.type = type;
        info.name = name;
        info.category = category;
        info.description = name + " operation";
        info.headerColor = color;
        info.isMath = true;
        for (int i = 0; i < inputCount; ++i) {
            info.defaultInputs.append(ShaderPort{QUuid::createUuid(), QString("In%1").arg(i), ShaderPortType::Float, true, false, 0.0f, "", i});
        }
        info.defaultOutputs = QVector<ShaderPort>{
            ShaderPort{QUuid::createUuid(), "Out", ShaderPortType::Float, false, false, QVariant(), "", 0}
        };
        s_nodeTypeInfo[type] = info;
    };
    
    registerMath(ShaderNodeType::MathAdd, "Add", 2);
    registerMath(ShaderNodeType::MathSubtract, "Subtract", 2);
    registerMath(ShaderNodeType::MathMultiply, "Multiply", 2);
    registerMath(ShaderNodeType::MathDivide, "Divide", 2);
    registerMath(ShaderNodeType::MathPower, "Power", 2);
    registerMath(ShaderNodeType::MathSqrt, "Sqrt", 1);
    registerMath(ShaderNodeType::MathAbs, "Abs", 1);
    registerMath(ShaderNodeType::MathMin, "Min", 2);
    registerMath(ShaderNodeType::MathMax, "Max", 2);
    registerMath(ShaderNodeType::MathClamp, "Clamp", 3);
    registerMath(ShaderNodeType::MathSaturate, "Saturate", 1);
    registerMath(ShaderNodeType::MathLerp, "Lerp", 3);
    registerMath(ShaderNodeType::MathStep, "Step", 2);
    registerMath(ShaderNodeType::MathSmoothstep, "Smoothstep", 3);
    registerMath(ShaderNodeType::MathSign, "Sign", 1);
    registerMath(ShaderNodeType::MathFloor, "Floor", 1);
    registerMath(ShaderNodeType::MathCeil, "Ceil", 1);
    registerMath(ShaderNodeType::MathFract, "Fract", 1);
    registerMath(ShaderNodeType::MathSin, "Sin", 1);
    registerMath(ShaderNodeType::MathCos, "Cos", 1);
    registerMath(ShaderNodeType::MathTan, "Tan", 1);
    registerMath(ShaderNodeType::MathAsin, "Asin", 1);
    registerMath(ShaderNodeType::MathAcos, "Acos", 1);
    registerMath(ShaderNodeType::MathAtan, "Atan", 1);
    registerMath(ShaderNodeType::MathAtan2, "Atan2", 2);
    registerMath(ShaderNodeType::MathExp, "Exp", 1);
    registerMath(ShaderNodeType::MathLog, "Log", 1);
    registerMath(ShaderNodeType::MathDot, "Dot", 2);
    registerMath(ShaderNodeType::MathCross, "Cross", 2);
    registerMath(ShaderNodeType::MathLength, "Length", 1);
    registerMath(ShaderNodeType::MathNormalize, "Normalize", 1);
    registerMath(ShaderNodeType::MathDistance, "Distance", 2);
    registerMath(ShaderNodeType::MathReflect, "Reflect", 2);
    registerMath(ShaderNodeType::MathRefract, "Refract", 3);
    registerMath(ShaderNodeType::MathFaceForward, "FaceForward", 3);
    registerMath(ShaderNodeType::MathMatrixMultiply, "Matrix Multiply", 2);
    registerMath(ShaderNodeType::MathTransformVector, "Transform Vector", 2);
    registerMath(ShaderNodeType::MathTransformPoint, "Transform Point", 2);

    // Texture nodes
    auto registerTexture = [&](ShaderNodeType type, const QString& name, const QString& category = "Texture", const QColor& color = Qt::magenta) {
        NodeTypeInfo info;
        info.type = type;
        info.name = name;
        info.category = category;
        info.description = name + " texture operation";
        info.headerColor = color;
        info.isTexture = true;
        info.defaultInputs = QVector<ShaderPort>{
            ShaderPort{QUuid::createUuid(), "UV", ShaderPortType::Float2, true, false, QVector2D(0,0), "", 0},
            ShaderPort{QUuid::createUuid(), "Texture", ShaderPortType::Texture2D, true, false, QVariant(), "", 1}
        };
        info.defaultOutputs = QVector<ShaderPort>{
            ShaderPort{QUuid::createUuid(), "Color", ShaderPortType::Color, false, false, QVariant(), "", 0}
        };
        s_nodeTypeInfo[type] = info;
    };
    
    registerTexture(ShaderNodeType::TextureSample, "Sample Texture");
    registerTexture(ShaderNodeType::TextureSampleGrad, "Sample Texture Grad");
    registerTexture(ShaderNodeType::TextureSampleLevel, "Sample Texture Level");
    registerTexture(ShaderNodeType::TextureSize, "Texture Size", "Texture", Qt::darkMagenta);
    registerTexture(ShaderNodeType::TextureCubeSample, "Sample Cube");
    registerTexture(ShaderNodeType::TextureArraySample, "Sample Array");
    registerTexture(ShaderNodeType::NormalMapSample, "Normal Map", "Texture", Qt::darkCyan);

    // Color nodes
    auto registerColor = [&](ShaderNodeType type, const QString& name, int inputCount, const QString& category = "Color", const QColor& color = Qt::red) {
        NodeTypeInfo info;
        info.type = type;
        info.name = name;
        info.category = category;
        info.description = name + " color operation";
        info.headerColor = color;
        for (int i = 0; i < inputCount; ++i) {
            info.defaultInputs.append(ShaderPort{QUuid::createUuid(), QString("In%1").arg(i), ShaderPortType::Color, true, false, QColor(0,0,0), "", i});
        }
        info.defaultOutputs = QVector<ShaderPort>{
            ShaderPort{QUuid::createUuid(), "Out", ShaderPortType::Color, false, false, QVariant(), "", 0}
        };
        s_nodeTypeInfo[type] = info;
    };
    
    registerColor(ShaderNodeType::ColorGammaToLinear, "Gamma to Linear", 1);
    registerColor(ShaderNodeType::ColorLinearToGamma, "Linear to Gamma", 1);
    registerColor(ShaderNodeType::ColorHSVToRGB, "HSV to RGB", 1);
    registerColor(ShaderNodeType::ColorRGBToHSV, "RGB to HSV", 1);
    registerColor(ShaderNodeType::ColorMix, "Mix", 3);
    registerColor(ShaderNodeType::ColorBrightnessContrast, "Brightness/Contrast", 3);
    registerColor(ShaderNodeType::ColorHueSaturation, "Hue/Saturation", 3);
    registerColor(ShaderNodeType::ColorInvert, "Invert", 1);
    registerColor(ShaderNodeType::ColorDesaturate, "Desaturate", 1);
    registerColor(ShaderNodeType::ColorChannelPack, "Channel Pack", 4);
    registerColor(ShaderNodeType::ColorChannelUnpack, "Channel Unpack", 1);

    // PBR nodes
    auto registerPBR = [&](ShaderNodeType type, const QString& name, const QString& category = "PBR", const QColor& color = Qt::blue) {
        NodeTypeInfo info;
        info.type = type;
        info.name = name;
        info.category = category;
        info.description = name + " PBR shading";
        info.headerColor = color;
        info.isPBR = true;
        info.defaultInputs = QVector<ShaderPort>{
            ShaderPort{QUuid::createUuid(), "Normal", ShaderPortType::Float3, true, false, QVector3D(0,1,0), "", 0},
            ShaderPort{QUuid::createUuid(), "ViewDir", ShaderPortType::Float3, true, false, QVector3D(0,0,1), "", 1},
            ShaderPort{QUuid::createUuid(), "BaseColor", ShaderPortType::Color, true, false, QColor(255,255,255), "", 2},
            ShaderPort{QUuid::createUuid(), "Metallic", ShaderPortType::Float, true, false, 0.0f, "", 3},
            ShaderPort{QUuid::createUuid(), "Roughness", ShaderPortType::Float, true, false, 0.5f, "", 4},
            ShaderPort{QUuid::createUuid(), "AO", ShaderPortType::Float, true, false, 1.0f, "", 5}
        };
        info.defaultOutputs = QVector<ShaderPort>{
            ShaderPort{QUuid::createUuid(), "BSDF", ShaderPortType::Generic, false, false, QVariant(), "", 0}
        };
        s_nodeTypeInfo[type] = info;
    };
    
    registerPBR(ShaderNodeType::PBRMetallicRoughness, "Metallic Roughness");
    registerPBR(ShaderNodeType::PBRSpecularGlossiness, "Specular Glossiness");
    registerPBR(ShaderNodeType::PBRDisneyPrincipled, "Disney Principled");
    registerPBR(ShaderNodeType::PBRClearcoat, "Clearcoat");
    registerPBR(ShaderNodeType::PBRSheen, "Sheen");
    registerPBR(ShaderNodeType::PBRAnisotropy, "Anisotropy");
    registerPBR(ShaderNodeType::PBRSubsurface, "Subsurface");
    registerPBR(ShaderNodeType::PBRTransmission, "Transmission");
    registerPBR(ShaderNodeType::PBRAmbientOcclusion, "Ambient Occlusion");

    // Output nodes
    auto registerOutput = [&](ShaderNodeType type, const QString& name, ShaderPortType portType, const QString& category = "Output", const QColor& color = Qt::darkBlue) {
        NodeTypeInfo info;
        info.type = type;
        info.name = name;
        info.category = category;
        info.description = name + " output";
        info.headerColor = color;
        info.isOutput = true;
        info.defaultInputs = QVector<ShaderPort>{
            ShaderPort{QUuid::createUuid(), "Input", portType, true, false, QVariant(), "", 0}
        };
        s_nodeTypeInfo[type] = info;
    };
    
    registerOutput(ShaderNodeType::OutputMaterial, "Material Output", ShaderPortType::Generic);
    registerOutput(ShaderNodeType::OutputSurface, "Surface", ShaderPortType::Color);
    registerOutput(ShaderNodeType::OutputDisplacement, "Displacement", ShaderPortType::Float);
    registerOutput(ShaderNodeType::OutputNormal, "Normal", ShaderPortType::Float3);
    registerOutput(ShaderNodeType::OutputEmissive, "Emissive", ShaderPortType::Color);
    registerOutput(ShaderNodeType::OutputAlpha, "Alpha", ShaderPortType::Float);
    registerOutput(ShaderNodeType::OutputCustom, "Custom Output", ShaderPortType::Generic);

    s_nodeTypesRegistered = true;
}

// ─── ShaderGraphManager Implementation ──────────────────────────────────────

static ShaderGraphManager* s_shaderGraphManagerInstance = nullptr;

ShaderGraphManager* ShaderGraphManager::instance() {
    if (!s_shaderGraphManagerInstance) {
        s_shaderGraphManagerInstance = new ShaderGraphManager();
    }
    return s_shaderGraphManagerInstance;
}

ShaderGraphManager::ShaderGraphManager(QObject* parent) : QObject(parent) {
    registerBuiltinNodeTypes();
}

ShaderGraphManager::~ShaderGraphManager() {
    s_shaderGraphManagerInstance = nullptr;
}

QUuid ShaderGraphManager::createGraph(const QString& name) {
    QUuid id = QUuid::createUuid();
    ShaderGraph graph;
    graph.id = id;
    graph.name = name.isEmpty() ? "New Graph" : name;
    graph.created = QDateTime::currentDateTime();
    graph.modified = graph.created;
    m_graphs[id] = graph;
    emit graphCreated(id);
    return id;
}

bool ShaderGraphManager::deleteGraph(const QUuid& graphId) {
    if (!m_graphs.contains(graphId)) return false;
    m_graphs.remove(graphId);
    emit graphDeleted(graphId);
    return true;
}

ShaderGraph* ShaderGraphManager::getGraph(const QUuid& graphId) {
    return m_graphs.contains(graphId) ? &m_graphs[graphId] : nullptr;
}

const ShaderGraph* ShaderGraphManager::getGraph(const QUuid& graphId) const {
    auto it = m_graphs.constFind(graphId);
    return it != m_graphs.constEnd() ? &it.value() : nullptr;
}

QVector<QUuid> ShaderGraphManager::getAllGraphIds() const {
    return m_graphs.keys();
}

QVector<ShaderGraph*> ShaderGraphManager::getAllGraphs() const {
    QVector<ShaderGraph*> result;
    for (auto it = m_graphs.begin(); it != m_graphs.end(); ++it) {
        result.append(const_cast<ShaderGraph*>(&it.value()));
    }
    return result;
}

ShaderNode* ShaderGraphManager::addNode(const QUuid& graphId, ShaderNodeType type, const QPointF& position) {
    auto* graph = getGraph(graphId);
    if (!graph) return nullptr;
    
    ShaderNode node(type, position);
    node.id = QUuid::createUuid();
    
    // Initialize from node type info
    const auto& typeInfo = getNodeTypeInfo(type);
    node.title = typeInfo.name;
    node.inputs = typeInfo.defaultInputs;
    node.outputs = typeInfo.defaultOutputs;
    node.properties = typeInfo.defaultProperties;
    node.headerColor = typeInfo.headerColor;
    node.position = position;
    
    // Assign port IDs
    for (auto& port : node.inputs) {
        if (port.id.isNull()) port.id = QUuid::createUuid();
    }
    for (auto& port : node.outputs) {
        if (port.id.isNull()) port.id = QUuid::createUuid();
    }
    
    graph->nodes.append(node);
    graph->modified = QDateTime::currentDateTime();
    
    emit nodeAdded(graphId, graph->nodes.last().id);
    emit graphChanged(graphId);
    return &graph->nodes.last();
}

bool ShaderGraphManager::removeNode(const QUuid& graphId, const QUuid& nodeId) {
    auto* graph = getGraph(graphId);
    if (!graph) return false;
    
    // Remove all connections to/from this node
    for (int i = graph->connections.size() - 1; i >= 0; --i) {
        if (graph->connections[i].fromNodeId == nodeId || graph->connections[i].toNodeId == nodeId) {
            emit connectionRemoved(graphId, graph->connections[i].id);
            graph->connections.removeAt(i);
        }
    }
    
    for (int i = 0; i < graph->nodes.size(); ++i) {
        if (graph->nodes[i].id == nodeId) {
            graph->nodes.removeAt(i);
            graph->modified = QDateTime::currentDateTime();
            emit nodeRemoved(graphId, nodeId);
            emit graphChanged(graphId);
            return true;
        }
    }
    return false;
}

ShaderNode* ShaderGraphManager::getNode(const QUuid& graphId, const QUuid& nodeId) {
    auto* graph = getGraph(graphId);
    if (!graph) return nullptr;
    for (auto& node : graph->nodes) {
        if (node.id == nodeId) return &node;
    }
    return nullptr;
}

const ShaderNode* ShaderGraphManager::getNode(const QUuid& graphId, const QUuid& nodeId) const {
    auto* graph = getGraph(graphId);
    if (!graph) return nullptr;
    for (const auto& node : graph->nodes) {
        if (node.id == nodeId) return &node;
    }
    return nullptr;
}

void ShaderGraphManager::moveNode(const QUuid& graphId, const QUuid& nodeId, const QPointF& position) {
    auto* node = getNode(graphId, nodeId);
    if (node) {
        node->position = position;
        if (auto* graph = getGraph(graphId)) {
            graph->modified = QDateTime::currentDateTime();
            emit nodeMoved(graphId, nodeId, position);
            emit graphChanged(graphId);
        }
    }
}

void ShaderGraphManager::setNodeProperty(const QUuid& graphId, const QUuid& nodeId, const QString& property, const QVariant& value) {
    auto* node = getNode(graphId, nodeId);
    if (node) {
        node->properties[property] = value;
        if (auto* graph = getGraph(graphId)) {
            graph->modified = QDateTime::currentDateTime();
            emit nodePropertyChanged(graphId, nodeId, property, value);
            emit graphChanged(graphId);
        }
    }
}

bool ShaderGraphManager::connectPorts(const QUuid& graphId, const QUuid& fromNodeId, const QUuid& fromPortId,
                                      const QUuid& toNodeId, const QUuid& toPortId) {
    auto* graph = getGraph(graphId);
    if (!graph) return false;
    
    // Validate nodes and ports
    auto* fromNode = getNode(graphId, fromNodeId);
    auto* toNode = getNode(graphId, toNodeId);
    if (!fromNode || !toNode) return false;
    
    // Find ports
    ShaderPort* fromPort = nullptr;
    for (auto& p : fromNode->outputs) if (p.id == fromPortId) { fromPort = &p; break; }
    ShaderPort* toPort = nullptr;
    for (auto& p : toNode->inputs) if (p.id == toPortId) { toPort = &p; break; }
    if (!fromPort || !toPort) return false;
    
    // Check type compatibility
    if (fromPort->type != toPort->type) return false;
    
    // Check for existing connection on input port
    for (const auto& conn : graph->connections) {
        if (conn.toNodeId == toNodeId && conn.toPortId == toPortId) return false;
    }
    
    ShaderConnection conn;
    conn.id = QUuid::createUuid();
    conn.fromNodeId = fromNodeId;
    conn.fromPortId = fromPortId;
    conn.toNodeId = toNodeId;
    conn.toPortId = toPortId;
    conn.color = Qt::cyan;
    
    graph->connections.append(conn);
    fromPort->isConnected = true;
    toPort->isConnected = true;
    
    graph->modified = QDateTime::currentDateTime();
    emit connectionAdded(graphId, conn.id);
    emit graphChanged(graphId);
    return true;
}

bool ShaderGraphManager::disconnectPorts(const QUuid& graphId, const QUuid& connectionId) {
    auto* graph = getGraph(graphId);
    if (!graph) return false;
    
    for (int i = 0; i < graph->connections.size(); ++i) {
        if (graph->connections[i].id == connectionId) {
            auto conn = graph->connections[i];
            
            // Update port connection status
            if (auto* fromNode = getNode(graphId, conn.fromNodeId)) {
                for (auto& p : fromNode->outputs) if (p.id == conn.fromPortId) { p.isConnected = false; break; }
            }
            if (auto* toNode = getNode(graphId, conn.toNodeId)) {
                for (auto& p : toNode->inputs) if (p.id == conn.toPortId) { p.isConnected = false; break; }
            }
            
            graph->connections.removeAt(i);
            graph->modified = QDateTime::currentDateTime();
            emit connectionRemoved(graphId, connectionId);
            emit graphChanged(graphId);
            return true;
        }
    }
    return false;
}

ShaderConnection* ShaderGraphManager::getConnection(const QUuid& graphId, const QUuid& connectionId) {
    auto* graph = getGraph(graphId);
    if (!graph) return nullptr;
    for (auto& conn : graph->connections) {
        if (conn.id == connectionId) return &conn;
    }
    return nullptr;
}

QVector<ShaderConnection*> ShaderGraphManager::getNodeConnections(const QUuid& graphId, const QUuid& nodeId, bool inputs) {
    auto* graph = getGraph(graphId);
    if (!graph) return {};
    
    QVector<ShaderConnection*> result;
    for (auto& conn : graph->connections) {
        if (inputs && conn.toNodeId == nodeId) result.append(&conn);
        else if (!inputs && conn.fromNodeId == nodeId) result.append(&conn);
    }
    return result;
}

ShaderGraphManager::ValidationResult ShaderGraphManager::validateGraph(const QUuid& graphId) const {
    ValidationResult result;
    auto* graph = getGraph(graphId);
    if (!graph) {
        result.valid = false;
        return result;
    }
    
    // Check for disconnected required inputs
    for (const auto& node : graph->nodes) {
        const auto& typeInfo = getNodeTypeInfo(node.type);
        if (typeInfo.isOutput) {
            // Check if output node has connections
            bool hasConnection = false;
            for (const auto& conn : graph->connections) {
                if (conn.toNodeId == node.id) { hasConnection = true; break; }
            }
            if (!hasConnection && node.inputs.size() > 0) {
                result.warnings[node.id] = "Output node has no connections";
                result.warningNodes.append(node.id);
            }
        }
        
        // Check required inputs
        for (const auto& port : node.inputs) {
            bool connected = false;
            for (const auto& conn : graph->connections) {
                if (conn.toNodeId == node.id && conn.toPortId == port.id) { connected = true; break; }
            }
            if (!connected && !port.defaultValue.isNull()) {
                // Has default, OK
            } else if (!connected) {
                result.errors[node.id] = "Unconnected required input: " + port.name;
                result.errorNodes.append(node.id);
                result.valid = false;
            }
        }
    }
    
    // Check for cycles
    QMap<QUuid, QVector<QUuid>> adj;
    for (const auto& conn : graph->connections) {
        adj[conn.fromNodeId].append(conn.toNodeId);
    }
    
    QSet<QUuid> visited, recStack;
    std::function<void(const QUuid&, QVector<QUuid>&)> dfs = [&](const QUuid& u, QVector<QUuid>& path) {
        visited.insert(u);
        recStack.insert(u);
        path.append(u);
        
        for (const auto& v : adj[u]) {
            if (!visited.contains(v)) {
                dfs(v, path);
            } else if (recStack.contains(v)) {
                // Cycle detected
                int idx = path.indexOf(v);
                if (idx >= 0) {
                    QVector<QUuid> cycle = path.mid(idx);
                    cycle.append(v);
                    for (const auto& nodeId : cycle) {
                        result.errors[nodeId] = "Cycle detected in graph";
                        result.errorNodes.append(nodeId);
                        result.valid = false;
                    }
                }
            }
        }
        recStack.remove(u);
        path.removeLast();
    };
    
    for (const auto& node : graph->nodes) {
        if (!visited.contains(node.id)) {
            QVector<QUuid> path;
            dfs(node.id, path);
        }
    }
    
    // Remove duplicates
    std::sort(result.errorNodes.begin(), result.errorNodes.end());
    result.errorNodes.erase(std::unique(result.errorNodes.begin(), result.errorNodes.end()), result.errorNodes.end());
    std::sort(result.warningNodes.begin(), result.warningNodes.end());
    result.warningNodes.erase(std::unique(result.warningNodes.begin(), result.warningNodes.end()), result.warningNodes.end());
    
    return result;
}

QString ShaderGraphManager::generateGLSL(const QUuid& graphId, const QString& entryPoint) const {
    auto* graph = getGraph(graphId);
    if (!graph) return "// Graph not found";
    
    QString output = "#version 450 core\n\n";
    
    // Generate struct definitions
    output += "// Structs\n";
    output += "struct MaterialData {\n";
    output += "    vec3 baseColor;\n";
    output += "    float metallic;\n";
    output += "    float roughness;\n";
    output += "    float alpha;\n";
    output += "    vec3 emissive;\n";
    output += "};\n\n";
    
    // Generate function for each node
    output += "// Node functions\n";
    for (const auto& node : graph->nodes) {
        output += "// Node: " + node.title + " (" + node.id.toString() + ")\n";
        // Simplified - real implementation would generate full shader code
    }
    
    output += "\nvoid main() {\n";
    output += "    // Entry point\n";
    output += "}\n";
    
    return output;
}

QString ShaderGraphManager::generateHLSL(const QUuid& graphId, const QString& entryPoint) const
{
    auto* graph = getGraph(graphId);
    if (!graph) return QString();

    QString ep = entryPoint.isEmpty() ? "main" : entryPoint;
    QString output;

    output += "// HLSL Shader - Auto-generated by ksEditor\n";
    output += "// Entry point: " + ep + "\n\n";

    // Structs
    output += "cbuffer MaterialConstants : register(b0) {\n";
    output += "    float3 baseColor;\n";
    output += "    float metallic;\n";
    output += "    float roughness;\n";
    output += "    float alpha;\n";
    output += "    float3 emissive;\n";
    output += "};\n\n";

    // Texture declarations
    output += "Texture2D albedoTexture : register(t0);\n";
    output += "SamplerState albedoSampler : register(s0);\n\n";

    // Constant buffer for transforms
    output += "cbuffer TransformConstants : register(b1) {\n";
    output += "    float4x4 worldViewProj;\n";
    output += "    float4x4 world;\n";
    output += "    float3 cameraPos;\n";
    output += "};\n\n";

    // Struct definitions for input/output
    output += "struct VSInput {\n";
    output += "    float3 position : POSITION;\n";
    output += "    float3 normal : NORMAL;\n";
    output += "    float2 uv : TEXCOORD0;\n";
    output += "    float4 color : COLOR0;\n";
    output += "    float3 tangent : TANGENT;\n";
    output += "};\n\n";

    output += "struct PSInput {\n";
    output += "    float4 position : SV_POSITION;\n";
    output += "    float3 worldPos : TEXCOORD0;\n";
    output += "    float3 normal : TEXCOORD1;\n";
    output += "    float2 uv : TEXCOORD2;\n";
    output += "    float4 color : COLOR0;\n";
    output += "    float3 tangent : TEXCOORD3;\n";
    output += "};\n\n";

    output += "struct PSOutput {\n";
    output += "    float4 color : SV_TARGET;\n";
    output += "};\n\n";

    // Vertex shader
    output += "PSInput " + ep + "VS(VSInput input) {\n";
    output += "    PSInput output;\n";
    output += "    output.position = mul(float4(input.position, 1.0), worldViewProj);\n";
    output += "    output.worldPos = mul(float4(input.position, 1.0), world).xyz;\n";
    output += "    output.normal = normalize(mul(input.normal, (float3x3)world));\n";
    output += "    output.uv = input.uv;\n";
    output += "    output.color = input.color;\n";
    output += "    output.tangent = normalize(mul(input.tangent, (float3x3)world));\n";
    output += "    return output;\n";
    output += "}\n\n";

    // Pixel shader
    output += "PSOutput " + ep + "PS(PSInput input) {\n";
    output += "    PSOutput output;\n";
    output += "    float3 albedo = albedoTexture.Sample(albedoSampler, input.uv).rgb * input.color.rgb;\n";
    output += "    float3 N = normalize(input.normal);\n";
    output += "    float3 L = normalize(float3(1, 1, 1));\n";
    output += "    float3 V = normalize(cameraPos - input.worldPos);\n";
    output += "    float3 H = normalize(L + V);\n";
    output += "    float NdotL = max(dot(N, L), 0.0);\n";
    output += "    float NdotH = max(dot(N, H), 0.0);\n";
    output += "    float spec = pow(NdotH, (1.0 - roughness) * 256.0);\n";
    output += "    float3 diffuse = albedo * (1.0 - metallic) * NdotL;\n";
    output += "    float3 specular = lerp(float3(0.04, 0.04, 0.04), albedo, metallic) * spec;\n";
    output += "    output.color = float4(diffuse + specular + emissive, alpha);\n";
    output += "    return output;\n";
    output += "}\n";

    return output;
}

QString ShaderGraphManager::generateSPIRV(const QUuid& graphId) const
{
    auto* graph = getGraph(graphId);
    if (!graph) return QString();

    QString output;
    output += "; SPIR-V Shader - Auto-generated by ksEditor\n";
    output += "; Requires glslangValidator or spirv-cross to compile\n\n";

    output += "; Capability declarations\n";
    output += "OpCapability Shader\n";
    output += "OpCapability ImageQuery\n";
    output += "OpExtension \"SP_GOOGLE_hlsl_functionality1\"\n\n";

    output += "; Memory model\n";
    output += "OpMemoryModel Logical GLSL450\n";
    output += "OpEntryPoint Vertex %main \"main\" %gl_Position %in_position %in_normal %in_uv\n";
    output += "OpEntryPoint Fragment %mainPS \"mainPS\" %out_color %in_worldPos %in_normal2 %in_uv2\n\n";

    output += "; Decorations\n";
    output += "OpDecorate %in_position Location 0\n";
    output += "OpDecorate %in_normal Location 1\n";
    output += "OpDecorate %in_uv Location 2\n";
    output += "OpDecorate %out_color Location 0\n\n";

    output += "; Type declarations\n";
    output += "%void = OpTypeVoid\n";
    output += "%float = OpTypeFloat 32\n";
    output += "%v3float = OpTypeVector %float 3\n";
    output += "%v4float = OpTypeVector %float 4\n";
    output += "%v2float = OpTypeVector %float 2\n";
    output += "%mat4x4 = OpTypeMatrix %v4float 4\n";
    output += "%_ptr_Input_v3float = OpTypePointer Input %v3float\n";
    output += "%_ptr_Input_v2float = OpTypePointer Input %v2float\n";
    output += "%_ptr_Output_v4float = OpTypePointer Output %v4float\n";
    output += "%_ptr_Uniform_mat4x4 = OpTypePointer Uniform %mat4x4\n\n";

    output += "; Uniform block\n";
    output += "OpMemberDecorate %TransformBlock 0 Offset 0\n";
    output += "OpMemberDecorate %TransformBlock 1 Offset 64\n";
    output += "%TransformBlock = OpTypeStruct %mat4x4 %mat4x4\n";
    output += "%_ptr_Uniform_TransformBlock = OpTypePointer Uniform %TransformBlock\n";
    output += "OpDecorate %TransformBlock Block\n\n";

    output += "; Note: Full SPIR-V generation requires cross-compilation from GLSL/HLSL\n";
    output += "; Use spirv-cross for decompilation or glslangValidator for compilation\n";

    return output;
}

QString ShaderGraphManager::generateMetal(const QUuid& graphId) const
{
    auto* graph = getGraph(graphId);
    if (!graph) return QString();

    QString output;
    output += "// Metal Shader - Auto-generated by ksEditor\n";
    output += "// Requires Metal Shading Language compiler\n\n";

    output += "#include <metal_stdlib>\n";
    output += "using namespace metal;\n\n";

    // Structs
    output += "struct VertexIn {\n";
    output += "    float3 position [[attribute(0)]];\n";
    output += "    float3 normal [[attribute(1)]];\n";
    output += "    float2 uv [[attribute(2)]];\n";
    output += "    float4 color [[attribute(3)]];\n";
    output += "    float3 tangent [[attribute(4)]];\n";
    output += "};\n\n";

    output += "struct VertexOut {\n";
    output += "    float4 position [[position]];\n";
    output += "    float3 worldPos;\n";
    output += "    float3 normal;\n";
    output += "    float2 uv;\n";
    output += "    float4 color;\n";
    output += "    float3 tangent;\n";
    output += "};\n\n";

    output += "struct MaterialUniforms {\n";
    output += "    float4x4 worldViewProj;\n";
    output += "    float4x4 world;\n";
    output += "    float3 cameraPos;\n";
    output += "    float3 baseColor;\n";
    output += "    float metallic;\n";
    output += "    float roughness;\n";
    output += "    float alpha;\n";
    output += "    float3 emissive;\n";
    output += "};\n\n";

    // Vertex shader
    output += "vertex VertexOut vertexMain(VertexIn in [[stage_in]],\n";
    output += "                             constant MaterialUniforms &uniforms [[buffer(1)]]) {\n";
    output += "    VertexOut out;\n";
    output += "    out.position = uniforms.worldViewProj * float4(in.position, 1.0);\n";
    output += "    out.worldPos = (uniforms.world * float4(in.position, 1.0)).xyz;\n";
    output += "    out.normal = normalize((uniforms.world * float4(in.normal, 0.0)).xyz);\n";
    output += "    out.uv = in.uv;\n";
    output += "    out.color = in.color;\n";
    output += "    out.tangent = normalize((uniforms.world * float4(in.tangent, 0.0)).xyz);\n";
    output += "    return out;\n";
    output += "}\n\n";

    // Fragment shader
    output += "fragment float4 fragmentMain(VertexOut in [[stage_in]],\n";
    output += "                             texture2d<float> albedoTexture [[texture(0)]],\n";
    output += "                             sampler albedoSampler [[sampler(0)]],\n";
    output += "                             constant MaterialUniforms &uniforms [[buffer(1)]]) {\n";
    output += "    float3 albedo = albedoTexture.sample(albedoSampler, in.uv).rgb * in.color.rgb;\n";
    output += "    float3 N = normalize(in.normal);\n";
    output += "    float3 L = normalize(float3(1, 1, 1));\n";
    output += "    float3 V = normalize(uniforms.cameraPos - in.worldPos);\n";
    output += "    float3 H = normalize(L + V);\n";
    output += "    float NdotL = max(dot(N, L), 0.0);\n";
    output += "    float NdotH = max(dot(N, H), 0.0);\n";
    output += "    float spec = pow(NdotH, (1.0 - uniforms.roughness) * 256.0);\n";
    output += "    float3 diffuse = albedo * (1.0 - uniforms.metallic) * NdotL;\n";
    output += "    float3 specular = mix(float3(0.04), albedo, uniforms.metallic) * spec;\n";
    output += "    return float4(diffuse + specular + uniforms.emissive, uniforms.alpha);\n";
    output += "}\n";

    return output;
}

ShaderPermutation ShaderGraphManager::compilePermutation(const QUuid& graphId, const QMap<QString, bool>& defines) {
    ShaderPermutation perm;
    perm.name = "Permutation_" + QUuid::createUuid().toString().mid(0, 8);
    perm.defines = defines;
    // Simplified - real implementation would compile shader
    perm.isValid = true;
    return perm;
}

QVector<ShaderPermutation> ShaderGraphManager::compileAllPermutations(const QUuid& graphId, const QVector<QMap<QString, bool>>& permutationDefines) {
    QVector<ShaderPermutation> results;
    for (const auto& defs : permutationDefines) {
        results.append(compilePermutation(graphId, defs));
    }
    return results;
}

bool ShaderGraphManager::exportToFiles(const QUuid& graphId, const ShaderExportOptions& options) {
    auto* graph = getGraph(graphId);
    if (!graph) return false;
    
    QDir dir(options.outputDirectory);
    if (!dir.exists()) dir.mkpath(".");
    
    QString content;
    switch (options.target) {
        case ShaderExportOptions::Target::GLSL:
            content = generateGLSL(graphId);
            break;
        case ShaderExportOptions::Target::HLSL:
            content = generateHLSL(graphId);
            break;
        case ShaderExportOptions::Target::SPIRV:
            content = generateSPIRV(graphId);
            break;
        case ShaderExportOptions::Target::Metal:
            content = generateMetal(graphId);
            break;
    }
    
    QString ext;
    switch (options.target) {
        case ShaderExportOptions::Target::GLSL: ext = ".glsl"; break;
        case ShaderExportOptions::Target::HLSL: ext = ".hlsl"; break;
        case ShaderExportOptions::Target::SPIRV: ext = ".spv"; break;
        case ShaderExportOptions::Target::Metal: ext = ".metal"; break;
    }
    
    QString fileName = graph->name + ext;
    QFile file(dir.filePath(fileName));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(content.toUtf8());
        file.close();
        return true;
    }
    return false;
}

QString ShaderGraphManager::exportToString(const QUuid& graphId, ShaderExportOptions::Target target) const {
    switch (target) {
        case ShaderExportOptions::Target::GLSL: return generateGLSL(graphId);
        case ShaderExportOptions::Target::HLSL: return generateHLSL(graphId);
        case ShaderExportOptions::Target::SPIRV: return generateSPIRV(graphId);
        case ShaderExportOptions::Target::Metal: return generateMetal(graphId);
    }
    return QString();
}

ShaderGraph ShaderGraphManager::createTemplateGraph(const QString& templateName) {
    ShaderGraph graph;
    graph.name = templateName;
    // Create template based on name
    return graph;
}

QVector<QString> ShaderGraphManager::getAvailableTemplates() const {
    return {"PBR_Metallic_Roughness", "PBR_Specular_Glossiness", "Unlit", "Vertex_Color", "Water", "Glass", "Hair", "Fabric"};
}

bool ShaderGraphManager::saveGraph(const QUuid& graphId, const QString& filePath) {
    auto* graph = getGraph(graphId);
    if (!graph) return false;
    
    QJsonObject obj = graph->toJson();
    QJsonDocument doc(obj);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        return true;
    }
    return false;
}

QUuid ShaderGraphManager::loadGraph(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return QUuid();
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (doc.isObject()) {
        ShaderGraph graph = ShaderGraph::fromJson(doc.object());
        QUuid id = QUuid::createUuid();
        graph.id = id;
        m_graphs[id] = graph;
        emit graphCreated(id);
        return id;
    }
    return QUuid();
}

// ─── ShaderGraph Serialization ──────────────────────────────────────────────

QJsonObject ShaderGraph::toJson() const {
    QJsonObject obj;
    obj["id"] = id.toString(QUuid::WithoutBraces);
    obj["name"] = name;
    obj["version"] = version;
    obj["description"] = description;
    obj["author"] = author;
    obj["created"] = created.toString(Qt::ISODate);
    obj["modified"] = modified.toString(Qt::ISODate);
    
    QJsonArray nodesArr;
    for (const auto& node : nodes) {
        QJsonObject n;
        n["id"] = node.id.toString(QUuid::WithoutBraces);
        n["type"] = static_cast<int>(node.type);
        n["title"] = node.title;
        n["subtitle"] = node.subtitle;
        n["position"] = QJsonObject{{"x", node.position.x()}, {"y", node.position.y()}};
        n["size"] = QJsonObject{{"w", node.size.width()}, {"h", node.size.height()}};
        n["selected"] = node.selected;
        n["minimized"] = node.minimized;
        n["error"] = node.error;
        n["errorMessage"] = node.errorMessage;
        n["comment"] = node.comment;
        n["headerColor"] = QJsonObject{{"r", node.headerColor.red()}, {"g", node.headerColor.green()}, {"b", node.headerColor.blue()}, {"a", node.headerColor.alpha()}};
        n["isGroup"] = node.isGroup;
        if (!node.groupId.isNull()) n["groupId"] = node.groupId.toString(QUuid::WithoutBraces);
        
        QJsonArray inputsArr;
        for (const auto& p : node.inputs) {
            QJsonObject pObj;
            pObj["id"] = p.id.toString(QUuid::WithoutBraces);
            pObj["name"] = p.name;
            pObj["type"] = static_cast<int>(p.type);
            pObj["isInput"] = p.isInput;
            pObj["isConnected"] = p.isConnected;
            pObj["defaultValue"] = QJsonValue::fromVariant(p.defaultValue);
            pObj["description"] = p.description;
            pObj["sortOrder"] = p.sortOrder;
            inputsArr.append(pObj);
        }
        n["inputs"] = inputsArr;
        
        QJsonArray outputsArr;
        for (const auto& p : node.outputs) {
            QJsonObject pObj;
            pObj["id"] = p.id.toString(QUuid::WithoutBraces);
            pObj["name"] = p.name;
            pObj["type"] = static_cast<int>(p.type);
            pObj["isInput"] = p.isInput;
            pObj["isConnected"] = p.isConnected;
            pObj["defaultValue"] = QJsonValue::fromVariant(p.defaultValue);
            pObj["description"] = p.description;
            pObj["sortOrder"] = p.sortOrder;
            outputsArr.append(pObj);
        }
        n["outputs"] = outputsArr;
        
        QJsonObject propsObj;
        for (auto it = node.properties.constBegin(); it != node.properties.constEnd(); ++it) {
            propsObj[it.key()] = QJsonValue::fromVariant(it.value());
        }
        n["properties"] = propsObj;
        
        nodesArr.append(n);
    }
    obj["nodes"] = nodesArr;
    
    QJsonArray connsArr;
    for (const auto& conn : connections) {
        QJsonObject c;
        c["id"] = conn.id.toString(QUuid::WithoutBraces);
        c["fromNodeId"] = conn.fromNodeId.toString(QUuid::WithoutBraces);
        c["fromPortId"] = conn.fromPortId.toString(QUuid::WithoutBraces);
        c["toNodeId"] = conn.toNodeId.toString(QUuid::WithoutBraces);
        c["toPortId"] = conn.toPortId.toString(QUuid::WithoutBraces);
        c["color"] = QJsonObject{{"r", conn.color.red()}, {"g", conn.color.green()}, {"b", conn.color.blue()}, {"a", conn.color.alpha()}};
        c["isValid"] = conn.isValid;
        connsArr.append(c);
    }
    obj["connections"] = connsArr;
    
    QJsonObject metaObj;
    for (auto it = metadata.constBegin(); it != metadata.constEnd(); ++it) {
        metaObj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    obj["metadata"] = metaObj;
    
    obj["viewRect"] = QJsonObject{{"x", viewRect.x()}, {"y", viewRect.y()}, {"w", viewRect.width()}, {"h", viewRect.height()}};
    obj["zoom"] = zoom;
    
    return obj;
}

ShaderGraph ShaderGraph::fromJson(const QJsonObject& obj) {
    ShaderGraph graph;
    graph.id = QUuid(obj["id"].toString());
    graph.name = obj["name"].toString();
    graph.version = obj["version"].toString();
    graph.description = obj["description"].toString();
    graph.author = obj["author"].toString();
    graph.created = QDateTime::fromString(obj["created"].toString(), Qt::ISODate);
    graph.modified = QDateTime::fromString(obj["modified"].toString(), Qt::ISODate);
    
    QJsonArray nodesArr = obj["nodes"].toArray();
    for (const auto& v : nodesArr) {
        QJsonObject n = v.toObject();
        ShaderNode node;
        node.id = QUuid(n["id"].toString());
        node.type = static_cast<ShaderNodeType>(n["type"].toInt());
        node.title = n["title"].toString();
        node.subtitle = n["subtitle"].toString();
        QJsonObject pos = n["position"].toObject();
        node.position = QPointF(pos["x"].toDouble(), pos["y"].toDouble());
        QJsonObject sz = n["size"].toObject();
        node.size = QSize(sz["w"].toInt(), sz["h"].toInt());
        node.selected = n["selected"].toBool();
        node.minimized = n["minimized"].toBool();
        node.error = n["error"].toBool();
        node.errorMessage = n["errorMessage"].toString();
        node.comment = n["comment"].toString();
        QJsonObject color = n["headerColor"].toObject();
        node.headerColor = QColor(color["r"].toInt(), color["g"].toInt(), color["b"].toInt(), color["a"].toInt());
        node.isGroup = n["isGroup"].toBool();
        if (n.contains("groupId")) node.groupId = QUuid(n["groupId"].toString());
        
        QJsonArray inputsArr = n["inputs"].toArray();
        for (const auto& v : inputsArr) {
            QJsonObject p = v.toObject();
            ShaderPort port;
            port.id = QUuid(p["id"].toString());
            port.name = p["name"].toString();
            port.type = static_cast<ShaderPortType>(p["type"].toInt());
            port.isInput = p["isInput"].toBool();
            port.isConnected = p["isConnected"].toBool();
            port.defaultValue = p["defaultValue"].toVariant();
            port.description = p["description"].toString();
            port.sortOrder = p["sortOrder"].toInt();
            node.inputs.append(port);
        }
        
        QJsonArray outputsArr = n["outputs"].toArray();
        for (const auto& v : outputsArr) {
            QJsonObject p = v.toObject();
            ShaderPort port;
            port.id = QUuid(p["id"].toString());
            port.name = p["name"].toString();
            port.type = static_cast<ShaderPortType>(p["type"].toInt());
            port.isInput = p["isInput"].toBool();
            port.isConnected = p["isConnected"].toBool();
            port.defaultValue = p["defaultValue"].toVariant();
            port.description = p["description"].toString();
            port.sortOrder = p["sortOrder"].toInt();
            node.outputs.append(port);
        }
        
        QJsonObject props = n["properties"].toObject();
        for (auto it = props.constBegin(); it != props.constEnd(); ++it) {
            node.properties[it.key()] = it.value().toVariant();
        }
        
        graph.nodes.append(node);
    }
    
    QJsonArray connsArr = obj["connections"].toArray();
    for (const auto& v : connsArr) {
        QJsonObject c = v.toObject();
        ShaderConnection conn;
        conn.id = QUuid(c["id"].toString());
        conn.fromNodeId = QUuid(c["fromNodeId"].toString());
        conn.fromPortId = QUuid(c["fromPortId"].toString());
        conn.toNodeId = QUuid(c["toNodeId"].toString());
        conn.toPortId = QUuid(c["toPortId"].toString());
        QJsonObject color = c["color"].toObject();
        conn.color = QColor(color["r"].toInt(), color["g"].toInt(), color["b"].toInt(), color["a"].toInt());
        conn.isValid = c["isValid"].toBool();
        graph.connections.append(conn);
    }
    
    QJsonObject meta = obj["metadata"].toObject();
    for (auto it = meta.constBegin(); it != meta.constEnd(); ++it) {
        graph.metadata[it.key()] = it.value().toVariant();
    }
    
    if (obj.contains("viewRect")) {
        QJsonObject vr = obj["viewRect"].toObject();
        graph.viewRect = QRectF(vr["x"].toDouble(), vr["y"].toDouble(), vr["w"].toDouble(), vr["h"].toDouble());
    }
    graph.zoom = obj["zoom"].toDouble(1.0);
    
    return graph;
}

// ─── ShaderNode Constructor ──────────────────────────────────────────────────

ShaderNode::ShaderNode(ShaderNodeType t, const QPointF& pos)
    : type(t), position(pos) {
    id = QUuid::createUuid();
    const auto& info = ShaderGraphManager::getNodeTypeInfo(t);
    title = info.name;
    headerColor = info.headerColor;
    inputs = info.defaultInputs;
    outputs = info.defaultOutputs;
    properties = info.defaultProperties;
}

// ─── ShaderPermutationManager Implementation ────────────────────────────────

static ShaderPermutationManager* s_permManagerInstance = nullptr;

ShaderPermutationManager* ShaderPermutationManager::instance() {
    if (!s_permManagerInstance) s_permManagerInstance = new ShaderPermutationManager();
    return s_permManagerInstance;
}

ShaderPermutationManager::ShaderPermutationManager(QObject* parent) : QObject(parent) {}

ShaderPermutationManager::~ShaderPermutationManager() {
    s_permManagerInstance = nullptr;
}

void ShaderPermutationManager::registerPermutationAxis(const PermutationAxis& axis) {
    // Remove existing with same name
    for (int i = m_axes.size() - 1; i >= 0; --i) {
        if (m_axes[i].name == axis.name) m_axes.removeAt(i);
    }
    m_axes.append(axis);
}

void ShaderPermutationManager::unregisterPermutationAxis(const QString& name) {
    for (int i = m_axes.size() - 1; i >= 0; --i) {
        if (m_axes[i].name == name) m_axes.removeAt(i);
    }
}

QVector<ShaderPermutationManager::PermutationAxis> ShaderPermutationManager::getPermutationAxes() const {
    return m_axes;
}

QVector<QMap<QString, bool>> ShaderPermutationManager::generatePermutations() const {
    QVector<QMap<QString, bool>> result;
    result.append(QMap<QString, bool>()); // Start with empty
    
    for (const auto& axis : m_axes) {
        QVector<QMap<QString, bool>> newResult;
        for (const auto& existing : result) {
            for (const auto& option : axis.options) {
                QMap<QString, bool> perm = existing;
                perm[axis.defineName] = (option == "ON" || option == "TRUE" || option == "1" || option == "HIGH");
                if (!perm[axis.defineName] && option != "OFF" && option != "FALSE" && option != "0" && option != "LOW") {
                    // Try parsing as number
                    bool ok;
                    int val = option.toInt(&ok);
                    if (ok) perm[axis.defineName] = (val != 0);
                }
                newResult.append(perm);
            }
        }
        result = newResult;
    }
    return result;
}

QVector<QMap<QString, bool>> ShaderPermutationManager::generatePermutations(const QVector<QString>& enabledAxes) const {
    QVector<PermutationAxis> filtered;
    for (const auto& axis : m_axes) {
        if (enabledAxes.contains(axis.name)) filtered.append(axis);
    }
    
    QVector<QMap<QString, bool>> result;
    result.append(QMap<QString, bool>());
    
    for (const auto& axis : filtered) {
        QVector<QMap<QString, bool>> newResult;
        for (const auto& existing : result) {
            for (const auto& option : axis.options) {
                QMap<QString, bool> perm = existing;
                perm[axis.defineName] = (option == "ON" || option == "TRUE" || option == "1" || option == "HIGH");
                newResult.append(perm);
            }
        }
        result = newResult;
    }
    return result;
}

void ShaderPermutationManager::setFilter(FilterFunction filter) {
    m_filter = filter;
}

QVector<ShaderPermutation> ShaderPermutationManager::compilePermutations(const QUuid& graphId, const QVector<QMap<QString, bool>>& permutations) {
    QVector<ShaderPermutation> results;
    results.reserve(permutations.size());
    
    int current = 0;
    for (const auto& perm : permutations) {
        if (m_filter && !m_filter(perm)) continue;
        
        uint32_t hash = computeHash(perm);
        
        // Check cache
        if (m_cache.contains(hash)) {
            m_stats.cacheHits++;
            results.append(m_cache[hash]);
            continue;
        }
        
        m_stats.cacheMisses++;
        
        // Compile
        ShaderPermutation result = ShaderGraphManager::instance()->compilePermutation(graphId, perm);
        result.compileTime = QDateTime::currentDateTime();
        
        // Cache
        cachePermutation(result);
        
        results.append(result);
        m_stats.totalPermutations++;
        if (result.isValid) m_stats.compiledPermutations++;
        else m_stats.failedPermutations++;
        
        current++;
        emit progressChanged(current, permutations.size());
    }
    
    emit allPermutationsCompleted(permutations.size(), m_stats.compiledPermutations, m_stats.failedPermutations);
    return results;
}

void ShaderPermutationManager::cachePermutation(const ShaderPermutation& permutation) {
    uint32_t hash = computeHash(permutation.defines);
    m_cache[hash] = permutation;
}

ShaderPermutation ShaderPermutationManager::getCachedPermutation(uint32_t hash) const {
    return m_cache.value(hash);
}

void ShaderPermutationManager::clearCache() {
    m_cache.clear();
}

void ShaderPermutationManager::saveCache(const QString& filePath) {
    QJsonObject obj;
    QJsonArray arr;
    for (auto it = m_cache.constBegin(); it != m_cache.constEnd(); ++it) {
        QJsonObject p;
        p["hash"] = QString::number(it.key());
        p["name"] = it.value().name;
        p["vsSource"] = it.value().vsSource;
        p["fsSource"] = it.value().fsSource;
        p["gsSource"] = it.value().gsSource;
        p["csSource"] = it.value().csSource;
        p["isValid"] = it.value().isValid;
        p["error"] = it.value().error;
        p["compileTime"] = it.value().compileTime.toString(Qt::ISODate);
        
        QJsonObject defines;
        for (auto dit = it.value().defines.constBegin(); dit != it.value().defines.constEnd(); ++dit) {
            defines[dit.key()] = dit.value();
        }
        p["defines"] = defines;
        
        arr.append(p);
    }
    obj["cache"] = arr;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void ShaderPermutationManager::loadCache(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (doc.isObject()) {
        QJsonArray arr = doc.object()["cache"].toArray();
        for (const auto& v : arr) {
            QJsonObject p = v.toObject();
            ShaderPermutation perm;
            perm.name = p["name"].toString();
            perm.vsSource = p["vsSource"].toString();
            perm.fsSource = p["fsSource"].toString();
            perm.gsSource = p["gsSource"].toString();
            perm.csSource = p["csSource"].toString();
            perm.isValid = p["isValid"].toBool();
            perm.error = p["error"].toString();
            perm.compileTime = QDateTime::fromString(p["compileTime"].toString(), Qt::ISODate);
            
            QJsonObject defines = p["defines"].toObject();
            for (auto it = defines.constBegin(); it != defines.constEnd(); ++it) {
                perm.defines[it.key()] = it.value().toBool();
            }
            
            uint32_t hash = computeHash(perm.defines);
            m_cache[hash] = perm;
        }
    }
}

ShaderPermutationManager::Stats ShaderPermutationManager::getStats() const {
    return m_stats;
}

uint32_t ShaderPermutationManager::computeHash(const QMap<QString, bool>& defines) const {
    QCryptographicHash hash(QCryptographicHash::Md5);
    QMap<QString, bool> sorted = defines; // QMap is already sorted by key
    for (auto it = sorted.constBegin(); it != sorted.constEnd(); ++it) {
        hash.addData(it.key().toUtf8());
        hash.addData(it.value() ? "1" : "0");
    }
    return *reinterpret_cast<const uint32_t*>(hash.result().constData());
}

// ─── ShaderExporter Implementation ──────────────────────────────────────────

static ShaderExporter* s_exporterInstance = nullptr;

ShaderExporter* ShaderExporter::instance() {
    if (!s_exporterInstance) s_exporterInstance = new ShaderExporter();
    return s_exporterInstance;
}

ShaderExporter::ShaderExporter(QObject* parent) : QObject(parent) {}

ShaderExporter::~ShaderExporter() {
    s_exporterInstance = nullptr;
}

bool ShaderExporter::exportGraph(const QUuid& graphId, const ShaderExportOptions& options) {
    return ShaderGraphManager::instance()->exportToFiles(graphId, options);
}

bool ShaderExporter::exportPermutation(const ShaderPermutation& permutation, const ShaderExportOptions& options) {
    // Export a specific permutation
    QDir dir(options.outputDirectory);
    if (!dir.exists()) dir.mkpath(".");
    
    QString content = permutation.fsSource.isEmpty() ? permutation.vsSource : permutation.fsSource;
    QString fileName = permutation.name + ".glsl";
    QFile file(dir.filePath(fileName));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(content.toUtf8());
        file.close();
        return true;
    }
    return false;
}

bool ShaderExporter::exportAllPermutations(const QUuid& graphId, const ShaderExportOptions& options) {
    // Would need to compile all permutations first
    return exportGraph(graphId, options);
}

QString ShaderExporter::generateTemplate(ShaderExportOptions::Target target, const QString& shaderType) {
    if (target == ShaderExportOptions::Target::GLSL) {
        return R"(#version 450 core

// {{SHADER_TYPE}} Shader Template
// Generated by ksEditor ShaderExporter

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec2 vTexCoord;

layout(std140, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 cameraPos;
};

layout(std140, binding = 1) uniform Material {
    vec4 baseColor;
    float metallic;
    float roughness;
    float alpha;
    vec3 emissive;
};

void main() {
    vNormal = normalize(mat3(model) * aNormal);
    vTexCoord = aTexCoord;
    gl_Position = viewProjection * model * vec4(aPosition, 1.0);
}
)";
    }
    return QString();
}

bool ShaderExporter::validateExport(const QUuid& graphId, ShaderExportOptions::Target target, QString* error) {
    auto* graph = ShaderGraphManager::instance()->getGraph(graphId);
    if (!graph) {
        if (error) *error = "Graph not found";
        return false;
    }
    
    auto validation = ShaderGraphManager::instance()->validateGraph(graphId);
    if (!validation.valid) {
        if (error) {
            QStringList errors;
            for (auto it = validation.errors.constBegin(); it != validation.errors.constEnd(); ++it) {
                errors.append(it.key().toString() + ": " + it.value());
            }
            *error = errors.join("\n");
        }
        return false;
    }
    
    return true;
}

} // namespace material
} // namespace ks