#include "MorphTargetEditor.h"
#include <cmath>
#include <algorithm>
#include <QSet>

MorphTargetEditor::MorphTargetEditor(QObject* parent) : QObject(parent) {}

MorphTargetEditor::~MorphTargetEditor() = default;

int MorphTargetEditor::addMorphTarget(const QString& name, int vertexCount) {
    MorphTarget target(name, QVector<QVector3D>(vertexCount, QVector3D(0, 0, 0)));
    m_targets.append(target);
    if (m_targets.size() == 1) {
        setCurrentTarget(0);
    }
    emit targetAdded(m_targets.size() - 1);
    emit targetsChanged();
    return m_targets.size() - 1;
}

bool MorphTargetEditor::removeMorphTarget(int index) {
    if (index < 0 || index >= m_targets.size()) return false;
    if (m_targets.size() <= 1) return false; // Keep at least one
    
    m_targets.removeAt(index);
    
    // Adjust current target index if needed
    if (m_currentTarget >= m_targets.size()) {
        m_currentTarget = m_targets.size() - 1;
    }
    if (m_currentTarget < 0) m_currentTarget = 0;
    
    emit targetRemoved(index);
    emit targetsChanged();
    emit currentTargetChanged(m_currentTarget);
    return true;
}

bool MorphTargetEditor::renameMorphTarget(int index, const QString& newName) {
    if (index < 0 || index >= m_targets.size()) return false;
    m_targets[index].name = newName;
    emit targetRenamed(index, newName);
    return true;
}

void MorphTargetEditor::setMorphTargetWeight(int index, float weight) {
    if (index < 0 || index >= m_targets.size()) return;
    m_targets[index].weight = qBound(0.0f, weight, 1.0f);
    emit targetWeightChanged(index, m_targets[index].weight);
}

float MorphTargetEditor::morphTargetWeight(int index) const {
    if (index < 0 || index >= m_targets.size()) return 0.0f;
    return m_targets[index].weight;
}

QString MorphTargetEditor::currentTargetName() const {
    if (m_currentTarget < 0 || m_currentTarget >= m_targets.size()) return "";
    return m_targets[m_currentTarget].name;
}

QStringList MorphTargetEditor::targetNames() const {
    QStringList names;
    for (const auto& target : m_targets) {
        names.append(target.name);
    }
    return names;
}

void MorphTargetEditor::setCurrentTarget(int index) {
    if (index < 0 || index >= m_targets.size()) return;
    if (m_currentTarget != index) {
        m_currentTarget = index;
        emit currentTargetChanged(index);
    }
}

int MorphTargetEditor::sculptBrushToTarget(int targetIndex, const QVector3D& center, float radius, float strength, int mode,
    const QVector3D& drag, const QVector3D& previousCenter,
    float falloffPower, const QSet<int>* pinned) {
    if (targetIndex < 0 || targetIndex >= m_targets.size()) return 0;
    if (m_targets[targetIndex].enabled == false) return 0;
    
    MorphTarget& target = m_targets[targetIndex];
    int affected = 0;
    
    // Apply brush to this morph target's position deltas
    for (int vi = 0; vi < target.positionDeltas.size(); ++vi) {
        // Skip pinned vertices
        if (pinned && pinned->contains(vi)) continue;
        
        float d = (QVector3D(vi % 100 - 50, 0, vi / 100 - 50) - center).length(); // placeholder distance
        if (d > radius) continue;
        
        float t = 1.0f - (d / radius);
        float falloff;
        if (qAbs(falloffPower - 2.0f) < 1e-4f)
            falloff = t * t * (3.0f - 2.0f * t);
        else
            falloff = t > 0.0f ? qPow(t, qBound(0.25f, falloffPower, 8.0f)) : 0.0f;
        if (falloff <= 0.001f) continue;
        affected++;
        
        // Apply based on mode
        switch (mode) {
            case 0: // draw - add displacement along normal
                // In a real implementation, would use the actual vertex normal
                target.positionDeltas[vi] += QVector3D(0, 1, 0) * (strength * falloff); // placeholder up direction
                break;
            case 1: // smooth - average with neighbors (placeholder)
                // Would need neighbor averaging
                break;
            case 2: // grab - pull toward center
                // Would need center position logic
                break;
            case 3: // flatten toward plane
                // Would need plane calculation
                break;
            case 4: // crease
                target.positionDeltas[vi] -= QVector3D(0, 1, 0) * (strength * falloff * 0.005f); // placeholder
                break;
            case 5: // inflate - push outward
                target.positionDeltas[vi] += QVector3D(0, 1, 0) * (strength * falloff * (1.0f + falloff)); // placeholder boost at center
                break;
            case 6: // pinch - pull toward center
                // Would need center position logic
                break;
            case 7: // smear - follow cursor movement
                // Would need cursor delta
                break;
            case 8: // negate - inverse of draw
                target.positionDeltas[vi] -= QVector3D(0, 1, 0) * (strength * falloff * 1.5f); // placeholder
                break;
            default:
                break;
        }
    }
    
    emit sculptUpdated();
    return affected;
}