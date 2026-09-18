#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector3D>
#include <QVariant>

namespace ks {

struct KSAnimKeyframe {
    QQuaternion rotation;
    QVector3D translation;
    QVector3D scale;

    QMatrix4x4 toMatrix() const;
};

class KSAnimEntryBase {
public:
    virtual ~KSAnimEntryBase() = default;
    virtual int keyframeCount() const = 0;
    virtual QString nodeName() const = 0;
    virtual KSAnimEntryBase* clone() const = 0;
};

class KSAnimEntryV1 : public KSAnimEntryBase {
public:
    QString nodeNameValue;
    QVector<QMatrix4x4> matrices;

    int keyframeCount() const override { return matrices.size(); }
    QString nodeName() const override { return nodeNameValue; }
    KSAnimEntryBase* clone() const override { return new KSAnimEntryV1(*this); }
};

class KSAnimEntryV2 : public KSAnimEntryBase {
public:
    QString nodeNameValue;
    QVector<KSAnimKeyframe> keyframes;

    int keyframeCount() const override { return keyframes.size(); }
    QString nodeName() const override { return nodeNameValue; }
    KSAnimEntryBase* clone() const override { return new KSAnimEntryV2(*this); }
};

struct KSAnimHeader {
    int version;
};

class KSAnim {
public:
    KSAnim();
    explicit KSAnim(const QString& filename);

    static KSAnim* load(const QString& filename);
    bool save(const QString& filename) const;
    bool save(const QString& filename, bool overwrite) const;

    KSAnimHeader header;
    QMap<QString, KSAnimEntryBase*> entries;
    QString originalFilename;

    int totalKeyframes() const;
    QStringList animatedNodes() const;
    int maxKeyframesForNode(const QString& node) const;

    void addEntry(const QString& nodeName, KSAnimEntryV2* entry);
    void removeEntry(const QString& nodeName);

    QMatrix4x4 getTransform(const QString& nodeName, float t) const;
    QMatrix4x4 getTransform(const QString& nodeName, int keyframeIndex) const;

    static bool isValidFile(const QString& filename);

private:
    void fromFile(const QString& filename);
    void readHeader(QDataStream& stream);
    void readEntries(QDataStream& stream);
};

class KSAnimAnimator {
public:
    KSAnimAnimator(const QString& filename, float duration = 1.0f, bool skipFixed = true);

    void setProgress(float t);
    float progress() const { return m_Progress; }
    float duration() const { return m_Duration; }
    bool isValid() const { return m_IsValid; }
    const QString& error() const { return m_Error; }

    void link(KSAnimAnimator* other);
    QVector<KSAnim*> animations() const;

private:
    void updateLinkedAnimators();

    float m_Progress = 0.0f;
    float m_Duration = 1.0f;
    float m_Speed = 1.0f;
    bool m_Clamp = true;
    bool m_SkipFixed = true;
    bool m_IsValid = false;
    QString m_Error;
    QVector<KSAnim*> m_Animations;
    QVector<KSAnimAnimator*> m_Linked;
};

}