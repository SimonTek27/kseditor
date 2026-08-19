#include "ICEParticleSystem.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QHash>
#include <QSet>
#include <QElapsedTimer>
#include <functional>
#include <cmath>
#include <random>
#include <limits>

namespace ks {

// ============================================================================
// ICEParticleGraph
// ============================================================================

void ICEParticleGraph::addNode(const ui::GraphNode& node)
{
    nodes[node.id] = node;
    invalidateEvalOrder();
}

void ICEParticleGraph::removeNode(const QUuid& nodeId)
{
    nodes.remove(nodeId);
    // Remove connected connections
    for (int i = connections.size() - 1; i >= 0; --i) {
        if (connections[i].fromNodeId == nodeId || connections[i].toNodeId == nodeId)
            connections.removeAt(i);
    }
    invalidateEvalOrder();
}

void ICEParticleGraph::addConnection(const ui::GraphConnection& conn)
{
    connections.append(conn);
    invalidateEvalOrder();
}

void ICEParticleGraph::removeConnection(const QUuid& connId)
{
    for (int i = connections.size() - 1; i >= 0; --i) {
        if (connections[i].id == connId) {
            connections.removeAt(i);
            break;
        }
    }
    invalidateEvalOrder();
}

void ICEParticleGraph::clear()
{
    nodes.clear();
    connections.clear();
    invalidateEvalOrder();
}

QVector<QUuid> ICEParticleGraph::getEvalOrder() const
{
    if (evalOrderValid) return evalOrder;

    evalOrder.clear();
    QSet<QUuid> visited;
    QSet<QUuid> visiting;
    std::function<void(const QUuid&)> visit = [&](const QUuid& nodeId) {
        if (visited.contains(nodeId) || visiting.contains(nodeId)) return;
        visiting.insert(nodeId);
        // Visit dependents first (nodes that consume this node's output)
        for (const auto& conn : connections) {
            if (conn.fromNodeId == nodeId)
                visit(conn.toNodeId);
        }
        visiting.remove(nodeId);
        visited.insert(nodeId);
        evalOrder.append(nodeId);
    };

    for (auto it = nodes.constBegin(); it != nodes.constEnd(); ++it)
        visit(it.key());

    evalOrderValid = true;
    return evalOrder;
}

QJsonObject ICEParticleGraph::toJson() const
{
    QJsonObject root;
    root["name"] = name;
    root["id"] = id.toString();

    QJsonArray nodesArr;
    for (auto it = nodes.constBegin(); it != nodes.constEnd(); ++it) {
        QJsonObject n;
        n["id"] = it.key().toString();
        n["type"] = it.value().typeName;
        n["title"] = it.value().title;
        n["x"] = it.value().position.x();
        n["y"] = it.value().position.y();
        QJsonObject props;
        for (auto p = it.value().properties.constBegin(); p != it.value().properties.constEnd(); ++p)
            props[p.key()] = QJsonValue::fromVariant(p.value());
        n["properties"] = props;
        nodesArr.append(n);
    }
    root["nodes"] = nodesArr;

    QJsonArray connsArr;
    for (const auto& c : connections) {
        QJsonObject cj;
        cj["fromNode"] = c.fromNodeId.toString();
        cj["fromPort"] = c.fromPortId.toString();
        cj["toNode"] = c.toNodeId.toString();
        cj["toPort"] = c.toPortId.toString();
        connsArr.append(cj);
    }
    root["connections"] = connsArr;
    return root;
}

bool ICEParticleGraph::fromJson(const QJsonObject& json)
{
    clear();
    name = json["name"].toString();
    id = QUuid(json["id"].toString());
    if (id.isNull()) id = QUuid::createUuid();

    const QJsonArray nodesArr = json["nodes"].toArray();
    for (const auto& nv : nodesArr) {
        QJsonObject n = nv.toObject();
        ui::GraphNode node;
        node.id = QUuid(n["id"].toString());
        if (node.id.isNull()) continue;
        node.typeName = n["type"].toString();
        node.title = n["title"].toString();
        node.position = QPointF(n["x"].toDouble(), n["y"].toDouble());
        QJsonObject props = n["properties"].toObject();
        for (auto p = props.constBegin(); p != props.constEnd(); ++p)
            node.properties[p.key()] = p.value().toVariant();
        nodes[node.id] = node;
    }

    const QJsonArray connsArr = json["connections"].toArray();
    for (const auto& cv : connsArr) {
        QJsonObject c = cv.toObject();
        ui::GraphConnection conn;
        conn.id = QUuid::createUuid();
        conn.fromNodeId = QUuid(c["fromNode"].toString());
        conn.fromPortId = QUuid(c["fromPort"].toString());
        conn.toNodeId = QUuid(c["toNode"].toString());
        conn.toPortId = QUuid(c["toPort"].toString());
        connections.append(conn);
    }
    invalidateEvalOrder();
    return true;
}

// ============================================================================
// ICEParticleState
// ============================================================================

void ICEParticleState::ensureCapacity(int count)
{
    if (count <= positions.size()) return;
    positions.resize(count);
    velocities.resize(count);
    accelerations.resize(count);
    ages.resize(count);
    lifetimes.resize(count);
    sizes.resize(count);
    colors.resize(count);
    masses.resize(count);
    ids.resize(count);
}

int ICEParticleState::allocate(int count)
{
    int start = aliveCount;
    int avail = positions.size() - aliveCount;
    int toAllocate = qMin(count, avail);
    if (toAllocate <= 0) return -1;
    for (int i = start; i < start + toAllocate; ++i) {
        ids[i] = nextId++;
        ages[i] = 0.0f;
    }
    aliveCount += toAllocate;
    return start;
}

void ICEParticleState::kill(int index)
{
    if (index < 0 || index >= aliveCount) return;
    --aliveCount;
    // Move last alive particle into the dead slot
    if (index != aliveCount) {
        positions[index] = positions[aliveCount];
        velocities[index] = velocities[aliveCount];
        accelerations[index] = accelerations[aliveCount];
        ages[index] = ages[aliveCount];
        lifetimes[index] = lifetimes[aliveCount];
        sizes[index] = sizes[aliveCount];
        colors[index] = colors[aliveCount];
        masses[index] = masses[aliveCount];
        ids[index] = ids[aliveCount];
    }
}

void ICEParticleState::compact()
{
    // No-op: alive particles are kept packed at the front
}

// ============================================================================
// ICEParticleEvaluator
// ============================================================================

ICEParticleEvaluator::ICEParticleEvaluator(QObject* parent)
    : QObject(parent)
{
    m_state.ensureCapacity(10000);

    // Register built-in node evaluators
    registerNodeType("ICE.EmitterPoint", ICEParticleEvaluator::evalEmitterPoint);
    registerNodeType("ICE.EmitterSphere", [this](const QUuid& nid, const ui::GraphNode& n,
                                                 ICEParticleState& s, float dt, const QMap<QUuid, QVariant>& pv) {
        evalEmitterSphere(nid, n, s, dt, pv);
    });
    registerNodeType("ICE.EmitterMesh", [this](const QUuid& nid, const ui::GraphNode& n,
                                               ICEParticleState& s, float dt, const QMap<QUuid, QVariant>& pv) {
        evalEmitterMesh(nid, n, s, dt, pv);
    });
    registerNodeType("ICE.EmitterCircle", [this](const QUuid& nid, const ui::GraphNode& n,
                                                 ICEParticleState& s, float dt, const QMap<QUuid, QVariant>& pv) {
        evalEmitterCircle(nid, n, s, dt, pv);
    });
    registerNodeType("ICE.ForceGravity", ICEParticleEvaluator::evalForceGravity);
    registerNodeType("ICE.ForceWind", ICEParticleEvaluator::evalForceWind);
    registerNodeType("ICE.ForceTurbulence", [this](const QUuid& nid, const ui::GraphNode& n,
                                                   ICEParticleState& s, float dt, const QMap<QUuid, QVariant>& pv) {
        evalForceTurbulence(nid, n, s, dt, pv);
    });
    registerNodeType("ICE.ForceDrag", ICEParticleEvaluator::evalForceDrag);
    registerNodeType("ICE.ForceVortex", ICEParticleEvaluator::evalForceVortex);
    registerNodeType("ICE.ForceAttractor", ICEParticleEvaluator::evalForceAttractor);
    registerNodeType("ICE.CollisionPlane", ICEParticleEvaluator::evalCollisionPlane);
    registerNodeType("ICE.CollisionSphere", ICEParticleEvaluator::evalCollisionSphere);
    registerNodeType("ICE.CollisionMesh", [this](const QUuid& nid, const ui::GraphNode& n,
                                                 ICEParticleState& s, float dt, const QMap<QUuid, QVariant>& pv) {
        evalCollisionMesh(nid, n, s, dt, pv);
    });
    registerNodeType("ICE.FilterAge", ICEParticleEvaluator::evalFilterAge);
    registerNodeType("ICE.FilterVelocity", ICEParticleEvaluator::evalFilterVelocity);
    registerNodeType("ICE.FilterRandom", [this](const QUuid& nid, const ui::GraphNode& n,
                                                ICEParticleState& s, float dt, const QMap<QUuid, QVariant>& pv) {
        evalFilterRandom(nid, n, s, dt, pv);
    });
    registerNodeType("ICE.FilterPosition", ICEParticleEvaluator::evalFilterPosition);
    registerNodeType("ICE.OpAdd", ICEParticleEvaluator::evalOpAdd);
    registerNodeType("ICE.OpMultiply", ICEParticleEvaluator::evalOpMultiply);
    registerNodeType("ICE.OpLerp", ICEParticleEvaluator::evalOpLerp);
    registerNodeType("ICE.OpVectorMath", ICEParticleEvaluator::evalOpVectorMath);
    registerNodeType("ICE.OpCurve", ICEParticleEvaluator::evalOpCurve);
    registerNodeType("ICE.FCurve", ICEParticleEvaluator::evalFCurve);
    registerNodeType("ICE.PropColor", ICEParticleEvaluator::evalPropColor);
    registerNodeType("ICE.PropSize", ICEParticleEvaluator::evalPropSize);
    registerNodeType("ICE.PropLifetime", ICEParticleEvaluator::evalPropLifetime);
    registerNodeType("ICE.PropMass", ICEParticleEvaluator::evalPropMass);
    registerNodeType("ICE.OutputPoints", ICEParticleEvaluator::evalOutputPoints);
    registerNodeType("ICE.OutputRibbons", ICEParticleEvaluator::evalOutputPoints);
    registerNodeType("ICE.OutputMesh", ICEParticleEvaluator::evalOutputPoints);
registerNodeType("ICE.Branch", [this](const QUuid& nid, const ui::GraphNode& node,
                                          ICEParticleState& s, float dt, const QMap<QUuid, QVariant>& pv) {
    // Branch node: ratio property (0-1) determines which output branch to follow
    float ratio = node.properties.value("ratio", 0.5).toFloat();
    // Collect output port values
    QVector<QVariant> outputs;
    for (const auto& conn : m_graph.connections) {
        if (conn.fromNodeId == nid) {
            QVariant val = m_portValues.value(conn.toPortId);
            if (val.isValid()) outputs.append(val);
        }
    }
    // If we have at least 2 outputs, pick based on ratio
    if (outputs.size() >= 2) {
        // Simple: if ratio < 0.5 use first output, else second
        // Could also interpolate between them
        if (ratio < 0.5) {
            // Set the first connected input's value to propagate
            // (In a real system, this would connect to specific downstream nodes)
        } else {
            // Use second output
        }
    }
    // Mark as evaluated - don't kill particles, just pass through
});

registerNodeType("ICE.Switch", [this](const QUuid& nid, const ui::GraphNode& node,
                                        ICEParticleState& s, float dt, const QMap<QUuid, QVariant>& pv) {
    // Switch node: "which" property (integer index) selects which output
    int which = node.properties.value("which", 0).toInt();
    // Collect output port values
    QVector<QVariant> outputs;
    for (const auto& conn : m_graph.connections) {
        if (conn.fromNodeId == nid) {
            QVariant val = m_portValues.value(conn.toPortId);
            if (val.isValid()) outputs.append(val);
        }
    }
    // If we have outputs and which is valid, use that output
    if (!outputs.isEmpty() && which >= 0 && which < (int)outputs.size()) {
        // Propagate the selected output value
        // In a full implementation, this would connect to specific downstream nodes
    }
    // Mark as evaluated
});

registerNodeType("ICE.Loop", [this](const QUuid& nid, const ui::GraphNode& node,
                                      ICEParticleState& s, float dt, const QMap<QUuid, QVariant>& pv) {
    // Loop node: re-evaluate the graph portion, effectively creating an iteration
    // For now, just ensure particles aren't killed and the graph continues
    // A full implementation would track iteration count and re-evaluate sub-graphs
    // Connection to "loop start" and "loop end" nodes would enable true looping
    // For this release, just pass through without killing
});
}

ICEParticleEvaluator::~ICEParticleEvaluator() = default;

void ICEParticleEvaluator::setGraph(const ICEParticleGraph& graph)
{
    m_graph = graph;
    m_portValues.clear();
}

void ICEParticleEvaluator::step()
{
    m_accumulator += m_deltaTime * m_timeScale;
    const float frameTime = 1.0f / 60.0f;
    if (m_accumulator < frameTime) return;
    m_accumulator = 0.0f;

    m_portValues.clear();
    evaluateGraph();
    emit particlesUpdated(m_state.aliveCount);
}

void ICEParticleEvaluator::registerNodeType(const QString& typeName, NodeEvalFunc func)
{
    m_nodeEvaluators[typeName] = func;
}

void ICEParticleEvaluator::addParticles(const QVector<QVector3D>& positions, const QVector<QVector3D>& velocities, int count)
{
    int start = m_state.allocate(count);
    if (start < 0) return;
    for (int i = 0; i < count && start + i < positions.size(); ++i) {
        int idx = start + i;
        m_state.positions[idx] = i < positions.size() ? positions[i] : QVector3D();
        m_state.velocities[idx] = i < velocities.size() ? velocities[i] : QVector3D();
        m_state.accelerations[idx] = QVector3D();
        m_state.ages[idx] = 0.0f;
        m_state.lifetimes[idx] = 5.0f;
        m_state.sizes[idx] = 0.05f;
        m_state.colors[idx] = QVector4D(1, 1, 1, 1);
        m_state.masses[idx] = 1.0f;
    }
}

void ICEParticleEvaluator::clearParticles()
{
    m_state.aliveCount = 0;
}

QVector<QVector3D> ICEParticleEvaluator::getPositions() const
{
    QVector<QVector3D> out;
    out.reserve(m_state.aliveCount);
    for (int i = 0; i < m_state.aliveCount; ++i)
        out.append(m_state.positions[i]);
    return out;
}

QVector<QVector4D> ICEParticleEvaluator::getColors() const
{
    QVector<QVector4D> out;
    out.reserve(m_state.aliveCount);
    for (int i = 0; i < m_state.aliveCount; ++i)
        out.append(m_state.colors[i]);
    return out;
}

QVector<float> ICEParticleEvaluator::getSizes() const
{
    QVector<float> out;
    out.reserve(m_state.aliveCount);
    for (int i = 0; i < m_state.aliveCount; ++i)
        out.append(m_state.sizes[i]);
    return out;
}

void ICEParticleEvaluator::evaluateGraph()
{
    QVector<QUuid> order = m_graph.getEvalOrder();
    for (const QUuid& nodeId : order)
        evaluateNode(nodeId);
}

void ICEParticleEvaluator::evaluateNode(const QUuid& nodeId)
{
    auto it = m_graph.nodes.find(nodeId);
    if (it == m_graph.nodes.end()) return;

    auto evalIt = m_nodeEvaluators.find(it->typeName);
    if (evalIt == m_nodeEvaluators.end()) {
        // Unknown node type - skip
        return;
    }

    try {
        evalIt.value()(nodeId, it.value(), m_state, m_deltaTime, m_portValues);
    } catch (...) {
        emit evaluationError("Evaluation error in node: " + it->title);
    }
}

QVariant ICEParticleEvaluator::getInputValue(const QUuid& nodeId, const QString& portName)
{
    // Find connected source port
    for (const auto& conn : m_graph.connections) {
        if (conn.toNodeId == nodeId) {
            // Need to find the port name - stored in graph node
            auto it = m_graph.nodes.find(conn.fromNodeId);
            if (it != m_graph.nodes.end()) {
                QVariant v = m_portValues.value(conn.fromPortId);
                if (v.isValid()) return v;
            }
        }
    }
    return QVariant();
}

void ICEParticleEvaluator::setOutputValue(const QUuid& nodeId, const QString& portName, const QVariant& value)
{
    // Find the output port id for this node
    auto it = m_graph.nodes.find(nodeId);
    if (it == m_graph.nodes.end()) return;
    // Use port name hashed as key if port ids unavailable
    m_portValues[QUuid(portName + nodeId.toString())] = value;
}

// ============================================================================
// Built-in Node Evaluators
// ============================================================================

void ICEParticleEvaluator::evalEmitterPoint(const QUuid&, const ui::GraphNode& node,
                                            ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    QVector3D pos = node.properties.value("position").toList().size() >= 3
        ? QVector3D(node.properties["position"].toList()[0].toFloat(),
                    node.properties["position"].toList()[1].toFloat(),
                    node.properties["position"].toList()[2].toFloat())
        : QVector3D(0, 0, 0);
    float rate = node.properties.value("rate", 100).toFloat();
    int count = int(rate / 60.0f) + 1;

    int start = state.allocate(count);
    if (start < 0) return;
    QVector3D vel = node.properties.value("velocity").toList().size() >= 3
        ? QVector3D(node.properties["velocity"].toList()[0].toFloat(),
                    node.properties["velocity"].toList()[1].toFloat(),
                    node.properties["velocity"].toList()[2].toFloat())
        : QVector3D(0, 1, 0);
    for (int i = 0; i < count; ++i) {
        int idx = start + i;
        state.positions[idx] = pos;
        state.velocities[idx] = vel;
        state.accelerations[idx] = QVector3D();
        state.ages[idx] = 0;
        state.lifetimes[idx] = node.properties.value("lifetime", 5).toFloat();
        state.sizes[idx] = node.properties.value("size", 0.05).toFloat();
        state.colors[idx] = QVector4D(1, 1, 1, 1);
        state.masses[idx] = 1;
    }
}

void ICEParticleEvaluator::setEmitterMesh(const QVector<QVector3D>& triangleVerts)
{
    m_emitterTriangles = triangleVerts;
}

void ICEParticleEvaluator::evalEmitterMesh(const QUuid&, const ui::GraphNode& node,
                                           ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    if (m_emitterTriangles.size() < 3) return;
    const int triCount = m_emitterTriangles.size() / 3;
    float rate = node.properties.value("rate", 100).toFloat();
    float speed = node.properties.value("velocity", 1.0).toFloat();
    int count = int(rate / 60.0f) + 1;

    // Precompute triangle areas for weighted random selection
    static QVector<float> areas;
    if (areas.size() != triCount) {
        areas.resize(triCount);
        float total = 0.0f;
        for (int t = 0; t < triCount; ++t) {
            QVector3D a = m_emitterTriangles[t * 3];
            QVector3D b = m_emitterTriangles[t * 3 + 1];
            QVector3D c = m_emitterTriangles[t * 3 + 2];
            areas[t] = QVector3D::crossProduct(b - a, c - a).length() * 0.5f;
            total += areas[t];
        }
        if (total > 1e-8f)
            for (int t = 0; t < triCount; ++t) areas[t] /= total;
    }

    static std::uniform_real_distribution<float> urnd(0.0f, 1.0f);

    int start = state.allocate(count);
    if (start < 0) return;
    for (int i = 0; i < count; ++i) {
        int idx = start + i;
        // Pick triangle by cumulative area
        float r = urnd(m_rng);
        int t = triCount - 1;
        float acc = 0.0f;
        for (int j = 0; j < triCount; ++j) { acc += areas[j]; if (r <= acc) { t = j; break; } }
        const QVector3D& a = m_emitterTriangles[t * 3];
        const QVector3D& b = m_emitterTriangles[t * 3 + 1];
        const QVector3D& c = m_emitterTriangles[t * 3 + 2];
        float u = urnd(m_rng);
        float v = urnd(m_rng);
        if (u + v > 1.0f) { u = 1.0f - u; v = 1.0f - v; }
        QVector3D pos = a + (b - a) * u + (c - a) * v;
        QVector3D n = QVector3D::crossProduct(b - a, c - a);
        float nl = n.length();
        n = nl > 1e-8f ? n / nl : QVector3D(0, 1, 0);
        state.positions[idx] = pos;
        state.velocities[idx] = n * speed;
        state.accelerations[idx] = QVector3D();
        state.ages[idx] = 0;
        state.lifetimes[idx] = node.properties.value("lifetime", 5).toFloat();
        state.sizes[idx] = node.properties.value("size", 0.05).toFloat();
        state.colors[idx] = QVector4D(1, 1, 1, 1);
        state.masses[idx] = 1;
    }
}

void ICEParticleEvaluator::evalEmitterCircle(const QUuid&, const ui::GraphNode& node,
                                             ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    QVector3D pos = node.properties.value("position").toList().size() >= 3
        ? QVector3D(node.properties["position"].toList()[0].toFloat(),
                    node.properties["position"].toList()[1].toFloat(),
                    node.properties["position"].toList()[2].toFloat())
        : QVector3D(0, 0, 0);
    float radius = qMax(0.0f, node.properties.value("radius", 1.0).toFloat());
    float rate = node.properties.value("rate", 100).toFloat();
    float speed = node.properties.value("velocity", 1.0).toFloat();
    int count = int(rate / 60.0f) + 1;

    std::uniform_real_distribution<float> urnd(0.0f, 1.0f);
    std::uniform_real_distribution<float> ang(0.0f, 6.2831853f);

    int start = state.allocate(count);
    if (start < 0) return;
    for (int i = 0; i < count; ++i) {
        int idx = start + i;
        const float a = ang(m_rng);
        const float r = radius * std::sqrt(urnd(m_rng));
        const float x = pos.x() + std::cos(a) * r;
        const float z = pos.z() + std::sin(a) * r;
        state.positions[idx] = QVector3D(x, pos.y(), z);
        state.velocities[idx] = QVector3D(std::cos(a) * speed, urnd(m_rng) * speed * 0.3f, std::sin(a) * speed);
        state.accelerations[idx] = QVector3D();
        state.ages[idx] = 0;
        state.lifetimes[idx] = node.properties.value("lifetime", 5).toFloat();
        state.sizes[idx] = node.properties.value("size", 0.05).toFloat();
        state.colors[idx] = QVector4D(1, 1, 1, 1);
        state.masses[idx] = 1;
    }
}

void ICEParticleEvaluator::evalFilterRandom(const QUuid&, const ui::GraphNode& node,
                                            ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    float probability = qBound(0.0f, node.properties.value("probability", 0.5).toFloat(), 1.0f);
    if (probability <= 0.0f) return;
    std::uniform_real_distribution<float> urnd(0.0f, 1.0f);
    for (int i = state.aliveCount - 1; i >= 0; --i)
        if (urnd(m_rng) < probability) state.kill(i);
}

void ICEParticleEvaluator::evalFilterPosition(const QUuid&, const ui::GraphNode& node,
                                              ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    QVariantList mn = node.properties.value("min").toList();
    QVariantList mx = node.properties.value("max").toList();
    if (mn.size() < 3 || mx.size() < 3) return;
    const QVector3D minV(mn[0].toFloat(), mn[1].toFloat(), mn[2].toFloat());
    const QVector3D maxV(mx[0].toFloat(), mx[1].toFloat(), mx[2].toFloat());
    const bool killInside = node.properties.value("killInside", false).toBool();
    for (int i = state.aliveCount - 1; i >= 0; --i) {
        const QVector3D& p = state.positions[i];
        const bool inside = p.x() >= minV.x() && p.x() <= maxV.x()
            && p.y() >= minV.y() && p.y() <= maxV.y()
            && p.z() >= minV.z() && p.z() <= maxV.z();
        if (inside == killInside) state.kill(i);
    }
}

void ICEParticleEvaluator::evalOpCurve(const QUuid&, const ui::GraphNode& node,
                                       ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    float power = qBound(0.1f, node.properties.value("power", 2.0).toFloat(), 8.0f);
    float amount = qBound(0.0f, node.properties.value("amount", 1.0).toFloat(), 10.0f);
    bool fade = node.properties.value("fade", true).toBool();
    for (int i = 0; i < state.aliveCount; ++i) {
        const float lt = state.lifetimes[i] > 1e-6f ? state.lifetimes[i] : 1.0f;
        float t = qBound(0.0f, state.ages[i] / lt, 1.0f);
        float curve = fade ? std::pow(1.0f - t, power) : std::pow(t, power);
        const float scale = 1.0f + (curve - 1.0f) * amount;
        state.velocities[i] *= scale;
        state.accelerations[i] *= scale;
    }
}

void ICEParticleEvaluator::evalFCurve(const QUuid&, const ui::GraphNode& node,
                                      ICEParticleState& state, float dt, const QMap<QUuid, QVariant>& pv)
{
    // F-Curve evaluator: evaluates a curve based on particle age fraction
    // Properties:
    //   "controlPoints" - QVariantList of QVector2D (x,y control points)
    //   "loop" - whether the curve loops
    //   "scale" - output scale factor
    //   "offset" - output offset
    QVariantList points = node.properties.value("controlPoints").toList();
    if (points.isEmpty()) return;

    // Parse control points
    QVector<float> kpX, kpY;
    for (const auto& p : points) {
        QVariantList pt = p.toList();
        if (pt.size() >= 2) {
            kpX.append(pt[0].toFloat());
            kpY.append(pt[1].toFloat());
        }
    }

    if (kpX.size() < 2) return;

    // Apply scale and offset
    float scale = node.properties.value("scale", 1.0).toFloat();
    float offset = node.properties.value("offset", 0.0).toFloat();

    for (int i = 0; i < state.aliveCount; ++i) {
        float t = state.ages[i] / state.lifetimes[i];
        t = qBound(0.0f, t, 1.0f);

        // Clamp t to range
        float tClamped = qBound(kpX.first(), kpX.last(), t);

        // Linear interpolation between control points
        float y = kpY.first(); // default to first point
        for (int k = 0; k < kpX.size() - 1; ++k) {
            if (tClamped >= kpX[k] && tClamped <= kpX[k + 1]) {
                float segmentT = (tClamped - kpX[k]) / (kpX[k + 1] - kpX[k]);
                y = kpY[k] + (kpY[k + 1] - kpY[k]) * segmentT;
                break;
            }
        }

        const float factor = 1.0f + (scale - 1.0f) * y + offset;
        state.velocities[i] *= factor;
        state.accelerations[i] *= factor;
    }
}

void ICEParticleEvaluator::evalPropLifetime(const QUuid&, const ui::GraphNode& node,
                                            ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    float lifetime = qMax(0.01f, node.properties.value("lifetime", 5.0).toFloat());
    for (int i = 0; i < state.aliveCount; ++i) state.lifetimes[i] = lifetime;
}

void ICEParticleEvaluator::evalPropMass(const QUuid&, const ui::GraphNode& node,
                                        ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    float mass = qMax(0.001f, node.properties.value("mass", 1.0).toFloat());
    for (int i = 0; i < state.aliveCount; ++i) state.masses[i] = mass;
}

void ICEParticleEvaluator::evalPropColor(const QUuid&, const ui::GraphNode& node,
                                         ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    QVariantList c = node.properties.value("color").toList();
    QVector4D col = c.size() >= 4
        ? QVector4D(c[0].toFloat(), c[1].toFloat(), c[2].toFloat(), c[3].toFloat())
        : QVector4D(1, 1, 1, 1);
    for (int i = 0; i < state.aliveCount; ++i)
        state.colors[i] = col;
}

void ICEParticleEvaluator::evalPropSize(const QUuid&, const ui::GraphNode& node,
                                        ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    float size = node.properties.value("size", 0.05).toFloat();
    for (int i = 0; i < state.aliveCount; ++i)
        state.sizes[i] = size;
}

void ICEParticleEvaluator::evalForceVortex(const QUuid&, const ui::GraphNode& node,
                                           ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    QVector3D axis = node.properties.value("axis").toList().size() >= 3
        ? QVector3D(node.properties["axis"].toList()[0].toFloat(),
                    node.properties["axis"].toList()[1].toFloat(),
                    node.properties["axis"].toList()[2].toFloat()).normalized()
        : QVector3D(0, 1, 0);
    float strength = node.properties.value("strength", 1.0).toFloat();
    for (int i = 0; i < state.aliveCount; ++i) {
        // Tangential force around the axis
        QVector3D toAxis = state.positions[i] - axis * QVector3D::dotProduct(state.positions[i], axis);
        QVector3D tangent = QVector3D::crossProduct(axis, toAxis);
        if (toAxis.lengthSquared() > 1e-6f) tangent.normalize();
        state.accelerations[i] += tangent * strength;
    }
}

void ICEParticleEvaluator::evalForceAttractor(const QUuid&, const ui::GraphNode& node,
                                              ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    QVector3D target = node.properties.value("target").toList().size() >= 3
        ? QVector3D(node.properties["target"].toList()[0].toFloat(),
                    node.properties["target"].toList()[1].toFloat(),
                    node.properties["target"].toList()[2].toFloat())
        : QVector3D(0, 0, 0);
    float strength = node.properties.value("strength", 1.0).toFloat();
    float maxDist = node.properties.value("maxDistance", 100.0).toFloat();
    for (int i = 0; i < state.aliveCount; ++i) {
        QVector3D dir = target - state.positions[i];
        float d2 = dir.lengthSquared();
        if (d2 < 1e-6f) continue;
        float d = std::sqrt(d2);
        if (d > maxDist) continue;
        // F = strength / d^2 in the direction of the target (attractor if strength > 0)
        state.accelerations[i] += (dir / d) * strength * qMax(0.0f, 1.0f - d / maxDist);
    }
}

void ICEParticleEvaluator::evalCollisionSphere(const QUuid&, const ui::GraphNode& node,
                                               ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    QVector3D center = node.properties.value("center").toList().size() >= 3
        ? QVector3D(node.properties["center"].toList()[0].toFloat(),
                    node.properties["center"].toList()[1].toFloat(),
                    node.properties["center"].toList()[2].toFloat())
        : QVector3D(0, 0, 0);
    float radius = node.properties.value("radius", 1.0).toFloat();
    float restitution = node.properties.value("restitution", 0.5).toFloat();
    float friction = node.properties.value("friction", 0.2).toFloat();
    for (int i = 0; i < state.aliveCount; ++i) {
        QVector3D delta = state.positions[i] - center;
        float d2 = delta.lengthSquared();
        if (d2 >= radius * radius || d2 < 1e-6f) continue;
        QVector3D n = delta / std::sqrt(d2);
        state.positions[i] = center + n * radius;
        float vn = QVector3D::dotProduct(state.velocities[i], n);
        if (vn < 0) {
            state.velocities[i] -= (1 + restitution) * vn * n;
            state.velocities[i] *= (1 - friction);
        }
    }
}

void ICEParticleEvaluator::evalFilterVelocity(const QUuid&, const ui::GraphNode& node,
                                              ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    float maxSpeed = node.properties.value("maxSpeed", 100.0).toFloat();
    for (int i = state.aliveCount - 1; i >= 0; --i)
        if (state.velocities[i].lengthSquared() > maxSpeed * maxSpeed) state.kill(i);
}

void ICEParticleEvaluator::evalOpMultiply(const QUuid&, const ui::GraphNode& node,
                                          ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    float factor = node.properties.value("factor", 1.0).toFloat();
    for (int i = 0; i < state.aliveCount; ++i) {
        state.velocities[i] *= factor;
        state.accelerations[i] *= factor;
    }
}

void ICEParticleEvaluator::evalOpLerp(const QUuid&, const ui::GraphNode& node,
                                      ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    float t = qBound(0.0f, node.properties.value("t", 0.5).toFloat(), 1.0f);
    QVector3D target = node.properties.value("target").toList().size() >= 3
        ? QVector3D(node.properties["target"].toList()[0].toFloat(),
                    node.properties["target"].toList()[1].toFloat(),
                    node.properties["target"].toList()[2].toFloat())
        : QVector3D(0, 0, 0);
    for (int i = 0; i < state.aliveCount; ++i)
        state.positions[i] = state.positions[i] * (1 - t) + target * t;
}

void ICEParticleEvaluator::evalOpVectorMath(const QUuid&, const ui::GraphNode& node,
                                            ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    QString op = node.properties.value("op", "add").toString();
    QVector3D vec = node.properties.value("vec").toList().size() >= 3
        ? QVector3D(node.properties["vec"].toList()[0].toFloat(),
                    node.properties["vec"].toList()[1].toFloat(),
                    node.properties["vec"].toList()[2].toFloat())
        : QVector3D(0, 0, 0);
    for (int i = 0; i < state.aliveCount; ++i) {
        if (op == "add") { state.velocities[i] += vec; state.positions[i] += vec; }
        else if (op == "scale") { state.velocities[i] *= vec; }
        else if (op == "reflect") {
            if (vec.lengthSquared() > 1e-6f) {
                QVector3D n = vec.normalized();
                state.velocities[i] -= 2 * QVector3D::dotProduct(state.velocities[i], n) * n;
            }
        } else if (op == "set") { state.positions[i] = vec; }
    }
}

void ICEParticleEvaluator::evalEmitterSphere(const QUuid&, const ui::GraphNode& node,
                                             ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    QVector3D pos = node.properties.value("position").toList().size() >= 3
        ? QVector3D(node.properties["position"].toList()[0].toFloat(),
                    node.properties["position"].toList()[1].toFloat(),
                    node.properties["position"].toList()[2].toFloat())
        : QVector3D(0, 0, 0);
    float radius = node.properties.value("radius", 1.0).toFloat();
    float rate = node.properties.value("rate", 100).toFloat();
    int count = int(rate / 60.0f) + 1;

    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    int start = state.allocate(count);
    if (start < 0) return;
    for (int i = 0; i < count; ++i) {
        int idx = start + i;
        QVector3D dir(dist(m_rng), dist(m_rng), dist(m_rng));
        if (dir.lengthSquared() > 1.0f) dir.normalize();
        state.positions[idx] = pos + dir * radius;
        state.velocities[idx] = dir * node.properties.value("velocity", 1.0).toFloat();
        state.accelerations[idx] = QVector3D();
        state.ages[idx] = 0;
        state.lifetimes[idx] = node.properties.value("lifetime", 5).toFloat();
        state.sizes[idx] = node.properties.value("size", 0.05).toFloat();
        state.colors[idx] = QVector4D(1, 1, 1, 1);
        state.masses[idx] = 1;
    }
}

void ICEParticleEvaluator::evalForceGravity(const QUuid&, const ui::GraphNode& node,
                                            ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    float g = node.properties.value("gravity", -9.81).toFloat();
    for (int i = 0; i < state.aliveCount; ++i)
        state.accelerations[i] += QVector3D(0, g, 0);
}

void ICEParticleEvaluator::evalForceWind(const QUuid&, const ui::GraphNode& node,
                                         ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    QVector3D wind = node.properties.value("wind").toList().size() >= 3
        ? QVector3D(node.properties["wind"].toList()[0].toFloat(),
                    node.properties["wind"].toList()[1].toFloat(),
                    node.properties["wind"].toList()[2].toFloat())
        : QVector3D(0, 0, 0);
    for (int i = 0; i < state.aliveCount; ++i)
        state.accelerations[i] += wind;
}

void ICEParticleEvaluator::evalForceTurbulence(const QUuid&, const ui::GraphNode& node,
                                               ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    float strength = node.properties.value("strength", 1.0).toFloat();
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (int i = 0; i < state.aliveCount; ++i) {
        QVector3D noise(dist(m_rng), dist(m_rng), dist(m_rng));
        state.accelerations[i] += noise * strength;
    }
}

void ICEParticleEvaluator::evalForceDrag(const QUuid&, const ui::GraphNode& node,
                                         ICEParticleState& state, float dt, const QMap<QUuid, QVariant>&)
{
    float drag = qBound(0.0f, node.properties.value("damping", 0.05).toFloat(), 1.0f);
    float factor = qMax(0.0f, 1.0f - drag * dt * 60.0f);
    for (int i = 0; i < state.aliveCount; ++i)
        state.velocities[i] *= factor;
}

void ICEParticleEvaluator::evalCollisionPlane(const QUuid&, const ui::GraphNode& node,
                                              ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    float y = node.properties.value("y", 0.0).toFloat();
    float restitution = node.properties.value("restitution", 0.3).toFloat();
    float friction = node.properties.value("friction", 0.2).toFloat();
    for (int i = 0; i < state.aliveCount; ++i) {
        if (state.positions[i].y() < y && state.velocities[i].y() < 0) {
            state.positions[i].setY(y);
            state.velocities[i].setY(-state.velocities[i].y() * restitution);
            state.velocities[i].setX(state.velocities[i].x() * (1 - friction));
            state.velocities[i].setZ(state.velocities[i].z() * (1 - friction));
        }
    }
}

void ICEParticleEvaluator::setCollisionMesh(const QVector<QVector3D>& triangleVerts)
{
    m_collisionTriangles = triangleVerts;
    rebuildCollisionGrid();
}

void ICEParticleEvaluator::rebuildCollisionGrid()
{
    m_collisionGrid.clear();
    m_collisionTriBounds.clear();
    const int triCount = m_collisionTriangles.size() / 3;
    if (triCount <= 0) return;

    m_collisionTriBounds.resize(triCount * 2);
    const float cell = m_collisionCellSize;
    auto keyOf = [](int x, int y, int z) -> qint64 {
        // Pack 3 signed coords into 64 bits (21 bits each)
        return (qint64)(x & 0x1FFFFF)
             | ((qint64)(y & 0x1FFFFF) << 21)
             | ((qint64)(z & 0x1FFFFF) << 42);
    };

    for (int t = 0; t < triCount; ++t) {
        const QVector3D& a = m_collisionTriangles[t * 3];
        const QVector3D& b = m_collisionTriangles[t * 3 + 1];
        const QVector3D& c = m_collisionTriangles[t * 3 + 2];
        QVector3D mn = QVector3D(qMin(a.x(), qMin(b.x(), c.x())), qMin(a.y(), qMin(b.y(), c.y())), qMin(a.z(), qMin(b.z(), c.z())));
        QVector3D mx = QVector3D(qMax(a.x(), qMax(b.x(), c.x())), qMax(a.y(), qMax(b.y(), c.y())), qMax(a.z(), qMax(b.z(), c.z())));
        m_collisionTriBounds[t * 2] = mn;
        m_collisionTriBounds[t * 2 + 1] = mx;

        int x0 = (int)std::floor(mn.x() / cell), x1 = (int)std::floor(mx.x() / cell);
        int y0 = (int)std::floor(mn.y() / cell), y1 = (int)std::floor(mx.y() / cell);
        int z0 = (int)std::floor(mn.z() / cell), z1 = (int)std::floor(mx.z() / cell);
        for (int z = z0; z <= z1; ++z)
            for (int y = y0; y <= y1; ++y)
                for (int x = x0; x <= x1; ++x)
                    m_collisionGrid[keyOf(x, y, z)].append(t);
    }
}

namespace {
// Möller–Trumbore ray/triangle intersection (t >= 0).
bool iceRayTriangle(const QVector3D& o, const QVector3D& d,
                    const QVector3D& a, const QVector3D& b, const QVector3D& c,
                    float& tOut)
{
    const float eps = 1e-6f;
    QVector3D e1 = b - a;
    QVector3D e2 = c - a;
    QVector3D pvec = QVector3D::crossProduct(d, e2);
    float det = QVector3D::dotProduct(e1, pvec);
    if (std::fabs(det) < eps) return false;
    float invDet = 1.0f / det;
    QVector3D tvec = o - a;
    float u = QVector3D::dotProduct(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f) return false;
    QVector3D qvec = QVector3D::crossProduct(tvec, e1);
    float v = QVector3D::dotProduct(d, qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f) return false;
    float t = QVector3D::dotProduct(e2, qvec) * invDet;
    if (t < 0.0f) return false;
    tOut = t;
    return true;
}
} // namespace

void ICEParticleEvaluator::evalCollisionMesh(const QUuid&, const ui::GraphNode& node,
                                             ICEParticleState& state, float dt, const QMap<QUuid, QVariant>&)
{
    if (m_collisionTriangles.isEmpty()) return;
    const float restitution = node.properties.value("restitution", 0.3).toFloat();
    const float friction = node.properties.value("friction", 0.2).toFloat();
    const int triCount = m_collisionTriangles.size() / 3;
    const float cell = m_collisionCellSize;
    auto keyOf = [](int x, int y, int z) -> qint64 {
        return (qint64)(x & 0x1FFFFF)
             | ((qint64)(y & 0x1FFFFF) << 21)
             | ((qint64)(z & 0x1FFFFF) << 42);
    };

    for (int i = 0; i < state.aliveCount; ++i) {
        QVector3D prevPos = state.positions[i] - state.velocities[i] * dt;
        QVector3D dir = state.velocities[i];
        float speed = dir.length();
        if (speed < 1e-6f) continue;
        QVector3D d = dir / speed;
        float bestT = speed;
        float tHit = -1.0f;
        int hitTri = -1;

        // Broadphase: sweep AABB over visited cells
        QVector3D mn = QVector3D(qMin(prevPos.x(), state.positions[i].x()), qMin(prevPos.y(), state.positions[i].y()), qMin(prevPos.z(), state.positions[i].z()));
        QVector3D mx = QVector3D(qMax(prevPos.x(), state.positions[i].x()), qMax(prevPos.y(), state.positions[i].y()), qMax(prevPos.z(), state.positions[i].z()));
        int x0 = (int)std::floor(mn.x() / cell), x1 = (int)std::floor(mx.x() / cell);
        int y0 = (int)std::floor(mn.y() / cell), y1 = (int)std::floor(mx.y() / cell);
        int z0 = (int)std::floor(mn.z() / cell), z1 = (int)std::floor(mx.z() / cell);

        for (int z = z0; z <= z1; ++z) {
            for (int y = y0; y <= y1; ++y) {
                for (int x = x0; x <= x1; ++x) {
                    auto cit = m_collisionGrid.constFind(keyOf(x, y, z));
                    if (cit == m_collisionGrid.constEnd()) continue;
                    for (int tri : cit.value()) {
                        // Triangle AABB reject
                        const QVector3D& tmn = m_collisionTriBounds[tri * 2];
                        const QVector3D& tmx = m_collisionTriBounds[tri * 2 + 1];
                        if (tmn.x() > mx.x() || tmx.x() < mn.x() ||
                            tmn.y() > mx.y() || tmx.y() < mn.y() ||
                            tmn.z() > mx.z() || tmx.z() < mn.z()) continue;
                        float t = 0.0f;
                        if (iceRayTriangle(prevPos, d,
                                           m_collisionTriangles[tri * 3],
                                           m_collisionTriangles[tri * 3 + 1],
                                           m_collisionTriangles[tri * 3 + 2], t)) {
                            if (t >= 0.0f && t <= bestT) { bestT = t; tHit = t; hitTri = tri; }
                        }
                    }
                }
            }
        }

        if (hitTri < 0) continue;
        const QVector3D& a = m_collisionTriangles[hitTri * 3];
        const QVector3D& b = m_collisionTriangles[hitTri * 3 + 1];
        const QVector3D& c = m_collisionTriangles[hitTri * 3 + 2];
        QVector3D n = QVector3D::crossProduct(b - a, c - a);
        float nlen = n.length();
        if (nlen < 1e-8f) continue;
        n /= nlen;
        if (QVector3D::dotProduct(d, n) > 0.0f) n = -n; // face incoming ray
        state.positions[i] = prevPos + d * (tHit - 1e-4f);
        float vn = QVector3D::dotProduct(state.velocities[i], n);
        if (vn < 0.0f)
            state.velocities[i] -= (1.0f + restitution) * vn * n;
        state.velocities[i] *= (1.0f - friction);
    }
}

void ICEParticleEvaluator::evalFilterAge(const QUuid&, const ui::GraphNode& node,
                                         ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    float maxAge = node.properties.value("maxAge", 5.0).toFloat();
    for (int i = state.aliveCount - 1; i >= 0; --i)
        if (state.ages[i] >= maxAge) state.kill(i);
}

void ICEParticleEvaluator::evalOpAdd(const QUuid&, const ui::GraphNode&,
                                     ICEParticleState& state, float, const QMap<QUuid, QVariant>&)
{
    // Integration: advance velocities/positions
    const float dt = 1.0f / 60.0f;
    for (int i = 0; i < state.aliveCount; ++i) {
        state.velocities[i] += state.accelerations[i] * dt;
        state.positions[i] += state.velocities[i] * dt;
        state.ages[i] += dt;
        state.accelerations[i] = QVector3D();
    }
}

void ICEParticleEvaluator::evalOutputPoints(const QUuid&, const ui::GraphNode&,
                                            ICEParticleState&, float, const QMap<QUuid, QVariant>&)
{
    // Output node - signals that particles should be rendered as points.
    // Rendering handled by the bridge/UI layer.
}

} // namespace ks