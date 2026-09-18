#include "SetupComparison.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace ks {

SetupComparison* SetupComparison::s_instance = nullptr;

SetupComparison::SetupComparison(QObject* parent)
    : QObject(parent)
{}

SetupComparison* SetupComparison::instance() {
    if (!s_instance) {
        s_instance = new SetupComparison();
    }
    return s_instance;
}

static void readSetupFromJson(QJsonObject& obj, CarSetup& s) {
    s.name = obj["name"].toString();
    s.track = obj["track"].toString();
    s.timestamp = obj["timestamp"].toString();
    s.frontRideHeight = obj["frontRideHeight"].toDouble();
    s.rearRideHeight = obj["rearRideHeight"].toDouble();
    s.frontSpringRate = obj["frontSpringRate"].toDouble();
    s.rearSpringRate = obj["rearSpringRate"].toDouble();
    s.frontComp = obj["frontCompression"].toDouble();
    if (s.frontComp == 0) s.frontComp = obj["frontComp"].toDouble();
    s.rearComp = obj["rearCompression"].toDouble();
    if (s.rearComp == 0) s.rearComp = obj["rearComp"].toDouble();
    s.frontReb = obj["frontRebound"].toDouble();
    if (s.frontReb == 0) s.frontReb = obj["frontReb"].toDouble();
    s.rearReb = obj["rearRebound"].toDouble();
    if (s.rearReb == 0) s.rearReb = obj["rearReb"].toDouble();
    s.frontWing = obj["frontWing"].toDouble();
    s.rearWing = obj["rearWing"].toDouble();
    s.diffPreload = obj["diffPreload"].toInt();
    s.diffAccel = obj["diffAccel"].toInt();
    if (s.diffAccel == 0) s.diffAccel = obj["diffAcceleration"].toInt();
    s.diffDecel = obj["diffDecel"].toInt();
    if (s.diffDecel == 0) s.diffDecel = obj["diffDeceleration"].toInt();
    s.brakeBias = obj["brakeBias"].toDouble();
    s.frontBrakePressure = obj["frontBrakePressure"].toDouble();
    s.rearBrakePressure = obj["rearBrakePressure"].toDouble();
    s.frontPressureL = obj["frontPressureL"].toDouble();
    s.frontPressureR = obj["frontPressureR"].toDouble();
    s.rearPressureL = obj["rearPressureL"].toDouble();
    s.rearPressureR = obj["rearPressureR"].toDouble();
    s.frontCamber = obj["frontCamber"].toDouble();
    s.rearCamber = obj["rearCamber"].toDouble();
    s.frontToe = obj["frontToe"].toDouble();
    s.rearToe = obj["rearToe"].toDouble();
    s.tractionControl = obj["tractionControl"].toInt();
    s.absLevel = obj["absLevel"].toInt();
    s.engineMap = obj["engineMap"].toInt();

    // TC2, TC cut, ABS ECU, steering lock, drs, ers, mgu, brake shape
    QJsonArray extra = obj["extra"].toArray();
    for (int i = 0; i < extra.size(); ++i) {
        s.extra << extra[i].toString();
    }
}

void SetupComparison::loadSetupA(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        readSetupFromJson(obj, m_setupA);
        emit setupLoaded("A");
    }
}

void SetupComparison::loadSetupB(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        readSetupFromJson(obj, m_setupB);
        emit setupLoaded("B");
    }
}

void SetupComparison::saveSetup(const QString& name, const CarSetup& setup) {
    m_savedSetups.insert(name, setup);
}

static bool isFieldDifferent(double a, double b, double tolerance) {
    return qAbs(a - b) > tolerance;
}

static bool isFieldDifferent(int a, int b) {
    return a != b;
}

static void compareField(SetupComparisonData& result, const QString& key,
                         double a, double b, double tol) {
    if (isFieldDifferent(a, b, tol)) {
        result.differences[key] = QString("A=%1 B=%2 diff=%3")
            .arg(a, 0, 'f', 3).arg(b, 0, 'f', 3).arg(qAbs(a - b), 0, 'f', 3);
    }
}

static void compareField(SetupComparisonData& result, const QString& key,
                         int a, int b) {
    if (isFieldDifferent(a, b)) {
        result.differences[key] = QString("A=%1 B=%2 diff=%3")
            .arg(a).arg(b).arg(qAbs(a - b));
    }
}

SetupComparisonData SetupComparison::compare() {
    SetupComparisonData result;
    result.setupA = m_setupA.name;
    result.setupB = m_setupB.name;

    const auto& A = m_setupA;
    const auto& B = m_setupB;

    // Suspension
    compareField(result, "frontRideHeight", A.frontRideHeight, B.frontRideHeight, 0.5);
    compareField(result, "rearRideHeight", A.rearRideHeight, B.rearRideHeight, 0.5);
    compareField(result, "frontSpringRate", A.frontSpringRate, B.frontSpringRate, 0.1);
    compareField(result, "rearSpringRate", A.rearSpringRate, B.rearSpringRate, 0.1);
    compareField(result, "frontCompression", A.frontComp, B.frontComp, 0.1);
    compareField(result, "rearCompression", A.rearComp, B.rearComp, 0.1);
    compareField(result, "frontRebound", A.frontReb, B.frontReb, 0.1);
    compareField(result, "rearRebound", A.rearReb, B.rearReb, 0.1);

    // Aero
    compareField(result, "frontWing", A.frontWing, B.frontWing, 0.1);
    compareField(result, "rearWing", A.rearWing, B.rearWing, 0.1);
    compareField(result, "diffPreload", A.diffPreload, B.diffPreload);
    compareField(result, "diffAccel", A.diffAccel, B.diffAccel);
    compareField(result, "diffDecel", A.diffDecel, B.diffDecel);

    // Brakes
    compareField(result, "brakeBias", A.brakeBias, B.brakeBias, 0.005);
    compareField(result, "frontBrakePressure", A.frontBrakePressure, B.frontBrakePressure, 0.1);
    compareField(result, "rearBrakePressure", A.rearBrakePressure, B.rearBrakePressure, 0.1);

    // Tires
    compareField(result, "frontPressureL", A.frontPressureL, B.frontPressureL, 0.02);
    compareField(result, "frontPressureR", A.frontPressureR, B.frontPressureR, 0.02);
    compareField(result, "rearPressureL", A.rearPressureL, B.rearPressureL, 0.02);
    compareField(result, "rearPressureR", A.rearPressureR, B.rearPressureR, 0.02);
    compareField(result, "frontCamber", A.frontCamber, B.frontCamber, 0.05);
    compareField(result, "rearCamber", A.rearCamber, B.rearCamber, 0.05);
    compareField(result, "frontToe", A.frontToe, B.frontToe, 0.02);
    compareField(result, "rearToe", A.rearToe, B.rearToe, 0.02);

    // Electronics
    compareField(result, "tractionControl", A.tractionControl, B.tractionControl);
    compareField(result, "absLevel", A.absLevel, B.absLevel);
    compareField(result, "engineMap", A.engineMap, B.engineMap);

    // Extra fields
    if (A.extra != B.extra) {
        result.differences["extra"] = QString("A has %1 items, B has %2 items")
            .arg(A.extra.size()).arg(B.extra.size());
    }

    result.deltaTime = -1; // Not computed — requires telemetry data
    emit comparisonReady(result);
    return result;
}

QStringList SetupComparison::getSavedSetups() const {
    return m_savedSetups.keys();
}

CarSetup SetupComparison::getSetup(const QString& name) const {
    return m_savedSetups.value(name);
}

}