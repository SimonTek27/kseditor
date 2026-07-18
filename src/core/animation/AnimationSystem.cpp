#include "AnimationSystem.h"

namespace ks {
namespace animation {

StateMachine::StateMachine(QObject* parent) : QObject(parent)
{
}

void StateMachine::addState(const State& state)
{
    m_states[state.id] = state;
}

void StateMachine::removeState(const QString& stateId)
{
    m_states.remove(stateId);
}

void StateMachine::addTransition(const Transition& transition)
{
    m_transitions.append(transition);
}

void StateMachine::removeTransition(const QString& transitionId)
{
    for (int i = 0; i < m_transitions.size(); ++i) {
        if (m_transitions[i].id == transitionId) {
            m_transitions.removeAt(i);
            return;
        }
    }
}

void StateMachine::start(const QString& initialState)
{
    if (!initialState.isEmpty() && m_states.contains(initialState)) {
        m_currentState = initialState;
    } else if (!m_states.isEmpty()) {
        m_currentState = m_states.begin().key();
    }
    m_running = true;
}

void StateMachine::stop()
{
    m_running = false;
}

void StateMachine::setState(const QString& stateId)
{
    if (!m_states.contains(stateId)) return;
    
    QString fromState = m_currentState;
    m_currentState = stateId;
    m_running = true;
    emit stateChanged(fromState, m_currentState);
}

void StateMachine::update(double deltaTime)
{
    Q_UNUSED(deltaTime);
    if (!m_running) return;
    
    evaluateTransitions();
    
    if (!m_targetState.isEmpty()) {
        m_transitionProgress += deltaTime;
        if (m_transitionProgress >= m_transitionDuration) {
            m_currentState = m_targetState;
            m_targetState.clear();
            m_transitionProgress = 0.0;
            m_transitionDuration = 0.0;
            emit transitionFinished(m_currentState, m_currentState);
        }
    }
}

void StateMachine::evaluateTransitions()
{
    if (!m_targetState.isEmpty()) return;
    
    for (const auto& t : m_transitions) {
        if (t.fromState == m_currentState) {
            if (t.condition.isEmpty()) {
                m_targetState = t.toState;
                m_transitionDuration = t.duration;
                m_transitionProgress = 0.0;
                emit transitionStarted(m_currentState, m_targetState);
                return;
            }
        }
    }
}

double StateMachine::applyEasing(double t, const QString& easing) const
{
    t = qBound(0.0, t, 1.0);

    if (easing == "linear" || easing.isEmpty()) return t;

    // Standard easing functions
    if (easing == "easeInQuad") return t * t;
    if (easing == "easeOutQuad") return t * (2.0 - t);
    if (easing == "easeInOutQuad") {
        return t < 0.5 ? 2.0 * t * t : -1.0 + (4.0 - 2.0 * t) * t;
    }

    if (easing == "easeInCubic") return t * t * t;
    if (easing == "easeOutCubic") {
        double t1 = t - 1.0;
        return t1 * t1 * t1 + 1.0;
    }
    if (easing == "easeInOutCubic") {
        return t < 0.5 ? 4.0 * t * t * t : (t - 1.0) * (2.0 * t - 2.0) * (2.0 * t - 2.0) + 1.0;
    }

    if (easing == "easeInQuart") {
        double t2 = t * t;
        return t2 * t2;
    }
    if (easing == "easeOutQuart") {
        double t1 = t - 1.0;
        return 1.0 - t1 * t1 * t1 * t1;
    }
    if (easing == "easeInOutQuart") {
        double t1 = t - 1.0;
        double t2 = t * t;
        return t < 0.5 ? 8.0 * t2 * t2 : 1.0 - 8.0 * t1 * t1 * t1 * t1;
    }

    if (easing == "easeInExpo") {
        return (t == 0.0) ? 0.0 : std::pow(2.0, 10.0 * (t - 1.0));
    }
    if (easing == "easeOutExpo") {
        return (t == 1.0) ? 1.0 : 1.0 - std::pow(2.0, -10.0 * t);
    }
    if (easing == "easeInOutExpo") {
        if (t == 0.0) return 0.0;
        if (t == 1.0) return 1.0;
        double tt = t * 2.0 - 1.0;
        return (tt < 0.0) ? std::pow(2.0, 10.0 * tt) * 0.5 : (2.0 - std::pow(2.0, -10.0 * tt)) * 0.5;
    }

    if (easing == "easeInSine") return 1.0 - std::cos(t * M_PI * 0.5);
    if (easing == "easeOutSine") return std::sin(t * M_PI * 0.5);
    if (easing == "easeInOutSine") return -0.5 * (std::cos(M_PI * t) - 1.0);

    if (easing == "easeInBack") {
        const double s = 1.70158;
        return t * t * ((s + 1.0) * t - s);
    }
    if (easing == "easeOutBack") {
        const double s = 1.70158;
        double t1 = t - 1.0;
        return t1 * t1 * ((s + 1.0) * t1 + s) + 1.0;
    }
    if (easing == "easeInOutBack") {
        const double s = 1.70158 * 1.525;
        double t2 = t * 2.0;
        if (t2 < 1.0) return 0.5 * (t2 * t2 * ((s + 1.0) * t2 - s));
        double t1 = t2 - 2.0;
        return 0.5 * (t1 * t1 * ((s + 1.0) * t1 + s) + 2.0);
    }

    if (easing == "easeInElastic") {
        if (t == 0.0 || t == 1.0) return t;
        return -std::pow(2.0, 10.0 * (t - 1.0)) * std::sin((t - 1.1) * 5.0 * M_PI);
    }
    if (easing == "easeOutElastic") {
        if (t == 0.0 || t == 1.0) return t;
        return std::pow(2.0, -10.0 * t) * std::sin((t - 0.1) * 5.0 * M_PI) + 1.0;
    }
    if (easing == "easeInOutElastic") {
        if (t == 0.0 || t == 1.0) return t;
        double tt = t * 2.0 - 1.0;
        if (tt < 0.0) return -0.5 * std::pow(2.0, 10.0 * tt) * std::sin((tt - 0.1) * 5.0 * M_PI);
        return 0.5 * std::pow(2.0, -10.0 * tt) * std::sin((tt - 0.1) * 5.0 * M_PI) + 1.0;
    }

    if (easing == "easeInBounce") return 1.0 - applyEasing(1.0 - t, "easeOutBounce");
    if (easing == "easeOutBounce") {
        if (t < 1.0 / 2.75) return 7.5625 * t * t;
        if (t < 2.0 / 2.75) { double n = t - 1.5 / 2.75; return 7.5625 * n * n + 0.75; }
        if (t < 2.5 / 2.75) { double n = t - 2.25 / 2.75; return 7.5625 * n * n + 0.9375; }
        double n = t - 2.625 / 2.75;
        return 7.5625 * n * n + 0.984375;
    }
    if (easing == "easeInOutBounce") {
        return t < 0.5
            ? (1.0 - applyEasing(1.0 - 2.0 * t, "easeOutBounce")) * 0.5
            : (1.0 + applyEasing(2.0 * t - 1.0, "easeOutBounce")) * 0.5;
    }

    // Spring interpolation
    if (easing == "spring") {
        double c4 = (2.0 * M_PI) / 3.0;
        return std::pow(2.0, -10.0 * t) * std::sin((t * 10.0 - 0.75) * c4) + 1.0;
    }

    // Fallback to linear
    return t;
}

AnimationCurve::AnimationCurve(QObject* parent) : QObject(parent) {}

BlendTree::BlendTree(QObject* parent) : QObject(parent) {}

AnimationSystem::AnimationSystem(QObject* parent) : QObject(parent) {}

void AnimationCurve::addKeyframe(const Keyframe& kf)
{
    m_keyframes.append(kf);
    std::sort(m_keyframes.begin(), m_keyframes.end(), [](const Keyframe& a, const Keyframe& b) {
        return a.time < b.time;
    });
}

void AnimationCurve::removeKeyframe(double time)
{
    for (int i = 0; i < m_keyframes.size(); ++i) {
        if (qFuzzyCompare(m_keyframes[i].time, time)) {
            m_keyframes.removeAt(i);
            return;
        }
    }
}

void AnimationCurve::clearKeyframes()
{
    m_keyframes.clear();
    emit keyframesChanged();
}

void AnimationCurve::setExtrapolationMode(ExtrapolationMode pre, ExtrapolationMode post)
{
    m_preExtrapolation = pre;
    m_postExtrapolation = post;
}

QVariant AnimationCurve::evaluate(double time) const
{
    if (m_keyframes.isEmpty()) return QVariant();

    if (time <= m_keyframes.first().time) {
        if (m_preExtrapolation == Cycle) {
            double range = m_keyframes.last().time - m_keyframes.first().time;
            if (range > 0.0) time += std::ceil((m_keyframes.first().time - time) / range) * range;
            else return m_keyframes.first().value;
        } else if (m_preExtrapolation == Linear) {
            if (m_keyframes.size() >= 2) {
                const auto& k0 = m_keyframes[0];
                const auto& k1 = m_keyframes[1];
                double slope = (k1.value.toDouble() - k0.value.toDouble()) / (k1.time - k0.time);
                return QVariant(k0.value.toDouble() + slope * (time - k0.time));
            }
        }
        return m_keyframes.first().value;
    }

    if (time >= m_keyframes.last().time) {
        if (m_postExtrapolation == Cycle) {
            double range = m_keyframes.last().time - m_keyframes.first().time;
            if (range > 0.0) time -= std::floor((time - m_keyframes.first().time) / range) * range;
            else return m_keyframes.last().value;
        } else if (m_postExtrapolation == Linear) {
            if (m_keyframes.size() >= 2) {
                const auto& k0 = m_keyframes[m_keyframes.size() - 2];
                const auto& k1 = m_keyframes.last();
                double slope = (k1.value.toDouble() - k0.value.toDouble()) / (k1.time - k0.time);
                return QVariant(k1.value.toDouble() + slope * (time - k1.time));
            }
        }
        return m_keyframes.last().value;
    }

    for (int i = 0; i < m_keyframes.size() - 1; ++i) {
        if (time >= m_keyframes[i].time && time <= m_keyframes[i+1].time) {
            double t = (m_keyframes[i+1].time == m_keyframes[i].time) ? 0.0 :
                       (time - m_keyframes[i].time) / (m_keyframes[i+1].time - m_keyframes[i].time);
            return interpolate(m_keyframes[i], m_keyframes[i+1], t);
        }
    }
    return QVariant();
}

QVariant AnimationCurve::interpolate(const Keyframe& a, const Keyframe& b, double t) const
{
    if (a.value.userType() != b.value.userType())
        return a.value;

    switch (a.value.userType()) {
    case QMetaType::Double:
    case QMetaType::Float:
        return QVariant(interpolateDouble(a.value.toDouble(), b.value.toDouble(), t, a.interpolation));
    case QMetaType::QColor:
        return QVariant::fromValue(interpolateColor(a.value.value<QColor>(), b.value.value<QColor>(), t));
    case QMetaType::QVector3D:
        return QVariant::fromValue(interpolateVector3D(a.value.value<QVector3D>(), b.value.value<QVector3D>(), t));
    case QMetaType::QQuaternion:
        return QVariant::fromValue(interpolateQuaternion(a.value.value<QQuaternion>(), b.value.value<QQuaternion>(), t));
    default:
        return a.value;
    }
}

namespace {
double lerp(double a, double b, double t, const QString& interpolation)
{
    if (interpolation == "constant") return a;
    if (interpolation == "cubic") {
        double s = t * t * (3.0 - 2.0 * t);
        return a + s * (b - a);
    }
    return a + t * (b - a);
}
}

QVariant AnimationCurve::interpolateDouble(double a, double b, double t, const QString& interpolation) const
{
    return QVariant(lerp(a, b, t, interpolation));
}

QColor AnimationCurve::interpolateColor(const QColor& a, const QColor& b, double t) const
{
    return QColor(
        static_cast<int>(a.red()   + t * (b.red()   - a.red())),
        static_cast<int>(a.green() + t * (b.green() - a.green())),
        static_cast<int>(a.blue()  + t * (b.blue()  - a.blue())),
        static_cast<int>(a.alpha() + t * (b.alpha() - a.alpha()))
    );
}

QVector3D AnimationCurve::interpolateVector3D(const QVector3D& a, const QVector3D& b, double t) const
{
    return a + static_cast<float>(t) * (b - a);
}

QQuaternion AnimationCurve::interpolateQuaternion(const QQuaternion& a, const QQuaternion& b, double t) const
{
    return QQuaternion::slerp(a, b, static_cast<float>(t));
}

// ---------------------------------------------------------------------------
// IKSolver
// ---------------------------------------------------------------------------
IKSolver::IKSolver() {}
void IKSolver::addJoint(const Joint& joint) { m_joints[joint.name].joint = joint; }
void IKSolver::removeJoint(const QString& name) { m_joints.remove(name); }
void IKSolver::addChain(const Chain& chain) { m_chains[chain.name] = chain; }
void IKSolver::removeChain(const QString& name) { m_chains.remove(name); }
QMap<QString, IKSolver::Joint> IKSolver::joints() const
{
    QMap<QString, Joint> result;
    for (auto it = m_joints.begin(); it != m_joints.end(); ++it)
        result[it.key()] = it->joint;
    return result;
}

void IKSolver::setTarget(const QString& chainName, const QVector3D& position, const QVector3D& rotation)
{
    if (m_chains.contains(chainName)) {
        m_chains[chainName].targetPosition = position;
        m_chains[chainName].targetRotation = rotation;
    }
}

void IKSolver::solve(double deltaTime)
{
    Q_UNUSED(deltaTime);
    updateWorldTransforms();
    for (auto& chain : m_chains)
        solveFABRIK(chain.name);
    updateWorldTransforms();
}

void IKSolver::updateWorldTransforms()
{
    // Build parent-child hierarchy for forward propagation
    for (auto it = m_joints.begin(); it != m_joints.end(); ++it) {
        const QString& p = it->joint.parent;
        if (!p.isEmpty())
            m_children[p].append(it.key());
    }

    // Find roots and propagate
    for (auto it = m_joints.begin(); it != m_joints.end(); ++it) {
        if (it->joint.parent.isEmpty() || !m_joints.contains(it->joint.parent)) {
            QMatrix4x4 ident;
            QQuaternion qIdent(1, 0, 0, 0);
            QVector3D zero(0, 0, 0);
            computeWorldTransform(it.key(), ident, qIdent, zero);
        }
    }
}

QMatrix4x4 IKSolver::computeLocalToWorld(const QString& jointName) const
{
    return m_joints.contains(jointName) ? m_joints[jointName].localToWorld : QMatrix4x4();
}

void IKSolver::computeWorldTransform(const QString& jointName, const QMatrix4x4& parentMatrix,
                                     const QQuaternion& parentRotation, const QVector3D& parentPos)
{
    if (!m_joints.contains(jointName)) return;
    JointData& jd = m_joints[jointName];
    const Joint& j = jd.joint;

    QQuaternion localRot = QQuaternion::fromEulerAngles(j.rotation);
    QQuaternion worldRot = parentRotation * localRot;
    QVector3D worldPos = parentPos + parentMatrix.mapVector(j.position);

    QMatrix4x4 mat;
    mat.translate(worldPos);
    mat.rotate(worldRot);
    jd.worldPos = worldPos;
    jd.worldRot = worldRot;
    jd.localToWorld = mat;

    // Propagate to children
    for (const auto& child : m_children.value(jointName))
        computeWorldTransform(child, mat, worldRot, worldPos);
}

bool IKSolver::solveFABRIK(const QString& chainName)
{
    if (!m_chains.contains(chainName)) return false;
    Chain& chain = m_chains[chainName];

    QVector<QString> jointNames;
    for (const auto& jn : chain.joints)
        if (m_joints.contains(jn)) jointNames.append(jn);
    if (jointNames.size() < 2) return false;

    // Current world positions
    QVector<QVector3D> pos(jointNames.size());
    float totalLen = 0.0f;
    for (int i = 0; i < jointNames.size(); ++i) {
        pos[i] = m_joints[jointNames[i]].worldPos;
        if (i > 0) totalLen += (pos[i] - pos[i-1]).length();
    }

    QVector3D target = chain.targetPosition;
    QVector3D rootPos = pos[0];
    int maxIter = qBound(1, static_cast<int>(chain.maxIterations), 100);
    float reach = (pos.last() - rootPos).length();
    if (reach < 0.001f) return true;

    // Scale target distance if unreachable
    float targetDist = (target - rootPos).length();
    if (targetDist > totalLen) {
        QVector3D dir = (target - rootPos).normalized();
        target = rootPos + dir * totalLen;
    }

    for (int iter = 0; iter < maxIter; ++iter) {
        float err = (pos.last() - target).length();
        if (err < static_cast<float>(chain.tolerance)) break;

        // Forward reach: set end effector to target, pull chain
        pos.last() = target;
        for (int i = jointNames.size() - 2; i >= 0; --i) {
            QVector3D dir = (pos[i] - pos[i+1]).normalized();
            float segLen = (m_joints[jointNames[i]].joint.position -
                            m_joints[jointNames[i+1]].joint.position).length();
            if (segLen < 0.0001f) segLen = 0.1f;
            pos[i] = pos[i+1] + dir * segLen;
        }

        // Backward reach: fix root at original, pull chain
        pos[0] = rootPos;
        for (int i = 0; i < jointNames.size() - 1; ++i) {
            QVector3D dir = (pos[i+1] - pos[i]).normalized();
            float segLen = (m_joints[jointNames[i]].joint.position -
                            m_joints[jointNames[i+1]].joint.position).length();
            if (segLen < 0.0001f) segLen = 0.1f;
            pos[i+1] = pos[i] + dir * segLen;
        }
    }

    // Write back positions and compute rotations
    for (int i = 0; i < jointNames.size(); ++i) {
        JointData& jd = m_joints[jointNames[i]];
        jd.worldPos = pos[i];
        if (i > 0) {
            QVector3D dir = (pos[i] - pos[i-1]).normalized();
            QVector3D up(0, 1, 0);
            if (qAbs(QVector3D::dotProduct(dir, up)) > 0.99f) up = QVector3D(1, 0, 0);
            QQuaternion q = QQuaternion::fromDirection(dir, up);
            if (!q.isNull()) jd.worldRot = q;
        }
    }
    return true;
}

bool IKSolver::solveCCD(const QString& chainName)
{
    if (!m_chains.contains(chainName)) return false;
    Chain& chain = m_chains[chainName];

    QVector<QString> jointNames;
    for (const auto& jn : chain.joints)
        if (m_joints.contains(jn)) jointNames.append(jn);
    if (jointNames.size() < 2) return false;

    QVector<QVector3D> pos(jointNames.size());
    for (int i = 0; i < jointNames.size(); ++i)
        pos[i] = m_joints[jointNames[i]].worldPos;

    QVector3D target = chain.targetPosition;
    int endIdx = jointNames.size() - 1;
    int maxIter = qBound(1, static_cast<int>(chain.maxIterations), 100);

    for (int iter = 0; iter < maxIter; ++iter) {
        float err = (pos[endIdx] - target).length();
        if (err < static_cast<float>(chain.tolerance)) break;

        // Iterate joints from end-1 down to root
        for (int j = endIdx - 1; j >= 0; --j) {
            QVector3D jointPos = pos[j];
            QVector3D endEffector = pos[endIdx];
            QVector3D toEnd = endEffector - jointPos;
            float toEndLen = toEnd.length();
            if (toEndLen < 1e-6f) continue;

            QVector3D toTarget = target - jointPos;
            QVector3D axis = QVector3D::crossProduct(toEnd, toTarget);
            float axisLen = axis.length();
            if (axisLen < 1e-6f) continue;

            float cosAngle = QVector3D::dotProduct(toEnd, toTarget) / (toEndLen * toTarget.length());
            float angle = std::acos(qBound(-1.0f, cosAngle, 1.0f));
            if (std::abs(angle) < 1e-4f) continue;

            axis /= axisLen;
            QQuaternion rot = QQuaternion::fromAxisAndAngle(axis, qRadiansToDegrees(angle));

            // Rotate all subsequent joints
            for (int k = j + 1; k <= endIdx; ++k) {
                pos[k] = jointPos + rot.rotatedVector(pos[k] - jointPos);
            }
        }
    }

    // Write back world positions
    for (int i = 0; i < jointNames.size(); ++i) {
        m_joints[jointNames[i]].worldPos = pos[i];
        if (i > 0) {
            QVector3D dir = (pos[i] - pos[i-1]).normalized();
            QVector3D up(0, 1, 0);
            if (qAbs(QVector3D::dotProduct(dir, up)) > 0.99f) up = QVector3D(1, 0, 0);
            QQuaternion q = QQuaternion::fromDirection(dir, up);
            if (!q.isNull()) m_joints[jointNames[i]].worldRot = q;
        }
    }
    return true;
}

void IKSolver::applyLimits(const QString& jointName, QQuaternion& rotation) const
{
    if (!m_joints.contains(jointName)) return;
    const Joint& j = m_joints[jointName].joint;
    QVector3D euler = rotation.toEulerAngles();
    euler.setX(qBound(j.minLimits.x(), euler.x(), j.maxLimits.x()));
    euler.setY(qBound(j.minLimits.y(), euler.y(), j.maxLimits.y()));
    euler.setZ(qBound(j.minLimits.z(), euler.z(), j.maxLimits.z()));
    rotation = QQuaternion::fromEulerAngles(euler);
}

// ---------------------------------------------------------------------------
// PhysicsAnimation
// ---------------------------------------------------------------------------
PhysicsAnimation::PhysicsAnimation() {}
void PhysicsAnimation::addBody(const Body& body) { m_bodies[body.name] = body; }
void PhysicsAnimation::removeBody(const QString& name) { m_bodies.remove(name); }
void PhysicsAnimation::addConstraint(const Constraint& constraint)
{
    m_constraints.append(constraint);
}
void PhysicsAnimation::removeConstraint(const QString& name)
{
    for (int i = 0; i < m_constraints.size(); ++i)
        if (m_constraints[i].bodyA + ":" + m_constraints[i].bodyB == name) {
            m_constraints.removeAt(i); return;
        }
}
void PhysicsAnimation::addSpring(const Spring& spring) { m_springs.append(spring); }

void PhysicsAnimation::step(double deltaTime)
{
    if (deltaTime <= 0.0) return;
    deltaTime = qMin(deltaTime, 1.0 / 30.0); // cap dt
    applyGravity(deltaTime);
    integrateBodies(deltaTime);
    solveConstraints(deltaTime);
    solveSprings(deltaTime);
}

void PhysicsAnimation::applyGravity(double dt)
{
    for (auto it = m_bodies.begin(); it != m_bodies.end(); ++it) {
        Body& b = it.value();
        if (!b.kinematic)
            b.velocity += m_gravity * static_cast<float>(dt);
    }
}

void PhysicsAnimation::integrateBodies(double dt)
{
    float fdt = static_cast<float>(dt);
    for (auto it = m_bodies.begin(); it != m_bodies.end(); ++it) {
        Body& b = it.value();
        if (b.kinematic) continue;

        QVector3D posDelta = b.velocity * fdt + m_gravity * (0.5f * fdt * fdt);
        b.position += posDelta;

        QVector3D angDelta = b.angularVelocity * fdt;
        if (angDelta.lengthSquared() > 0.0001f) {
            float angle = angDelta.length();
            QVector3D axis = angDelta.normalized();
            QQuaternion dq = QQuaternion::fromAxisAndAngle(axis, qRadiansToDegrees(angle));
            b.rotation = dq * b.rotation;
            b.rotation.normalize();
        }

        // Simple drag
        b.velocity *= 0.999f;
        b.angularVelocity *= 0.999f;
    }
}

void PhysicsAnimation::solveConstraints(double dt)
{
    float fdt = static_cast<float>(dt);
    for (const auto& c : m_constraints) {
        if (!m_bodies.contains(c.bodyA) || !m_bodies.contains(c.bodyB)) continue;
        Body& a = m_bodies[c.bodyA];
        Body& b = m_bodies[c.bodyB];

        QVector3D worldPivotA = a.position + a.rotation.rotatedVector(c.pivotA);
        QVector3D worldPivotB = b.position + b.rotation.rotatedVector(c.pivotB);
        QVector3D diff = worldPivotB - worldPivotA;
        float dist = diff.length();

        if (dist < 0.0001f) continue;

        QVector3D dir = diff / dist;
        float compliance = 1.0f / (c.stiffness + 0.0001f);
        float corr = dist * 0.5f;
        if (c.type == Constraint::Fixed) {
            if (!a.kinematic) a.position += dir * (corr * compliance);
            if (!b.kinematic) b.position -= dir * (corr * compliance);
        } else if (c.type == Constraint::Hinge) {
            // Constrain rotation around local axis
            QVector3D worldAxis = a.rotation.rotatedVector(c.axis);
            QQuaternion rotCorr = QQuaternion::rotationTo(
                a.rotation.rotatedVector(c.axis).normalized(),
                b.rotation.rotatedVector(c.axis).normalized());
            if (!a.kinematic) a.rotation = QQuaternion::slerp(a.rotation, a.rotation * rotCorr, 0.01f);
        }
    }
}

void PhysicsAnimation::solveSprings(double dt)
{
    for (const auto& s : m_springs) {
        if (!m_bodies.contains(s.bodyA) || !m_bodies.contains(s.bodyB)) continue;
        Body& a = m_bodies[s.bodyA];
        Body& b = m_bodies[s.bodyB];

        QVector3D worldA = a.position + a.rotation.rotatedVector(s.anchorA);
        QVector3D worldB = b.position + b.rotation.rotatedVector(s.anchorB);
        QVector3D diff = worldB - worldA;
        float currentLen = diff.length();
        if (currentLen < 0.0001f) continue;

        float stretch = currentLen - static_cast<float>(s.restLength);
        QVector3D force = diff / currentLen * (stretch * static_cast<float>(s.stiffness));
        force -= a.velocity * static_cast<float>(s.damping);
        QVector3D invForce = -force;
        invForce -= b.velocity * static_cast<float>(s.damping);

        if (!a.kinematic) a.velocity += force / static_cast<float>(a.mass) * static_cast<float>(dt);
        if (!b.kinematic) b.velocity += invForce / static_cast<float>(b.mass) * static_cast<float>(dt);
    }
}

QVector<PhysicsAnimation::Body> PhysicsAnimation::createRagdoll(const QVector3D& rootPos, double height, double mass)
{
    QVector<Body> bodies;
    double halfHeight = height * 0.5;

    // Simple capsule ragdoll: head, torso, upper/lower arms, upper/lower legs
    auto addBody = [&](const QString& name, const QVector3D& pos, double m, const QVector3D& size) {
        Body b;
        b.name = name;
        b.position = pos;
        b.mass = m;
        QMatrix3x3 inertia;
        float r2 = size.x() * size.x();
        for (int i = 0; i < 3; ++i)
            inertia(i, i) = 0.4f * static_cast<float>(m) * r2;
        b.inertiaTensor = inertia;
        bodies.append(b);
    };

    addBody("head", rootPos + QVector3D(0, halfHeight * 0.85, 0), mass * 0.07, QVector3D(0.12, 0.12, 0.12));
    addBody("torso", rootPos + QVector3D(0, halfHeight * 0.35, 0), mass * 0.45, QVector3D(0.25, 0.35, 0.15));
    addBody("upperArm_L", rootPos + QVector3D(-0.3, halfHeight * 0.45, 0), mass * 0.04, QVector3D(0.08, 0.25, 0.08));
    addBody("upperArm_R", rootPos + QVector3D(0.3, halfHeight * 0.45, 0), mass * 0.04, QVector3D(0.08, 0.25, 0.08));
    addBody("forearm_L", rootPos + QVector3D(-0.3, halfHeight * 0.15, 0), mass * 0.02, QVector3D(0.06, 0.20, 0.06));
    addBody("forearm_R", rootPos + QVector3D(0.3, halfHeight * 0.15, 0), mass * 0.02, QVector3D(0.06, 0.20, 0.06));
    addBody("thigh_L", rootPos + QVector3D(-0.12, -halfHeight * 0.2, 0), mass * 0.14, QVector3D(0.12, 0.35, 0.12));
    addBody("thigh_R", rootPos + QVector3D(0.12, -halfHeight * 0.2, 0), mass * 0.14, QVector3D(0.12, 0.35, 0.12));
    addBody("shin_L", rootPos + QVector3D(-0.12, -halfHeight * 0.6, 0), mass * 0.08, QVector3D(0.10, 0.35, 0.10));
    addBody("shin_R", rootPos + QVector3D(0.12, -halfHeight * 0.6, 0), mass * 0.08, QVector3D(0.10, 0.35, 0.10));

    return bodies;
}

QVector<PhysicsAnimation::Constraint> PhysicsAnimation::createRagdollConstraints(const QVector<Body>& bodies)
{
    QVector<Constraint> constraints;

    auto findBody = [&](const QString& name) -> int {
        for (int i = 0; i < bodies.size(); ++i)
            if (bodies[i].name == name) return i;
        return -1;
    };

    struct ChainLink { QString parent, child; QVector3D pivot; };
    QVector<ChainLink> links = {
        {"torso", "head", QVector3D(0, 0.35, 0)},
        {"torso", "upperArm_L", QVector3D(-0.25, 0.15, 0)},
        {"torso", "upperArm_R", QVector3D(0.25, 0.15, 0)},
        {"upperArm_L", "forearm_L", QVector3D(0, -0.25, 0)},
        {"upperArm_R", "forearm_R", QVector3D(0, -0.25, 0)},
        {"torso", "thigh_L", QVector3D(-0.1, -0.3, 0)},
        {"torso", "thigh_R", QVector3D(0.1, -0.3, 0)},
        {"thigh_L", "shin_L", QVector3D(0, -0.35, 0)},
        {"thigh_R", "shin_R", QVector3D(0, -0.35, 0)},
    };

    for (const auto& l : links) {
        int pi = findBody(l.parent), ci = findBody(l.child);
        if (pi >= 0 && ci >= 0) {
            Constraint c;
            c.bodyA = bodies[pi].name;
            c.bodyB = bodies[ci].name;
            c.pivotA = l.pivot;
            c.pivotB = QVector3D(0, 0, 0);
            c.type = Constraint::ConeTwist;
            c.stiffness = 50.0;
            c.damping = 1.0;
            constraints.append(c);
        }
    }
    return constraints;
}

// ---------------------------------------------------------------------------
// BlendTree
// ---------------------------------------------------------------------------
void BlendTree::addNode(const BlendNode& node) { m_nodes[node.name] = node; }
void BlendTree::removeNode(const QString& name) { m_nodes.remove(name); }
void BlendTree::setParameter(const QString& param, double value) { m_parameters[param] = value; }
void BlendTree::setParameter2D(const QString& param, double x, double y)
{
    m_parameters2D[param] = QPointF(x, y);
}

QVector<QPair<QString, double>> BlendTree::evaluate() const
{
    QVector<QPair<QString, double>> result;
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        const BlendNode& node = it.value();
        double w = node.weight;

        if (node.type == Blend1D && !node.thresholds.isEmpty()) {
            double param = m_parameters.isEmpty() ? 0.0 : m_parameters.begin().value();
            w *= calculateBlend1D(node, param);
        } else if (node.type == Blend2D && !node.positions.isEmpty()) {
            QPointF param = m_parameters2D.isEmpty() ? QPointF(0, 0) : m_parameters2D.begin().value();
            auto wh = calculateBlend2D(node, param.x(), param.y());
            w *= wh.first * wh.second;
        }

        if (qAbs(w) > 0.001) {
            for (const auto& anim : node.animations)
                result.append({anim, w});
        }
    }
    return result;
}

double BlendTree::calculateBlend1D(const BlendNode& node, double param) const
{
    if (node.thresholds.isEmpty()) return 1.0;
    if (param <= node.thresholds.first()) return 1.0;
    if (param >= node.thresholds.last()) return 0.0;

    for (int i = 0; i < node.thresholds.size() - 1; ++i) {
        if (param >= node.thresholds[i] && param <= node.thresholds[i+1]) {
            double range = node.thresholds[i+1] - node.thresholds[i];
            if (range < 0.0001) return 0.5;
            double t = (param - node.thresholds[i]) / range;
            return 1.0 - t; // Blend from full (first) to 0 (last)
        }
    }
    return 0.0;
}

QPair<double, double> BlendTree::calculateBlend2D(const BlendNode& node, double x, double y) const
{
    if (node.positions.isEmpty()) return {1.0, 1.0};

    double minDist = std::numeric_limits<double>::max();
    int nearest = 0;
    for (int i = 0; i < node.positions.size(); ++i) {
        double dx = x - node.positions[i].x();
        double dy = y - node.positions[i].y();
        double d = dx * dx + dy * dy;
        if (d < minDist) { minDist = d; nearest = i; }
    }

    if (minDist < 0.001) return {1.0, 1.0};
    double weight = std::exp(-minDist * 2.0);
    return {weight, weight};
}

// ---------------------------------------------------------------------------
// AnimationSystem
// ---------------------------------------------------------------------------
void AnimationSystem::addClip(const Clip& clip) { m_clips[clip.name] = clip; }
void AnimationSystem::removeClip(const QString& name) { m_clips.remove(name); }
void AnimationSystem::addLayer(const Layer& layer) { m_layers[layer.name] = layer; }
void AnimationSystem::removeLayer(const QString& name) { m_layers.remove(name); }

void AnimationSystem::play(const QString& layerName, const QString& clipName, double blendTime)
{
    if (!m_layers.contains(layerName) || !m_clips.contains(clipName)) return;
    Layer& layer = m_layers[layerName];
    AnimationState& state = layer.states[layerName + "_main"];
    state.clipName = clipName;
    state.playing = true;
    state.time = 0.0;
    state.blendTime = blendTime;
    state.blendProgress = 0.0;
}

void AnimationSystem::stop(const QString& layerName)
{
    if (!m_layers.contains(layerName)) return;
    m_layers[layerName].currentState.clear();
    for (auto it = m_layers[layerName].states.begin();
         it != m_layers[layerName].states.end(); ++it)
        it->playing = false;
}

void AnimationSystem::setLayerWeight(const QString& layerName, double weight)
{
    if (m_layers.contains(layerName))
        m_layers[layerName].weight = qBound(0.0, weight, 2.0);
}

void AnimationSystem::setClipSpeed(const QString& layerName, double speed)
{
    if (m_layers.contains(layerName))
        for (auto& s : m_layers[layerName].states)
            s.speed = qMax(0.0, speed);
}

void AnimationSystem::setClipTime(const QString& layerName, double time)
{
    if (m_layers.contains(layerName))
        for (auto& s : m_layers[layerName].states)
            s.time = qMax(0.0, time);
}

void AnimationSystem::update(double deltaTime)
{
    for (auto it = m_layers.begin(); it != m_layers.end(); ++it)
        updateLayer(it.value(), deltaTime);

    QMap<QString, QVector3D> pos;
    QMap<QString, QQuaternion> rot;
    QMap<QString, QVector3D> scale;
    for (auto it = m_layers.begin(); it != m_layers.end(); ++it)
        for (auto& st : it->states)
            if (m_clips.contains(st.clipName) && st.playing)
                evaluateClip(m_clips[st.clipName], st.time, pos, rot, scale);
}

void AnimationSystem::updateLayer(Layer& layer, double deltaTime)
{
    for (auto it = layer.states.begin(); it != layer.states.end(); ++it) {
        AnimationState& state = it.value();
        if (!state.playing) continue;
        state.time += deltaTime * state.speed;

        // Blend progress
        if (state.blendProgress < 1.0) {
            state.blendProgress = qMin(1.0, state.blendProgress + deltaTime / qMax(0.001, state.blendTime));
            if (state.blendProgress >= 1.0 && !state.nextState.isEmpty()) {
                layer.currentState = state.nextState;
                state.nextState.clear();
                emit stateChanged(layer.name, layer.currentState);
            }
        }

        // Clip finished
        if (m_clips.contains(state.clipName)) {
            const Clip& clip = m_clips[state.clipName];
            if (state.time >= clip.duration) {
                if (clip.loop) {
                    state.time = fmod(state.time, clip.duration);
                } else {
                    state.playing = false;
                    state.time = clip.duration;
                    emit clipFinished(layer.name, state.clipName);
                }
            }
        }
    }
}

void AnimationSystem::blendStates(Layer& layer, double deltaTime)
{
    Q_UNUSED(deltaTime);
    for (auto it = layer.states.begin(); it != layer.states.end(); ++it) {
        AnimationState& state = it.value();
        if (state.blendProgress < 1.0)
            state.blendProgress = qMin(1.0, state.blendProgress + deltaTime / qMax(0.001, state.blendTime));
    }
}

void AnimationSystem::evaluateClip(const Clip& clip, double time,
                                   QMap<QString, QVector3D>& positions,
                                   QMap<QString, QQuaternion>& rotations,
                                   QMap<QString, QVector3D>& scales) const
{
    for (auto it = clip.curves.begin(); it != clip.curves.end(); ++it) {
        QVariant val = it->evaluate(time);
        // Map curve names to bone transforms by convention
        if (it.key().endsWith(".pos"))
            positions[it.key().chopped(4)] = val.value<QVector3D>();
        else if (it.key().endsWith(".rot"))
            rotations[it.key().chopped(4)] = val.value<QQuaternion>();
        else if (it.key().endsWith(".scl"))
            scales[it.key().chopped(4)] = val.value<QVector3D>();
    }
}

const AnimationSystem::Clip* AnimationSystem::clip(const QString& name) const
{
    auto it = m_clips.find(name);
    return it != m_clips.end() ? &it.value() : nullptr;
}

const AnimationSystem::Layer* AnimationSystem::layer(const QString& name) const
{
    auto it = m_layers.find(name);
    return it != m_layers.end() ? &it.value() : nullptr;
}

} // namespace animation
} // namespace ks