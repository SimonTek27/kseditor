#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

class QXmlStreamWriter;

namespace ks { namespace fileformat {

struct FSPROProject;

// ============================================================================
// KSFSPROExporter — export .ksaudio JSON → FMOD Studio .fspro XML
//
// Reverse of KSFSPROImporter: reads a v2 (FSPRO-compatible) .ksaudio project
// and writes it back as .fspro XML, completing the lossless round-trip.
// ============================================================================

class KSFSPROExporter : public QObject {
    Q_OBJECT
public:
    explicit KSFSPROExporter(QObject* parent = nullptr);

    // Export a .ksaudio project file to .fspro XML
    bool exportFile(const QString& ksaudioPath, const QString& fsproPath);

    // Export from a parsed JSON object
    bool exportJson(const QJsonObject& root, const QString& fsproPath);

    // Convenience: round-trip in one call (load .fspro → save .ksaudio → export .fspro)
    bool roundTrip(const QString& originalFsproPath, const QString& ksaudioPath,
                   const QString& outputFsproPath);

    QString lastError() const { return m_lastError; }

signals:
    void exportStarted(const QString& path);
    void exportProgress(int percent);
    void exportCompleted(const QString& path);
    void exportFailed(const QString& error);

private:
    QString m_lastError;

    void writeProject(QXmlStreamWriter& xml, const QJsonObject& root);
    void writeEventGroups(QXmlStreamWriter& xml, const QJsonArray& groups);
    void writeEventGroup(QXmlStreamWriter& xml, const QJsonObject& group);
    void writeEvent(QXmlStreamWriter& xml, const QJsonObject& event);
    void writeTimeline(QXmlStreamWriter& xml, const QJsonObject& timeline);
    void writeTrack(QXmlStreamWriter& xml, const QJsonObject& track);
    void writeKeyframe(QXmlStreamWriter& xml, const QJsonObject& keyframe);
    void writeBus(QXmlStreamWriter& xml, const QJsonObject& bus);
    void writeBuses(QXmlStreamWriter& xml, const QJsonArray& buses);
    void writeVCA(QXmlStreamWriter& xml, const QJsonObject& vca);
    void writeSnapshot(QXmlStreamWriter& xml, const QJsonObject& snapshot);
    void writeSound(QXmlStreamWriter& xml, const QJsonObject& sound);
    void writeBank(QXmlStreamWriter& xml, const QJsonObject& bank);
    void writeParameter(QXmlStreamWriter& xml, const QJsonObject& param);
    void writeMixer(QXmlStreamWriter& xml, const QJsonObject& mixer);
};

}} // namespace ks::fileformat
