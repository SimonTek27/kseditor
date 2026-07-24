#pragma once

#include <QString>
#include <QByteArray>
#include <QVector>
#include <QMap>

namespace ks {

struct ParticleKeyframe {
    float time;
    float value;
};

struct ParticleEmitter {
    QString name;
    QString type = "point";
    QString texturePath;
    QString blendMode = "additive";

    // Emission
    int maxParticles = 1000;
    float emissionRate = 100.0f;
    float lifetime = 2.0f;
    float lifetimeRandom = 0.0f;

    // Spawn
    float spawnRadius = 0.0f;
    float spawnConeAngle = 0.0f;
    float velocity = 5.0f;
    float velocityRandom = 0.0f;

    // Size
    float startSize = 0.1f;
    float endSize = 0.0f;
    float sizeRandom = 0.0f;

    // Color
    float startColor[4] = {1, 1, 1, 1};
    float endColor[4] = {0, 0, 0, 0};

    // Physics
    float gravity[3] = {0, -9.81f, 0};
    float damping = 0.98f;
    bool worldSpace = false;

    // Rotation
    float startRotation = 0.0f;
    float endRotation = 0.0f;
    float angularVelocity = 0.0f;

    // Over Life curves
    QVector<ParticleKeyframe> sizeOverLife;
    QVector<ParticleKeyframe> colorOverLife;
    QVector<ParticleKeyframe> alphaOverLife;
    QVector<ParticleKeyframe> velocityOverLife;

    QMap<QString, float> floatProperties;
    QMap<QString, QString> stringProperties;
};

struct ParticleFile {
    QString version = "1.0";
    QString name;
    QVector<ParticleEmitter> emitters;
    QMap<QString, QByteArray> embeddedTextures;
};

class ParticleFormatParser {
public:
    static bool load(const QString& filePath, ParticleFile& outFile);
    static bool save(const QString& filePath, const ParticleFile& file);
    static QString lastError() { return s_lastError; }

private:
    static QString s_lastError;

    static ParticleEmitter parseEmitter(const QJsonObject& obj);
    static QJsonObject serializeEmitter(const ParticleEmitter& emitter);
    static QVector<ParticleKeyframe> parseCurve(const QJsonArray& arr);
    static QJsonArray serializeCurve(const QVector<ParticleKeyframe>& curve);
};

} // namespace ks