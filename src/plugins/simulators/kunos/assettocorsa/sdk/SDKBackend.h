#pragma once
#include <QObject>
#include <QString>
#include <QVector3D>
#include <QStringList>
#include <QVariant>
#include <QMap>
#include <QList>
#include <QColor>

#include "plugins/simulators/kunos/KsPlugin.h"

#define KS_SDK_VERSION "1.4"

namespace ks {

class Paths;

#define KS_SDK_PATH ks::plugins::kunos::KsPlugin::instance()->installPath().toUtf8().constData()

enum class KsCameraMode {
    Cockpit = 0,
    Car = 1,
    Drivable = 2,
    Track = 3,
    Helicopter = 4,
    OnBoardFree = 5,
    Free = 6,
    Deprecated = 7,
    ImageGeneratorCamera = 8,
    Start = 9
};

enum class KsWheel {
    FrontLeft = 0,
    FrontRight = 1,
    RearLeft = 2,
    RearRight = 3,
    Front = 12,
    Rear = 48,
    Left = 20,
    Right = 40,
    All = 60
};

enum class KsSurfaceType {
    Grass = 0,
    Dirt = 1,
    Snow = 2,
    Default = 255
};

enum class KsSurfaceExtendedType {
    Base = 0,
    ExtraTurf = 1,
    Grass = 2,
    Gravel = 3,
    Kerb = 4,
    Old = 5,
    Sand = 6,
    Ice = 7,
    Snow = 8
};

enum class KsFolderID {
    AppData = 0,
    Documents = 1,
    Root = 4,
    Cfg = 5,
    Logs = 7,
    Screenshots = 8,
    Replays = 9,
    ReplaysTemp = 10,
    UserSetups = 11,
    PPFilters = 12,
    ContentCars = 13,
    ContentDrivers = 14,
    ContentTracks = 15,
    ExtRoot = 16,
    ExtCfgSys = 17,
    ExtCfgUser = 18,
    ExtTextures = 21,
    Apps = 23,
    ExtCfgState = 25,
    ContentFonts = 26,
    RaceResults = 27,
    AppDataLocal = 28,
    ExtFonts = 29,
    Documents2 = 31,
    ExtLua = 32,
    ExtCache = 33,
    AppDataTemp = 34,
    ExtInternal = 35,
    ScriptOrigin = 1024,
    ScriptConfig = 1025,
    CurrentTrack = 1026,
    CurrentTrackLayout = 1027,
    CurrentTrackLayoutUI = 1028,
    AppsLua = 1029
};

enum class KsWeatherType {
    LightThunderstorm = 0,
    Thunderstorm = 1,
    HeavyThunderstorm = 2,
    LightDrizzle = 3,
    Drizzle = 4,
    HeavyDrizzle = 5,
    LightRain = 6,
    Rain = 7,
    HeavyRain = 8,
    LightSnow = 9,
    Snow = 10,
    HeavySnow = 11,
    LightSleet = 12,
    Sleet = 13,
    HeavySleet = 14,
    Clear = 15,
    FewClouds = 16,
    ScatteredClouds = 17,
    BrokenClouds = 18,
    OvercastClouds = 19,
    Fog = 20,
    Mist = 21,
    Smoke = 22,
    Haze = 23,
    Sand = 24,
    Dust = 25,
    Squalls = 26,
    Tornado = 27,
    Hurricane = 28,
    Cold = 29,
    Hot = 30,
    Windy = 31,
    Hail = 32
};

enum class KsIniFormat {
    Default = 0,
    DefaultAcd = 1,
    Extended = 10,
    ExtendedIncludes = 11
};

enum class KsBlendMode {
    Opaque = 0,
    AlphaBlend = 1,
    AlphaTest = 2
};

enum class KsDepthMode {
    Normal = 0,
    NoZWrite = 1,
    Off = 2
};

enum class KsLightType {
    Regular = 1,
    Line = 2
};

enum class KsFogAlgorithm {
    Original = 0,
    New = 1
};

enum class KsTonemapFunction {
    Linear = 0,
    LinearClamped = 1,
    Sensitometric = 2,
    Reinhard = 3,
    ReinhardLum = 4,
    Log = 5,
    LogLum = 6,
    ACES = 7,
    Uchimura = 8,
    RomBinDaHouse = 9,
    Lottes = 10,
    Uncharted = 11,
    Unreal = 12,
    Filmic = 13,
    ReinhardWp = 14,
    Juicy = 15,
    AgX = 16
};

struct KsContentFolder {
    KsFolderID id;
    QString path;
    QString description;
};

struct KsCarSpec {
    QString screenName;
    QString shortName;
    float graphicsOffset[3];
    float graphicsPitchRotation;
    float totalMass;
    float inertia[3];
    float driverEyes[3];
    float onboardExposure;
    float outboardExposure;
    float onBoardPitchAngle;
    float bumperCameraPos[3];
    float cameraPos[3];
    bool useAnimatedSuspensions;
    float steerLock;
    float steerRatio;
    float ffMult;
};

struct KsTyreSpec {
    float radius;
    float width;
    float height;
    float rim;
    QString compound;
    float pressure;
    float temperature;
    float wear;
};

struct KsEngineSpec {
    float maxPower;
    float maxTorque;
    float redline;
    float maxRpm;
    float minRpm;
    int gears;
    float gearRatios[8];
    float finalDrive;
};

struct KsTrackSpec {
    QString trackId;
    QString name;
    QString country;
    float length;
    float width;
    float pitCount;
    float heightDiff;
    QStringList layouts;
    QList<float> layoutLengths;
};

struct KsTrackLayout {
    QString id;
    QString name;
    float length;
    float width;
    int pits;
    QString config;
};

struct KsMeshInfo {
    QString name;
    QString material;
    int triangles;
    float center[3];
    float radius;
    int layer;
    bool castShadow;
    bool isTransparent;
};

struct KsCamera {
    QString name;
    float position[3];
    float rotation[3];
    float fov;
    float nearPlane;
    float farPlane;
};

struct KsLight {
    QString name;
    float position[3];
    float direction[3];
    KsLightType type;
    float intensity;
    float angle;
    float penumbra;
    QColor color;
};

struct KsWaypoint {
    float position[3];
    float direction[3];
    float curvature;
    float width;
    float camber;
    float grade;
};

class KsPhysicsState {
public:
    float position[3];
    float velocity[3];
    float acceleration[3];
    float rotation[3];
    float angularVelocity[3];

    float speedKmh;
    float speedMs;
    float rpm;
    float throttle;
    float brake;
    float steeringWheel;
    float steeringAckerman;

    float engineTorque;
    float wheelTorque[4];
    float brakeForce[4];
    float wheels[4];
    float suspension[4];
    float tyrePressure[4];

    float slipRatio[4];
    float slipAngle[4];
    float normalForce[4];

    float aeroDownforce;
    float dragForce;

    int currentGear;
    float gearRatio;
    float finalDriveRatio;

    float dt;

    void reset() {
        speedKmh = 0; speedMs = 0;
        rpm = 0; throttle = 0; brake = 0;
        steeringWheel = 0; steeringAckerman = 0;
        for (int i = 0; i < 4; ++i) {
            wheels[i] = 0; suspension[i] = 0; tyrePressure[i] = 0;
            slipRatio[i] = 0; slipAngle[i] = 0; normalForce[i] = 0;
        }
        aeroDownforce = 0; dragForce = 0;
        currentGear = 0; gearRatio = 0; finalDriveRatio = 0;
        dt = 0;
    }
};

class KsTelemetryData {
public:
    QString carId;
    int carIndex;
    int lap;
    float lapTime;
    float lastLapTime;
    float bestLapTime;

    float position[3];
    float velocity[3];
    float speedKmh;

    float heading;
    float pitch;
    float roll;

    float rpm;
    float throttle;
    float brake;

    int gear;
    float engineTemp;
    float oilTemp;
    float waterTemp;

    float tyreTemp[4];
    float tyreWear[4];
    float tyrePressure[4];

    float brakeTemp[4];
    float brakeWear[4];

    float drs;
    float abs;
    float tc;

    float fuelLevel;
    float fuelConsumption;
};

class SDKBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString sdkVersion READ getSdkVersion CONSTANT)
    Q_PROPERTY(QString installPath READ getInstallPath CONSTANT)

public:
    static SDKBackend* instance();

    explicit SDKBackend(QObject* parent = nullptr);
    ~SDKBackend();

    bool initialize();
    void shutdown();

    QString getSdkVersion() const { return KS_SDK_VERSION; }
    QString getInstallPath() const { return KS_SDK_PATH; }

    static QString getContentPath(KsFolderID folderId);
    static QStringList getCarList();
    static QStringList getTrackList();
    static QStringList getCarDataFiles(const QString& carId);
    static QStringList getTrackDataFiles(const QString& trackId);

    static bool loadCarSpec(const QString& carId, KsCarSpec& spec);
    static bool loadTrackSpec(const QString& trackId, KsTrackSpec& spec);
    static bool loadTrackLayouts(const QString& trackId, QList<KsTrackLayout>& layouts);
    static bool loadCarModelFiles(const QString& carId, QStringList& files);
    static bool loadTrackModelFiles(const QString& trackId, QStringList& files);

    static bool loadCarTyres(const QString& carId, QList<KsTyreSpec>& tyres);
    static bool loadCarEngine(const QString& carId, KsEngineSpec& engine);

    static QString getFolderPath(KsFolderID folder);
    static QString getCarModelPath(const QString& carId);
    static QString getTrackPath(const QString& trackId);
    static QString getCarDataPath(const QString& carId);
    static QString getTrackDataPath(const QString& trackId, const QString& layout = QString());

    static QMap<QString, QString> getCarBrands();
    static QMap<QString, QString> getTrackCountries();

    static bool exportCar(const QString& carId, const QString& outputPath);
    static bool exportTrack(const QString& trackId, const QString& outputPath);

    static float calculateDownforce(float speedMs, float aoa, float cl);
    static float calculateDrag(float speedMs, float cd, float area);
    static float calculateCornerG(float speedMs, float radius);
    static float calculateBrakeDistance(float speedMs, float deceleration);
    static float calculateStoppingDistance(float speedMs, float reactionTime);

signals:
    void initialized(bool success);

private:
    static SDKBackend* s_instance;
    bool m_initialized = false;
};

} // namespace ks