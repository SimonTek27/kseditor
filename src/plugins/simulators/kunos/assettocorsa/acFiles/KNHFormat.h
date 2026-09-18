#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include <QVector3D>
#include <QVector2D>
#include <QFile>
#include <QDataStream>

namespace ks {

// ============================================================================
// KNH - Kunos Nav Highway (AI Racing Line / Drive Base Position)
// ============================================================================

struct KNHWaypoint {
    QVector3D position;        // World position
    QVector3D tangent;         // Direction of travel
    float curvature = 0.0f;    // Track curvature at this point
    float width = 10.0f;       // Track width
    float preferredRadius = 0.0f;
    float turnIn = 0.0f;
    float apex = 0.0f;
    float brakePoint = 0.0f;
    float throttlePoint = 0.0f;
    float targetSpeed = 0.0f;  // Ideal speed at this waypoint (km/h)
    float gear = 4.0f;         // Suggested gear
    int sector = 0;            // Sector index (0-based)
    bool isCorner = false;
    bool isStraight = false;
    bool isBrakingZone = false;
    bool isApexZone = false;
};

struct KNHSector {
    int index = 0;
    QVector3D startPos;
    QVector3D endPos;
    float length = 0.0f;
    int startWaypoint = 0;
    int endWaypoint = 0;
};

struct KNHCameraSpline {
    QString name;
    QVector<QVector3D> controlPoints;
    QVector<float> knots;
    int degree = 3;
    bool cyclic = false;
};

struct KNHCameraSplineConfig {
    float fov = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float exposure = 1.0f;
    float minExposure = 0.0f;
    float maxExposure = 4.0f;
    float dofFocus = 10.0f;
    float dofFactor = 2.0f;
    float dofRange = 20.0f;
    bool dofManual = false;
    float shadowSplits[3] = {2.0f, 12.0f, 50.0f};
    float inPoint = 0.0f;
    float outPoint = 60.0f;
    bool isFixed = false;
    float splineRotation = 0.0f;
    float splineAnimationLength = 10.0f;
};

struct KNHHeader {
    char magic[4] = {'K', 'N', 'H', '\0'};
    quint32 version = 1;
    quint32 flags = 0;
    quint32 waypointCount = 0;
    quint32 sectorCount = 0;
    quint32 cameraSplineCount = 0;
    quint32 cameraConfigCount = 0;
};

class KNHRacingLine {
public:
    QString trackName;
    QString trackConfig;
    KNHHeader header;
    QVector<KNHWaypoint> waypoints;
    QVector<KNHSector> sectors;
    QVector<KNHCameraSpline> cameraSplines;
    QVector<KNHCameraSplineConfig> cameraConfigs;

    void clear();
    bool isEmpty() const;
    int totalWaypoints() const;
    float totalLength() const;
    float totalTime() const;

    // Interpolation
    QVector3D getPositionAt(float distance) const;
    float getSpeedAt(float distance) const;
    int getSectorAt(float distance) const;

    // Analysis
    float estimateLapTime() const;
    QVector<int> findBrakingZones(float threshold = 0.3f) const;
    QVector<int> findApexes() const;

    // Validation
    bool validate(QString& error) const;
};

class KNHReader {
public:
    bool read(const QString& path, KNHRacingLine& line);
    QString errorString() const { return m_error; }

private:
    bool readHeader(QDataStream& stream, KNHHeader& header);
    bool readWaypoints(QDataStream& stream, KNHRacingLine& line);
    bool readSectors(QDataStream& stream, KNHRacingLine& line);
    bool readCameraSplines(QDataStream& stream, KNHRacingLine& line);
    bool readCameraConfigs(QDataStream& stream, KNHRacingLine& line);

    QString m_error;
};

class KNHWriter {
public:
    bool write(const QString& path, const KNHRacingLine& line);
    bool writeBinary(const QString& path, const KNHRacingLine& line);
    bool writeJson(const QString& path, const KNHRacingLine& line);
    QString errorString() const { return m_error; }

private:
    void writeHeader(QDataStream& stream, const KNHHeader& header);
    void writeWaypoints(QDataStream& stream, const KNHRacingLine& line);
    void writeSectors(QDataStream& stream, const KNHRacingLine& line);
    void writeCameraSplines(QDataStream& stream, const KNHRacingLine& line);
    void writeCameraConfigs(QDataStream& stream, const KNHRacingLine& line);

    QString m_error;
};

class KNHConverter {
public:
    // Convert from AC's native KNH format (fast_lane.ai, ideal_line.ai, etc.)
    static bool convertFromFastLane(const QString& fastLanePath, const QString& knhPath);
    static bool convertFromIdealLine(const QString& idealLinePath, const QString& knhPath);
    static bool convertFromCameraSpline(const QString& splinePath, const QString& knhPath);

    // Generate racing line from track geometry
    static bool generateFromTrackGeometry(const QString& trackPath, const QString& knhPath, float targetSpeed = 150.0f);

    // Export to AC native formats
    static bool exportFastLane(const QString& knhPath, const QString& outputPath);
    static bool exportIdealLine(const QString& knhPath, const QString& outputPath);
    static bool exportCameraSplines(const QString& knhPath, const QString& outputDir);
};

} // namespace ks