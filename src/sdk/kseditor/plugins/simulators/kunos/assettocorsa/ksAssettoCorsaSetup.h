#pragma once

#include <QString>
#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QSettings>

namespace ks { namespace kunos {

struct KsSetupData {
    QString carId;
    QMap<QString, double> parameters;

    float steerRatio = 0;
    float frontCaster = 0;
    float rearCaster = 0;
    float frontCamber[2] = {0, 0};
    float rearCamber[2] = {0, 0};
    float toeOut[4] = {0, 0, 0, 0};
    float rideHeight[4] = {0, 0, 0, 0};
    float brakeBias = 0;
    float springRate[4] = {0, 0, 0, 0};
    float compression[4] = {0, 0, 0, 0};
    float rebound[4] = {0, 0, 0, 0};
    float frontARB = 0;
    float rearARB = 0;
    float frontWing = 0;
    float rearWing = 0;
    float diffPower = 0;
    float diffCoast = 0;
    float diffDrive = 0;
    float frontTyrePressure[2] = {0, 0};
    float rearTyrePressure[2] = {0, 0};
    float fuelLevel = 0;
    float diffusers = 0;
};

class KsSetupManager {
public:
    bool load(const QString& path, KsSetupData& out) {
        return loadSetupFromFile(path, out);
    }
    bool save(const QString& path, const KsSetupData& data) {
        return saveSetupToFile(path, data);
    }

    static bool loadSetupFromFile(const QString& path, KsSetupData& setup) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

        QString currentSection;
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith(';') || line.startsWith('#')) continue;

            if (line.startsWith('[') && line.endsWith(']')) {
                currentSection = line.mid(1, line.length() - 2).toUpper();
                continue;
            }

            int eq = line.indexOf('=');
            if (eq < 0) continue;
            QString key = line.left(eq).trimmed().toUpper();
            QString value = line.mid(eq + 1).trimmed();
            bool ok;
            double dval = value.toDouble(&ok);
            if (ok) setup.parameters[currentSection + "." + key] = dval;

            if (currentSection == "TYRES") {
                if (key == "PRESSURE_FL") setup.frontTyrePressure[0] = value.toFloat();
                else if (key == "PRESSURE_FR") setup.frontTyrePressure[1] = value.toFloat();
                else if (key == "PRESSURE_RL") setup.rearTyrePressure[0] = value.toFloat();
                else if (key == "PRESSURE_RR") setup.rearTyrePressure[1] = value.toFloat();
                else if (key == "CAMBER_FL") setup.frontCamber[0] = value.toFloat();
                else if (key == "CAMBER_FR") setup.frontCamber[1] = value.toFloat();
                else if (key == "CAMBER_RL") setup.rearCamber[0] = value.toFloat();
                else if (key == "CAMBER_RR") setup.rearCamber[1] = value.toFloat();
                else if (key == "TOE_OUT_FL") setup.toeOut[0] = value.toFloat();
                else if (key == "TOE_OUT_FR") setup.toeOut[1] = value.toFloat();
                else if (key == "TOE_OUT_RL") setup.toeOut[2] = value.toFloat();
                else if (key == "TOE_OUT_RR") setup.toeOut[3] = value.toFloat();
            } else if (currentSection == "SUSPENSION") {
                if (key == "SPRING_RATE_FL") setup.springRate[0] = value.toFloat();
                else if (key == "SPRING_RATE_FR") setup.springRate[1] = value.toFloat();
                else if (key == "SPRING_RATE_RL") setup.springRate[2] = value.toFloat();
                else if (key == "SPRING_RATE_RR") setup.springRate[3] = value.toFloat();
                else if (key == "COMPRESSION_FL") setup.compression[0] = value.toFloat();
                else if (key == "COMPRESSION_FR") setup.compression[1] = value.toFloat();
                else if (key == "COMPRESSION_RL") setup.compression[2] = value.toFloat();
                else if (key == "COMPRESSION_RR") setup.compression[3] = value.toFloat();
                else if (key == "REBOUND_FL") setup.rebound[0] = value.toFloat();
                else if (key == "REBOUND_FR") setup.rebound[1] = value.toFloat();
                else if (key == "REBOUND_RL") setup.rebound[2] = value.toFloat();
                else if (key == "REBOUND_RR") setup.rebound[3] = value.toFloat();
                else if (key == "RIDE_HEIGHT_FL") setup.rideHeight[0] = value.toFloat();
                else if (key == "RIDE_HEIGHT_FR") setup.rideHeight[1] = value.toFloat();
                else if (key == "RIDE_HEIGHT_RL") setup.rideHeight[2] = value.toFloat();
                else if (key == "RIDE_HEIGHT_RR") setup.rideHeight[3] = value.toFloat();
                else if (key == "ARB_FRONT") setup.frontARB = value.toFloat();
                else if (key == "ARB_REAR") setup.rearARB = value.toFloat();
                else if (key == "CASTER") setup.frontCaster = value.toFloat();
            } else if (currentSection == "AERO") {
                if (key == "WING_FRONT") setup.frontWing = value.toFloat();
                else if (key == "WING_REAR") setup.rearWing = value.toFloat();
                else if (key == "DIFFUSERS") setup.diffusers = value.toFloat();
            } else if (currentSection == "ELECTRONICS") {
                if (key == "TC") setup.parameters["ELECTRONICS.TC"] = dval;
                else if (key == "ABS") setup.parameters["ELECTRONICS.ABS"] = dval;
            } else if (currentSection == "FUEL") {
                if (key == "FUEL_LOAD") setup.fuelLevel = value.toFloat();
            } else if (currentSection == "DIFFERENTIAL") {
                if (key == "POWER") setup.diffPower = value.toFloat();
                else if (key == "COAST") setup.diffCoast = value.toFloat();
                else if (key == "DRIVE") setup.diffDrive = value.toFloat();
            }
        }
        file.close();
        return true;
    }

    static bool saveSetupToFile(const QString& path, const KsSetupData& setup) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

        QTextStream out(&file);
        out << "; Car Setup - Generated by ksEditor\n\n";

        out << "[TYRES]\n";
        out << "PRESSURE_FL=" << QString::number(setup.frontTyrePressure[0], 'f', 2) << "\n";
        out << "PRESSURE_FR=" << QString::number(setup.frontTyrePressure[1], 'f', 2) << "\n";
        out << "PRESSURE_RL=" << QString::number(setup.rearTyrePressure[0], 'f', 2) << "\n";
        out << "PRESSURE_RR=" << QString::number(setup.rearTyrePressure[1], 'f', 2) << "\n";
        out << "CAMBER_FL=" << QString::number(setup.frontCamber[0], 'f', 2) << "\n";
        out << "CAMBER_FR=" << QString::number(setup.frontCamber[1], 'f', 2) << "\n";
        out << "CAMBER_RL=" << QString::number(setup.rearCamber[0], 'f', 2) << "\n";
        out << "CAMBER_RR=" << QString::number(setup.rearCamber[1], 'f', 2) << "\n";
        out << "TOE_OUT_FL=" << QString::number(setup.toeOut[0], 'f', 3) << "\n";
        out << "TOE_OUT_FR=" << QString::number(setup.toeOut[1], 'f', 3) << "\n";
        out << "TOE_OUT_RL=" << QString::number(setup.toeOut[2], 'f', 3) << "\n";
        out << "TOE_OUT_RR=" << QString::number(setup.toeOut[3], 'f', 3) << "\n\n";

        out << "[SUSPENSION]\n";
        out << "SPRING_RATE_FL=" << QString::number(setup.springRate[0], 'f', 0) << "\n";
        out << "SPRING_RATE_FR=" << QString::number(setup.springRate[1], 'f', 0) << "\n";
        out << "SPRING_RATE_RL=" << QString::number(setup.springRate[2], 'f', 0) << "\n";
        out << "SPRING_RATE_RR=" << QString::number(setup.springRate[3], 'f', 0) << "\n";
        out << "COMPRESSION_FL=" << QString::number(setup.compression[0], 'f', 0) << "\n";
        out << "COMPRESSION_FR=" << QString::number(setup.compression[1], 'f', 0) << "\n";
        out << "COMPRESSION_RL=" << QString::number(setup.compression[2], 'f', 0) << "\n";
        out << "COMPRESSION_RR=" << QString::number(setup.compression[3], 'f', 0) << "\n";
        out << "REBOUND_FL=" << QString::number(setup.rebound[0], 'f', 0) << "\n";
        out << "REBOUND_FR=" << QString::number(setup.rebound[1], 'f', 0) << "\n";
        out << "REBOUND_RL=" << QString::number(setup.rebound[2], 'f', 0) << "\n";
        out << "REBOUND_RR=" << QString::number(setup.rebound[3], 'f', 0) << "\n";
        out << "RIDE_HEIGHT_FL=" << QString::number(setup.rideHeight[0], 'f', 2) << "\n";
        out << "RIDE_HEIGHT_FR=" << QString::number(setup.rideHeight[1], 'f', 2) << "\n";
        out << "RIDE_HEIGHT_RL=" << QString::number(setup.rideHeight[2], 'f', 2) << "\n";
        out << "RIDE_HEIGHT_RR=" << QString::number(setup.rideHeight[3], 'f', 2) << "\n";
        out << "ARB_FRONT=" << QString::number(setup.frontARB, 'f', 2) << "\n";
        out << "ARB_REAR=" << QString::number(setup.rearARB, 'f', 2) << "\n";
        out << "CASTER=" << QString::number(setup.frontCaster, 'f', 2) << "\n\n";

        out << "[AERO]\n";
        out << "WING_FRONT=" << QString::number(setup.frontWing, 'f', 2) << "\n";
        out << "WING_REAR=" << QString::number(setup.rearWing, 'f', 2) << "\n";
        out << "DIFFUSERS=" << QString::number(setup.diffusers, 'f', 2) << "\n\n";

        out << "[DIFFERENTIAL]\n";
        out << "POWER=" << QString::number(setup.diffPower, 'f', 4) << "\n";
        out << "COAST=" << QString::number(setup.diffCoast, 'f', 4) << "\n";
        out << "DRIVE=" << QString::number(setup.diffDrive, 'f', 4) << "\n\n";

        out << "[ELECTRONICS]\n";
        out << "TC=" << QString::number(setup.parameters.value("ELECTRONICS.TC", 0), 'f', 2) << "\n";
        out << "ABS=" << QString::number(setup.parameters.value("ELECTRONICS.ABS", 0), 'f', 2) << "\n\n";

        out << "[FUEL]\n";
        out << "FUEL_LOAD=" << QString::number(setup.fuelLevel, 'f', 2) << "\n";

        file.close();
        return true;
    }
};

}} // namespace ks::kunos
