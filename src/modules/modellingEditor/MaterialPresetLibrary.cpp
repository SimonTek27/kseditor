#include "MaterialPresetLibrary.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QUuid>

namespace ks {

// ============================================================================
// MaterialPreset Implementation
// ============================================================================

QJsonObject MaterialPreset::toJson() const {
	QJsonObject obj;
	obj["id"] = id;
	obj["name"] = name;
	obj["category"] = category;
	obj["description"] = description;

	obj["albedo_r"] = albedo.red();
	obj["albedo_g"] = albedo.green();
	obj["albedo_b"] = albedo.blue();
	obj["albedo_a"] = albedo.alpha();

	obj["metallic"] = metallic;
	obj["roughness"] = roughness;
	obj["normalStrength"] = normalStrength;

	obj["emissive_r"] = emissive.red();
	obj["emissive_g"] = emissive.green();
	obj["emissive_b"] = emissive.blue();

	obj["opacity"] = opacity;
	obj["thumbnailPath"] = thumbnailPath;
	obj["author"] = author;
	obj["tags"] = tags;

	return obj;
}

MaterialPreset MaterialPreset::fromJson(const QJsonObject& obj) {
	MaterialPreset preset;
	preset.id = obj["id"].toString();
	preset.name = obj["name"].toString();
	preset.category = obj["category"].toString();
	preset.description = obj["description"].toString();

	preset.albedo = QColor(
		obj["albedo_r"].toInt(200),
		obj["albedo_g"].toInt(200),
		obj["albedo_b"].toInt(200),
		obj["albedo_a"].toInt(255)
	);

	preset.metallic = obj["metallic"].toDouble(0.0);
	preset.roughness = obj["roughness"].toDouble(0.5);
	preset.normalStrength = obj["normalStrength"].toDouble(1.0);

	preset.emissive = QColor(
		obj["emissive_r"].toInt(0),
		obj["emissive_g"].toInt(0),
		obj["emissive_b"].toInt(0)
	);

	preset.opacity = obj["opacity"].toDouble(1.0);
	preset.thumbnailPath = obj["thumbnailPath"].toString();
	preset.author = obj["author"].toString();
	preset.tags = obj["tags"].toString();

	return preset;
}

// ============================================================================
// MaterialPresetLibrary Implementation
// ============================================================================

MaterialPresetLibrary::MaterialPresetLibrary(QObject* parent)
	: QObject(parent)
{
}

bool MaterialPresetLibrary::loadFromDirectory(const QString& dirPath) {
	m_libraryPath = dirPath;
	QDir dir(dirPath);

	if (!dir.exists()) {
		qWarning() << "Presets directory does not exist:" << dirPath;
		return false;
	}

	QStringList jsonFiles = dir.entryList({"*.json"}, QDir::Files);
	int loadedCount = 0;

	for (const auto& fileName : jsonFiles) {
		QFile file(dir.filePath(fileName));
		if (!file.open(QIODevice::ReadOnly)) {
			qWarning() << "Failed to open preset file:" << fileName;
			continue;
		}

		QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
		file.close();

		if (!doc.isObject()) continue;

		MaterialPreset preset = MaterialPreset::fromJson(doc.object());
		if (!preset.id.isEmpty()) {
			m_presets[preset.id] = preset;
			loadedCount++;
		}
	}

	emit presetsLoaded(loadedCount);
	return loadedCount > 0;
}

bool MaterialPresetLibrary::saveToDirectory(const QString& dirPath) {
	QDir dir(dirPath);
	if (!dir.exists()) {
		if (!dir.mkpath(".")) {
			qWarning() << "Failed to create presets directory:" << dirPath;
			return false;
		}
	}

	for (const auto& preset : m_presets) {
		QString fileName = preset.id + ".json";
		QFile file(dir.filePath(fileName));

		if (!file.open(QIODevice::WriteOnly)) {
			qWarning() << "Failed to save preset:" << fileName;
			continue;
		}

		QJsonDocument doc(preset.toJson());
		file.write(doc.toJson());
		file.close();
	}

	return true;
}

void MaterialPresetLibrary::addPreset(const MaterialPreset& preset) {
	m_presets[preset.id] = preset;
	emit presetAdded(preset.id);
}

bool MaterialPresetLibrary::removePreset(const QString& presetId) {
	if (m_presets.remove(presetId) > 0) {
		emit presetRemoved(presetId);
		return true;
	}
	return false;
}

MaterialPreset MaterialPresetLibrary::getPreset(const QString& presetId) const {
	if (m_presets.contains(presetId)) {
		return m_presets.value(presetId);
	}
	return MaterialPreset();
}

QVector<MaterialPreset> MaterialPresetLibrary::getPresetsByCategory(const QString& category) const {
	QVector<MaterialPreset> result;
	for (const auto& preset : m_presets) {
		if (preset.category == category) {
			result.append(preset);
		}
	}
	return result;
}

QVector<QString> MaterialPresetLibrary::getAllCategories() const {
	QVector<QString> categories;
	for (const auto& preset : m_presets) {
		if (!categories.contains(preset.category)) {
			categories.append(preset.category);
		}
	}
	return categories;
}

QVector<MaterialPreset> MaterialPresetLibrary::getAllPresets() const {
	return m_presets.values().toVector();
}

QVector<MaterialPreset> MaterialPresetLibrary::searchByTags(const QString& tag) const {
	QVector<MaterialPreset> result;
	for (const auto& preset : m_presets) {
		if (preset.tags.contains(tag, Qt::CaseInsensitive)) {
			result.append(preset);
		}
	}
	return result;
}

MaterialPreset MaterialPresetLibrary::duplicatePreset(const QString& sourceId, const QString& newName) {
	MaterialPreset source = getPreset(sourceId);
	if (source.id.isEmpty()) {
		return MaterialPreset();
	}

	MaterialPreset duplicate = source;
	duplicate.id = generateUniqueId(newName);
	duplicate.name = newName;

	addPreset(duplicate);
	return duplicate;
}

bool MaterialPresetLibrary::exportPreset(const QString& presetId, const QString& filePath) {
	MaterialPreset preset = getPreset(presetId);
	if (preset.id.isEmpty()) {
		return false;
	}

	QJsonDocument doc(preset.toJson());
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly)) {
		qWarning() << "Failed to export preset:" << filePath;
		return false;
	}

	file.write(doc.toJson());
	file.close();
	return true;
}

bool MaterialPresetLibrary::importPreset(const QString& filePath) {
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Failed to import preset:" << filePath;
		return false;
	}

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	file.close();

	if (!doc.isObject()) {
		return false;
	}

	MaterialPreset preset = MaterialPreset::fromJson(doc.object());
	if (!preset.id.isEmpty()) {
		preset.id = generateUniqueId(preset.name);
		addPreset(preset);
		return true;
	}

	return false;
}

bool MaterialPresetLibrary::exportCollection(const QVector<QString>& presetIds, const QString& filePath) {
	QJsonArray array;

	for (const auto& id : presetIds) {
		MaterialPreset preset = getPreset(id);
		if (!preset.id.isEmpty()) {
			array.append(preset.toJson());
		}
	}

	QJsonDocument doc(array);
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly)) {
		qWarning() << "Failed to export collection:" << filePath;
		return false;
	}

	file.write(doc.toJson());
	file.close();
	return true;
}

bool MaterialPresetLibrary::importCollection(const QString& filePath) {
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Failed to import collection:" << filePath;
		return false;
	}

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	file.close();

	if (!doc.isArray()) {
		return false;
	}

	QJsonArray array = doc.array();
	int count = 0;

	for (const auto& value : array) {
		if (value.isObject()) {
			MaterialPreset preset = MaterialPreset::fromJson(value.toObject());
			if (!preset.id.isEmpty()) {
				preset.id = generateUniqueId(preset.name);
				addPreset(preset);
				count++;
			}
		}
	}

	return count > 0;
}

QString MaterialPresetLibrary::generateUniqueId(const QString& baseName) {
	QString id = baseName.toLower().replace(" ", "_");
	id += "_" + QUuid::createUuid().toString(QUuid::Id128);
	return id;
}

} // namespace ks
