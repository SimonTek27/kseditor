#include "GeometryNodes.h"
#include <QDebug>
#include <QUuid>
#include <QRandomGenerator>
#include <QQueue>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QQuaternion>
#include <QMatrix4x4>

namespace ks {
namespace geometry_nodes {

Node::Node(NodeType t)
    : type(t)
{
    id = QUuid::createUuid();
}

QVariant Node::getProperty(const QString& name, const QVariant& defaultValue) const
{
    return properties.value(name, defaultValue);
}

void Node::setProperty(const QString& name, const QVariant& value)
{
    properties[name] = value;
}

NodeTree::NodeTree(const QString& name, QObject* parent)
    : QObject(parent)
    , m_name(name)
{
}

NodeTree::~NodeTree()
{
    for (Node* node : m_nodes) {
        delete node;
    }
}

void NodeTree::clear()
{
    for (Node* node : m_nodes) {
        delete node;
    }
    m_nodes.clear();
    m_nodeMap.clear();
}

void NodeTree::clearCaches()
{
    emit treeChanged();
}

Node* NodeTree::addNode(NodeType type, int x, int y)
{
    Node* node = new Node(type);
    node->positionX = x;
    node->positionY = y;

    m_nodes.append(node);
    m_nodeMap[node->id] = node;

    return node;
}

void NodeTree::removeNode(const QUuid& id)
{
    if (m_nodeMap.contains(id)) {
        Node* node = m_nodeMap[id];
        m_nodes.removeOne(node);
        m_nodeMap.remove(id);
        delete node;
    }
}

Node* NodeTree::getNode(const QUuid& id)
{
    return m_nodeMap.value(id, nullptr);
}

void NodeTree::connectSockets(Node* fromNode, int fromSocket, Node* toNode, int toSocket)
{
    if (!fromNode || !toNode) return;
    
    Connection conn;
    conn.fromNode = fromNode->id;
    conn.fromSocket = fromSocket;
    conn.toNode = toNode->id;
    conn.toSocket = toSocket;
    
    m_connections.append(conn);
    emit nodeGraphModified();
}

void NodeTree::disconnectSockets(Node* fromNode, int fromSocket, Node* toNode, int toSocket)
{
    if (!fromNode || !toNode) return;
    
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
            [&](const Connection& c) {
                return c.fromNode == fromNode->id && c.fromSocket == fromSocket &&
                       c.toNode == toNode->id && c.toSocket == toSocket;
            }),
        m_connections.end()
    );
    
    emit nodeGraphModified();
}

MeshData NodeTree::execute(MeshData input)
{
    MeshData output = input;

    sortNodesByDependency();

    for (Node* node : m_nodes) {
        output = processNode(node, output);
    }

    return output;
}

void NodeTree::sortNodesByDependency()
{
    if (m_nodes.size() <= 1) return;
    
    QMap<QUuid, QVector<QUuid>> adjList;
    QMap<QUuid, int> inDegree;
    
    for (Node* node : m_nodes) {
        adjList[node->id] = {};
        inDegree[node->id] = 0;
    }
    
    for (const Connection& conn : m_connections) {
        if (adjList.contains(conn.fromNode)) {
            adjList[conn.fromNode].append(conn.toNode);
            inDegree[conn.toNode]++;
        }
    }
    
    QQueue<QUuid> queue;
    for (auto it = inDegree.begin(); it != inDegree.end(); ++it) {
        if (it.value() == 0) {
            queue.enqueue(it.key());
        }
    }
    
    QVector<Node*> sorted;
    while (!queue.isEmpty()) {
        QUuid id = queue.dequeue();
        if (Node* node = getNode(id)) {
            sorted.append(node);
        }
        for (const QUuid& neighbor : adjList[id]) {
            inDegree[neighbor]--;
            if (inDegree[neighbor] == 0) {
                queue.enqueue(neighbor);
            }
        }
    }
    
    if (sorted.size() == m_nodes.size()) {
        m_nodes = sorted;
    }
}

MeshData NodeTree::processNode(Node* node, MeshData input)
{
    if (!node) return input;

    switch (node->type) {
        case NodeType::MeshPrimitive: {
            return MeshOperations::createSphere(0.5f, 32, 16);
        }
        case NodeType::TransformOp: {
            QVector3D pos = getSocketValue(node, 0).value<QVector3D>();
            QVector3D rot = getSocketValue(node, 1).value<QVector3D>();
            QVector3D scl = getSocketValue(node, 2).value<QVector3D>();
            if (qIsNaN(scl.x())) scl = QVector3D(1, 1, 1);
            QMatrix4x4 transform;
            transform.translate(pos);
            transform.rotate(QQuaternion::fromEulerAngles(rot));
            transform.scale(scl);
            for (Vertex& v : input.vertices) {
                v.position = transform.map(v.position);
            }
            break;
        }
        case NodeType::SetPosition: {
            QVector3D offset = getSocketValue(node, 0).value<QVector3D>();
            for (Vertex& v : input.vertices) {
                v.position += offset;
            }
            break;
        }
        case NodeType::Random: {
            MeshData result;
            int count = getSocketValue(node, 0).toInt();
            if (count <= 0) count = 100;
            for (int i = 0; i < count; ++i) {
                Vertex v;
                v.position = QVector3D(
                    QRandomGenerator::global()->generateDouble() * 10 - 5,
                    QRandomGenerator::global()->generateDouble() * 10 - 5,
                    QRandomGenerator::global()->generateDouble() * 10 - 5);
                result.vertices.append(v);
            }
            return result;
        }
        case NodeType::MathOp: {
            float a = getSocketValue(node, 1).toFloat();
            float b = getSocketValue(node, 2).toFloat();
            int op = getSocketValue(node, 0).toInt();
            for (Vertex& v : input.vertices) {
                switch (op) {
                    case 0: v.position.setX(v.position.x() + a + b); break;
                    case 1: v.position.setX(v.position.x() - b); break;
                    case 2: v.position.setX(v.position.x() * b); break;
                    case 3: if (b != 0) v.position.setX(v.position.x() / b); break;
                    default: break;
                }
            }
            break;
        }
        case NodeType::InstanceOnPoints: {
            int count = getSocketValue(node, 0).toInt();
            if (count <= 0) count = 1;
            MeshData result;
            for (const Vertex& v : input.vertices) {
                for (int i = 0; i < count; ++i) {
                    result.vertices.append(v);
                }
            }
            return result;
        }
        case NodeType::MeshToPoints: {
            int count = getSocketValue(node, 0).toInt();
            if (count <= 0) count = 10;
            MeshData result;
            int step = qMax(1, input.vertices.size() / count);
            for (int i = 0; i < input.vertices.size(); i += step) {
                result.vertices.append(input.vertices[i]);
            }
            return result;
        }
        case NodeType::DuplicateElements: {
            int count = getSocketValue(node, 0).toInt();
            if (count <= 0) count = 2;
            MeshData result = input;
            int origSize = result.vertices.size();
            for (int i = 0; i < count - 1 && origSize > 0; ++i) {
                for (int j = 0; j < origSize; ++j) {
                    result.vertices.append(input.vertices[j]);
                }
            }
            return result;
        }
        case NodeType::FillCurve: {
            int segments = getSocketValue(node, 0).toInt();
            if (segments <= 0) segments = 16;
            MeshData result;
            if (input.vertices.size() >= 2) {
                for (int i = 0; i < input.vertices.size(); ++i) {
                    int next = (i + 1) % input.vertices.size();
                    const QVector3D& p0 = input.vertices[i].position;
                    const QVector3D& p1 = input.vertices[next].position;
                    for (int j = 0; j < segments; ++j) {
                        float t = (float)j / segments;
                        Vertex v;
                        v.position = p0 + (p1 - p0) * t;
                        result.vertices.append(v);
                    }
                }
            }
            return result;
        }
        case NodeType::VolumeToMesh: {
            int resolution = getSocketValue(node, 0).toInt();
            if (resolution <= 0) resolution = 4;
            float size = getSocketValue(node, 1).toFloat();
            if (size <= 0) size = 2.0f;
            float half = size / 2.0f;
            float step = size / resolution;
            MeshData result;
            for (int x = 0; x <= resolution; ++x) {
                for (int y = 0; y <= resolution; ++y) {
                    for (int z = 0; z <= resolution; ++z) {
                        Vertex v;
                        v.position = QVector3D(
                            -half + x * step, -half + y * step, -half + z * step);
                        result.vertices.append(v);
                    }
                }
            }
            return result;
        }
        case NodeType::WriteSetShadeSmooth: {
            bool smooth = getSocketValue(node, 0).toBool();
            for (auto& v : input.vertices) {
                v.normal = smooth ? QVector3D(0, 1, 0) : QVector3D(0, 0, 0);
            }
            break;
        }
        default:
            break;
    }

    return input;
}

QVariant NodeTree::getSocketValue(Node* node, int socketIndex)
{
    if (node && socketIndex >= 0 && socketIndex < node->outputs.size()) {
        return node->outputs[socketIndex].value;
    }
    return QVariant();
}

QString NodeTree::toJson() const
{
    QJsonObject root;
    QJsonArray nodesArray;
    QJsonArray connectionsArray;

    for (const Node* node : m_nodes) {
        QJsonObject obj;
        obj["type"] = static_cast<int>(node->type);
        obj["positionX"] = node->positionX;
        obj["positionY"] = node->positionY;
        QJsonArray inputs;
        for (const auto& socket : node->inputs)
            inputs.append(QJsonValue::fromVariant(socket.value));
        obj["inputs"] = inputs;
        nodesArray.append(obj);
    }

    for (const auto& conn : m_connections) {
        QJsonObject obj;
        obj["fromNode"] = conn.fromNode.toString();
        obj["toNode"] = conn.toNode.toString();
        obj["fromSocket"] = conn.fromSocket;
        obj["toSocket"] = conn.toSocket;
        connectionsArray.append(obj);
    }

    root["nodes"] = nodesArray;
    root["connections"] = connectionsArray;
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

void NodeTree::fromJson(const QString& json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) return;
    
    QJsonObject root = doc.object();
    QJsonArray nodesArray = root["nodes"].toArray();
    QJsonArray connectionsArray = root["connections"].toArray();
    
    // Clear existing data
    clear();
    
    // Load nodes
    for (const QJsonValue& val : nodesArray) {
        QJsonObject obj = val.toObject();
        NodeType type = static_cast<NodeType>(obj["type"].toInt());
        int x = obj["positionX"].toInt();
        int y = obj["positionY"].toInt();
        Node* node = addNode(type, x, y);
        
        // Load socket values
        QJsonArray inputs = obj["inputs"].toArray();
        for (int i = 0; i < qMin(inputs.size(), node->inputs.size()); ++i) {
            node->inputs[i].value = inputs[i].toVariant();
        }
    }
    
    // Load connections
    for (const QJsonValue& val : connectionsArray) {
        QJsonObject obj = val.toObject();
        QUuid fromId(obj["fromNode"].toString());
        QUuid toId(obj["toNode"].toString());
        int fromSocket = obj["fromSocket"].toInt();
        int toSocket = obj["toSocket"].toInt();
        Node* fromNode = getNode(fromId);
        Node* toNode = getNode(toId);
        if (fromNode && toNode) {
            connectSockets(fromNode, fromSocket, toNode, toSocket);
        }
    }
    
    emit treeChanged();
}

NodeEditor::NodeEditor(QObject* parent)
    : QObject(parent)
{
}

NodeEditor::~NodeEditor()
{
    for (NodeTree* tree : m_library) {
        delete tree;
    }
}

void NodeEditor::newTree(const QString& name)
{
    NodeTree* tree = new NodeTree(name, this);
    m_library.append(tree);
    m_activeTree = tree;
    emit treeAdded(tree);
}

void NodeEditor::loadTree(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[NodeEditor] Failed to load tree:" << path;
        return;
    }
    
    QString json = file.readAll();
    file.close();
    
    NodeTree* tree = new NodeTree(QFileInfo(path).baseName(), this);
    tree->fromJson(json);
    m_library.append(tree);
    m_activeTree = tree;
    emit treeAdded(tree);
}

void NodeEditor::saveTree(const QString& path)
{
    if (!m_activeTree) return;
    
    QString json = m_activeTree->toJson();
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[NodeEditor] Failed to save tree:" << path;
        return;
    }
    
    file.write(json.toUtf8());
    file.close();
}

static NodeType stringToNodeType(const QString& type)
{
    if (type == "MeshPrimitive") return NodeType::MeshPrimitive;
    if (type == "Transform") return NodeType::TransformOp;
    if (type == "SetPosition") return NodeType::SetPosition;
    if (type == "Random") return NodeType::Random;
    if (type == "Math") return NodeType::MathOp;
    if (type == "VectorMath") return NodeType::VectorMath;
    if (type == "MeshToPoints") return NodeType::MeshToPoints;
    if (type == "CurveToPoints") return NodeType::CurveToPoints;
    if (type == "DistributePoints") return NodeType::DistributePoints;
    if (type == "DuplicateElements") return NodeType::DuplicateElements;
    if (type == "CurvePrimitive") return NodeType::Curveprimitive;
    if (type == "FillCurve") return NodeType::FillCurve;
    if (type == "VolumeToMesh") return NodeType::VolumeToMesh;
    if (type == "InstanceOnPoints") return NodeType::InstanceOnPoints;
    if (type == "SetShadeSmooth") return NodeType::WriteSetShadeSmooth;
    if (type == "PointCount") return NodeType::PointCount;
    if (type == "SplineLerp") return NodeType::SplineLerp;
    if (type == "InputPosition") return NodeType::InputPosition;
    if (type == "InputNormal") return NodeType::InputNormal;
    if (type == "InputUV") return NodeType::InputUV;
    return NodeType::MeshPrimitive;
}

void NodeEditor::addNode(const QString& type, int x, int y)
{
    if (m_activeTree) {
        NodeType nodeType = stringToNodeType(type);
        m_activeTree->addNode(nodeType, x, y);
    }
}

void NodeEditor::removeNode(const QString& nodeId)
{
    if (!m_activeTree) return;
    QUuid id(nodeId);
    if (!id.isNull()) {
        m_activeTree->removeNode(id);
        emit treeModified();
    }
}

void NodeEditor::connectNodes(const QString& fromNode, int fromSocket, const QString& toNode, int toSocket)
{
    if (!m_activeTree) return;
    QUuid fromId(fromNode);
    QUuid toId(toNode);
    Node* from = m_activeTree->getNode(fromId);
    Node* to = m_activeTree->getNode(toId);
    if (from && to) {
        m_activeTree->connectSockets(from, fromSocket, to, toSocket);
        emit treeModified();
    }
}

void NodeEditor::setSocketValue(const QString& nodeId, int socketIndex, const QVariant& value)
{
    if (!m_activeTree) return;
    QUuid id(nodeId);
    Node* node = m_activeTree->getNode(id);
    if (node && socketIndex >= 0 && socketIndex < node->inputs.size()) {
        node->inputs[socketIndex].value = value;
    }
}

QVariant NodeEditor::getSocketValue(const QString& nodeId, int socketIndex)
{
    if (!m_activeTree) return QVariant();
    QUuid id(nodeId);
    Node* node = m_activeTree->getNode(id);
    if (node && socketIndex >= 0 && socketIndex < node->inputs.size()) {
        return node->inputs[socketIndex].value;
    }
    return QVariant();
}

void NodeEditor::addToLibrary(NodeTree* tree)
{
    m_library.append(tree);
}

void NodeEditor::removeFromLibrary(int index)
{
    if (index >= 0 && index < m_library.size()) {
        m_library.removeAt(index);
    }
}

GeometryNodeExecutor::GeometryNodeExecutor(QObject* parent)
    : QObject(parent)
{
}

GeometryNodeExecutor::~GeometryNodeExecutor() = default;

MeshData GeometryNodeExecutor::execute(NodeTree* tree, const MeshData& input)
{
    m_currentInput = input;
    m_currentOutput = input;

    if (!tree) return input;

    emit executionStarted();

    MeshData result = tree->execute(input);

    emit executionComplete();
    return result;
}

MeshData GeometryNodeExecutor::executeTransform(Node* node, const MeshData& input)
{
    QMatrix4x4 transform;
    transform.setToIdentity();

    QVector3D pos = getVectorInput(node, 0, QVector3D(0, 0, 0));
    QVector3D rot = getVectorInput(node, 1, QVector3D(0, 0, 0));
    QVector3D scl = getVectorInput(node, 2, QVector3D(1, 1, 1));

    transform.translate(pos);
    transform.rotate(QQuaternion::fromEulerAngles(rot));
    transform.scale(scl);

    QMatrix3x3 nm = transform.normalMatrix();

    MeshData result = input;
    for (auto& v : result.vertices) {
        v.position = transform * v.position;
        v.normal = QVector3D(
            nm(0,0)*v.normal.x() + nm(0,1)*v.normal.y() + nm(0,2)*v.normal.z(),
            nm(1,0)*v.normal.x() + nm(1,1)*v.normal.y() + nm(1,2)*v.normal.z(),
            nm(2,0)*v.normal.x() + nm(2,1)*v.normal.y() + nm(2,2)*v.normal.z()
        ).normalized();
    }
    return result;
}

MeshData GeometryNodeExecutor::executeMeshPrimitive(Node* node)
{
    if (!node) return MeshOperations::createSphere(0.5f, 32, 16);

    QString shape = node->getProperty("shape", "Sphere").toString();
    float size = node->getProperty("size", 1.0f).toFloat();
    int segments = node->getProperty("segments", 32).toInt();

    if (shape == "Cube") {
        return MeshOperations::createBox(size, size, size);
    } else if (shape == "Cylinder") {
        return MeshOperations::createCylinder(size * 0.5f, size, segments);
    } else if (shape == "Cone") {
        return MeshOperations::createCone(size * 0.5f, size, segments);
    } else if (shape == "Torus") {
        return MeshOperations::createTorus(size * 0.5f, size * 0.2f, segments, segments / 2);
    } else if (shape == "Plane") {
        return MeshOperations::createPlane(size, size, 1, 1);
    } else if (shape == "Grid") {
        return MeshOperations::createGrid(size, size, segments / 4, segments / 4);
    } else if (shape == "Icosphere") {
        return MeshOperations::createIcosphere(size * 0.5f, 2);
    } else {
        return MeshOperations::createSphere(size * 0.5f, segments, segments / 2);
    }
}

MeshData GeometryNodeExecutor::executePosition(Node* node, const MeshData& input)
{
    QVector3D offset = getVectorInput(node, 0, QVector3D(0, 0, 0));
    MeshData result = input;
    for (auto& v : result.vertices) {
        v.position += offset;
    }
    return result;
}

MeshData GeometryNodeExecutor::executeSetPosition(Node* node, const MeshData& input)
{
    MeshData result = input;
    QVector3D offset = getVectorInput(node, 0, QVector3D(0, 0, 0));

    for (auto& v : result.vertices) {
        v.position += offset;
    }
    return result;
}

MeshData GeometryNodeExecutor::executeRandom(Node* node)
{
    MeshData result;
    int count = getIntInput(node, 0, 100);

    for (int i = 0; i < count; ++i) {
        Vertex v;
        v.position = QVector3D(
            randomFloat(i) * 2.0f - 1.0f,
            randomFloat(i + 1000) * 2.0f - 1.0f,
            randomFloat(i + 2000) * 2.0f - 1.0f
        );
        v.normal = QVector3D(0, 1, 0);
        result.vertices.append(v);
    }

    return result;
}

MeshData GeometryNodeExecutor::executeMath(Node* node)
{
    MeshData result;
    int operation = getIntInput(node, 0, 0);
    float a = getFloatInput(node, 1, 0.0f);
    float b = getFloatInput(node, 2, 0.0f);
    float output = 0.0f;

    switch (operation) {
    case 0: output = a + b; break;
    case 1: output = a - b; break;
    case 2: output = a * b; break;
    case 3: output = (b != 0.0f) ? a / b : 0.0f; break;
    case 4: output = std::fmod(a, b); break;
    case 5: output = std::pow(a, b); break;
    case 6: output = std::min(a, b); break;
    case 7: output = std::max(a, b); break;
    default: output = a + b; break;
    }

    Vertex v;
    v.position = QVector3D(output, 0, 0);
    result.vertices.append(v);
    return result;
}

MeshData GeometryNodeExecutor::executeCurveprimitive(Node* node)
{
    MeshData result;
    int segments = getIntInput(node, 0, 8);
    float radius = getFloatInput(node, 1, 1.0f);
    float height = getFloatInput(node, 2, 2.0f);

    for (int i = 0; i <= segments; ++i) {
        float angle = (2.0f * M_PI * i) / segments;
        Vertex v;
        v.position = QVector3D(
            radius * std::cos(angle),
            height * ((float)i / segments),
            radius * std::sin(angle)
        );
        v.normal = QVector3D(std::cos(angle), 0, std::sin(angle));
        result.vertices.append(v);
    }
    return result;
}

MeshData GeometryNodeExecutor::executeInstanceOnPoints(Node* node, const MeshData& input)
{
    MeshData result = input;
    int instanceCount = getIntInput(node, 0, 1);

    for (const Vertex& v : input.vertices) {
        for (int i = 0; i < instanceCount; ++i) {
            Vertex iv = v;
            iv.position += QVector3D(
                randomFloat(i) * 2.0f - 1.0f,
                0,
                randomFloat(i + 500) * 2.0f - 1.0f
            );
            result.vertices.append(iv);
        }
    }
    return result;
}

MeshData GeometryNodeExecutor::executeMeshToPoints(Node* node, const MeshData& input)
{
    MeshData output;

    for (const Vertex& v : input.vertices) {
        Vertex pv;
        pv.position = v.position;
        pv.normal = v.normal;
        output.vertices.append(pv);
    }

    return output;
}

MeshData GeometryNodeExecutor::executeCurveToPoints(Node* node, const MeshData& input)
{
    MeshData result;
    int count = getIntInput(node, 0, 10);

    for (const Vertex& v : input.vertices) {
        result.vertices.append(v);
        if (result.vertices.size() >= count) break;
    }

    return result;
}

MeshData GeometryNodeExecutor::executeDistributePoints(Node* node)
{
    MeshData result;
    int count = getIntInput(node, 0, 100);

    for (int i = 0; i < count; ++i) {
        Vertex v;
        v.position = QVector3D(
            randomFloat(i) * 2.0f - 1.0f,
            0,
            randomFloat(i + 500) * 2.0f - 1.0f
        );
        result.vertices.append(v);
    }

    return result;
}

MeshData GeometryNodeExecutor::executeDuplicateElements(Node* node, const MeshData& input)
{
    MeshData result;
    int count = getIntInput(node, 0, 2);

    for (int c = 0; c < count; ++c) {
        for (const Vertex& v : input.vertices) {
            Vertex dv = v;
            dv.position += QVector3D(
                (float)c * 2.0f,
                0,
                0
            );
            result.vertices.append(dv);
        }
    }
    return result;
}

MeshData GeometryNodeExecutor::executeFillCurve(Node* node)
{
    MeshData result;
    int segments = getIntInput(node, 0, 16);

    for (int i = 0; i < segments; ++i) {
        float angle1 = (2.0f * M_PI * i) / segments;
        float angle2 = (2.0f * M_PI * (i + 1)) / segments;

        Vertex v0;
        v0.position = QVector3D(0, 0, 0);
        result.vertices.append(v0);

        Vertex v1;
        v1.position = QVector3D(std::cos(angle1), 0, std::sin(angle1));
        result.vertices.append(v1);

        Vertex v2;
        v2.position = QVector3D(std::cos(angle2), 0, std::sin(angle2));
        result.vertices.append(v2);
    }
    return result;
}

MeshData GeometryNodeExecutor::executeVolumeToMesh(Node* node)
{
    MeshData result;
    int resolution = getIntInput(node, 0, 4);
    float size = getFloatInput(node, 1, 2.0f);

    for (int x = 0; x < resolution; ++x) {
        for (int y = 0; y < resolution; ++y) {
            for (int z = 0; z < resolution; ++z) {
                Vertex v;
                v.position = QVector3D(
                    size * ((float)x / resolution - 0.5f),
                    size * ((float)y / resolution - 0.5f),
                    size * ((float)z / resolution - 0.5f)
                );
                v.normal = QVector3D(0, 1, 0);
                result.vertices.append(v);
            }
        }
    }
    return result;
}

MeshData GeometryNodeExecutor::executeSetShadeSmooth(Node* node, const MeshData& input)
{
    bool smooth = getBoolInput(node, 0, true);
    MeshData result = input;

    if (smooth && !result.vertices.isEmpty()) {
        // Average normals at shared vertices for smooth shading
        QMap<QPair<float, float>, QVector3D> normalAccum;
        for (const auto& v : result.vertices) {
            auto key = qMakePair(qRound(v.position.x() * 1000) / 1000.0f,
                                  qRound(v.position.y() * 1000) / 1000.0f);
            normalAccum[key] += v.normal;
        }
        for (auto& v : result.vertices) {
            auto key = qMakePair(qRound(v.position.x() * 1000) / 1000.0f,
                                  qRound(v.position.y() * 1000) / 1000.0f);
            if (normalAccum.contains(key)) {
                v.normal = normalAccum[key].normalized();
            }
        }
    }
    return result;
}

float GeometryNodeExecutor::getFloatInput(Node* node, int index, float defaultValue)
{
    if (node && index >= 0 && index < node->inputs.size()) {
        const Socket& socket = node->inputs[index];
        if (socket.value.isValid() && socket.value.canConvert<float>()) {
            return socket.value.toFloat();
        }
    }
    return defaultValue;
}

int GeometryNodeExecutor::getIntInput(Node* node, int index, int defaultValue)
{
    if (node && index >= 0 && index < node->inputs.size()) {
        const Socket& socket = node->inputs[index];
        if (socket.value.isValid() && socket.value.canConvert<int>()) {
            return socket.value.toInt();
        }
    }
    return defaultValue;
}

bool GeometryNodeExecutor::getBoolInput(Node* node, int index, bool defaultValue)
{
    if (node && index >= 0 && index < node->inputs.size()) {
        const Socket& socket = node->inputs[index];
        if (socket.value.isValid() && socket.value.canConvert<bool>()) {
            return socket.value.toBool();
        }
    }
    return defaultValue;
}

QVector3D GeometryNodeExecutor::getVectorInput(Node* node, int index, const QVector3D& defaultValue)
{
    if (node && index >= 0 && index < node->inputs.size()) {
        const Socket& socket = node->inputs[index];
        if (socket.value.isValid()) {
            QVariantList list = socket.value.toList();
            if (list.size() >= 3) {
                return QVector3D(list[0].toFloat(), list[1].toFloat(), list[2].toFloat());
            }
        }
    }
    return defaultValue;
}

float GeometryNodeExecutor::randomFloat(int seed)
{
    // Use seed for reproducible random values
    QRandomGenerator gen(static_cast<quint32>(seed));
    return (float)(gen.bounded(1000)) / 1000.0f;
}

QVector3D GeometryNodeExecutor::randomVector3D(int seed)
{
    return QVector3D(
        randomFloat(seed),
        randomFloat(seed + 1),
        randomFloat(seed + 2)
    );
}

bool GeometryNodeExecutor::isFloatSocket(Socket::Type type)
{
    return type == Socket::Type::Float;
}

bool GeometryNodeExecutor::isVectorSocket(Socket::Type type)
{
    return type == Socket::Type::Vector;
}

void NodeSocket::transferData()
{
    if (!m_connectedSocket || !m_node || !m_connectedSocket->getNode()) return;
    auto& outputs = m_connectedSocket->getNode()->outputs;
    QString targetName = m_connectedSocket->getName();
    for (const auto& s : outputs) {
        if (s.name == targetName) {
            m_socket.value = s.value;
            break;
        }
    }
    m_socket.isLinked = true;
}

}
}