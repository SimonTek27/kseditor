#include "ACEventBridge.h"
#include "Audio/ACEventDefs.h"

namespace ks {
namespace audio {

ACEventBridge* ACEventBridge::s_instance = nullptr;

ACEventBridge* ACEventBridge::instance()
{
    return s_instance;
}

ACEventBridge::ACEventBridge(QObject* parent)
    : QObject(parent)
{
    s_instance = this;
}

ACEventBridge::~ACEventBridge()
{
    s_instance = nullptr;
}

QStringList ACEventBridge::categories() const
{
    return ACEventDefs::instance().categories();
}

QStringList ACEventBridge::eventNames() const
{
    return ACEventDefs::instance().eventNames();
}

int ACEventBridge::eventCount() const
{
    return ACEventDefs::instance().eventNames().size();
}

int ACEventBridge::parameterCount() const
{
    return ACEventDefs::instance().parameterNames().size();
}

QStringList ACEventBridge::eventsByCategory(const QString& category) const
{
    return ACEventDefs::instance().eventsByCategory(category);
}

QString ACEventBridge::eventDescription(const QString& eventName) const
{
    auto* ev = ACEventDefs::instance().getEvent(eventName);
    return ev ? ev->description : QString();
}

QString ACEventBridge::eventCategory(const QString& eventName) const
{
    auto* ev = ACEventDefs::instance().getEvent(eventName);
    return ev ? ev->category : QString();
}

QString ACEventBridge::eventPath(const QString& eventName) const
{
    auto* ev = ACEventDefs::instance().getEvent(eventName);
    return ev ? ev->path : QString();
}

bool ACEventBridge::eventIs3D(const QString& eventName) const
{
    auto* ev = ACEventDefs::instance().getEvent(eventName);
    return ev ? ev->is3D : false;
}

bool ACEventBridge::eventLoops(const QString& eventName) const
{
    auto* ev = ACEventDefs::instance().getEvent(eventName);
    return ev ? ev->loops : false;
}

double ACEventBridge::eventDefaultVolume(const QString& eventName) const
{
    auto* ev = ACEventDefs::instance().getEvent(eventName);
    return ev ? static_cast<double>(ev->defaultVolume) : 0.0;
}

QStringList ACEventBridge::eventParameters(const QString& eventName) const
{
    auto* ev = ACEventDefs::instance().getEvent(eventName);
    return ev ? ev->parameters : QStringList();
}

QVariantMap ACEventBridge::eventInfo(const QString& eventName) const
{
    QVariantMap info;
    auto* ev = ACEventDefs::instance().getEvent(eventName);
    if (!ev) return info;
    info["name"] = ev->name;
    info["path"] = ev->path;
    info["category"] = ev->category;
    info["description"] = ev->description;
    info["is3D"] = ev->is3D;
    info["loops"] = ev->loops;
    info["defaultVolume"] = static_cast<double>(ev->defaultVolume);
    QVariantList params;
    for (const auto& p : ev->parameters)
        params.append(p);
    info["parameters"] = params;
    return info;
}

QStringList ACEventBridge::parameterNames() const
{
    return ACEventDefs::instance().parameterNames();
}

double ACEventBridge::parameterMin(const QString& paramName) const
{
    auto* p = ACEventDefs::instance().getParameter(paramName);
    return p ? static_cast<double>(p->min) : 0.0;
}

double ACEventBridge::parameterMax(const QString& paramName) const
{
    auto* p = ACEventDefs::instance().getParameter(paramName);
    return p ? static_cast<double>(p->max) : 0.0;
}

double ACEventBridge::parameterDefault(const QString& paramName) const
{
    auto* p = ACEventDefs::instance().getParameter(paramName);
    return p ? static_cast<double>(p->defaultValue) : 0.0;
}

QString ACEventBridge::parameterDescription(const QString& paramName) const
{
    auto* p = ACEventDefs::instance().getParameter(paramName);
    return p ? p->description : QString();
}

QVariantMap ACEventBridge::parameterInfo(const QString& paramName) const
{
    QVariantMap info;
    auto* p = ACEventDefs::instance().getParameter(paramName);
    if (!p) return info;
    info["name"] = p->name;
    info["min"] = static_cast<double>(p->min);
    info["max"] = static_cast<double>(p->max);
    info["defaultValue"] = static_cast<double>(p->defaultValue);
    info["description"] = p->description;
    return info;
}

QVariantList ACEventBridge::allEvents() const
{
    QVariantList list;
    const auto& defs = ACEventDefs::instance();
    for (const auto& name : defs.eventNames()) {
        list.append(eventInfo(name));
    }
    return list;
}

} // namespace audio
} // namespace ks
