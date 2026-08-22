#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QVector3D>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDataStream>

/**
 * @brief AI Spline Editor for Assetto Corsa
 *
 * Reads, writes, and edits AI racing line spline data.
 * Based on community tools and documentation:
 * - AI Line Helper (RaceDepartment)
 * - ac-track-tools (github.com/nendotools/ac-track-tools)
 * - AC Python API documentation
 *
 * AI spline files:
 * - fast_lane.ai - Main racing line
 * - pit_lane.ai - Pit lane racing line
 * - ideal_line.ai - Alternative ideal line
 * - side_l.csv / side_r.csv - Track boundaries
 */
class AiSplineEditor {
public:
    struct AiSplinePoint {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float curvature = 0.0f;
        float speed = 0.0f;
        float distance = 0.0f;

        AiSplinePoint() = default;
        AiSplinePoint(float x, float y, float z) : x(x), y(y), z(z) {}

        float distanceTo(const AiSplinePoint& other) const {
            float dx = x - other.x;
            float dy = y - other.y;
            float dz = z - other.z;
            return std::sqrt(dx*dx + dy*dy + dz*dz);
        }
    };

    struct AiSpline {
        QString name;
        QVector<AiSplinePoint> points;
        float totalDistance = 0.0f;
        bool isClosed = false;

        bool isValid() const { return points.size() >= 2; }
        int pointCount() const { return points.size(); }

        float getLength() const;
        AiSplinePoint getPointAtDistance(float distance) const;
        float getCurvatureAt(int index) const;
    };

    struct AiBorder {
        QString name;
        QVector<AiSplinePoint> points;

        bool isValid() const { return !points.isEmpty(); }
        int pointCount() const { return points.size(); }
    };

    struct AiTrackData {
        QString trackName;
        AiSpline fastLane;
        AiSpline pitLane;
        AiSpline idealLine;
        AiBorder leftBorder;
        AiBorder rightBorder;

        bool hasFastLane() const { return fastLane.isValid(); }
        bool hasPitLane() const { return pitLane.isValid(); }
        bool hasBorders() const { return leftBorder.isValid() && rightBorder.isValid(); }
    };

    // File operations
    static AiTrackData loadTrack(const QString& trackPath);
    static bool saveTrack(const AiTrackData& data, const QString& trackPath);

    // AI spline file operations
    static AiSpline loadAiFile(const QString& filePath);
    static bool saveAiFile(const AiSpline& spline, const QString& filePath);

    // CSV border operations
    static AiBorder loadCsvBorder(const QString& filePath);
    static bool saveCsvBorder(const AiBorder& border, const QString& filePath);

    // AI hints operations
    static QJsonObject loadAiHints(const QString& trackPath);
    static bool saveAiHints(const QJsonObject& hints, const QString& trackPath);

    // Spline manipulation
    static AiSpline smoothSpline(const AiSpline& input, int iterations = 3);
    static AiSpline resampleSpline(const AiSpline& input, int targetPoints);
    static AiSpline subdivideSpline(const AiSpline& input, int subdivisions = 2);
    static AiSpline optimizeSpline(const AiSpline& input, float minDistance = 1.0f);

    // Analysis
    static float calculateTotalLength(const AiSpline& spline);
    static QVector<float> calculateCurvatures(const AiSpline& spline);
    static QVector<float> calculateSpeeds(const AiSpline& spline, float maxSpeed = 250.0f);
    static QVector<float> calculateDistances(const AiSpline& spline);
    static float getMaxCurvature(const AiSpline& spline);
    static float getMaxSpeed(const AiSpline& spline);

    // Validation
    static bool validateSpline(const AiSpline& spline, QString* error = nullptr);
    static bool validateBorders(const AiBorder& left, const AiBorder& right, QString* error = nullptr);
    static bool validateTrackData(const AiTrackData& data, QString* error = nullptr);

    // Utility
    static AiSplinePoint interpolate(const AiSplinePoint& a, const AiSplinePoint& b, float t);
    static AiSplinePoint lerp(const AiSplinePoint& a, const AiSplinePoint& b, float t);
    static float cross2D(const AiSplinePoint& a, const AiSplinePoint& b, const AiSplinePoint& c);

private:
    static bool parseAiBinary(QDataStream& stream, AiSpline& spline);
    static bool writeAiBinary(QDataStream& stream, const AiSpline& spline);
    static bool parseCsvLine(const QString& line, AiSplinePoint& point);
    static QString formatCsvLine(const AiSplinePoint& point);
};

/**
 * @brief AI Spline Manager - High-level interface
 */
class AiSplineManager : public QObject {
    Q_OBJECT
public:
    explicit AiSplineManager(QObject* parent = nullptr);

    bool loadSpline(const QString& filePath);
    bool saveSpline(const QString& filePath);
    bool hasSpline() const { return m_data.hasFastLane(); }
    QString getSplineInfo() const;
    bool smoothSpline(int iterations, int targetPoints);
    bool resampleSpline(int targetPoints);

    // Access
    AiSplineEditor::AiTrackData& data() { return m_data; }
    const AiSplineEditor::AiTrackData& data() const { return m_data; }

    // Fast lane
    bool hasFastLane() const { return m_data.hasFastLane(); }
    AiSplineEditor::AiSpline& fastLane() { return m_data.fastLane; }
    float getFastLaneLength() const;

    // Pit lane
    bool hasPitLane() const { return m_data.hasPitLane(); }
    AiSplineEditor::AiSpline& pitLane() { return m_data.pitLane; }

    // Borders
    bool hasBorders() const { return m_data.hasBorders(); }
    AiSplineEditor::AiBorder& leftBorder() { return m_data.leftBorder; }
    AiSplineEditor::AiBorder& rightBorder() { return m_data.rightBorder; }

    // Operations
    bool smoothFastLane(int iterations = 3);
    bool resampleFastLane(int targetPoints);
    bool generateBorders(float width = 10.0f);

    // Validation
    bool validate(QString* error = nullptr) const;

    // Convenience accessors for spline control points as QVector3D
    QVector<QVector3D> getSplinePoints() const {
        QVector<QVector3D> result;
        if (!m_data.hasFastLane()) return result;
        for (const auto& p : m_data.fastLane.points) {
            result.append(QVector3D(p.x, p.y, p.z));
        }
        return result;
    }

    void setSplinePoints(const QVector<QVector3D>& pts) {
        QVector<AiSplineEditor::AiSplinePoint> sp;
        sp.reserve(pts.size());
        for (const auto& p : pts) {
            sp.append(AiSplineEditor::AiSplinePoint(p.x(), p.y(), p.z()));
        }
        m_data.fastLane.points = sp;
    }

private:
    QString m_trackPath;
    AiSplineEditor::AiTrackData m_data;
    QString m_currentSplinePath;
};
