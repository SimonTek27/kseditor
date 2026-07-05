#ifndef ACEVENTDEFS_H
#define ACEVENTDEFS_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QSet>

namespace ks {
namespace audio {

struct ACEventDef {
    QString path;
    QString name;
    QString category;
    QString description;
    bool is3D;
    bool loops;
    float defaultVolume;
    QStringList parameters;
};

struct ACParameterDef {
    QString name;
    float min;
    float max;
    float defaultValue;
    QString description;
};

class ACEventDefs {
public:
    static const ACEventDefs& instance() {
        static ACEventDefs defs;
        return defs;
    }

    static QString carEventPath(const QString& carId, const QString& eventName) {
        return QString("/cars/%1/%2").arg(carId, eventName);
    }

    const ACEventDef* getEvent(const QString& name) const {
        auto it = m_events.constFind(name);
        return it != m_events.constEnd() ? &(it.value()) : nullptr;
    }

    const ACEventDef* getEventByPath(const QString& path) const {
        for (auto it = m_events.constBegin(); it != m_events.constEnd(); ++it) {
            if (it.value().path == path) return &(it.value());
        }
        return nullptr;
    }

    QStringList eventNames() const { return m_events.keys(); }
    QStringList eventPaths() const {
        QStringList paths;
        for (auto it = m_events.constBegin(); it != m_events.constEnd(); ++it) {
            paths.append(it.value().path);
        }
        return paths;
    }

    QStringList eventsByCategory(const QString& category) const {
        QStringList names;
        for (auto it = m_events.constBegin(); it != m_events.constEnd(); ++it) {
            if (it.value().category == category) names.append(it.value().name);
        }
        return names;
    }

    const ACParameterDef* getParameter(const QString& name) const {
        auto it = m_parameters.constFind(name);
        return it != m_parameters.constEnd() ? &(it.value()) : nullptr;
    }

    QStringList parameterNames() const { return m_parameters.keys(); }

    QStringList categories() const {
        QSet<QString> cats;
        for (auto it = m_events.constBegin(); it != m_events.constEnd(); ++it) {
            cats.insert(it.value().category);
        }
        return cats.values();
    }

    static QStringList defaultCarEvents() {
        return {"engine_int", "engine_ext", "turbo", "limiter", "door", "horn"};
    }

    static QString soundbankPath(const QString& carDir, const QString& carId) {
        return QString("%1/sfx/%2.bank").arg(carDir, carId);
    }

    static QString guidsPath(const QString& carDir) {
        return QString("%1/sfx/GUIDs.txt").arg(carDir);
    }

    static QString globalGuidsPath(const QString& acRoot) {
        return QString("%1/content/sfx/GUIDs.txt").arg(acRoot);
    }

private:
    QMap<QString, ACEventDef> m_events;
    QMap<QString, ACParameterDef> m_parameters;

    ACEventDefs() {
        addEvent("engine_int", "event:/cars/{car}/engine_int", "Engine", "Interior engine sound", true, true, 1.0f,
                 {"rpms", "throttle"});
        addEvent("engine_ext", "event:/cars/{car}/engine_ext", "Engine", "Exterior engine sound", true, true, 1.0f,
                 {"rpms", "throttle"});
        addEvent("turbo", "event:/cars/{car}/turbo", "Engine", "Turbo whistle", true, true, 0.6f,
                 {"boost"});
        addEvent("limiter", "event:/cars/{car}/limiter", "Engine", "Rev limiter", true, false, 0.8f,
                 {"decay"});
        addEvent("door", "event:/cars/{car}/door", "Body", "Door open/close", true, false, 0.7f,
                 {"state"});
        addEvent("horn", "event:/cars/{car}/horn", "Body", "Horn", true, false, 0.9f,
                 {});
		addEvent("backfire_ext", "event:/cars/{car}/backfire_ext", "Backfire", "Backfire sound", true, false, 0.8f,
			{ "throttle","event cone angle"});
		addEvent("backfire_int", "event:/cars/{car}/backfire_int", "Backfire", "Interior backfire sound", true, false, 0.8f,
			{ "throttle","event cone angle" });
		addEvent("gear_ext", "event:/cars/{car}/gear_ext", "Engine", "Gear change sound (exterior)", true, false, 0.8f,
			{ "state", "event cone angle" });
		addEvent("gear_int", "event:/cars/{car}/gear_int", "Engine", "Gear change sound (interior)", true, false, 0.8f,
			{ "state" });
        addEvent("gear_grind", "event:/cars/{car}/gear_grind", "Engine", "Gear grind sound", true, false, 0.8f,
            { "timeline" });
        addEvent("skid_ext", "event:/cars/{car}/skid_ext", "Tires", "Skid sound (exterior)", true, false, 0.8f,
            { "timeline", "event cone angle" });
		addEvent("skid_int", "event:/cars/{car}/skid_int", "Tires", "Skid sound (interior)", true, false, 0.8f,
			{ "timeline" });
		addEvent("transmission", "event:/cars/{car}/transmission", "Transmission", "Transmission sound", true, true, 0.7f,
			{ "timeline", "throttle","drivetrain_speed"});
        addEvent("wheel", "event:/cars/{car}/wheel", "Tires", "Wheel sound", true, false, 0.8f,
            { "timeline" });
		addEvent("wind", "event:/cars/{car}/wind", "Environment", "Wind noise", true, true, 0.5f,
			{ "timeline", "air_pressure", "speed" });

        addParameter("timeline", 0.0f, 1.0f, 0.0f, "Timeline position (0-1)");
        addParameter("event cone angle", 0.0f, 360.0f, 0.0f, "Angle between event forward and listener (degrees)");
        addParameter("state", 0.0f, 1.0f, 0.0f, "Door state (0=closed, 1=open)");
        addParameter("rpms", 0.0f, 9000.0f, 1000.0f, "Engine RPM");
        addParameter("throttle", 0.0f, 1.0f, 0.0f, "Throttle position (0-1)");
		addParameter("brake", 0.0f, 1.0f, 0.0f, "Brake position (0-1)");
		addParameter("inflation", 0.0f, 1.0f, 0.0f, "Tire inflation (0-1)");
		addParameter("suspension_damage", 0.0f, 1.0f, 0.0f, "Suspension compression (0-1)");
        addParameter("boost", 0.0f, 3.0f, 0.0f, "Turbo boost pressure (bar)");
        addParameter("bov", 0.0f, 1.0f, 0.0f, "Blow-off valve position (0-1)");
		addParameter("boc-decay", 0.0f, 1.0f, 1.0f, "Blow-off valve decay (1 = fresh hit)");
        addParameter("decay", 0.0f, 1.0f, 1.0f, "Rev limiter decay (1 = fresh hit)");
		addParameter("drivetrain_speed", 0.0f, 300.0f, 0.0f, "Drivetrain speed (km/h)");
		addParameter("air_pressure", 0.0f, 2.0f, 1.0f, "Air pressure (1 = sea level)");
		addParameter("speed", 0.0f, 300.0f, 0.0f, "Vehicle speed (km/h)");
    }

    void addEvent(const QString& name, const QString& path, const QString& category,
                  const QString& desc, bool is3D, bool loops, float vol, const QStringList& params) {
        ACEventDef def;
        def.name = name;
        def.path = path;
        def.category = category;
        def.description = desc;
        def.is3D = is3D;
        def.loops = loops;
        def.defaultVolume = vol;
        def.parameters = params;
        m_events[name] = def;
    }

    void addParameter(const QString& name, float min, float max, float def, const QString& desc) {
        ACParameterDef param;
        param.name = name;
        param.min = min;
        param.max = max;
        param.defaultValue = def;
        param.description = desc;
        m_parameters[name] = param;
    }
};

} // namespace audio
} // namespace ks

#endif // ACEVENTDEFS_H
