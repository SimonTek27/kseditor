#ifndef KSASSETTOCORSAPHYSDEFS_H
#define KSASSETTOCORSAPHYSDEFS_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QSet>
#include <QVector>
#include <QPair>

namespace ks {
namespace plugins {
namespace kunos {
namespace assettocorsa {

// Forward declarations
struct ksAssettoCorsaPhysIniFile;
struct ksAssettoCorsaPhysParameter;
struct ksAssettoCorsaPhysSection;
struct ksAssettoCorsaPhysCategory;

// ============================================================================
// Physics Constants (from ac_constants.h and Kunos engine)
// ============================================================================

struct ksAssettoCorsaPhysConstants {
    // Universal
    static constexpr float GRAVITY = 9.81f;
    static constexpr float AIR_DENSITY_SEA_LEVEL = 1.225f;
    static constexpr float PI = 3.14159265359f;
    static constexpr float DEG_TO_RAD = PI / 180.0f;
    static constexpr float RAD_TO_DEG = 180.0f / PI;

    // Vehicle limits
    static constexpr float MAX_RPM = 20000.0f;
    static constexpr float MAX_SPEED_KMH = 500.0f;
    static constexpr float MAX_STEER_DEG = 900.0f;
    static constexpr float MAX_BRAKE_TEMP_C = 1500.0f;
    static constexpr float MAX_TYRE_TEMP_C = 200.0f;
    static constexpr float MAX_BOOST_BAR = 5.0f;

    // Default values
    static constexpr float DEFAULT_MASS = 1200.0f;
    static constexpr float DEFAULT_CG_HEIGHT = 0.45f;
    static constexpr float DEFAULT_WHEELBASE = 2.7f;
    static constexpr float DEFAULT_TRACK_WIDTH = 1.6f;
    static constexpr float DEFAULT_TYRE_RADIUS = 0.33f;
    static constexpr float DEFAULT_TYRE_WIDTH = 0.265f;
    static constexpr float DEFAULT_TYRE_PRESSURE = 2.2f; // bar
    static constexpr float DEFAULT_SPRING_RATE = 50000.0f; // N/m
    static constexpr float DEFAULT_DAMPER_BUMP = 3000.0f; // N·s/m
    static constexpr float DEFAULT_DAMPER_REBOUND = 4000.0f; // N·s/m
    static constexpr float DEFAULT_ARB_RATE = 20000.0f; // N/m
    static constexpr float DEFAULT_BRAKE_TORQUE = 3000.0f; // Nm
    static constexpr float DEFAULT_BRAKE_BIAS = 0.55f; // front
    static constexpr float DEFAULT_DRAG_CD = 0.32f;
    static constexpr float DEFAULT_LIFT_CL = -0.2f;
    static constexpr float DEFAULT_FINAL_DRIVE = 3.5f;
    static constexpr float DEFAULT_IDLE_RPM = 900.0f;
    static constexpr float DEFAULT_MAX_POWER_HP = 300.0f;
    static constexpr float DEFAULT_MAX_TORQUE_NM = 400.0f;

    // Conversion
    static constexpr float HP_TO_KW = 0.7457f;
    static constexpr float KW_TO_HP = 1.34102f;
    static constexpr float BAR_TO_PSI = 14.5038f;
    static constexpr float PSI_TO_BAR = 0.0689476f;
    static constexpr float NM_TO_LBFT = 0.737562f;
    static constexpr float KG_TO_LB = 2.20462f;
    static constexpr float M_TO_FT = 3.28084f;
    static constexpr float KMH_TO_MS = 0.277778f;
    static constexpr float MS_TO_KMH = 3.6f;
};

// ============================================================================
// INI File Definitions
// ============================================================================

enum class ksPhysIniFileType {
    Car,
    Engine,
    Drivetrain,
    Gearbox,
    Suspension,
    Tyres,
    Aero,
    Brakes,
    Steering,
    Dashboard,
    Electronics,
    Damage,
    Lights,
    FFEffects,
    Setup,
    ExampleSetup,
    Hybrid,
    Thermal,
    DRS,
    Fuel,
    Driver3D,
    Cameras,
    PowerLUT,
    TorqueLUT,
    AI,
    AITyres,
    AmbientShadows,
    Bumpstops,
    Dampers,
    Colliders,
    LODs,
    Mirrors,
    AnalogInstruments,
    DigitalInstruments,
    DashCam,
    Flames,
    FlamePresets,
    Sounds,
    FuelCons,
    ERS,
    CTRL_ArbFront,
    CTRL_ArbRear,
    CTRL_BrakePower,
    CTRL_EBB,
    CTRL_FrontShare,
    CTRL_ERS,
    CTRL_ERSFront,
    CTRL_Turbo,
    Script,
    ScriptECU,
    ScriptTractionControl,
    ScriptERSKinetic,
    ScriptAWD,
    ScriptAntirollBarFront,
    ScriptAntirollBarRear,
    ScriptBrakes,
    ScriptEngineBrake,
    ScriptIdle,
    ScriptSensors,
    ScriptThrottleBody,
    ScriptAWD,
    COUNT
};

struct ksAssettoCorsaPhysIniFile {
    ksPhysIniFileType type;
    QString name;
    QString filename;
    QString category;
    QString description;
    QStringList sections;
    bool required;
    bool cspExtended;
};

class ksAssettocorsaphysdefs {
public:
    // Singleton
    static const ksAssettocorsaphysdefs& instance() {
        static ksAssettocorsaphysdefs defs;
        return defs;
    }

    // File info
    const ksAssettoCorsaPhysIniFile* getIniFile(ksPhysIniFileType type) const {
        auto it = m_iniFiles.find(static_cast<int>(type));
        return it != m_iniFiles.end() ? &it.value() : nullptr;
    }

    const ksAssettoCorsaPhysIniFile* getIniFileByName(const QString& name) const {
        for (auto it = m_iniFiles.constBegin(); it != m_iniFiles.constEnd(); ++it) {
            if (it.value().name == name || it.value().filename == name) return &it.value();
        }
        return nullptr;
    }

    QList<ksAssettoCorsaPhysIniFile> iniFiles() const { return m_iniFiles.values(); }
    QList<ksAssettoCorsaPhysIniFile> iniFilesByCategory(const QString& category) const;
    QStringList iniCategories() const;

    // Sections
    QStringList sectionsForFile(ksPhysIniFileType type) const;

    // Parameters
    struct ParameterInfo {
        QString section;
        QString key;
        QString type; // "int", "float", "bool", "string", "vector3", "vector2", "array"
        QString unit;
        QString description;
        float minValue;
        float maxValue;
        float defaultValue;
        bool isArray;
        int arraySize;
        QStringList enumValues;
        bool cspOnly;
    };

    QList<ParameterInfo> parametersForFile(ksPhysIniFileType type) const;
    QList<ParameterInfo> parametersForSection(ksPhysIniFileType type, const QString& section) const;
    const ParameterInfo* findParameter(ksPhysIniFileType type, const QString& section, const QString& key) const;

    // Validation
    struct ValidationRule {
        QString parameter;
        QString rule; // "range", "required", "dependency", "custom"
        QString message;
        QVariant min;
        QVariant max;
        QString dependsOn;
    };

    QList<ValidationRule> validationRulesForFile(ksPhysIniFileType type) const;

    // Categories
    static QStringList physicsCategories() {
        return {"Core", "Powertrain", "Chassis", "Tyres", "Aerodynamics", "Brakes", "Electronics", "Driver", "AI", "CSP"};
    }

    static QStringList requiredFiles() {
        return {"car.ini", "engine.ini", "drivetrain.ini", "gearbox.ini", "suspension.ini", "tyres.ini", "aero.ini", "brakes.ini"};
    }

    static QStringList optionalFiles() {
        return {"steering.ini", "dashboard.ini", "electronics.ini", "damage.ini", "lights.ini", "ff_effects.ini",
                "setup.ini", "example_setup.ini", "hybrid.ini", "thermal.ini", "drs.ini", "fuel.ini",
                "driver3d.ini", "cameras.ini", "power.lut", "torque.lut", "ai.ini", "ai_tyres.ini"};
    }

    static QStringList cspFiles() {
        return {"ext_config.ini", "thermal.ini", "drs.ini", "hybrid.ini", "ers.ini", "ctrl_*.ini", "script*.lua",
                "digital_instruments.ini", "mirrors.ini", "flames.ini", "flame_presets.ini", "ambient_shadows.ini",
                "colliders.ini", "lods.ini", "blurred_objects.ini", "ctrl_arb_front.ini", "ctrl_arb_rear.ini",
                "ctrl_brake_power.ini", "ctrl_ebb.ini", "ctrl_front_share.ini", "ctrl_ers_*.ini",
                "ctrl_ers_front_*.ini", "ctrl_turbo*.ini"};
    }

    // Lookup table types
    enum class LUTType {
        OneD,
        TwoD,
        GearDependent,
        CompoundDependent,
        SpeedDependent,
        RPMDependent,
        LoadDependent,
        TemperatureDependent
    };

    struct LUTDefinition {
        QString filename;
        LUTType type;
        QString xAxis;
        QString yAxis;
        QString zAxis;
        QString description;
        QStringList validForFiles;
    };

    QList<LUTDefinition> lutDefinitions() const;

    // Suspension geometry indices
    enum class WheelIndex { FL = 0, FR = 1, RL = 2, RR = 3 };
    static QString wheelName(WheelIndex idx);
    static WheelIndex wheelIndex(const QString& name);

    // Compound types
    enum class TyreCompound { Soft = 0, Medium = 1, Hard = 2, Wet = 3, Custom = 4 };
    static QString compoundName(TyreCompound c);
    static TyreCompound compoundFromName(const QString& name);

    // Drivetrain types
    enum class DrivetrainType { FWD, RWD, AWD, AWD2 };
    static QString drivetrainName(DrivetrainType t);

    // Diff types
    enum class DiffType { Open, Viscous, ClutchPack, Torsen, Locked };
    static QString diffName(DiffType t);

    // Gearbox types
    enum class GearboxType { Sequential, HPattern, Automatic };
    static QString gearboxName(GearboxType t);

    // Aero map types
    enum class AeroMapType { RideHeight, Yaw, Roll, Steer, Speed };
    static QString aeroMapName(AeroMapType t);

private:
    QMap<int, ksAssettoCorsaPhysIniFile> m_iniFiles;
    QMap<int, QList<ParameterInfo>> m_fileParameters;
    QMap<int, QList<ValidationRule>> m_fileValidationRules;
    QList<LUTDefinition> m_lutDefinitions;

    ksAssettocorsaphysdefs();
    void initializeIniFiles();
    void initializeParameters();
    void initializeValidationRules();
    void initializeLUTDefinitions();
    void addParameter(int fileType, const ParameterInfo& param);
    void addValidationRule(int fileType, const ValidationRule& rule);
};

// ============================================================================
// Inline implementations
// ============================================================================

inline QString ksAssettocorsaphysdefs::wheelName(WheelIndex idx) {
    switch (idx) {
        case WheelIndex::FL: return "FL";
        case WheelIndex::FR: return "FR";
        case WheelIndex::RL: return "RL";
        case WheelIndex::RR: return "RR";
    }
    return "FL";
}

inline ksAssettocorsaphysdefs::WheelIndex ksAssettocorsaphysdefs::wheelIndex(const QString& name) {
    QString n = name.toUpper();
    if (n == "FL" || n == "LF" || n == "FRONT_LEFT") return WheelIndex::FL;
    if (n == "FR" || n == "RF" || n == "FRONT_RIGHT") return WheelIndex::FR;
    if (n == "RL" || n == "LR" || n == "REAR_LEFT") return WheelIndex::RL;
    if (n == "RR" || n == "RL" || n == "REAR_RIGHT") return WheelIndex::RR;
    return WheelIndex::FL;
}

inline QString ksAssettocorsaphysdefs::compoundName(TyreCompound c) {
    switch (c) {
        case TyreCompound::Soft: return "SOFT";
        case TyreCompound::Medium: return "MEDIUM";
        case TyreCompound::Hard: return "HARD";
        case TyreCompound::Wet: return "WET";
        case TyreCompound::Custom: return "CUSTOM";
    }
    return "SOFT";
}

inline ksAssettocorsaphysdefs::TyreCompound ksAssettocorsaphysdefs::compoundFromName(const QString& name) {
    QString n = name.toUpper();
    if (n.contains("SOFT")) return TyreCompound::Soft;
    if (n.contains("MEDIUM") || n.contains("MED")) return TyreCompound::Medium;
    if (n.contains("HARD")) return TyreCompound::Hard;
    if (n.contains("WET") || n.contains("RAIN")) return TyreCompound::Wet;
    return TyreCompound::Custom;
}

inline QString ksAssettocorsaphysdefs::drivetrainName(DrivetrainType t) {
    switch (t) {
        case DrivetrainType::FWD: return "FWD";
        case DrivetrainType::RWD: return "RWD";
        case DrivetrainType::AWD: return "AWD";
        case DrivetrainType::AWD2: return "AWD2";
    }
    return "RWD";
}

inline QString ksAssettocorsaphysdefs::diffName(DiffType t) {
    switch (t) {
        case DiffType::Open: return "OPEN";
        case DiffType::Viscous: return "VISCOUS";
        case DiffType::ClutchPack: return "CLUTCH_PACK";
        case DiffType::Torsen: return "TORSEN";
        case DiffType::Locked: return "LOCKED";
    }
    return "OPEN";
}

inline QString ksAssettocorsaphysdefs::gearboxName(GearboxType t) {
    switch (t) {
        case GearboxType::Sequential: return "SEQUENTIAL";
        case GearboxType::HPattern: return "H_PATTERN";
        case GearboxType::Automatic: return "AUTOMATIC";
    }
    return "SEQUENTIAL";
}

inline QString ksAssettocorsaphysdefs::aeroMapName(AeroMapType t) {
    switch (t) {
        case AeroMapType::RideHeight: return "RIDE_HEIGHT";
        case AeroMapType::Yaw: return "YAW";
        case AeroMapType::Roll: return "ROLL";
        case AeroMapType::Steer: return "STEER";
        case AeroMapType::Speed: return "SPEED";
    }
    return "RIDE_HEIGHT";
}

} // namespace assettocorsa
} // namespace kunos
} // namespace plugins
} // namespace ks

#endif // KSASSETTOCORSAPHYSDEFS_H