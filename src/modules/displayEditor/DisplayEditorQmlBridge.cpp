#include "DisplayEditorQmlBridge.h"
#include "DisplayEditor.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QUuid>

namespace ks {

DisplayEditorQmlBridge* DisplayEditorQmlBridge::s_instance = nullptr;

DisplayEditorQmlBridge* DisplayEditorQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new DisplayEditorQmlBridge();
    }
    return s_instance;
}

bool DisplayEditorQmlBridge::loadFromFile(const QString& path) {
    m_editor = std::make_unique<ksDisplayEditor>();

    connect(m_editor.get(), &ksDisplayEditor::elementAdded, this, [this](const QString& id) {
        m_elementCount = m_editor->getAllElements().size();
        emit elementCountChanged();
        emit elementAdded(id);
    });
    connect(m_editor.get(), &ksDisplayEditor::elementRemoved, this, [this](const QString& id) {
        m_elementCount = m_editor->getAllElements().size();
        emit elementCountChanged();
        emit elementRemoved(id);
    });
    connect(m_editor.get(), &ksDisplayEditor::displayChanged, this, [this]() {
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

bool DisplayEditorQmlBridge::saveToFile(const QString& path) {
    if (!m_editor) return false;

    bool success = m_editor->saveToFile(path);
    if (success) {
        m_currentFile = path;
        emit currentFileChanged();
    }
    return success;
}

bool DisplayEditorQmlBridge::loadFromLua(const QString& path) {
    if (!m_editor) m_editor = std::make_unique<ksDisplayEditor>();

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

bool DisplayEditorQmlBridge::saveToLua(const QString& path) {
    if (!m_editor) return false;

    bool success = m_editor->saveToLua(path);
    if (success) {
        m_currentFile = path;
        emit currentFileChanged();
    }
    return success;
}

QVariantList DisplayEditorQmlBridge::getElements() {
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

QVariantMap DisplayEditorQmlBridge::getElement(const QString& id) {
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

void DisplayEditorQmlBridge::addElement(const QVariantMap& element) {
    if (!m_editor) m_editor = std::make_unique<ksDisplayEditor>();

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

void DisplayEditorQmlBridge::removeElement(const QString& id) {
    if (!m_editor) return;

    m_editor->removeElement(id);
    m_elementCount = m_editor->getAllElements().size();
    emit elementCountChanged();
    emit elementRemoved(id);
}

void DisplayEditorQmlBridge::updateElement(const QString& id, const QVariantMap& data) {
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

void DisplayEditorQmlBridge::clearElements() {
    if (!m_editor) m_editor = std::make_unique<ksDisplayEditor>();

    m_editor->clearElements();
    m_elementCount = 0;
    emit elementCountChanged();
}

void DisplayEditorQmlBridge::setDisplayName(const QString& name) {
    m_displayName = name;
    emit displayNameChanged();
}

void DisplayEditorQmlBridge::setDisplaySize(int width, int height) {
    if (m_editor) {
        m_editor->setDisplaySize(QSize(width, height));
    }
}

QVariantMap DisplayEditorQmlBridge::getDisplaySettings() {
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

QStringList DisplayEditorQmlBridge::getAvailableDataSources() {
    return {"speed", "rpm", "gear", "lap_time", "best_lap", "last_lap", "delta",
            "fuel", "tire_temp", "water_temp", "oil_temp", "turbo_boost",
            "speed_kmh", "speed_mpg", "current_lap", "lap_count", "position",
            "brake_bias", "tire_pressure", "steering_angle", "suspension_travel"};
}

QStringList DisplayEditorQmlBridge::getAvailableElementTypes() {
    return {"text", "image", "bar", "circle", "digit_group", "animated_text", "progress_ring"};
}

void DisplayEditorQmlBridge::setBackgroundImage(const QString& path) {
    if (m_editor) {
        m_editor->setBackgroundImage(path);
    }
}

QString DisplayEditorQmlBridge::exportAsImage(const QString& path) {
    if (!m_editor) return "No display loaded";
    
    bool success = m_editor->exportAsImage(path);
    return success ? "" : "Failed to export image";
}

QVariantList DisplayEditorQmlBridge::getElementTemplates() {
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

bool DisplayEditorQmlBridge::addElementFromTemplate(const QString& templateName) {
    QVariantList templates = getElementTemplates();
    for (const auto& t : templates) {
        QVariantMap tmpl = t.toMap();
        if (tmpl["name"].toString() == templateName) {
            if (!m_editor) m_editor = std::make_unique<ksDisplayEditor>();

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

void DisplayEditorQmlBridge::updatePhysicsValue(const QString& source, double value) {
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

} // namespace ks
