#include "FSPROExporter.h"
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamWriter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace ks { namespace fileformat {

KSFSPROExporter::KSFSPROExporter(QObject* parent)
    : QObject(parent) {}

bool KSFSPROExporter::exportFile(const QString& ksaudioPath, const QString& fsproPath)
{
    m_lastError.clear();
    emit exportStarted(fsproPath);

    QFile file(ksaudioPath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open .ksaudio file: " + ksaudioPath;
        emit exportFailed(m_lastError);
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        m_lastError = "JSON parse error: " + parseError.errorString();
        emit exportFailed(m_lastError);
        return false;
    }

    return exportJson(doc.object(), fsproPath);
}

bool KSFSPROExporter::exportJson(const QJsonObject& root, const QString& fsproPath)
{
    m_lastError.clear();
    emit exportStarted(fsproPath);

    QFile file(fsproPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_lastError = "Cannot write .fspro file: " + fsproPath;
        emit exportFailed(m_lastError);
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(2);
    xml.writeStartDocument();
    xml.writeStartElement("project");

    writeProject(xml, root);

    xml.writeEndElement(); // project
    xml.writeEndDocument();
    file.close();

    emit exportCompleted(fsproPath);
    return true;
}

bool KSFSPROExporter::roundTrip(const QString& originalFsproPath, const QString& ksaudioPath,
                                 const QString& outputFsproPath)
{
    // Step 1: Load the .ksaudio file
    QFile ksaudioFile(ksaudioPath);
    if (!ksaudioFile.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open .ksaudio file for round-trip: " + ksaudioPath;
        emit exportFailed(m_lastError);
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(ksaudioFile.readAll(), &parseError);
    ksaudioFile.close();

    if (parseError.error != QJsonParseError::NoError) {
        m_lastError = "JSON parse error during round-trip: " + parseError.errorString();
        emit exportFailed(m_lastError);
        return false;
    }

    return exportJson(doc.object(), outputFsproPath);
}

void KSFSPROExporter::writeProject(QXmlStreamWriter& xml, const QJsonObject& root)
{
    xml.writeAttribute("name", root["name"].toString());
    xml.writeAttribute("guid", root["guid"].toString());

    // Global parameters
    if (root.contains("globalParameters")) {
        xml.writeStartElement("globalParameters");
        QJsonArray params = root["globalParameters"].toArray();
        for (const auto& p : params)
            writeParameter(xml, p.toObject());
        xml.writeEndElement();
    }

    // Event groups
    if (root.contains("eventGroups")) {
        xml.writeStartElement("eventGroups");
        writeEventGroups(xml, root["eventGroups"].toArray());
        xml.writeEndElement();
    }

    // Buses
    if (root.contains("buses")) {
        xml.writeStartElement("mixer");
        writeBuses(xml, root["buses"].toArray());
        xml.writeEndElement();
    }

    // VCAs
    if (root.contains("vcas")) {
        xml.writeStartElement("vcas");
        QJsonArray vcas = root["vcas"].toArray();
        for (const auto& v : vcas)
            writeVCA(xml, v.toObject());
        xml.writeEndElement();
    }

    // Snapshots
    if (root.contains("snapshots")) {
        xml.writeStartElement("snapshots");
        QJsonArray snaps = root["snapshots"].toArray();
        for (const auto& s : snaps)
            writeSnapshot(xml, s.toObject());
        xml.writeEndElement();
    }

    // Sounds
    if (root.contains("sounds")) {
        xml.writeStartElement("sounds");
        QJsonArray snds = root["sounds"].toArray();
        for (const auto& s : snds)
            writeSound(xml, s.toObject());
        xml.writeEndElement();
    }

    // Banks
    if (root.contains("banks")) {
        xml.writeStartElement("banks");
        QJsonArray bks = root["banks"].toArray();
        for (const auto& b : bks)
            writeBank(xml, b.toObject());
        xml.writeEndElement();
    }

    // Mixer snapshots
    if (root.contains("mixerSnapshots")) {
        writeMixer(xml, root["mixerSnapshots"].toObject());
    }

    emit exportProgress(100);
}

void KSFSPROExporter::writeEventGroups(QXmlStreamWriter& xml, const QJsonArray& groups)
{
    for (const auto& g : groups) {
        writeEventGroup(xml, g.toObject());
    }
}

void KSFSPROExporter::writeEventGroup(QXmlStreamWriter& xml, const QJsonObject& group)
{
    xml.writeStartElement("eventGroup");
    xml.writeAttribute("guid", group["guid"].toString());
    xml.writeAttribute("name", group["name"].toString());

    if (group.contains("events")) {
        xml.writeStartElement("events");
        QJsonArray events = group["events"].toArray();
        for (const auto& e : events)
            writeEvent(xml, e.toObject());
        xml.writeEndElement();
    }

    if (group.contains("childGroups")) {
        xml.writeStartElement("childGroups");
        writeEventGroups(xml, group["childGroups"].toArray());
        xml.writeEndElement();
    }

    xml.writeEndElement();
}

void KSFSPROExporter::writeEvent(QXmlStreamWriter& xml, const QJsonObject& event)
{
    xml.writeStartElement("event");
    xml.writeAttribute("guid", event["guid"].toString());
    xml.writeAttribute("name", event["name"].toString());
    xml.writeAttribute("type", event["type"].toString("2D"));

    if (event.contains("maxInstances"))
        xml.writeAttribute("maxInstances", QString::number(event["maxInstances"].toInt()));
    if (event.contains("priority"))
        xml.writeAttribute("priority", QString::number(event["priority"].toInt()));

    // Local parameters
    if (event.contains("parameters")) {
        xml.writeStartElement("parameters");
        QJsonArray params = event["parameters"].toArray();
        for (const auto& p : params)
            writeParameter(xml, p.toObject());
        xml.writeEndElement();
    }

    // Timeline
    if (event.contains("timeline")) {
        writeTimeline(xml, event["timeline"].toObject());
    }

    // Spatialization
    if (event.contains("spatialization")) {
        QJsonObject sp = event["spatialization"].toObject();
        xml.writeStartElement("spatialization");
        xml.writeAttribute("enabled", sp["enabled"].toBool() ? "true" : "false");
        xml.writeAttribute("panLevel", QString::number(sp["panLevel"].toDouble()));
        xml.writeAttribute("speakerMode", sp["speakerMode"].toString("auto"));

        xml.writeStartElement("attenuation");
        xml.writeAttribute("minDistance", QString::number(sp["minDistance"].toDouble()));
        xml.writeAttribute("maxDistance", QString::number(sp["maxDistance"].toDouble()));
        xml.writeAttribute("model", sp["model"].toString("inverse"));
        xml.writeEndElement(); // attenuation

        xml.writeEndElement(); // spatialization
    }

    xml.writeEndElement();
}

void KSFSPROExporter::writeTimeline(QXmlStreamWriter& xml, const QJsonObject& timeline)
{
    xml.writeStartElement("timeline");

    if (timeline.contains("tracks")) {
        QJsonArray tracks = timeline["tracks"].toArray();
        for (const auto& t : tracks)
            writeTrack(xml, t.toObject());
    }

    xml.writeEndElement();
}

void KSFSPROExporter::writeTrack(QXmlStreamWriter& xml, const QJsonObject& track)
{
    xml.writeStartElement("track");
    xml.writeAttribute("name", track["name"].toString());
    xml.writeAttribute("id", track["id"].toString());

    if (track.contains("keyframes")) {
        QJsonArray keyframes = track["keyframes"].toArray();
        for (const auto& kf : keyframes)
            writeKeyframe(xml, kf.toObject());
    }

    xml.writeEndElement();
}

void KSFSPROExporter::writeKeyframe(QXmlStreamWriter& xml, const QJsonObject& keyframe)
{
    xml.writeStartElement("keyframe");
    xml.writeAttribute("time", QString::number(keyframe["time"].toDouble()));
    xml.writeAttribute("id", keyframe["id"].toString());

    if (keyframe.contains("action")) {
        QJsonObject action = keyframe["action"].toObject();
        xml.writeStartElement("action");
        xml.writeAttribute("type", action["type"].toString());
        if (action.contains("sound"))
            xml.writeAttribute("sound", action["sound"].toString());
        if (action.contains("parameter"))
            xml.writeAttribute("parameter", action["parameter"].toString());
        if (action.contains("value"))
            xml.writeAttribute("value", QString::number(action["value"].toDouble()));
        xml.writeEndElement();
    }

    xml.writeEndElement();
}

void KSFSPROExporter::writeBuses(QXmlStreamWriter& xml, const QJsonArray& buses)
{
    for (const auto& b : buses) {
        writeBus(xml, b.toObject());
    }
}

void KSFSPROExporter::writeBus(QXmlStreamWriter& xml, const QJsonObject& bus)
{
    xml.writeStartElement("bus");
    xml.writeAttribute("guid", bus["guid"].toString());
    xml.writeAttribute("name", bus["name"].toString());

    if (bus.contains("volume"))
        xml.writeAttribute("volume", QString::number(bus["volume"].toDouble()));
    if (bus.contains("mute"))
        xml.writeAttribute("mute", bus["mute"].toBool() ? "true" : "false");
    if (bus.contains("solo"))
        xml.writeAttribute("solo", bus["solo"].toBool() ? "true" : "false");

    // Linked VCAs
    if (bus.contains("linkedVCAs")) {
        xml.writeStartElement("linkedVCAs");
        QJsonArray vcas = bus["linkedVCAs"].toArray();
        for (const auto& v : vcas)
            xml.writeTextElement("vca", v.toString());
        xml.writeEndElement();
    }

    // Effects
    if (bus.contains("effects")) {
        xml.writeStartElement("effects");
        QJsonArray effects = bus["effects"].toArray();
        for (const auto& e : effects) {
            QJsonObject eff = e.toObject();
            xml.writeStartElement("effect");
            xml.writeAttribute("name", eff["name"].toString());
            xml.writeAttribute("type", eff["type"].toString());
            xml.writeAttribute("enabled", eff["enabled"].toBool() ? "true" : "false");

            if (eff.contains("parameters")) {
                QJsonObject params = eff["parameters"].toObject();
                for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
                    xml.writeStartElement("parameter");
                    xml.writeAttribute("name", it.key());
                    xml.writeAttribute("value", QString::number(it.value().toDouble()));
                    xml.writeEndElement();
                }
            }

            xml.writeEndElement(); // effect
        }
        xml.writeEndElement(); // effects
    }

    // Child buses
    if (bus.contains("childBuses")) {
        xml.writeStartElement("childBuses");
        writeBuses(xml, bus["childBuses"].toArray());
        xml.writeEndElement();
    }

    xml.writeEndElement(); // bus
}

void KSFSPROExporter::writeVCA(QXmlStreamWriter& xml, const QJsonObject& vca)
{
    xml.writeStartElement("vca");
    xml.writeAttribute("guid", vca["guid"].toString());
    xml.writeAttribute("name", vca["name"].toString());
    xml.writeAttribute("volume", QString::number(vca["volume"].toDouble()));

    if (vca.contains("objects")) {
        QJsonArray objects = vca["objects"].toArray();
        for (const auto& o : objects) {
            xml.writeTextElement("object", o.toString());
        }
    }

    xml.writeEndElement();
}

void KSFSPROExporter::writeSnapshot(QXmlStreamWriter& xml, const QJsonObject& snapshot)
{
    xml.writeStartElement("snapshot");
    xml.writeAttribute("guid", snapshot["guid"].toString());
    xml.writeAttribute("name", snapshot["name"].toString());
    xml.writeAttribute("type", snapshot["type"].toString());

    if (snapshot.contains("busVolumes")) {
        QJsonObject busVolumes = snapshot["busVolumes"].toObject();
        for (auto it = busVolumes.constBegin(); it != busVolumes.constEnd(); ++it) {
            xml.writeStartElement("busVolume");
            xml.writeAttribute("bus", it.key());
            xml.writeAttribute("value", QString::number(it.value().toDouble()));
            xml.writeEndElement();
        }
    }

    xml.writeEndElement();
}

void KSFSPROExporter::writeSound(QXmlStreamWriter& xml, const QJsonObject& sound)
{
    xml.writeStartElement("sound");
    xml.writeAttribute("guid", sound["guid"].toString());
    xml.writeAttribute("name", sound["name"].toString());
    xml.writeAttribute("file", sound["file"].toString());

    if (sound.contains("sampleRate"))
        xml.writeAttribute("sampleRate", QString::number(sound["sampleRate"].toInt()));
    if (sound.contains("channels"))
        xml.writeAttribute("channels", QString::number(sound["channels"].toInt()));
    if (sound.contains("format"))
        xml.writeAttribute("format", sound["format"].toString());

    xml.writeEndElement();
}

void KSFSPROExporter::writeBank(QXmlStreamWriter& xml, const QJsonObject& bank)
{
    xml.writeStartElement("bank");
    xml.writeAttribute("guid", bank["guid"].toString());
    xml.writeAttribute("name", bank["name"].toString());

    if (bank.contains("eventGuids")) {
        QJsonArray eventGuids = bank["eventGuids"].toArray();
        for (const auto& eg : eventGuids) {
            xml.writeTextElement("eventGuid", eg.toString());
        }
    }

    xml.writeEndElement();
}

void KSFSPROExporter::writeParameter(QXmlStreamWriter& xml, const QJsonObject& param)
{
    xml.writeStartElement("parameter");
    xml.writeAttribute("guid", param["guid"].toString());
    xml.writeAttribute("name", param["name"].toString());
    xml.writeAttribute("type", param["type"].toString("float"));
    xml.writeAttribute("scope", param["scope"].toString("global"));

    if (param.contains("unit"))
        xml.writeAttribute("unit", param["unit"].toString());
    if (param.contains("min"))
        xml.writeAttribute("min", QString::number(param["min"].toDouble()));
    if (param.contains("max"))
        xml.writeAttribute("max", QString::number(param["max"].toDouble()));
    if (param.contains("default"))
        xml.writeAttribute("default", QString::number(param["default"].toDouble()));

    xml.writeEndElement();
}

void KSFSPROExporter::writeMixer(QXmlStreamWriter& xml, const QJsonObject& mixer)
{
    xml.writeStartElement("mixerSnapshots");

    if (mixer.contains("snapshots")) {
        QJsonArray snapshots = mixer["snapshots"].toArray();
        for (const auto& s : snapshots) {
            QJsonObject snap = s.toObject();
            xml.writeStartElement("mixerSnapshot");
            xml.writeAttribute("guid", snap["guid"].toString());
            xml.writeAttribute("name", snap["name"].toString());

            if (snap.contains("busVolumes")) {
                QJsonObject busVolumes = snap["busVolumes"].toObject();
                for (auto it = busVolumes.constBegin(); it != busVolumes.constEnd(); ++it) {
                    xml.writeStartElement("busVolume");
                    xml.writeAttribute("bus", it.key());
                    xml.writeAttribute("value", QString::number(it.value().toDouble()));
                    xml.writeEndElement();
                }
            }

            xml.writeEndElement();
        }
    }

    xml.writeEndElement();
}

}} // namespace ks::fileformat
