#include "ParticleFormatParser.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace ks {

QString ParticleFormatParser::s_lastError;

bool ParticleFormatParser::load(const QString& filePath, ParticleFile& outFile)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        s_lastError = "Cannot open file: " + filePath;
        return false;
    }

    QByteArray rawData = file.readAll();
    file.close();

    if (rawData.left(1) == "{") {
        // JSON format
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(rawData, &error);
        if (error.error != QJsonParseError::NoError) {
            s_lastError = "JSON parse error: " + error.errorString();
            return false;
        }

        if (!doc.isObject()) {
            s_lastError = "Invalid particle file";
            return false;
        }

        QJsonObject root = doc.object();
        outFile.version = root.value("version").toString("1.0");
        outFile.name = root.value("name").toString();

        QJsonArray emittersArr = root.value("emitters").toArray();
        for (const QJsonValue& val : emittersArr) {
            outFile.emitters.append(parseEmitter(val.toObject()));
        }
    } else {
        // Legacy binary/text format fallback - TODO
        s_lastError = "Unsupported particle file format";
        return false;
    }

    return true;
}

bool ParticleFormatParser::save(const QString& filePath, const ParticleFile& file)
{
    QJsonObject root;
    root["version"] = file.version;
    root["name"] = file.name;

    QJsonArray emittersArr;
    for (const auto& emitter : file.emitters) {
        emittersArr.append(serializeEmitter(emitter));
    }
    root["emitters"] = emittersArr;

    QJsonDocument doc(root);
    QFile qFile(filePath);
    if (!qFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        s_lastError = "Cannot write file: " + filePath;
        return false;
    }

    qFile.write(doc.toJson(QJsonDocument::Indented));
    qFile.close();
    return true;
}

ParticleEmitter ParticleFormatParser::parseEmitter(const QJsonObject& obj)
{
    ParticleEmitter e;
    e.name = obj.value("name").toString();
    e.type = obj.value("type").toString("point");
    e.texturePath = obj.value("texture").toString();
    e.blendMode = obj.value("blendMode").toString("additive");

    e.maxParticles = obj.value("maxParticles").toInt(1000);
    e.emissionRate = obj.value("emissionRate").toDouble(100.0);
    e.lifetime = obj.value("lifetime").toDouble(2.0);
    e.lifetimeRandom = obj.value("lifetimeRandom").toDouble(0.0);

    e.spawnRadius = obj.value("spawnRadius").toDouble(0.0);
    e.spawnConeAngle = obj.value("spawnConeAngle").toDouble(0.0);
    e.velocity = obj.value("velocity").toDouble(5.0);
    e.velocityRandom = obj.value("velocityRandom").toDouble(0.0);

    e.startSize = obj.value("startSize").toDouble(0.1);
    e.endSize = obj.value("endSize").toDouble(0.0);
    e.sizeRandom = obj.value("sizeRandom").toDouble(0.0);

    auto readColor = [&](const QString& key, float* dst) {
        QJsonArray arr = obj.value(key).toArray();
        for (int i = 0; i < 4 && i < arr.size(); ++i) dst[i] = arr[i].toDouble();
    };
    readColor("startColor", e.startColor);
    readColor("endColor", e.endColor);

    auto readVec3 = [&](const QString& key, float* dst) {
        QJsonArray arr = obj.value(key).toArray();
        for (int i = 0; i < 3 && i < arr.size(); ++i) dst[i] = arr[i].toDouble();
    };
    readVec3("gravity", e.gravity);
    e.damping = obj.value("damping").toDouble(0.98);
    e.worldSpace = obj.value("worldSpace").toBool(false);

    e.startRotation = obj.value("startRotation").toDouble(0.0);
    e.endRotation = obj.value("endRotation").toDouble(0.0);
    e.angularVelocity = obj.value("angularVelocity").toDouble(0.0);

    e.sizeOverLife = parseCurve(obj.value("sizeOverLife").toArray());
    e.colorOverLife = parseCurve(obj.value("colorOverLife").toArray());
    e.alphaOverLife = parseCurve(obj.value("alphaOverLife").toArray());
    e.velocityOverLife = parseCurve(obj.value("velocityOverLife").toArray());

    if (obj.contains("floatProperties")) {
        QJsonObject fp = obj["floatProperties"].toObject();
        for (auto it = fp.begin(); it != fp.end(); ++it) {
            e.floatProperties[it.key()] = it.value().toDouble();
        }
    }

    if (obj.contains("stringProperties")) {
        QJsonObject sp = obj["stringProperties"].toObject();
        for (auto it = sp.begin(); it != sp.end(); ++it) {
            e.stringProperties[it.key()] = it.value().toString();
        }
    }

    return e;
}

QJsonObject ParticleFormatParser::serializeEmitter(const ParticleEmitter& emitter)
{
    QJsonObject obj;
    obj["name"] = emitter.name;
    obj["type"] = emitter.type;
    obj["texture"] = emitter.texturePath;
    obj["blendMode"] = emitter.blendMode;

    obj["maxParticles"] = emitter.maxParticles;
    obj["emissionRate"] = emitter.emissionRate;
    obj["lifetime"] = emitter.lifetime;
    obj["lifetimeRandom"] = emitter.lifetimeRandom;

    obj["spawnRadius"] = emitter.spawnRadius;
    obj["spawnConeAngle"] = emitter.spawnConeAngle;
    obj["velocity"] = emitter.velocity;
    obj["velocityRandom"] = emitter.velocityRandom;

    obj["startSize"] = emitter.startSize;
    obj["endSize"] = emitter.endSize;
    obj["sizeRandom"] = emitter.sizeRandom;

    obj["startColor"] = QJsonArray{emitter.startColor[0], emitter.startColor[1], emitter.startColor[2], emitter.startColor[3]};
    obj["endColor"] = QJsonArray{emitter.endColor[0], emitter.endColor[1], emitter.endColor[2], emitter.endColor[3]};
    obj["gravity"] = QJsonArray{emitter.gravity[0], emitter.gravity[1], emitter.gravity[2]};
    obj["damping"] = emitter.damping;
    obj["worldSpace"] = emitter.worldSpace;

    obj["startRotation"] = emitter.startRotation;
    obj["endRotation"] = emitter.endRotation;
    obj["angularVelocity"] = emitter.angularVelocity;

    obj["sizeOverLife"] = serializeCurve(emitter.sizeOverLife);
    obj["colorOverLife"] = serializeCurve(emitter.colorOverLife);
    obj["alphaOverLife"] = serializeCurve(emitter.alphaOverLife);
    obj["velocityOverLife"] = serializeCurve(emitter.velocityOverLife);

    QJsonObject fp;
    for (auto it = emitter.floatProperties.begin(); it != emitter.floatProperties.end(); ++it) {
        fp[it.key()] = it.value();
    }
    if (!fp.isEmpty()) obj["floatProperties"] = fp;

    QJsonObject sp;
    for (auto it = emitter.stringProperties.begin(); it != emitter.stringProperties.end(); ++it) {
        sp[it.key()] = it.value();
    }
    if (!sp.isEmpty()) obj["stringProperties"] = sp;

    return obj;
}

QVector<ParticleKeyframe> ParticleFormatParser::parseCurve(const QJsonArray& arr)
{
    QVector<ParticleKeyframe> curve;
    for (const QJsonValue& val : arr) {
        QJsonObject kf = val.toObject();
        ParticleKeyframe pk;
        pk.time = kf.value("time").toDouble();
        pk.value = kf.value("value").toDouble();
        curve.append(pk);
    }
    return curve;
}

QJsonArray ParticleFormatParser::serializeCurve(const QVector<ParticleKeyframe>& curve)
{
    QJsonArray arr;
    for (const auto& kf : curve) {
        QJsonObject obj;
        obj["time"] = kf.time;
        obj["value"] = kf.value;
        arr.append(obj);
    }
    return arr;
}

} // namespace ks