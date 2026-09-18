#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include <QColor>
#include <QObject>
#include <QJsonObject>

namespace ks {

/**
 * @brief Definizione di un materiale (preset)
 */
struct MaterialPreset {
	QString id;
	QString name;
	QString category;
	QString description;

	// Parametri PBR
	QColor albedo = QColor(200, 200, 200, 255);
	float metallic = 0.0f;
	float roughness = 0.5f;
	float normalStrength = 1.0f;
	QColor emissive = QColor(0, 0, 0);
	float opacity = 1.0f;

	// Metadati
	QString thumbnailPath;
	QString author;
	QString tags;

	QJsonObject toJson() const;
	static MaterialPreset fromJson(const QJsonObject& obj);
};

/**
 * @brief Libreria di materiali/presets
 */
class MaterialPresetLibrary : public QObject {
	Q_OBJECT

public:
	explicit MaterialPresetLibrary(QObject* parent = nullptr);

	/**
	 * @brief Carica presets da cartella
	 */
	bool loadFromDirectory(const QString& dirPath);

	/**
	 * @brief Salva presets in cartella
	 */
	bool saveToDirectory(const QString& dirPath);

	/**
	 * @brief Aggiungi un preset
	 */
	void addPreset(const MaterialPreset& preset);

	/**
	 * @brief Rimuovi un preset
	 */
	bool removePreset(const QString& presetId);

	/**
	 * @brief Ottieni preset per ID
	 */
	MaterialPreset getPreset(const QString& presetId) const;

	/**
	 * @brief Ottieni tutti i presets di una categoria
	 */
	QVector<MaterialPreset> getPresetsByCategory(const QString& category) const;

	/**
	 * @brief Ottieni tutte le categorie
	 */
	QVector<QString> getAllCategories() const;

	/**
	 * @brief Ottieni lista di tutti i presets
	 */
	QVector<MaterialPreset> getAllPresets() const;

	/**
	 * @brief Ricerca presets per tag
	 */
	QVector<MaterialPreset> searchByTags(const QString& tag) const;

	/**
	 * @brief Duplicate un preset con nuovo nome
	 */
	MaterialPreset duplicatePreset(const QString& sourceId, const QString& newName);

	/**
	 * @brief Esporta singolo preset in file
	 */
	bool exportPreset(const QString& presetId, const QString& filePath);

	/**
	 * @brief Importa preset da file
	 */
	bool importPreset(const QString& filePath);

	/**
	 * @brief Esporta collection di presets
	 */
	bool exportCollection(const QVector<QString>& presetIds, const QString& filePath);

	/**
	 * @brief Importa collection di presets
	 */
	bool importCollection(const QString& filePath);

	/**
	 * @brief Conta presets totali
	 */
	int presetCount() const { return m_presets.size(); }

signals:
	/**
	 * @brief Emesso quando un preset viene aggiunto
	 */
	void presetAdded(const QString& presetId);

	/**
	 * @brief Emesso quando un preset viene rimosso
	 */
	void presetRemoved(const QString& presetId);

	/**
	 * @brief Emesso quando presets vengono caricati
	 */
	void presetsLoaded(int count);

	/**
	 * @brief Emesso quando un preset viene modificato
	 */
	void presetModified(const QString& presetId);

private:
	QMap<QString, MaterialPreset> m_presets;
	QString m_libraryPath;

	QString generateUniqueId(const QString& baseName);
};

} // namespace ks
