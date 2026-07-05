#pragma once

#include <QString>
#include <QMap>
#include <QObject>
#include <QKeySequence>
#include <QJsonObject>
#include <QVector>
#include <functional>

namespace ks {

/**
 * @brief Gestore di scorciatoie da tastiera (keyboard shortcuts)
 * Carica da JSON, consente remapping, supporta callbacks
 */
class ShortcutManager : public QObject {
	Q_OBJECT

public:
	struct Shortcut {
		QString id;
		QString key;
		QString description;
		QKeySequence keySequence;
	};

	explicit ShortcutManager(QObject* parent = nullptr);

	/**
	 * @brief Carica shortcuts da file JSON
	 */
	bool loadFromFile(const QString& filePath);

	/**
	 * @brief Salva shortcuts attuali in file JSON
	 */
	bool saveToFile(const QString& filePath);

	/**
	 * @brief Ottieni shortcut per ID
	 */
	Shortcut getShortcut(const QString& id) const;

	/**
	 * @brief Remappa un shortcut
	 */
	void remapShortcut(const QString& id, const QString& newKeySequence);

	/**
	 * @brief Ottieni lista di tutti gli shortcuts
	 */
	QVector<Shortcut> allShortcuts() const;

	/**
	 * @brief Registra una callback per uno shortcut
	 */
	using ShortcutCallback = std::function<void()>;
	void registerCallback(const QString& id, ShortcutCallback callback);

	/**
	 * @brief Trigger uno shortcut (per testing)
	 */
	void triggerShortcut(const QString& id);

	/**
	 * @brief Ottieni descrizione per display
	 */
	QString getShortcutDisplay(const QString& id) const;

	/**
	 * @brief Ripristina shortcuts di default
	 */
	void resetToDefaults();

	/**
	 * @brief Controlla se è disponibile
	 */
	bool isShortcutAvailable(const QString& id) const;

signals:
	/**
	 * @brief Emesso quando uno shortcut viene remappato
	 */
	void shortcutChanged(const QString& id, const QString& newKey);

	/**
	 * @brief Emesso quando shortcuts vengono caricati
	 */
	void shortcutsLoaded();

	/**
	 * @brief Emesso quando uno shortcut viene eseguito
	 */
	void shortcutTriggered(const QString& id);

private:
	QMap<QString, Shortcut> m_shortcuts;
	QMap<QString, ShortcutCallback> m_callbacks;
	QString m_configFilePath;

	void loadDefaults();
};

} // namespace ks
