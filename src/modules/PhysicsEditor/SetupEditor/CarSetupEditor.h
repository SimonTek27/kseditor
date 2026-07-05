#pragma once

#include <QString>
#include <QMap>
#include <QJsonObject>

/**
 * @brief Car Setup Editor for Assetto Corsa
 *
 * Manages car setup files (.ini) for Assetto Corsa.
 * Based on community tools and AC SDK documentation.
 *
 * Setup files include:
 * - setup.ini - Main setup configuration
 * - brakes.ini - Brake bias and pressure
 * - suspension.ini - Spring rates, dampers, ride height
 * - tyres.ini - Tyre pressures, camber, toe
 * - electronics.ini - TC, ABS, ECU maps
 * - aero.ini - Wing angles, ride height
 * - differential.ini - Differential settings
 * - engine.ini - Engine mappings
 */
class CarSetupEditor {
public:
    struct BrakeSetup {
        float frontBrakeBias = 56.0f;      // Percentage
        float brakePressure = 100.0f;      // Percentage
        float brakePadCompound = 1.0f;
        float brakeDuctOpening = 1.0f;

        // Per-wheel
        float frontBrakePower[2] = {1.0f, 1.0f};
        float rearBrakePower[2] = {1.0f, 1.0f};
    };

    struct SuspensionSetup {
        // Spring rates (N/mm)
        float frontSpringRate[2] = {150.0f, 150.0f};
        float rearSpringRate[2] = {180.0f, 180.0f};

        // Dampers (bump/rebound in mm/s)
        float frontBumpDamper[2] = {4000.0f, 4000.0f};
        float frontReboundDamper[2] = {5000.0f, 5000.0f};
        float rearBumpDamper[2] = {4500.0f, 4500.0f};
        float rearReboundDamper[2] = {5500.0f, 5500.0f};

        // Ride height (mm)
        float frontRideHeight = 55.0f;
        float rearRideHeight = 70.0f;

        // Anti-roll bars (N/mm)
        float frontARB = 25.0f;
        float rearARB = 30.0f;

        // Bump stops
        float frontBumpStopRate = 8000.0f;
        float rearBumpStopRate = 9000.0f;
    };

    struct TyreSetup {
        // Pressures (PSI)
        float frontTyrePressure[2] = {26.0f, 26.0f};
        float rearTyrePressure[2] = {24.0f, 24.0f};

        // Camber (degrees, negative = inward lean)
        float frontCamber[2] = {-3.5f, -3.5f};
        float rearCamber[2] = {-2.0f, -2.0f};

        // Toe (degrees, positive = toe-out)
        float frontToe[2] = {0.1f, 0.1f};
        float rearToe[2] = {-0.1f, -0.1f};

        // Tyre compound
        int frontCompound = 0;
        int rearCompound = 0;
    };

    struct AeroSetup {
        // Wing angles (degrees)
        float frontWingAngle = 8.0f;
        float rearWingAngle = 10.0f;

        // Splitter/diffuser
        float splitterHeight = 30.0f;
        float diffuserAngle = 8.0f;

        // Ride height targets
        float frontRideHeightTarget = 55.0f;
        float rearRideHeightTarget = 70.0f;
    };

    struct DifferentialSetup {
        float preload = 20.0f;              // Nm
        float coastPower = 0.3f;            // 0-1
        float drivePower = 0.5f;            // 0-1
        float coastBrake = 0.3f;            // 0-1
        float driveBrake = 0.4f;            // 0-1
        float centralDifferential = 0.0f;   // For AWD (0=RWD, 1=AWD)
    };

    struct ElectronicsSetup {
        float tractionControl = 0.0f;       // 0-10
        float abs = 0.0f;                   // 0-10
        int ecuMap = 0;                     // 0-8
        bool autoBlip = false;
        bool autoBrake = false;
    };

    struct EngineSetup {
        float fuelCapacity = 100.0f;
        float fuelLoad = 50.0f;
        int engineBrakeMap = 0;
        int powerBoostMap = 0;
    };

    struct CarSetup {
        QString name;
        QString carModel;
        BrakeSetup brakes;
        SuspensionSetup suspension;
        TyreSetup tyres;
        AeroSetup aero;
        DifferentialSetup differential;
        ElectronicsSetup electronics;
        EngineSetup engine;
    };

    // File operations
    static CarSetup loadSetup(const QString& setupPath);
    static bool saveSetup(const CarSetup& setup, const QString& setupPath);
    static bool loadFromIni(CarSetup& setup, const QString& iniPath);
    static bool saveToIni(const CarSetup& setup, const QString& iniPath);

    // Setup management
    static QStringList getAvailableSetups(const QString& carPath);
    static CarSetup getSetup(const QString& carPath, const QString& setupName);
    static bool deleteSetup(const QString& carPath, const QString& setupName);
    static bool copySetup(const QString& carPath, const QString& sourceName, const QString& destName);

    // Presets
    static CarSetup getQualifyingSetup();
    static CarSetup getRaceSetup();
    static CarSetup getWetSetup();
    static CarSetup getDriftSetup();
    static CarSetup getEnduranceSetup();

    // Comparison
    static QMap<QString, QPair<double, double>> compareSetups(const CarSetup& a, const CarSetup& b);

    // Validation
    static bool validateSetup(const CarSetup& setup, QString* error = nullptr);

    // Utility
    static QString getSetupTypeName(int type);

private:
    static void parseSection(const QString& section, const QMap<QString, QString>& keys, CarSetup& setup);
    static QMap<QString, QString> formatSection(const QString& section, const CarSetup& setup);
};
