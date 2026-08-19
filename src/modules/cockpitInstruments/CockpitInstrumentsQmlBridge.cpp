#include "CockpitInstrumentsQmlBridge.h"
#include "CockpitInstruments.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QUuid>

namespace ks {

CockpitInstrumentsQmlBridge* CockpitInstrumentsQmlBridge::s_instance = nullptr;

CockpitInstrumentsQmlBridge* CockpitInstrumentsQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new CockpitInstrumentsQmlBridge();
    }
    return s_instance;
}

bool CockpitInstrumentsQmlBridge::loadFromFile(const QString& path) {
    m_editor = std::make_unique<ksCockpitInstruments>();

    connect(m_editor.get(), &ksCockpitInstruments::elementAdded, this, [this](const QString& id) {
        m_elementCount = m_editor->getAllElements().size();
        emit elementCountChanged();
        emit elementAdded(id);
    });
    connect(m_editor.get(), &ksCockpitInstruments::elementRemoved, this, [this](const QString& id) {
        m_elementCount = m_editor->getAllElements().size();
        emit elementCountChanged();
        emit elementRemoved(id);
    });
    connect(m_editor.get(), &ksCockpitInstruments::displayChanged, this, [this]() {
        m_elementCount = m_editor->getAllElements().size();
        emit elementCountChanged();
    });

    bool success = m_editor->loadFromFile(path);
    if (success) {
        m_currentFile = path;
        m_displayName = QFileInfo(path).baseName();
        m_elementCount = m_editor->getAllElements().size();
        emit currentFileChanged();
        emit displayNameChanged();
        emit elementCountChanged();
    }
    return success;
}

bool CockpitInstrumentsQmlBridge::saveToFile(const QString& path) {
    if (!m_editor) return false;

    bool success = m_editor->saveToFile(path);
    if (success) {
        m_currentFile = path;
        emit currentFileChanged();
    }
    return success;
}

bool CockpitInstrumentsQmlBridge::loadFromLua(const QString& path) {
    if (!m_editor) m_editor = std::make_unique<ksCockpitInstruments>();

    bool success = m_editor->loadFromLua(path);
    if (success) {
        m_currentFile = path;
        m_displayName = QFileInfo(path).baseName();
        m_elementCount = m_editor->getAllElements().size();
        emit currentFileChanged();
        emit displayNameChanged();
        emit elementCountChanged();
    }
    return success;
}

bool CockpitInstrumentsQmlBridge::saveToLua(const QString& path) {
    if (!m_editor) return false;

    bool success = m_editor->saveToLua(path);
    if (success) {
        m_currentFile = path;
        emit currentFileChanged();
    }
    return success;
}

QVariantList CockpitInstrumentsQmlBridge::getElements() {
    if (!m_editor) return QVariantList();

    QVariantList result;
    const auto& elements = m_editor->getAllElements();

    for (const auto& elem : elements) {
        QVariantMap m;
        m["id"] = elem.id;
        m["type"] = static_cast<int>(elem.type);
        m["source"] = static_cast<int>(elem.source);
        m["positionX"] = elem.position.x();
        m["positionY"] = elem.position.y();
        m["width"] = elem.size.width();
        m["height"] = elem.size.height();
        m["color"] = elem.color.name();
        m["backgroundColor"] = elem.backgroundColor.name();
        m["fontSize"] = elem.fontSize;
        m["decimalPlaces"] = elem.decimalPlaces;
        m["fontFamily"] = elem.fontFamily;
        m["visible"] = elem.visible;
        m["updateInterval"] = elem.updateInterval;
        m["imagePath"] = elem.imagePath;
        m["minValue"] = elem.minValue;
        m["maxValue"] = elem.maxValue;
        result.append(m);
    }
    return result;
}

QVariantMap CockpitInstrumentsQmlBridge::getElement(const QString& id) {
    if (!m_editor) return QVariantMap();

    DisplayElement* elem = m_editor->getElement(id);
    if (!elem) return QVariantMap();

    QVariantMap m;
    m["id"] = elem->id;
    m["type"] = static_cast<int>(elem->type);
    m["source"] = static_cast<int>(elem->source);
    m["positionX"] = elem->position.x();
    m["positionY"] = elem->position.y();
    m["width"] = elem->size.width();
    m["height"] = elem->size.height();
    m["color"] = elem->color.name();
    m["backgroundColor"] = elem->backgroundColor.name();
    m["fontSize"] = elem->fontSize;
    m["decimalPlaces"] = elem->decimalPlaces;
    m["fontFamily"] = elem->fontFamily;
    m["visible"] = elem->visible;
    m["updateInterval"] = elem->updateInterval;
    m["imagePath"] = elem->imagePath;
    m["minValue"] = elem->minValue;
    m["maxValue"] = elem->maxValue;
    return m;
}

void CockpitInstrumentsQmlBridge::addElement(const QVariantMap& element) {
    if (!m_editor) m_editor = std::make_unique<ksCockpitInstruments>();

    DisplayElement elem;
    elem.id = element.value("id", "").toString();
    elem.type = static_cast<ElementType>(element.value("type", 0).toInt());
    elem.source = static_cast<DataSource>(element.value("source", 0).toInt());
    elem.position.setX(element.value("positionX", 0).toInt());
    elem.position.setY(element.value("positionY", 0).toInt());
    elem.size.setWidth(element.value("width", 100).toInt());
    elem.size.setHeight(element.value("height", 30).toInt());
    elem.color = QColor(element.value("color", "#FFFFFF").toString());
    elem.backgroundColor = QColor(element.value("backgroundColor", "#00000000").toString());
    elem.fontSize = element.value("fontSize", 24).toInt();
    elem.decimalPlaces = element.value("decimalPlaces", 0).toInt();
    elem.fontFamily = element.value("fontFamily", "Arial").toString();
    elem.visible = element.value("visible", true).toBool();
    elem.updateInterval = element.value("updateInterval", 100).toInt();
    elem.imagePath = element.value("imagePath", "").toString();
    elem.minValue = element.value("minValue", 0).toInt();
    elem.maxValue = element.value("maxValue", 100).toInt();

    m_editor->addElement(elem);
    m_elementCount = m_editor->getAllElements().size();
    emit elementCountChanged();
}

void CockpitInstrumentsQmlBridge::removeElement(const QString& id) {
    if (!m_editor) return;

    m_editor->removeElement(id);
    m_elementCount = m_editor->getAllElements().size();
    emit elementCountChanged();
    emit elementRemoved(id);
}

void CockpitInstrumentsQmlBridge::updateElement(const QString& id, const QVariantMap& data) {
    if (!m_editor) return;

    DisplayElement* elem = m_editor->getElement(id);
    if (!elem) return;

    if (data.contains("positionX")) elem->position.setX(data["positionX"].toInt());
    if (data.contains("positionY")) elem->position.setY(data["positionY"].toInt());
    if (data.contains("width")) elem->size.setWidth(data["width"].toInt());
    if (data.contains("height")) elem->size.setHeight(data["height"].toInt());
    if (data.contains("color")) elem->color = QColor(data["color"].toString());
    if (data.contains("fontSize")) elem->fontSize = data["fontSize"].toInt();
    if (data.contains("visible")) elem->visible = data["visible"].toBool();
    if (data.contains("fontFamily")) elem->fontFamily = data["fontFamily"].toString();

    emit elementModified(id);
}

void CockpitInstrumentsQmlBridge::clearElements() {
    if (!m_editor) m_editor = std::make_unique<ksCockpitInstruments>();

    m_editor->clearElements();
    m_elementCount = 0;
    emit elementCountChanged();
}

void CockpitInstrumentsQmlBridge::setDisplayName(const QString& name) {
    m_displayName = name;
    emit displayNameChanged();
}

void CockpitInstrumentsQmlBridge::setDisplaySize(int width, int height) {
    if (m_editor) {
        m_editor->setDisplaySize(QSize(width, height));
    }
}

QVariantMap CockpitInstrumentsQmlBridge::getDisplaySettings() {
    QVariantMap settings;
    settings["name"] = m_displayName;
    if (m_editor) {
        QSize size = m_editor->getDisplaySize();
        settings["width"] = size.width();
        settings["height"] = size.height();
    } else {
        settings["width"] = 800;
        settings["height"] = 480;
    }
    settings["elementCount"] = m_elementCount;
    return settings;
}

QStringList CockpitInstrumentsQmlBridge::getAvailableDataSources() {
    return {"speed", "rpm", "gear", "lap_time", "best_lap", "last_lap", "delta",
            "fuel", "tire_temp", "water_temp", "oil_temp", "turbo_boost",
            "speed_kmh", "speed_mpg", "current_lap", "lap_count", "position",
            "brake_bias", "tire_pressure", "steering_angle", "suspension_travel"};
}

QStringList CockpitInstrumentsQmlBridge::getAvailableElementTypes() {
    return {"text", "image", "bar", "circle", "digit_group", "animated_text", "progress_ring"};
}

void CockpitInstrumentsQmlBridge::setBackgroundImage(const QString& path) {
    if (m_editor) {
        m_editor->setBackgroundImage(path);
    }
}

QString CockpitInstrumentsQmlBridge::exportAsImage(const QString& path) {
    if (!m_editor) return "No display loaded";
    
    bool success = m_editor->exportAsImage(path);
    return success ? "" : "Failed to export image";
}

QVariantList CockpitInstrumentsQmlBridge::getElementTemplates() {
    QVariantList templates;

    auto addTemplate = [&](const QString& name, ElementType type, DataSource src,
                           int x, int y, int w, int h, const QColor& c, int fontSize) {
        QVariantMap t;
        t["name"] = name;
        t["type"] = static_cast<int>(type);
        t["source"] = static_cast<int>(src);
        t["x"] = x; t["y"] = y;
        t["width"] = w; t["height"] = h;
        t["color"] = c.name();
        t["fontSize"] = fontSize;
        templates.append(t);
    };

    addTemplate("Speed (large)",   ElementType::TEXT,       DataSource::SPEED,         40, 20,  160, 60,  QColor("#E10600"), 52);
    addTemplate("RPM bar",         ElementType::BAR,        DataSource::RPM,           40, 100, 300, 20,  QColor("#22c55e"), 10);
    addTemplate("Gear indicator",  ElementType::DIGIT_GROUP, DataSource::GEAR,          20, 20,  80,  60,  QColor("#ffffff"), 48);
    addTemplate("Lap time",        ElementType::TEXT,       DataSource::LAP_TIME,      300, 20, 200, 40,  QColor("#ffffff"), 28);
    addTemplate("Fuel bar",        ElementType::BAR,        DataSource::FUEL,          40, 140, 200, 16,  QColor("#f59e0b"), 10);
    addTemplate("Best lap",        ElementType::TEXT,       DataSource::BEST_LAP,      300, 60, 200, 30,  QColor("#888888"), 20);
    addTemplate("RPM progress",    ElementType::PROGRESS_RING, DataSource::RPM,        600, 20, 100, 100, QColor("#E10600"), 12);
    addTemplate("Boost gauge",     ElementType::BAR,        DataSource::TURBO_BOOST,   40, 170, 300, 12,  QColor("#a855f7"), 10);
    addTemplate("Speed MPH",       ElementType::TEXT,       DataSource::SPEED_MPH,     40, 20,  160, 60,  QColor("#ffffff"), 52);
    addTemplate("Delta",           ElementType::TEXT,       DataSource::DELTA,         300, 100, 200, 30,  QColor("#f87171"), 22);
    addTemplate("Steering angle",  ElementType::CIRCLE,     DataSource::STEERING_ANGLE, 700, 140, 60, 60, QColor("#60a5fa"), 10);
    addTemplate("Tire temp",       ElementType::TEXT,       DataSource::TIRE_TEMP,     40, 200, 160, 24,  QColor("#fb923c"), 16);

    return templates;
}

bool CockpitInstrumentsQmlBridge::addElementFromTemplate(const QString& templateName) {
    QVariantList templates = getElementTemplates();
    for (const auto& t : templates) {
        QVariantMap tmpl = t.toMap();
        if (tmpl["name"].toString() == templateName) {
            if (!m_editor) m_editor = std::make_unique<ksCockpitInstruments>();

            DisplayElement elem;
            elem.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            elem.type = static_cast<ElementType>(tmpl["type"].toInt());
            elem.source = static_cast<DataSource>(tmpl["source"].toInt());
            elem.position = QPoint(tmpl["x"].toInt(), tmpl["y"].toInt());
            elem.size = QSize(tmpl["width"].toInt(), tmpl["height"].toInt());
            elem.color = QColor(tmpl["color"].toString());
            elem.fontSize = tmpl["fontSize"].toInt();
            if (elem.type == ElementType::BAR) {
                elem.minValue = 0;
                elem.maxValue = (elem.source == DataSource::RPM) ? 8000 : 100;
            }
            if (elem.type == ElementType::PROGRESS_RING) {
                elem.minValue = 0;
                elem.maxValue = (elem.source == DataSource::RPM) ? 8000 : 100;
            }

            m_editor->addElement(elem);
            m_elementCount = m_editor->getAllElements().size();
            emit elementCountChanged();
            emit elementAdded(elem.id);
            return true;
        }
    }
    return false;
}

void CockpitInstrumentsQmlBridge::updatePhysicsValue(const QString& source, double value) {
    if (!m_editor) return;

    static const QMap<QString, DataSource> srcMap = {
        {"speed", DataSource::SPEED}, {"rpm", DataSource::RPM},
        {"gear", DataSource::GEAR}, {"lap_time", DataSource::LAP_TIME},
        {"best_lap", DataSource::BEST_LAP}, {"fuel", DataSource::FUEL},
        {"tire_temp", DataSource::TIRE_TEMP}, {"turbo_boost", DataSource::TURBO_BOOST},
        {"delta", DataSource::DELTA}, {"steering_angle", DataSource::STEERING_ANGLE},
        {"brake_bias", DataSource::BRAKE_BIAS},
        {"tire_pressure", DataSource::TIRE_PRESSURE},
        {"suspension_travel", DataSource::SUSPENSION_TRAVEL},
    };

    auto it = srcMap.find(source);
    if (it != srcMap.end()) {
        m_editor->updatePhysicsValue(it.value(), value);
    }
}

// ============================================================================
// AC CSP format accessors
// ============================================================================

QVariantList CockpitInstrumentsQmlBridge::getAcItems() {
    if (!m_editor) return QVariantList();
    QVariantList result;
    for (const auto& item : m_editor->getAcItems()) {
        QVariantMap m;
        m["index"] = item.index;
        m["type"] = static_cast<int>(item.type);
        m["typeStr"] = m_editor->acItemTypeToString(item.type);
        m["parent"] = item.parent == AcDisplayParent::DISPLAY_WHEEL ? "DISPLAY_WHEEL" : "DISPLAY_MONITOR";
        m["objectName"] = item.objectName;
        m["positionX"] = item.position.x();
        m["positionY"] = item.position.y();
        m["width"] = item.size.width();
        m["height"] = item.size.height();
        m["color"] = item.color.name();
        m["colorR"] = item.color.red();
        m["colorG"] = item.color.green();
        m["colorB"] = item.color.blue();
        m["intensity"] = item.intensity;
        m["font"] = item.font;
        m["version"] = item.version;
        m["align"] = item.align;
        m["decimals"] = item.decimals;
        m["prefix"] = item.prefix;
        m["postfix"] = item.postfix;
        m["units"] = item.units;
        m["tyreNumber"] = item.tyreNumber;
        m["nTime"] = item.nTime;
        m["rpmTrigger"] = item.rpmTrigger;
        m["visible"] = item.visible;
        result.append(m);
    }
    return result;
}

QVariantList CockpitInstrumentsQmlBridge::getAcLeds() {
    if (!m_editor) return QVariantList();
    QVariantList result;
    for (const auto& led : m_editor->getAcLeds()) {
        QVariantMap m;
        m["index"] = led.index;
        m["objectName"] = led.objectName;
        m["rpmSwitch"] = led.rpmSwitch;
        m["emissiveR"] = led.emissive.red();
        m["emissiveG"] = led.emissive.green();
        m["emissiveB"] = led.emissive.blue();
        m["diffuse"] = led.diffuse;
        m["blinkSwitch"] = led.blinkSwitch;
        m["blinkHz"] = led.blinkHz;
        m["visible"] = led.visible;
        result.append(m);
    }
    return result;
}

QVariantList CockpitInstrumentsQmlBridge::getAcTyreSlips() {
    if (!m_editor) return QVariantList();
    QVariantList result;
    for (const auto& slip : m_editor->getAcTyreSlips()) {
        QVariantMap m;
        m["index"] = slip.index;
        m["objectName"] = slip.objectName;
        m["tyreIndex"] = slip.tyreIndex;
        m["emissiveR"] = slip.emissive.red();
        m["emissiveG"] = slip.emissive.green();
        m["emissiveB"] = slip.emissive.blue();
        m["emissiveLockR"] = slip.emissiveLock.red();
        m["emissiveLockG"] = slip.emissiveLock.green();
        m["emissiveLockB"] = slip.emissiveLock.blue();
        m["diffuse"] = slip.diffuse;
        m["slipSwitch"] = slip.slipSwitch;
        m["showLock"] = slip.showLock;
        m["wheelSpeedMult"] = slip.wheelSpeedMult;
        m["visible"] = slip.visible;
        result.append(m);
    }
    return result;
}

QVariantList CockpitInstrumentsQmlBridge::getAcKersSeries() {
    if (!m_editor) return QVariantList();
    QVariantList result;
    for (const auto& serie : m_editor->getAcKersSeries()) {
        QVariantMap m;
        m["index"] = serie.index;
        m["prefix"] = serie.prefix;
        m["startIndex"] = serie.startIndex;
        m["endIndex"] = serie.endIndex;
        result.append(m);
    }
    return result;
}

void CockpitInstrumentsQmlBridge::addAcItem(const QVariantMap& item) {
    if (!m_editor) m_editor = std::make_unique<ksCockpitInstruments>();
    AcDisplayItem acItem;
    acItem.index = m_editor->getAcItems().size();
    acItem.type = static_cast<AcItemType>(item.value("type", 0).toInt());
    acItem.parent = item.value("parent", "DISPLAY_WHEEL").toString() == "DISPLAY_WHEEL"
        ? AcDisplayParent::DISPLAY_WHEEL : AcDisplayParent::DISPLAY_MONITOR;
    acItem.objectName = item.value("objectName", "").toString();
    acItem.position = QPoint(item.value("positionX", 0).toInt(), item.value("positionY", 0).toInt());
    acItem.size = QSize(item.value("width", 100).toInt(), item.value("height", 30).toInt());
    acItem.color = QColor(item.value("color", "#ffffff").toString());
    acItem.intensity = item.value("intensity", 1).toInt();
    acItem.font = item.value("font", "german_led").toString();
    acItem.version = item.value("version", 2).toInt();
    acItem.align = item.value("align", "CENTER").toString();
    acItem.decimals = item.value("decimals", 0).toInt();
    acItem.prefix = item.value("prefix", "").toString();
    acItem.postfix = item.value("postfix", "").toString();
    acItem.units = item.value("units", "").toString();
    acItem.tyreNumber = item.value("tyreNumber", -1).toInt();
    acItem.nTime = item.value("nTime", 0.1).toDouble();
    acItem.rpmTrigger = item.value("rpmTrigger", 0).toInt();
    m_editor->addAcItem(acItem);
}

void CockpitInstrumentsQmlBridge::addAcLed(const QVariantMap& led) {
    if (!m_editor) m_editor = std::make_unique<ksCockpitInstruments>();
    AcLed acLed;
    acLed.index = m_editor->getAcLeds().size();
    acLed.objectName = led.value("objectName", "").toString();
    acLed.rpmSwitch = led.value("rpmSwitch", 0).toInt();
    acLed.emissive = QColor(led.value("emissiveR", 255).toInt(), led.value("emissiveG", 0).toInt(), led.value("emissiveB", 0).toInt());
    acLed.diffuse = led.value("diffuse", 0.2).toDouble();
    acLed.blinkSwitch = led.value("blinkSwitch", 0).toInt();
    acLed.blinkHz = led.value("blinkHz", 5.0).toDouble();
    m_editor->addAcLed(acLed);
}

void CockpitInstrumentsQmlBridge::addAcTyreSlip(const QVariantMap& slip) {
    if (!m_editor) m_editor = std::make_unique<ksCockpitInstruments>();
    AcTyreLockSlip acSlip;
    acSlip.index = m_editor->getAcTyreSlips().size();
    acSlip.objectName = slip.value("objectName", "").toString();
    acSlip.tyreIndex = slip.value("tyreIndex", 0).toInt();
    acSlip.emissive = QColor(slip.value("emissiveR", 0).toInt(), slip.value("emissiveG", 0).toInt(), slip.value("emissiveB", 160).toInt());
    acSlip.emissiveLock = QColor(slip.value("emissiveLockR", 0).toInt(), slip.value("emissiveLockG", 0).toInt(), slip.value("emissiveLockB", 0).toInt());
    acSlip.diffuse = slip.value("diffuse", 0.25).toDouble();
    acSlip.slipSwitch = slip.value("slipSwitch", 0.6).toDouble();
    acSlip.showLock = slip.value("showLock", false).toBool();
    acSlip.wheelSpeedMult = slip.value("wheelSpeedMult", 0.8).toDouble();
    m_editor->addAcTyreSlip(acSlip);
}

void CockpitInstrumentsQmlBridge::removeAcItem(int index) {
    if (m_editor) m_editor->removeAcItem(index);
}

void CockpitInstrumentsQmlBridge::removeAcLed(int index) {
    if (m_editor) m_editor->removeAcLed(index);
}

void CockpitInstrumentsQmlBridge::removeAcTyreSlip(int index) {
    if (m_editor) m_editor->removeAcTyreSlip(index);
}

void CockpitInstrumentsQmlBridge::clearAcData() {
    if (m_editor) {
        m_editor->clearAcItems();
        m_editor->clearAcLeds();
        m_editor->clearAcTyreSlips();
    }
}

} // namespace ks
