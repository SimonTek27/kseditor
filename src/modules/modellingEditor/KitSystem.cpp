#include "KitSystem.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace ks {
namespace modelling {

KitSystem::KitSystem(QObject* parent) : QObject(parent) {}

KitSystem::~KitSystem() {}

void KitSystem::addPreset(const PresetData& preset) {
    // Check if preset with same name exists
    for (int i = 0; i < m_presets.size(); ++i) {
        if (m_presets[i].name == preset.name) {
            m_presets[i] = preset;
            emit presetRemoved(preset.name);
            emit presetAdded(preset);
            return;
        }
    }
    
    m_presets.append(preset);
    emit presetAdded(preset);
    emit presetsChanged();
}

bool KitSystem::removePreset(const QString& name) {
    for (int i = 0; i < m_presets.size(); ++i) {
        if (m_presets[i].name == name) {
            m_presets.removeAt(i);
            emit presetRemoved(name);
            emit presetsChanged();
            return true;
        }
    }
    return false;
}

PresetData KitSystem::getPreset(const QString& name) const {
    for (const auto& p : m_presets) {
        if (p.name == name) return p;
    }
    return PresetData{};
}

QVector<PresetData> KitSystem::getPresets(const QString& category) const {
    QVector<PresetData> result;
    for (const auto& p : m_presets) {
        if (category.isEmpty() || p.category == category) {
            result.append(p);
        }
    }
    return result;
}

int KitSystem::presetCount() const {
    return m_presets.size();
}

void KitSystem::addKit(const QString& kitName, const QVector<QString>& presetNames) {
    m_kits[kitName] = presetNames;
    emit kitAdded(kitName);
    emit presetsChanged();
}

QVector<QString> KitSystem::getKitPresets(const QString& kitName) const {
    auto it = m_kits.constFind(kitName);
    if (it != m_kits.constEnd()) return *it;
    return QVector<QString>();
}

QVector<QString> KitSystem::getAllKitNames() const {
    QVector<QString> names;
    for (auto it = m_kits.constBegin(); it != m_kits.constEnd(); ++it) {
        names.append(it.key());
    }
    return names;
}

bool KitSystem::saveToFile(const QString& path) const {
    QJsonObject rootDoc;
    QJsonArray presetsArray;
    for (const auto& p : m_presets) {
        QJsonObject obj;
        obj["name"] = p.name;
        obj["category"] = p.category;
        obj["position"] = QJsonArray{p.position.x(), p.position.y(), p.position.z()};
        obj["rotation"] = QJsonArray{p.rotation.x(), p.rotation.y(), p.rotation.z()};
        obj["scale"] = QJsonArray{p.scale.x(), p.scale.y(), p.scale.z()};
        obj["color"] = QJsonArray{p.color.x(), p.color.y(), p.color.z(), p.color.w()};
        obj["material"] = p.material;
        obj["description"] = p.description;
        
        QJsonObject params;
        for (auto it = p.parameters.constBegin(); it != p.parameters.constEnd(); ++it) {
            params[it.key()] = it.value().toJsonValue();
        }
        obj["parameters"] = params;
        
        presetsArray.append(obj);
    }
    rootDoc["presets"] = presetsArray;
    
    QJsonObject kitsArray;
    for (auto it = m_kits.constBegin(); it != m_kits.constEnd(); ++it) {
        QJsonArray presetNames;
        for (const auto& name : it.value()) {
            presetNames.append(name);
        }
        kitsArray[it.key()] = presetNames;
    }
    rootDoc["kits"] = kitsArray;
    
    QJsonDocument doc(rootDoc);
    QFile file(path);
    if (!file.open(QFile::WriteOnly)) return false;
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool KitSystem::loadFromFile(const QString& path) {
    QFile file(path);
    if (!file.open(QFile::ReadOnly)) return false;
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc(QJsonDocument::fromJson(data));
    if (doc.isNull()) return false;
    
    QJsonObject obj = doc.object();
    
    // Load presets
    m_presets.clear();
    if (obj.contains("presets") && obj["presets"].isArray()) {
        QJsonArray arr = obj["presets"].toArray();
        for (const auto& val : arr) {
            if (val.isObject()) {
                QJsonObject p = val.toObject();
                PresetData preset;
                preset.name = p["name"].toString();
                preset.category = p["category"].toString();
                
                QJsonArray posArr = p["position"].toArray();
                if (posArr.size() >= 3) {
                    preset.position = QVector3D(posArr[0].toDouble(), posArr[1].toDouble(), posArr[2].toDouble());
                }
                
                QJsonArray rotArr = p["rotation"].toArray();
                if (rotArr.size() >= 3) {
                    preset.rotation = QVector3D(rotArr[0].toDouble(), rotArr[1].toDouble(), rotArr[2].toDouble());
                }
                
                QJsonArray scaleArr = p["scale"].toArray();
                if (scaleArr.size() >= 3) {
                    preset.scale = QVector3D(scaleArr[0].toDouble(), scaleArr[1].toDouble(), scaleArr[2].toDouble());
                }
                
                QJsonArray colorArr = p["color"].toArray();
                if (colorArr.size() >= 4) {
                    preset.color = QVector4D(colorArr[0].toDouble(), colorArr[1].toDouble(), colorArr[2].toDouble(), colorArr[3].toDouble());
                }
                
                preset.material = p["material"].toString();
                preset.description = p["description"].toString();
                
                if (p.contains("parameters") && p["parameters"].isObject()) {
                    QJsonObject params = p["parameters"].toObject();
                    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
                        preset.parameters[it.key()] = it.value().toVariant();
                    }
                }
                
                m_presets.append(preset);
            }
        }
    }
    
    // Load kits
    m_kits.clear();
    if (obj.contains("kits") && obj["kits"].isObject()) {
        QJsonObject kits = obj["kits"].toObject();
        for (auto it = kits.constBegin(); it != kits.constEnd(); ++it) {
            QVector<QString> names;
            if (it.value().isArray()) {
                QJsonArray arr = it.value().toArray();
                for (const auto& val : arr) {
                    if (val.isString()) names.append(val.toString());
                }
            }
            m_kits[it.key()] = names;
        }
    }
    
    emit presetsChanged();
    return true;
}

void KitSystem::initializeBuiltInPresets() {
    // Add some default presets for common workflows
    
    // Car body panel preset
    PresetData carPanel;
    carPanel.name = "Car Body Panel";
    carPanel.category = "Automotive";
    carPanel.description = "Panel line and surface preset for car modeling";
    carPanel.color = QVector4D(0.8f, 0.3f, 0.1f, 1.0f);
    carPanel.material = "car_paint";
    addPreset(carPanel);
    
    // Architectural column preset
    PresetData column;
    column.name = "Architectural Column";
    column.category = "Architectural";
    column.description = "Column and pillar modeling preset";
    column.color = QVector4D(0.2f, 0.3f, 0.8f, 1.0f);
    column.material = "stone";
    addPreset(column);
    
    // Character head preset
    PresetData head;
    head.name = "Character Head";
    head.category = "Character";
    head.description = "Head topology and symmetry preset";
    head.color = QVector4D(0.8f, 0.5f, 0.1f, 1.0f);
    head.material = "skin";
    addPreset(head);
    
    // Organic curve preset
    PresetData curve;
    curve.name = "Organic Curve";
    curve.category = "Organic";
    curve.description = "Smooth curve and surface preset";
    curve.color = QVector4D(0.2f, 0.8f, 0.3f, 1.0f);
    addPreset(curve);
    
    // Initialize kits
    addKit("Automotive", {"Car Body Panel"});
    addKit("Architectural", {"Architectural Column"});
    addKit("Character", {"Character Head"});
    addKit("Organic", {"Organic Curve"});
    
    emit presetsChanged();
}

} // namespace modelling
} // namespace ks