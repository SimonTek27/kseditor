#include "AnimationSystem.h"
#include <QTimer>
#include <QRandomGenerator>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ks {

// AnimationTimeline is implemented in TimelineEditor.cpp

// ============================================================================
// GraphCurve
// ============================================================================

float GraphCurve::evaluate(float frame) const
{
    if (keyframes.isEmpty()) return 0.0f;

    if (frame <= keyframes.first().frame) return keyframes.first().value;
    if (frame >= keyframes.last().frame) return keyframes.last().value;

    for (int i = 0; i < keyframes.size() - 1; ++i) {
        const Keyframe& k0 = keyframes[i];
        const Keyframe& k1 = keyframes[i + 1];

        if (frame >= k0.frame && frame <= k1.frame) {
            float t = (frame - k0.frame) / (k1.frame - k0.frame);

            switch (k0.interpolation) {
                case Keyframe::InterpolationConstant:
                    return k0.value;

                case Keyframe::InterpolationLinear:
                    return k0.value + (k1.value - k0.value) * t;

                case Keyframe::InterpolationBezier: {
                    float t2 = t * t;
                    float t3 = t2 * t;
                    float m0 = k0.intensity;
                    float m1 = k1.intensity;
                    return (2 * t3 - 3 * t2 + 1) * k0.value +
                           (t3 - 2 * t2 + t) * m0 +
                           (-2 * t3 + 3 * t2) * k1.value +
                           (t3 - t2) * m1;
                }

                case Keyframe::InterpolationQuadratic: {
                    return k0.value + (k1.value - k0.value) * t * t;
                }

                case Keyframe::InterpolationElastic: {
                    float c4 = (2 * M_PI) / 3;
                    if (t == 0) return k0.value;
                    if (t == 1) return k1.value;
                    float diff = k1.value - k0.value;
                    return k0.value + diff * (std::pow(2, -10 * t) * std::sin((t * 10 - 0.75) * c4) + 1);
                }

                case Keyframe::InterpolationBack: {
                    float c1 = 1.70158f;
                    float c3 = c1 + 1;
                    float diff = k1.value - k0.value;
                    return k0.value + diff * (1 + c3 * std::pow(t - 1, 3) + c1 * std::pow(t - 1, 2));
                }
            }
        }
    }

    return keyframes.last().value;
}

void GraphCurve::sortKeyframes()
{
    std::sort(keyframes.begin(), keyframes.end(), [](const Keyframe& a, const Keyframe& b) {
        return a.frame < b.frame;
    });
}

void GraphCurve::addKeyframe(const Keyframe& keyframe)
{
    keyframes.append(keyframe);
    sortKeyframes();
}

void GraphCurve::removeKeyframe(int frame)
{
    for (int i = 0; i < keyframes.size(); ++i) {
        if (keyframes[i].frame == frame) {
            keyframes.removeAt(i);
            return;
        }
    }
}

Keyframe* GraphCurve::getKeyframe(int frame)
{
    for (auto& kf : keyframes) {
        if (kf.frame == frame) return &kf;
    }
    return nullptr;
}

// ============================================================================
// GraphEditor
// ============================================================================

GraphEditor::GraphEditor(QObject* parent) : QObject(parent) {}
GraphEditor::~GraphEditor() = default;

void GraphEditor::setActiveAction(const QString& name) { m_activeAction = name; }

GraphEditor::FCurve* GraphEditor::getCurve(const QString& dataPath)
{
    for (auto& curve : m_curves) {
        if (curve.dataPath == dataPath) return &curve;
    }
    return nullptr;
}

void GraphEditor::addCurve(const QString& dataPath)
{
    FCurve curve;
    curve.dataPath = dataPath;
    curve.actionName = m_activeAction;
    curve.arrayIndex = 0;
    curve.dimensionCount = 1;
    m_curves.append(curve);
    emit curveAdded(dataPath);
}

void GraphEditor::removeCurve(const QString& dataPath)
{
    for (int i = 0; i < m_curves.size(); ++i) {
        if (m_curves[i].dataPath == dataPath) {
            m_curves.removeAt(i);
            emit curveRemoved(dataPath);
            return;
        }
    }
}

void GraphEditor::setInterpolation(const QString& dataPath, Keyframe::Interpolation interp)
{
    FCurve* curve = getCurve(dataPath);
    if (curve) {
        for (auto& kf : curve->keyframes) {
            kf.interpolation = interp;
        }
        emit curveUpdated(dataPath);
    }
}

void GraphEditor::setExtrapolation(const QString& dataPath, int mode)
{
    FCurve* curve = getCurve(dataPath);
    if (!curve) return;

    curve->extrapolationBefore = (mode & 0x1) ? ExtrapolationMode::Linear : ExtrapolationMode::Constant;
    curve->extrapolationAfter = (mode & 0x2) ? ExtrapolationMode::Linear : ExtrapolationMode::Constant;
    emit curveUpdated(dataPath);
}

void GraphEditor::snapToFrames()
{
    for (auto& curve : m_curves) {
        for (auto& kf : curve.keyframes) {
            kf.frame = kf.frame;
        }
    }
}

void GraphEditor::smoothKeyframes(const QString& dataPath)
{
    FCurve* curve = getCurve(dataPath);
    if (!curve || curve->keyframes.size() < 3) return;

    QVector<float> values;
    for (const auto& kf : curve->keyframes) values.append(kf.value);

    for (int i = 1; i < curve->keyframes.size() - 1; ++i) {
        curve->keyframes[i].value = (values[i - 1] + values[i] + values[i + 1]) / 3.0f;
    }
    emit curveUpdated(dataPath);
}

bool GraphEditor::bakeAction(const MeshData& input, int startFrame, int endFrame, const QMap<int, QMatrix4x4>& transforms)
{
    if (transforms.isEmpty()) return false;

    addCurve("location");
    addCurve("rotation");
    addCurve("scale");

    FCurve* locX = getCurve("location");
    FCurve* rotX = getCurve("rotation");
    FCurve* sclX = getCurve("scale");

    for (int frame = startFrame; frame <= endFrame; ++frame) {
        if (transforms.contains(frame)) {
            QMatrix4x4 m = transforms[frame];
            QVector3D pos = m.map(QVector3D(0, 0, 0));
            QVector3D scl(
                QVector3D(m(0,0), m(1,0), m(2,0)).length(),
                QVector3D(m(0,1), m(1,1), m(2,1)).length(),
                QVector3D(m(0,2), m(1,2), m(2,2)).length()
            );
            QVector3D rot = QQuaternion::fromAxes(
                QVector3D(m(0,0)/scl.x(), m(1,0)/scl.x(), m(2,0)/scl.x()),
                QVector3D(m(0,1)/scl.y(), m(1,1)/scl.y(), m(2,1)/scl.y()),
                QVector3D(m(0,2)/scl.z(), m(1,2)/scl.z(), m(2,2)/scl.z())
            ).toEulerAngles();

            if (locX) locX->addKeyframe(frame, pos.x(), 1.0f, 0.5f);
            if (rotX) rotX->addKeyframe(frame, rot.x(), 1.0f, 0.5f);
            if (sclX) sclX->addKeyframe(frame, scl.x(), 1.0f, 0.5f);
        }
    }

    return true;
}

void GraphEditor::FCurve::addKeyframe(int frame, float value, float intensity, float ease)
{
    Keyframe kf;
    kf.frame = frame;
    kf.value = value;
    kf.intensity = intensity;
    kf.ease = ease;
    keyframes.append(kf);
    std::sort(keyframes.begin(), keyframes.end(), [](const Keyframe& a, const Keyframe& b) {
        return a.frame < b.frame;
    });
}

void GraphEditor::FCurve::removeKeyframe(int frame)
{
    for (int i = 0; i < keyframes.size(); ++i) {
        if (keyframes[i].frame == frame) {
            keyframes.removeAt(i);
            return;
        }
    }
}

float GraphEditor::FCurve::evaluate(float frame) const
{
    if (keyframes.isEmpty()) return 0.0f;
    if (frame <= keyframes.first().frame) return keyframes.first().value;
    if (frame >= keyframes.last().frame) return keyframes.last().value;

    for (int i = 0; i < keyframes.size() - 1; ++i) {
        if (frame >= keyframes[i].frame && frame <= keyframes[i + 1].frame) {
            float t = (frame - keyframes[i].frame) / (keyframes[i + 1].frame - keyframes[i].frame);
            return keyframes[i].value + (keyframes[i + 1].value - keyframes[i].value) * t;
        }
    }
    return keyframes.last().value;
}

QVector3D GraphEditor::FCurve::getKeyframeTangent(int index) const
{
    if (index < 0 || index >= keyframes.size()) return QVector3D();
    const Keyframe& kf = keyframes[index];
    return QVector3D(kf.intensity, kf.ease, 0);
}

// ============================================================================
// DopeSheet
// ============================================================================

DopeSheet::DopeSheet(QObject* parent) : QObject(parent), m_summaryStart(1), m_summaryEnd(250) {}
DopeSheet::~DopeSheet() = default;

void DopeSheet::setActions(const QVector<Action>& actions)
{
    m_channels.clear();
    for (const auto& action : actions) {
        DopeSheetChannel ch;
        ch.name = action.name;
        ch.type = DopeSheetChannel::ChannelType::Object;
        ch.expanded = true;
        ch.actions.append(action);
        m_channels.append(ch);
    }
}

void DopeSheet::setSummaryChannel(int start, int end)
{
    m_summaryStart = start;
    m_summaryEnd = end;
}

void DopeSheet::autoBlend()
{
    for (auto& ch : m_channels) {
        for (int i = 1; i < ch.actions.size(); ++i) {
            int overlap = ch.actions[i - 1].frameEnd - ch.actions[i].frameStart;
            if (overlap > 0) {
                ch.actions[i].frameStart = ch.actions[i - 1].frameEnd + 1;
                ch.actions[i].frameEnd += overlap;
            }
        }
    }
}

void DopeSheet::selectKeyframesInRange(int start, int end)
{
    for (auto& ch : m_channels) {
        for (auto& kf : ch.keyframes) {
            kf.selected = (kf.frame >= start && kf.frame <= end);
        }
    }
}

void DopeSheet::deleteSelectedKeyframes() {
    for (auto& ch : m_channels) {
        ch.keyframes.erase(std::remove_if(ch.keyframes.begin(), ch.keyframes.end(),
            [](const Keyframe& kf) { return kf.selected; }), ch.keyframes.end());
    }
}

void DopeSheet::copySelectedKeyframes() {
    m_clipboard.clear();
    for (const auto& ch : m_channels) {
        for (const auto& kf : ch.keyframes) {
            if (kf.selected) m_clipboard.append(kf);
        }
    }
}

void DopeSheet::pasteKeyframes() {
    if (m_clipboard.isEmpty()) return;
    for (auto& ch : m_channels) {
        for (const auto& kf : m_clipboard) {
            ch.keyframes.append(kf);
        }
        std::sort(ch.keyframes.begin(), ch.keyframes.end(),
            [](const Keyframe& a, const Keyframe& b) { return a.frame < b.frame; });
    }
}

// ============================================================================
// NLAEditor
// ============================================================================

NLAEditor::NLAEditor(QObject* parent) : QObject(parent) {}
NLAEditor::~NLAEditor() = default;

void NLAEditor::addTrack(const QString& name)
{
    NLATrack track;
    track.name = name;
    track.locked = false;
    track.muted = false;
    track.selected = false;
    m_tracks.append(track);
    emit tracksChanged();
}

void NLAEditor::removeTrack(int index)
{
    if (index >= 0 && index < m_tracks.size()) {
        m_tracks.removeAt(index);
        emit tracksChanged();
    }
}

void NLAEditor::addStrip(int trackIndex, const NLAStrip& strip)
{
    if (trackIndex >= 0 && trackIndex < m_tracks.size()) {
        m_tracks[trackIndex].strips.append(strip);
        emit tracksChanged();
    }
}

void NLAEditor::removeStrip(int trackIndex, int stripIndex)
{
    if (trackIndex >= 0 && trackIndex < m_tracks.size() &&
        stripIndex >= 0 && stripIndex < m_tracks[trackIndex].strips.size()) {
        m_tracks[trackIndex].strips.removeAt(stripIndex);
        emit tracksChanged();
    }
}

void NLAEditor::moveStrip(int fromTrack, int toTrack, int stripIndex)
{
    if (fromTrack >= 0 && fromTrack < m_tracks.size() &&
        toTrack >= 0 && toTrack < m_tracks.size() &&
        stripIndex >= 0 && stripIndex < m_tracks[fromTrack].strips.size()) {
        NLAStrip strip = m_tracks[fromTrack].strips.takeAt(stripIndex);
        m_tracks[toTrack].strips.append(strip);
        emit tracksChanged();
    }
}

void NLAEditor::scaleStrip(int trackIndex, int stripIndex, float scale)
{
    if (trackIndex >= 0 && trackIndex < m_tracks.size() &&
        stripIndex >= 0 && stripIndex < m_tracks[trackIndex].strips.size()) {
        auto& strip = m_tracks[trackIndex].strips[stripIndex];
        int duration = strip.frameEnd - strip.frameStart;
        int newDuration = static_cast<int>(duration * scale);
        strip.frameEnd = strip.frameStart + newDuration;
        emit tracksChanged();
    }
}

void NLAEditor::setBlending(int trackIndex, int stripIndex, float blendIn, float blendOut)
{
    if (trackIndex >= 0 && trackIndex < m_tracks.size() &&
        stripIndex >= 0 && stripIndex < m_tracks[trackIndex].strips.size()) {
        m_tracks[trackIndex].strips[stripIndex].blendIn = blendIn;
        m_tracks[trackIndex].strips[stripIndex].blendOut = blendOut;
        emit tracksChanged();
    }
}

void NLAEditor::setActiveAction(const QString& actionName) { m_activeAction = actionName; }

void NLAEditor::pushToStack(const QString& actionName) { m_actionStack.append(actionName); }
void NLAEditor::popFromStack() { if (!m_actionStack.isEmpty()) m_actionStack.removeLast(); }

// ============================================================================
// DriversEditor
// ============================================================================

DriversEditor::DriversEditor(QObject* parent) : QObject(parent) {}
DriversEditor::~DriversEditor() = default;

void DriversEditor::addDriver(const QString& dataPath, Driver::Type type)
{
    Driver driver;
    driver.dataPath = dataPath;
    driver.type = type;
    driver.mode = Driver::Mode::Averaging;
    driver.expression = 0;
    m_drivers[dataPath] = driver;
    emit driverUpdated(dataPath);
}

void DriversEditor::removeDriver(const QString& dataPath)
{
    m_drivers.remove(dataPath);
}

void DriversEditor::addVariable(const QString& driverPath, const DriverVariable& variable)
{
    if (m_drivers.contains(driverPath)) {
        m_drivers[driverPath].variables.append(variable);
        emit driverUpdated(driverPath);
    }
}

void DriversEditor::removeVariable(const QString& driverPath, const QString& variableName)
{
    if (m_drivers.contains(driverPath)) {
        auto& driver = m_drivers[driverPath];
        for (int i = 0; i < driver.variables.size(); ++i) {
            if (driver.variables[i].name == variableName) {
                driver.variables.removeAt(i);
                emit driverUpdated(driverPath);
                return;
            }
        }
    }
}

void DriversEditor::setExpression(const QString& driverPath, const QString& expression)
{
    if (m_drivers.contains(driverPath)) {
        m_drivers[driverPath].mode = Driver::Mode::Scripted;
        emit driverUpdated(driverPath);
    }
}

void DriversEditor::evaluateDriver(const Driver& driver, float time)
{
    float result = 0.0f;
    for (const auto& var : driver.variables) {
        float val = evaluateVariable(var, time);
        switch (driver.mode) {
            case Driver::Mode::Averaging: result += val; break;
            case Driver::Mode::Minimum: result = (result == 0.0f) ? val : std::min(result, val); break;
            case Driver::Mode::Maximum: result = (result == 0.0f) ? val : std::max(result, val); break;
            case Driver::Mode::Multiply: result = (result == 0.0f) ? val : result * val; break;
            default: result += val; break;
        }
    }
    if (driver.type == Driver::Type::Average && !driver.variables.isEmpty()) {
        result /= driver.variables.size();
    }
    emit driverEvaluated(driver.dataPath, result);
}

float DriversEditor::evaluateVariable(const DriverVariable& variable, float time)
{
    switch (variable.type) {
        case DriverVariable::Type::SingleProperty: {
            auto* curve = m_graphEditor->getCurve(variable.propertyPath);
            if (curve) return curve->evaluate(time);
            return 0.0f;
        }
        case DriverVariable::Type::TransformChannel: {
            return variable.value;
        }
        case DriverVariable::Type::Distance: {
            return variable.distance;
        }
    }
    return 0.0f;
}

} // namespace ks
