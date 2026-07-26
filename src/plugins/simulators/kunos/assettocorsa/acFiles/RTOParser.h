#pragma once

#include <QString>
#include <QVector>
#include <QFile>
#include <QTextStream>

namespace ks {
namespace plugins {
namespace kunos {
namespace assettocorsa {

struct RTOGearRatio {
    int gear = 0;
    float ratio = 0.0f;
    float rpmDrop = 0.0f;
};

struct RTOData {
    QString carName;
    int numGears = 6;
    float finalDrive = 3.5f;
    QVector<RTOGearRatio> gears;
    float maxRpm = 8000.0f;
    float shiftRpm = 7500.0f;
};

class RTOParser {
public:
    static bool load(const QString& path, RTOData& out);
    static bool save(const QString& path, const RTOData& data);
    static bool validate(const RTOData& data, QString& error);

    static RTOData createDefault(int numGears = 6);
    static float calculateSpeedAtRpm(const RTOData& data, int gear, float rpm, float tireRadius = 0.3f);
    static float calculateRpmAtSpeed(const RTOData& data, int gear, float speedKmh, float tireRadius = 0.3f);
};

} // namespace assettocorsa
} // namespace kunos
} // namespace plugins
} // namespace ks