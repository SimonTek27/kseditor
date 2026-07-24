#include "AnimationFileParser.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace ks {

QString AnimationFileParser::s_lastError;

bool AnimationFileParser::load(const QString& filePath, AnimFile& outAnim)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        s_lastError = "Cannot open file: " + filePath;
        return false;
    }

    QByteArray rawData = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(rawData, &error);
    if (error.error != QJsonParseError::NoError) {
        s_lastError = "JSON parse error: " + error.errorString();
        return false;
    }

    if (!doc.isObject()) {
        s_lastError = "Invalid animation file";
        return false;
    }

    QJsonObject root = doc.object();
    outAnim.version = root.value("version").toString("1.0");

    if (root.contains("metadata")) {
        QJsonObject meta = root["metadata"].toObject();
        for (auto it = meta.begin(); it != meta.end(); ++it) {
            outAnim.metadata[it.key()] = it.value().toString();
        }
    }

    QJsonArray clipsArr = root.value("clips").toArray();
    for (const QJsonValue& clipVal : clipsArr) {
        QJsonObject clipObj = clipVal.toObject();
        AnimClip clip;
        clip.name = clipObj.value("name").toString();
        clip.duration = clipObj.value("duration").toDouble(1.0);
        clip.startTime = clipObj.value("startTime").toDouble(0.0);
        clip.endTime = clipObj.value("endTime").toDouble(clip.duration);
        clip.loop = clipObj.value("loop").toBool(false);
        clip.wrapMode = clipObj.value("wrapMode").toString("clamp");

        QJsonArray channelsArr = clipObj.value("channels").toArray();
        for (const QJsonValue& chVal : channelsArr) {
            QJsonObject chObj = chVal.toObject();
            AnimChannel ch;
            ch.name = chObj.value("name").toString();
            ch.targetPath = chObj.value("target").toString();
            ch.property = chObj.value("property").toString("custom");
            ch.componentCount = chObj.value("components").toInt(1);

            QJsonArray kfArr = chObj.value("keyframes").toArray();
            for (const QJsonValue& kfVal : kfArr) {
                QJsonObject kfObj = kfVal.toObject();
                AnimKeyframe kf;
                kf.time = kfObj.value("time").toDouble();
                kf.interpolation = kfObj.value("interpolation").toString("linear");

                if (ch.componentCount == 1) {
                    kf.value = kfObj.value("value").toDouble();
                } else if (ch.componentCount == 3) {
                    QJsonArray v3 = kfObj.value("value").toArray();
                    if (v3.size() >= 3) kf.value3 = Vec3(v3[0].toDouble(), v3[1].toDouble(), v3[2].toDouble());
                } else if (ch.componentCount == 4) {
                    QJsonArray v4 = kfObj.value("value").toArray();
                    if (v4.size() >= 4) kf.value4 = Vec4(v4[0].toDouble(), v4[1].toDouble(), v4[2].toDouble(), v4[3].toDouble());
                }

                ch.keyframes.append(kf);
            }

            clip.channels.append(ch);
        }

        outAnim.clips.append(clip);
    }

    return true;
}

bool AnimationFileParser::save(const QString& filePath, const AnimFile& anim)
{
    QJsonObject root;
    root["version"] = anim.version;

    QJsonObject meta;
    for (auto it = anim.metadata.begin(); it != anim.metadata.end(); ++it) {
        meta[it.key()] = it.value();
    }
    if (!meta.isEmpty()) root["metadata"] = meta;

    QJsonArray clipsArr;
    for (const auto& clip : anim.clips) {
        QJsonObject clipObj;
        clipObj["name"] = clip.name;
        clipObj["duration"] = clip.duration;
        clipObj["startTime"] = clip.startTime;
        clipObj["endTime"] = clip.endTime;
        clipObj["loop"] = clip.loop;
        clipObj["wrapMode"] = clip.wrapMode;

        QJsonArray chArr;
        for (const auto& ch : clip.channels) {
            QJsonObject chObj;
            chObj["name"] = ch.name;
            chObj["target"] = ch.targetPath;
            chObj["property"] = ch.property;
            chObj["components"] = ch.componentCount;

            QJsonArray kfArr;
            for (const auto& kf : ch.keyframes) {
                QJsonObject kfObj;
                kfObj["time"] = kf.time;
                kfObj["interpolation"] = kf.interpolation;

                if (ch.componentCount == 1) {
                    kfObj["value"] = kf.value;
                } else if (ch.componentCount == 3) {
                    kfObj["value"] = QJsonArray{kf.value3.x, kf.value3.y, kf.value3.z};
                } else if (ch.componentCount == 4) {
                    kfObj["value"] = QJsonArray{kf.value4.x, kf.value4.y, kf.value4.z, kf.value4.w};
                }

                kfArr.append(kfObj);
            }
            chObj["keyframes"] = kfArr;
            chArr.append(chObj);
        }
        clipObj["channels"] = chArr;
        clipsArr.append(clipObj);
    }
    root["clips"] = clipsArr;

    QJsonDocument doc(root);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        s_lastError = "Cannot write file: " + filePath;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

float AnimationFileParser::sampleChannel(const AnimChannel& channel, float time)
{
    if (channel.keyframes.isEmpty()) return 0.0f;
    if (channel.keyframes.size() == 1) return channel.keyframes[0].value;

    int idx = findKeyframeIndex(channel.keyframes, time);
    if (idx < 0) return channel.keyframes.first().value;
    if (idx >= channel.keyframes.size() - 1) return channel.keyframes.last().value;

    const auto& kf0 = channel.keyframes[idx];
    const auto& kf1 = channel.keyframes[idx + 1];
    float t = (kf1.time > kf0.time) ? (time - kf0.time) / (kf1.time - kf0.time) : 0.0f;

    if (kf0.interpolation == "step") return kf0.value;
    return kf0.value * (1.0f - t) + kf1.value * t;
}

Vec3 AnimationFileParser::sampleChannel3(const AnimChannel& channel, float time)
{
    if (channel.keyframes.isEmpty()) return Vec3();
    if (channel.keyframes.size() == 1) return channel.keyframes[0].value3;

    int idx = findKeyframeIndex(channel.keyframes, time);
    if (idx < 0) return channel.keyframes.first().value3;
    if (idx >= channel.keyframes.size() - 1) return channel.keyframes.last().value3;

    const auto& kf0 = channel.keyframes[idx];
    const auto& kf1 = channel.keyframes[idx + 1];
    float t = (kf1.time > kf0.time) ? (time - kf0.time) / (kf1.time - kf0.time) : 0.0f;

    if (kf0.interpolation == "step") return kf0.value3;
    return lerp(kf0.value3, kf1.value3, t);
}

Quat AnimationFileParser::sampleRotation(const AnimChannel& channel, float time)
{
    if (channel.keyframes.isEmpty()) return Quat();
    if (channel.keyframes.size() == 1) {
        auto v = channel.keyframes[0].value4;
        return Quat(v.x, v.y, v.z, v.w);
    }

    int idx = findKeyframeIndex(channel.keyframes, time);
    if (idx < 0) {
        auto v = channel.keyframes.first().value4;
        return Quat(v.x, v.y, v.z, v.w);
    }
    if (idx >= channel.keyframes.size() - 1) {
        auto v = channel.keyframes.last().value4;
        return Quat(v.x, v.y, v.z, v.w);
    }

    const auto& kf0 = channel.keyframes[idx];
    const auto& kf1 = channel.keyframes[idx + 1];
    float t = (kf1.time > kf0.time) ? (time - kf0.time) / (kf1.time - kf0.time) : 0.0f;

    if (kf0.interpolation == "step") {
        auto v = kf0.value4;
        return Quat(v.x, v.y, v.z, v.w);
    }

    Quat q0(kf0.value4.x, kf0.value4.y, kf0.value4.z, kf0.value4.w);
    Quat q1(kf1.value4.x, kf1.value4.y, kf1.value4.z, kf1.value4.w);
    return q0.slerp(q1, t);
}

int AnimationFileParser::findKeyframeIndex(const QVector<AnimKeyframe>& keyframes, float time)
{
    if (keyframes.isEmpty()) return -1;
    if (time <= keyframes[0].time) return 0;

    for (int i = 0; i < keyframes.size() - 1; ++i) {
        if (time >= keyframes[i].time && time < keyframes[i + 1].time)
            return i;
    }

    return keyframes.size() - 1;
}

} // namespace ks