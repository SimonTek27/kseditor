#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QVector>

namespace ks {

struct CspLightConfig {
    QString name;
    QString meshName;
    QString materialName;
    QString color;
    QString colorOff;
    QString position;
    QString direction;
    double spot = 0.0;
    double spotSharpness = 0.5;
    double range = 20.0;
    double rangeGradientOffset = 0.2;
    double fadeAt = 500.0;
    double fadeSmooth = 30.0;
    double specularMult = 1.0;
    bool active = true;
    QString condition;
    int clusterThreshold = 10;
    double diffuseConcentration = 0.5;
};

struct CspEmissiveConfig {
    QString name;
    QString meshName;
    QString materialName;
    QString color;
    QString colorOff;
    QString location;
    double lag = 0.0;
    double simulateHeating = 0.0;
    bool bindToHeadlights = false;
    bool active = true;
    QString condition;
};

struct CspBrakeDiscConfig {
    double ambientMult = 0.6;
    double reflectionMult = 1.0;
    double discInternalRadius = 0.126;
    double discInternalRadiusSharpness = 250.0;
    double rimInternalRadius = 0.06;
    double simplifyNormalsK = 0.9;
    bool isFront = false;
    bool isRear = false;
};

struct CspTrackLightConfig {
    QString name;
    QString meshName;
    QString materialName;
    QString position;
    QString direction;
    QString color;
    QString colorOff;
    double spot = 150.0;
    double spotSharpness = 0.7;
    double range = 20.0;
    double fadeAt = 700.0;
    double fadeSmooth = 100.0;
    bool active = true;
    QString condition;
};

struct CspMaterialAdjustment {
    int index = 0;
    bool active = true;
    QStringList meshes;
    QStringList materials;
    QString condition;
    QString key0;
    QVariantMap values0;
    QVariantMap values0Off;
    QString key1;
    QVariantMap values1;
    QVariantMap values1Off;
    double range = 20.0;
    double rangeGradientOffset = 0.2;
    double spot = 75.0;
    double spotSharpness = 0.5;
    QString blendMode;
};

struct CspCondition {
    QString name;
    QString input;
    double flashingFrequency = 0.0;
    double flashingMinValue = 0.0;
    double flashingNoiseAmplitude = 0.0;
    double flashingNoiseBound = 0.0;
    double flashingNoiseSpeed = 500.0;
    bool flashingSkippedOffState = false;
    bool flashingSkippedDownhillState = false;
    bool flashingSynced = false;
    QString flashingSmoothness;
    QString flashingLut;
    double inputChangeDelay = 0.5;
    double inputStayFor = 1.0;
    double lag = 0.0;
    double lagDelayOff = 1.0;
    double lagDelayOn = 5.0;
    QString lagDelayFunc;
    double simulateHeating = 0.0;
};

class CspConfigParser : public QObject {
    Q_OBJECT

public:
    explicit CspConfigParser(QObject* parent = nullptr);

    static CspConfigParser* instance();

    QVariantMap loadCarConfig(const QString& path);
    QVariantMap loadTrackConfig(const QString& path);

    bool saveCarConfig(const QString& path, const QVariantMap& data);
    bool saveTrackConfig(const QString& path, const QVariantMap& data);

    QVariantList parseEmissives(const QVariantMap& sections);
    QVariantList parseBrakeDiscs(const QVariantMap& sections);
    QVariantList parseTrackLights(const QVariantMap& sections);
    QVariantList parseMaterialAdjustments(const QVariantMap& sections);
    QVariantList parseConditions(const QVariantMap& sections);

    QVariantMap serializeEmissives(const QVariantList& emissives);
    QVariantMap serializeBrakeDiscs(const QVariantList& brakeDiscs);
    QVariantMap serializeTrackLights(const QVariantList& trackLights);
    QVariantMap serializeMaterialAdjustments(const QVariantList& adjustments);
    QVariantMap serializeConditions(const QVariantList& conditions);

    // ── Editing API ────────────────────────────────────────────────────
    QVariantMap addEmissive(QVariantMap sections, const QVariantMap& emissive);
    QVariantMap removeEmissive(QVariantMap sections, const QString& sectionName);
    QVariantMap addTrackLight(QVariantMap sections, const QVariantMap& light);
    QVariantMap removeTrackLight(QVariantMap sections, const QString& sectionName);
    QVariantMap addMaterialAdjustment(QVariantMap sections, const QVariantMap& adjustment);
    QVariantMap removeMaterialAdjustment(QVariantMap sections, const QString& sectionName);
    QVariantMap addCondition(QVariantMap sections, const QVariantMap& condition);
    QVariantMap removeCondition(QVariantMap sections, const QString& sectionName);

    // ── Schema ─────────────────────────────────────────────────────────
    QVariantMap getCarSchema();
    QVariantMap getTrackSchema();

private:
    static CspConfigParser* s_instance;

    QVariantMap parseIniFile(const QString& path);
    bool writeIniFile(const QString& path, const QVariantMap& sections);

    QString colorToString(const QVariantMap& colorMap);
    QVariantMap stringToColor(const QString& colorStr);

    QString positionToString(const QVariantMap& posMap);
    QVariantMap stringToPosition(const QString& posStr);
};

}