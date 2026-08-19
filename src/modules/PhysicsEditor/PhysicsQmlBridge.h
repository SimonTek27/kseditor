#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QPair>

// Minimal CarPhysicsConfig to satisfy QML bridge (expanded elsewhere)
struct CarPhysicsConfig {
    QString carName;
    QString carDirectory;
    QString drivetrain;
    float mass = 0.0f;
    struct Tyres { float width=0.0f; float pressureOpt=0.0f; float radius=0.0f; } tyres;
    struct Suspension { float frontLeftSpring=0.0f; float rearLeftSpring=0.0f; } suspension;
    float cgHeight = 0.0f;
    float wheelbase = 0.0f;
    struct Aero { float frontDownforce=0.0f; float rearDownforce=0.0f; float drag=0.0f; bool drsEnabled=false; } aero;
    struct Brakes { float brakeBalance=0.0f; bool absEnabled=false; float brakePressure=150.0f; } brakes;
    struct Engine { int redlineRPM=0; int limiterRPM=0; float maxPowerKw=200.0f; float maxTorqueNm=400.0f; } engine;
    QVector<float> gearRatios;
    float finalDrive = 0.0f;
    // Wheels
    float wheelDiameter = 26.0f;
    float wheelWidth = 12.0f;
    float wheelMass = 12.0f;
    // Tires
    QString tireCompound = "Medium";
    float tireOptimalTemp = 90.0f;
    float tireMaxTemp = 120.0f;
    float tireTreadRemaining = 100.0f;
    // Steering
    float steeringRatio = 15.0f;
    float steeringLockAngle = 45.0f;
    bool powerSteering = true;
    // Driver
    float driverPositionX = 0.1f;
    float driverPositionY = 0.5f;
    float driverPositionZ = -0.2f;
    float driverMass = 75.0f;
    float driverHeight = 1.8f;
    // AI
    float aiAggression = 50.0f;
    float aiSkill = 80.0f;
    float aiConsistency = 90.0f;
    // Transmission
    int transmissionType = 0;
    int gearCount = 7;

    // Turbo
    struct Turbo {
        int type = 1;               // 0=None, 1=Single, 2=Twin, 3=Electric
        float boostPressure = 1.5f; // bar
        int threshold = 3000;       // RPM
        float lag = 0.5f;           // seconds
        float wastegate = 2.0f;     // bar
        int count = 1;              // 1 or 2
    } turbo;

    // ERS/Hybrid
    struct Ers {
        bool enabled = false;
        int architecture = 0;       // HybridArchitecture enum
        int deploymentMode = 7;     // ErsMode (auto = 7)
        float mgukPowerKw = 120.0f;
        float mgukRegenKw = 120.0f;
        float mguhPowerKw = 120.0f;
        float batteryCapacityMj = 4.0f;
        float batterySoc = 50.0f;
        float perLapEnergyMj = 4.0f;
        bool attackModeAvailable = false;
    } ers;

    // Damage
    struct Damage {
        bool enabled = false;
        float aeroDamage = 0.0f;
        float engineHealth = 100.0f;
        float bodyHealth = 100.0f;
    } damage;

    // Weather
    struct Weather {
        float trackWetness = 0.0f;
        float rainIntensity = 0.0f;
        float ambientTemp = 26.0f;
        float trackTemp = 30.0f;
    } weather;
};

namespace ks {

class PhysicsQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString carName READ carName WRITE setCarName NOTIFY carNameChanged)
    Q_PROPERTY(float totalMass READ totalMass WRITE setTotalMass NOTIFY massChanged)
    Q_PROPERTY(float cgHeight READ cgHeight WRITE setCgHeight NOTIFY massChanged)
    Q_PROPERTY(float wheelbase READ wheelbase WRITE setWheelbase NOTIFY drivetrainChanged)
    Q_PROPERTY(float frontDownforce READ frontDownforce WRITE setFrontDownforce NOTIFY aeroChanged)
    Q_PROPERTY(float rearDownforce READ rearDownforce WRITE setRearDownforce NOTIFY aeroChanged)
    Q_PROPERTY(float drag READ drag WRITE setDrag NOTIFY aeroChanged)
    Q_PROPERTY(float brakeBalance READ brakeBalance WRITE setBrakeBalance NOTIFY brakesChanged)
    Q_PROPERTY(bool absEnabled READ absEnabled WRITE setAbsEnabled NOTIFY brakesChanged)
    Q_PROPERTY(int redlineRPM READ redlineRPM WRITE setRedlineRPM NOTIFY engineChanged)
    Q_PROPERTY(int maxRPM READ maxRPM WRITE setMaxRPM NOTIFY engineChanged)
    Q_PROPERTY(float damping READ damping WRITE setDamping NOTIFY aeroChanged)
    Q_PROPERTY(float rideHeight READ rideHeight WRITE setRideHeight NOTIFY aeroChanged)
    Q_PROPERTY(float finalDrive READ finalDrive WRITE setFinalDrive NOTIFY drivetrainChanged)
    Q_PROPERTY(float maxTorqueNm READ maxTorqueNm WRITE setMaxTorqueNm NOTIFY engineChanged)
    Q_PROPERTY(float maxPowerKw READ maxPowerKw WRITE setMaxPowerKw NOTIFY engineChanged)
    Q_PROPERTY(float suspensionFrontSpring READ suspensionFrontSpring WRITE setSuspensionFrontSpring NOTIFY suspensionChanged)
    Q_PROPERTY(float suspensionRearSpring READ suspensionRearSpring WRITE setSuspensionRearSpring NOTIFY suspensionChanged)
    Q_PROPERTY(bool isSimulating READ isSimulating NOTIFY simulationChanged)
    Q_PROPERTY(bool isValid READ isValid NOTIFY validationChanged)
    Q_PROPERTY(QString currentFile READ currentFile WRITE setCurrentFile NOTIFY currentFileChanged)
    Q_PROPERTY(QString currentCategory READ currentCategory WRITE setCurrentCategory NOTIFY categoryChanged)
    // Wheels
    Q_PROPERTY(float wheelDiameter READ wheelDiameter WRITE setWheelDiameter NOTIFY wheelChanged)
    Q_PROPERTY(float wheelWidth READ wheelWidth WRITE setWheelWidth NOTIFY wheelChanged)
    Q_PROPERTY(float wheelMass READ wheelMass WRITE setWheelMass NOTIFY wheelChanged)
    // Tires
    Q_PROPERTY(QString tireCompound READ tireCompound WRITE setTireCompound NOTIFY tireChanged)
    Q_PROPERTY(float tireOptimalTemp READ tireOptimalTemp WRITE setTireOptimalTemp NOTIFY tireChanged)
    Q_PROPERTY(float tireMaxTemp READ tireMaxTemp WRITE setTireMaxTemp NOTIFY tireChanged)
    Q_PROPERTY(float tireTreadRemaining READ tireTreadRemaining WRITE setTireTreadRemaining NOTIFY tireChanged)
    // Brakes
    Q_PROPERTY(float brakePressure READ brakePressure WRITE setBrakePressure NOTIFY brakesChanged)
    // Steering
    Q_PROPERTY(float steeringRatio READ steeringRatio WRITE setSteeringRatio NOTIFY steeringChanged)
    Q_PROPERTY(float steeringLockAngle READ steeringLockAngle WRITE setSteeringLockAngle NOTIFY steeringChanged)
    Q_PROPERTY(bool powerSteering READ powerSteering WRITE setPowerSteering NOTIFY steeringChanged)
    // Driver
    Q_PROPERTY(float driverPositionX READ driverPositionX WRITE setDriverPositionX NOTIFY driverChanged)
    Q_PROPERTY(float driverPositionY READ driverPositionY WRITE setDriverPositionY NOTIFY driverChanged)
    Q_PROPERTY(float driverPositionZ READ driverPositionZ WRITE setDriverPositionZ NOTIFY driverChanged)
    Q_PROPERTY(float driverMass READ driverMass WRITE setDriverMass NOTIFY driverChanged)
    Q_PROPERTY(float driverHeight READ driverHeight WRITE setDriverHeight NOTIFY driverChanged)
    // AI
    Q_PROPERTY(float aiAggression READ aiAggression WRITE setAiAggression NOTIFY aiChanged)
    Q_PROPERTY(float aiSkill READ aiSkill WRITE setAiSkill NOTIFY aiChanged)
    Q_PROPERTY(float aiConsistency READ aiConsistency WRITE setAiConsistency NOTIFY aiChanged)
    // Transmission
    Q_PROPERTY(int transmissionType READ transmissionType WRITE setTransmissionType NOTIFY drivetrainChanged)
    Q_PROPERTY(int gearCount READ gearCount WRITE setGearCount NOTIFY drivetrainChanged)

    // ERS/Hybrid
    Q_PROPERTY(bool ersEnabled READ ersEnabled WRITE setErsEnabled NOTIFY ersChanged)
    Q_PROPERTY(int ersArchitecture READ ersArchitecture WRITE setErsArchitecture NOTIFY ersChanged)
    Q_PROPERTY(int ersDeploymentMode READ ersDeploymentMode WRITE setErsDeploymentMode NOTIFY ersChanged)
    Q_PROPERTY(float ersMgukPower READ ersMgukPower WRITE setErsMgukPower NOTIFY ersChanged)
    Q_PROPERTY(float ersMgukRegen READ ersMgukRegen WRITE setErsMgukRegen NOTIFY ersChanged)
    Q_PROPERTY(float ersMguhPower READ ersMguhPower WRITE setErsMguhPower NOTIFY ersChanged)
    Q_PROPERTY(float ersBatteryCapacity READ ersBatteryCapacity WRITE setErsBatteryCapacity NOTIFY ersChanged)
    Q_PROPERTY(float ersBatterySoc READ ersBatterySoc NOTIFY ersStateChanged)
    Q_PROPERTY(float ersBatteryTemp READ ersBatteryTemp NOTIFY ersStateChanged)
    Q_PROPERTY(float ersPerLapEnergy READ ersPerLapEnergy WRITE setErsPerLapEnergy NOTIFY ersChanged)
    Q_PROPERTY(float ersEnergyDeployed READ ersEnergyDeployed NOTIFY ersStateChanged)
    Q_PROPERTY(bool ersAttackAvailable READ ersAttackAvailable NOTIFY ersStateChanged)
    Q_PROPERTY(bool ersAttackActive READ ersAttackActive NOTIFY ersStateChanged)

    // Turbo
    Q_PROPERTY(int turboType READ turboType WRITE setTurboType NOTIFY turboChanged)
    Q_PROPERTY(float turboBoostPressure READ turboBoostPressure WRITE setTurboBoostPressure NOTIFY turboChanged)
    Q_PROPERTY(int turboThreshold READ turboThreshold WRITE setTurboThreshold NOTIFY turboChanged)
    Q_PROPERTY(float turboLag READ turboLag WRITE setTurboLag NOTIFY turboChanged)
    Q_PROPERTY(float turboWastegate READ turboWastegate WRITE setTurboWastegate NOTIFY turboChanged)
    Q_PROPERTY(int turboCount READ turboCount WRITE setTurboCount NOTIFY turboChanged)

    // DRS
    Q_PROPERTY(bool drsEnabled READ drsEnabled WRITE setDrsEnabled NOTIFY drsChanged)
    Q_PROPERTY(bool drsAutoActivate READ drsAutoActivate WRITE setDrsAutoActivate NOTIFY drsChanged)
    Q_PROPERTY(float drsSpeedThreshold READ drsSpeedThreshold WRITE setDrsSpeedThreshold NOTIFY drsChanged)
    Q_PROPERTY(bool drsActive READ drsActive NOTIFY drsStateChanged)
    Q_PROPERTY(float drsDragReduction READ drsDragReduction WRITE setDrsDragReduction NOTIFY drsChanged)

    // Damage Model
    Q_PROPERTY(bool damageEnabled READ damageEnabled WRITE setDamageEnabled NOTIFY damageChanged)
    Q_PROPERTY(float aeroDamage READ aeroDamage NOTIFY damageStateChanged)
    Q_PROPERTY(float engineDamage READ engineDamage NOTIFY damageStateChanged)
    Q_PROPERTY(float bodyDamage READ bodyDamage NOTIFY damageStateChanged)
    Q_PROPERTY(bool isEliminated READ isEliminated NOTIFY damageStateChanged)

    // Weather
    Q_PROPERTY(float trackWetness READ trackWetness WRITE setTrackWetness NOTIFY weatherChanged)
    Q_PROPERTY(float rainIntensity READ rainIntensity WRITE setRainIntensity NOTIFY weatherChanged)
    Q_PROPERTY(float ambientTemp READ ambientTemp WRITE setAmbientTemp NOTIFY weatherChanged)
    Q_PROPERTY(float trackTemp READ trackTemp NOTIFY weatherChanged)
    Q_PROPERTY(float aquaplaningRisk READ aquaplaningRisk NOTIFY weatherStateChanged)
    Q_PROPERTY(float trackGrip READ trackGrip NOTIFY weatherStateChanged)

    // Fuel
    Q_PROPERTY(float fuelKg READ fuelKg WRITE setFuelKg NOTIFY fuelChanged)
    Q_PROPERTY(float fuelCapacity READ fuelCapacity WRITE setFuelCapacity NOTIFY fuelChanged)
    Q_PROPERTY(bool fuelConsumptionEnabled READ fuelConsumptionEnabled WRITE setFuelConsumptionEnabled NOTIFY fuelChanged)

signals:
    void carNameChanged(const QString& name);
    void massChanged();
    void aeroChanged();
    void brakesChanged();
    void engineChanged();
    void drivetrainChanged();
    void suspensionChanged();
    void simulationChanged();
    void categoryChanged(const QString& category);
    void validationResult(bool valid, const QStringList& errors, const QStringList& warnings);
    void statusMessage(const QString& message);
    void validationChanged();
    void currentFileChanged(const QString& file);
    void wheelChanged();
    void tireChanged();
    void steeringChanged();
    void driverChanged();
    void aiChanged();
    void ersChanged();
    void ersStateChanged();
    void drsChanged();
    void drsStateChanged();
    void turboChanged();
    void damageChanged();
    void damageStateChanged();
    void weatherChanged();
    void weatherStateChanged();
    void fuelChanged();

public:
    explicit PhysicsQmlBridge(QObject* parent = nullptr);
    ~PhysicsQmlBridge();

    static PhysicsQmlBridge* instance() {
        static PhysicsQmlBridge inst;
        return &inst;
    }

    QString carName() const { return m_config.carName; }
    void setCarName(const QString& name) { m_config.carName = name; emit carNameChanged(name); }

    float totalMass() const { return m_config.mass; }
    void setTotalMass(float m) { m_config.mass = m; emit massChanged(); }

    float cgHeight() const { return m_config.cgHeight; }
    void setCgHeight(float h) { m_config.cgHeight = h; emit massChanged(); }

    float wheelbase() const { return m_config.wheelbase; }
    void setWheelbase(float w) { m_config.wheelbase = w; emit drivetrainChanged(); }

    float frontDownforce() const { return m_config.aero.frontDownforce; }
    void setFrontDownforce(float f) { m_config.aero.frontDownforce = f; emit aeroChanged(); }

    float rearDownforce() const { return m_config.aero.rearDownforce; }
    void setRearDownforce(float r) { m_config.aero.rearDownforce = r; emit aeroChanged(); }

    float drag() const { return m_config.aero.drag; }
    void setDrag(float d) { m_config.aero.drag = d; emit aeroChanged(); }

    bool drsEnabled() const { return m_config.aero.drsEnabled; }
    void setDrsEnabled(bool e);

    float brakeBalance() const { return m_config.brakes.brakeBalance; }
    void setBrakeBalance(float b) { m_config.brakes.brakeBalance = b; emit brakesChanged(); }

    bool absEnabled() const { return m_config.brakes.absEnabled; }
    void setAbsEnabled(bool e) { m_config.brakes.absEnabled = e; emit brakesChanged(); }

    int redlineRPM() const { return m_config.engine.redlineRPM; }
    void setRedlineRPM(int r) { m_config.engine.redlineRPM = r; emit engineChanged(); }

    int maxRPM() const { return m_config.engine.limiterRPM; }
    void setMaxRPM(int r) { m_config.engine.limiterRPM = r; emit engineChanged(); }

    float damping() const { return m_damping; }
    void setDamping(double d) { m_damping = d; emit aeroChanged(); }

    float rideHeight() const { return m_rideHeight; }
    void setRideHeight(double h) { m_rideHeight = h; emit aeroChanged(); }

    float finalDrive() const { return m_config.finalDrive; }
    void setFinalDrive(float v) { m_config.finalDrive = v; emit drivetrainChanged(); }

    float maxTorqueNm() const { return m_config.engine.maxTorqueNm; }
    void setMaxTorqueNm(float v) { m_config.engine.maxTorqueNm = v; emit engineChanged(); }

    float maxPowerKw() const { return m_config.engine.maxPowerKw; }
    void setMaxPowerKw(float v) { m_config.engine.maxPowerKw = v; emit engineChanged(); }

    float suspensionFrontSpring() const { return m_config.suspension.frontLeftSpring; }
    void setSuspensionFrontSpring(float v) { m_config.suspension.frontLeftSpring = v; emit suspensionChanged(); }

    float suspensionRearSpring() const { return m_config.suspension.rearLeftSpring; }
    void setSuspensionRearSpring(float v) { m_config.suspension.rearLeftSpring = v; emit suspensionChanged(); }

    bool isValid() const { return m_lastValidationValid; }
    QString currentFile() const { return m_currentFile; }
    void setCurrentFile(const QString& file);

    // Wheels
    float wheelDiameter() const { return m_config.wheelDiameter; }
    void setWheelDiameter(float v) { m_config.wheelDiameter = v; emit wheelChanged(); }
    float wheelWidth() const { return m_config.wheelWidth; }
    void setWheelWidth(float v) { m_config.wheelWidth = v; emit wheelChanged(); }
    float wheelMass() const { return m_config.wheelMass; }
    void setWheelMass(float v) { m_config.wheelMass = v; emit wheelChanged(); }

    // Tires
    QString tireCompound() const { return m_config.tireCompound; }
    void setTireCompound(const QString& v) { m_config.tireCompound = v; emit tireChanged(); }
    float tireOptimalTemp() const { return m_config.tireOptimalTemp; }
    void setTireOptimalTemp(float v) { m_config.tireOptimalTemp = v; emit tireChanged(); }
    float tireMaxTemp() const { return m_config.tireMaxTemp; }
    void setTireMaxTemp(float v) { m_config.tireMaxTemp = v; emit tireChanged(); }
    float tireTreadRemaining() const { return m_config.tireTreadRemaining; }
    void setTireTreadRemaining(float v) { m_config.tireTreadRemaining = v; emit tireChanged(); }

    // Brakes
    float brakePressure() const { return m_config.brakes.brakePressure; }
    void setBrakePressure(float v) { m_config.brakes.brakePressure = v; emit brakesChanged(); }

    // Steering
    float steeringRatio() const { return m_config.steeringRatio; }
    void setSteeringRatio(float v) { m_config.steeringRatio = v; emit steeringChanged(); }
    float steeringLockAngle() const { return m_config.steeringLockAngle; }
    void setSteeringLockAngle(float v) { m_config.steeringLockAngle = v; emit steeringChanged(); }
    bool powerSteering() const { return m_config.powerSteering; }
    void setPowerSteering(bool v) { m_config.powerSteering = v; emit steeringChanged(); }

    // Driver
    float driverPositionX() const { return m_config.driverPositionX; }
    void setDriverPositionX(float v) { m_config.driverPositionX = v; emit driverChanged(); }
    float driverPositionY() const { return m_config.driverPositionY; }
    void setDriverPositionY(float v) { m_config.driverPositionY = v; emit driverChanged(); }
    float driverPositionZ() const { return m_config.driverPositionZ; }
    void setDriverPositionZ(float v) { m_config.driverPositionZ = v; emit driverChanged(); }
    float driverMass() const { return m_config.driverMass; }
    void setDriverMass(float v) { m_config.driverMass = v; emit driverChanged(); }
    float driverHeight() const { return m_config.driverHeight; }
    void setDriverHeight(float v) { m_config.driverHeight = v; emit driverChanged(); }

    // AI
    float aiAggression() const { return m_config.aiAggression; }
    void setAiAggression(float v) { m_config.aiAggression = v; emit aiChanged(); }
    float aiSkill() const { return m_config.aiSkill; }
    void setAiSkill(float v) { m_config.aiSkill = v; emit aiChanged(); }
    float aiConsistency() const { return m_config.aiConsistency; }
    void setAiConsistency(float v) { m_config.aiConsistency = v; emit aiChanged(); }

    // Transmission
    int transmissionType() const { return m_config.transmissionType; }
    void setTransmissionType(int v) { m_config.transmissionType = v; emit drivetrainChanged(); }
    int gearCount() const { return m_config.gearCount; }
    void setGearCount(int v) { m_config.gearCount = v; emit drivetrainChanged(); }

    // ERS
    bool ersEnabled() const { return m_config.ers.enabled; }
    void setErsEnabled(bool e);
    int ersArchitecture() const { return m_config.ers.architecture; }
    void setErsArchitecture(int v);
    int ersDeploymentMode() const { return m_config.ers.deploymentMode; }
    void setErsDeploymentMode(int v);
    float ersMgukPower() const { return m_config.ers.mgukPowerKw; }
    void setErsMgukPower(float v);
    float ersMgukRegen() const { return m_config.ers.mgukRegenKw; }
    void setErsMgukRegen(float v);
    float ersMguhPower() const { return m_config.ers.mguhPowerKw; }
    void setErsMguhPower(float v);
    float ersBatteryCapacity() const { return m_config.ers.batteryCapacityMj; }
    void setErsBatteryCapacity(float v);
    float ersBatterySoc() const;
    float ersBatteryTemp() const;
    float ersPerLapEnergy() const { return m_config.ers.perLapEnergyMj; }
    void setErsPerLapEnergy(float v);
    float ersEnergyDeployed() const;
    bool ersAttackAvailable() const;
    bool ersAttackActive() const;
    Q_INVOKABLE void activateErsAttack();

    // DRS
    bool drsAutoActivate() const;
    void setDrsAutoActivate(bool v);
    float drsSpeedThreshold() const;
    void setDrsSpeedThreshold(float v);
    bool drsActive() const;
    float drsDragReduction() const;
    void setDrsDragReduction(float v);

    // Turbo
    int turboType() const { return m_config.turbo.type; }
    void setTurboType(int v) { m_config.turbo.type = v; emit turboChanged(); }
    float turboBoostPressure() const { return m_config.turbo.boostPressure; }
    void setTurboBoostPressure(float v) { m_config.turbo.boostPressure = v; emit turboChanged(); }
    int turboThreshold() const { return m_config.turbo.threshold; }
    void setTurboThreshold(int v) { m_config.turbo.threshold = v; emit turboChanged(); }
    float turboLag() const { return m_config.turbo.lag; }
    void setTurboLag(float v) { m_config.turbo.lag = v; emit turboChanged(); }
    float turboWastegate() const { return m_config.turbo.wastegate; }
    void setTurboWastegate(float v) { m_config.turbo.wastegate = v; emit turboChanged(); }
    int turboCount() const { return m_config.turbo.count; }
    void setTurboCount(int v) { m_config.turbo.count = v; emit turboChanged(); }

    // Damage
    bool damageEnabled() const { return m_config.damage.enabled; }
    void setDamageEnabled(bool e);
    float aeroDamage() const;
    float engineDamage() const;
    float bodyDamage() const;
    bool isEliminated() const;
    Q_INVOKABLE void resetDamage();

    // Weather
    float trackWetness() const { return m_config.weather.trackWetness; }
    void setTrackWetness(float v);
    float rainIntensity() const { return m_config.weather.rainIntensity; }
    void setRainIntensity(float v);
    float ambientTemp() const { return m_config.weather.ambientTemp; }
    void setAmbientTemp(float v);
    float trackTemp() const;
    float aquaplaningRisk() const;
    float trackGrip() const;

    // Fuel
    float fuelKg() const { return m_fuelKg; }
    void setFuelKg(float v);
    float fuelCapacity() const { return m_fuelCapacity; }
    void setFuelCapacity(float v);
    bool fuelConsumptionEnabled() const { return m_fuelConsumptionEnabled; }
    void setFuelConsumptionEnabled(bool v);

    void loadFromConfig(const CarPhysicsConfig& config);
    void saveToConfig(CarPhysicsConfig& config) const;
    Q_INVOKABLE bool validate();
    Q_INVOKABLE bool importFromCar(const QString& carPath);
    Q_INVOKABLE bool exportToCar(const QString& carDir);
    Q_INVOKABLE bool loadFile(const QString& path);
    Q_INVOKABLE bool saveFile(const QString& path);
    Q_INVOKABLE bool saveProject(const QString& path);
    Q_INVOKABLE bool openProject(const QString& path);
    Q_INVOKABLE void newProject();

    void setCurrentCategory(const QString& category);
    QString currentCategory() const { return m_currentCategory; }
    QStringList getCategories() const;
    float calculateDownforce(float speed) const;
    float calculatePower(int rpm) const;
    float calculateTorque(int rpm) const;
    float estimateLapTime(float trackLength) const;

    bool isSimulating() const { return m_isSimulating; }
    Q_INVOKABLE void startSimulation();
    Q_INVOKABLE void stopSimulation();

    // Tool panel methods
    Q_INVOKABLE void generateColliders(const QString& quality, const QString& mode, bool simplify, bool optimize);
    Q_INVOKABLE void autoGenerateColliders();
    Q_INVOKABLE void importLOD(const QString& filePath);
    Q_INVOKABLE void setupMirrors(int count, float angle, float distance);
    Q_INVOKABLE void setupExhaust(const QString& type, float length, float diameter);
    Q_INVOKABLE void setupLights(int count, const QString& type, float brightness);
    Q_INVOKABLE void setupInterior(const QString& material, float quality);
    Q_INVOKABLE void exportData(const QString& format, const QString& path);
    Q_INVOKABLE void paintConfig(const QString& region, const QString& color);
    Q_INVOKABLE void batchProcess(const QStringList& tasks);
    Q_INVOKABLE void runAllTools();

private:
    CarPhysicsConfig m_config;
    float m_damping = 0.1f;
    float m_rideHeight = 0.1f;
    float m_fuelKg = 80.0f;
    float m_fuelCapacity = 80.0f;
    bool m_fuelConsumptionEnabled = true;
    QString m_currentCategory;
    QString m_currentFile;
    bool m_isSimulating = false;
    bool m_lastValidationValid = true;
};

} // namespace ks