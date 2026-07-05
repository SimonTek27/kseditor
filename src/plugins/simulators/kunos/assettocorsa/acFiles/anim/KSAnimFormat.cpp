#include "KSAnimFormat.h"
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <cmath>

namespace {

QQuaternion quatSlerp(const QQuaternion &q1, const QQuaternion &q2, float t) {
    float dot = QQuaternion::dotProduct(q1, q2);
    float sign = 1.0f;
    if (dot < 0.0f) { dot = -dot; sign = -1.0f; }
    if (dot > 0.9999f) return (q1 * (1.0f - t) + q2 * t * sign).normalized();
    float angle = std::acos(dot);
    float sinAngle = std::sin(angle);
    float a1 = std::sin((1.0f - t) * angle) / sinAngle;
    float a2 = std::sin(t * angle) / sinAngle * sign;
    return q1 * a1 + q2 * a2;
}

} // anonymous namespace

namespace ks {

QMatrix4x4 KSAnimKeyframe::toMatrix() const {
    QMatrix4x4 m;
    m.rotate(rotation);
    m.translate(translation);
    m.scale(scale);
    return m;
}

KSAnim::KSAnim()
    : header({1})
    , originalFilename(QString())
{
}

KSAnim::KSAnim(const QString& filename) {
    fromFile(filename);
}

KSAnim* KSAnim::load(const QString& filename) {
    return new KSAnim(filename);
}

void KSAnim::fromFile(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open KSAnim file:" << filename;
        return;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    readHeader(stream);
    readEntries(stream);

    originalFilename = filename;
    file.close();
}

void KSAnim::readHeader(QDataStream& stream) {
    qint32 version;
    stream >> version;
    header.version = version;
}

void KSAnim::readEntries(QDataStream& stream) {
    qint32 count;
    stream >> count;

    for (qint32 i = 0; i < count; ++i) {
        QString nodeName;
        stream >> nodeName;

        qint32 keyframeCount;
        stream >> keyframeCount;

        if (keyframeCount <= 0) continue;

        if (header.version == 2) {
            auto* entry = new KSAnimEntryV2();
            entry->nodeNameValue = nodeName;
            entry->keyframes.resize(keyframeCount);

            for (qint32 k = 0; k < keyframeCount; ++k) {
                float qx, qy, qz, qw;
                float tx, ty, tz;
                float sx, sy, sz;

                stream >> qx >> qy >> qz >> qw;
                stream >> tx >> ty >> tz;
                stream >> sx >> sy >> sz;

                entry->keyframes[k].rotation = QQuaternion(qw, qx, qy, qz);
                entry->keyframes[k].translation = QVector3D(tx, ty, tz);
                entry->keyframes[k].scale = QVector3D(sx, sy, sz);
            }

            entries[nodeName] = entry;
        } else {
            auto* entry = new KSAnimEntryV1();
            entry->nodeNameValue = nodeName;
            entry->matrices.resize(keyframeCount);

            for (qint32 k = 0; k < keyframeCount; ++k) {
                float m[16];
                for (int j = 0; j < 16; ++j) {
                    stream >> m[j];
                }
                entry->matrices[k].setColumn(0, QVector4D(m[0], m[1], m[2], m[3]));
                entry->matrices[k].setColumn(1, QVector4D(m[4], m[5], m[6], m[7]));
                entry->matrices[k].setColumn(2, QVector4D(m[8], m[9], m[10], m[11]));
                entry->matrices[k].setColumn(3, QVector4D(m[12], m[13], m[14], m[15]));
            }

            entries[nodeName] = entry;
        }
    }
}

bool KSAnim::save(const QString& filename) const {
    return save(filename, false);
}

bool KSAnim::save(const QString& filename, bool overwrite) const {
    if (!overwrite && QFile::exists(filename)) {
        qWarning() << "File already exists:" << filename;
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Failed to open file for writing:" << filename;
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << static_cast<qint32>(header.version);
    stream << static_cast<qint32>(entries.size());

    for (auto it = entries.constBegin(); it != entries.constEnd(); ++it) {
        QString nodeName = it.key();
        stream << nodeName;

        KSAnimEntryBase* entry = it.value();
        stream << static_cast<qint32>(entry->keyframeCount());

        if (auto* v2 = dynamic_cast<KSAnimEntryV2*>(entry)) {
            for (const auto& kf : v2->keyframes) {
                stream << kf.rotation.x() << kf.rotation.y()
                       << kf.rotation.z() << kf.rotation.scalar();
                stream << kf.translation.x() << kf.translation.y() << kf.translation.z();
                stream << kf.scale.x() << kf.scale.y() << kf.scale.z();
            }
        } else if (auto* v1 = dynamic_cast<KSAnimEntryV1*>(entry)) {
            for (const auto& m : v1->matrices) {
                for (int j = 0; j < 16; ++j) {
                    stream << m.data()[j];
                }
            }
        }
    }

    file.close();
    return true;
}

int KSAnim::totalKeyframes() const {
    int total = 0;
    for (auto* entry : entries) {
        total += entry->keyframeCount();
    }
    return total;
}

QStringList KSAnim::animatedNodes() const {
    return QStringList(entries.keys());
}

int KSAnim::maxKeyframesForNode(const QString& node) const {
    if (!entries.contains(node)) return 0;
    return entries[node]->keyframeCount();
}

void KSAnim::addEntry(const QString& nodeName, KSAnimEntryV2* entry) {
    if (entries.contains(nodeName)) {
        delete entries[nodeName];
    }
    entries[nodeName] = entry;
}

void KSAnim::removeEntry(const QString& nodeName) {
    if (entries.contains(nodeName)) {
        delete entries[nodeName];
        entries.remove(nodeName);
    }
}

QMatrix4x4 KSAnim::getTransform(const QString& nodeName, float t) const {
    int index = 0;
    if (!entries.contains(nodeName)) return QMatrix4x4();

    int keyframeCount = entries[nodeName]->keyframeCount();
    if (keyframeCount <= 0) return QMatrix4x4();

    if (t >= 1.0f) {
        index = keyframeCount - 1;
    } else if (t > 0.0f) {
        float rawIndex = t * static_cast<float>(keyframeCount - 1);
        int i0 = static_cast<int>(std::floor(rawIndex));
        int i1 = std::min(i0 + 1, keyframeCount - 1);
        float frac = rawIndex - static_cast<float>(i0);

        if (auto* v2 = dynamic_cast<KSAnimEntryV2*>(entries[nodeName])) {
            const auto& kf0 = v2->keyframes[i0];
            const auto& kf1 = v2->keyframes[i1];

            QQuaternion r = quatSlerp(kf0.rotation, kf1.rotation, frac);
            QVector3D p = kf0.translation + (kf1.translation - kf0.translation) * frac;
            QVector3D s = kf0.scale + (kf1.scale - kf0.scale) * frac;

            QMatrix4x4 m;
            m.rotate(r);
            m.translate(p);
            m.scale(s);
            return m;
        }
    }
    return getTransform(nodeName, index);
}

QMatrix4x4 KSAnim::getTransform(const QString& nodeName, int keyframeIndex) const {
    if (!entries.contains(nodeName)) return QMatrix4x4();

    auto* entry = entries[nodeName];

    if (auto* v2 = dynamic_cast<KSAnimEntryV2*>(entry)) {
        if (keyframeIndex >= 0 && keyframeIndex < v2->keyframes.size()) {
            return v2->keyframes[keyframeIndex].toMatrix();
        }
    } else if (auto* v1 = dynamic_cast<KSAnimEntryV1*>(entry)) {
        if (keyframeIndex >= 0 && keyframeIndex < v1->matrices.size()) {
            return v1->matrices[keyframeIndex];
        }
    }

    return QMatrix4x4();
}

bool KSAnim::isValidFile(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    qint32 version;
    stream >> version;

    file.close();
    return version == 1 || version == 2;
}

KSAnimAnimator::KSAnimAnimator(const QString& filename, float duration, bool skipFixed)
    : m_Duration(duration)
    , m_SkipFixed(skipFixed)
{
    try {
        KSAnim* anim = KSAnim::load(filename);
        if (anim->entries.isEmpty()) {
            m_Error = "Animation has no entries";
            m_IsValid = false;
            return;
        }
        m_Animations.append(anim);
        m_IsValid = true;
    } catch (...) {
        m_Error = "Failed to load animation";
        m_IsValid = false;
    }
}

void KSAnimAnimator::setProgress(float t) {
    if (m_Clamp) {
        t = std::max(0.0f, std::min(1.0f, t));
    } else {
        float loopDuration = static_cast<float>(m_Animations.isEmpty() ? 1 : 1);
        t = std::fmod(t, loopDuration);
        if (t < 0) t += loopDuration;
    }

    m_Progress = t;
    updateLinkedAnimators();
}

void KSAnimAnimator::link(KSAnimAnimator* other) {
    if (other && !m_Linked.contains(other)) {
        m_Linked.append(other);
    }
}

void KSAnimAnimator::updateLinkedAnimators() {
    for (auto* linked : m_Linked) {
        linked->m_Progress = m_Progress;
    }
}

QVector<KSAnim*> KSAnimAnimator::animations() const {
    return m_Animations;
}

}