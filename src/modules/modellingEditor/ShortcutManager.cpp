#include "ShortcutManager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDebug>
#include <functional>

namespace ks {

ShortcutManager::ShortcutManager(QObject* parent)
	: QObject(parent)
{
	loadDefaults();
}

bool ShortcutManager::loadFromFile(const QString& filePath) {
	m_configFilePath = filePath;
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Failed to open shortcuts file:" << filePath;
		return false;
	}

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	file.close();

	if (!doc.isObject()) {
		qWarning() << "Invalid shortcuts JSON format";
		return false;
	}

	QJsonObject root = doc.object();
	QJsonObject shortcuts = root.value("shortcuts").toObject();

	for (const auto& key : shortcuts.keys()) {
		QJsonObject shortcutObj = shortcuts.value(key).toObject();

		Shortcut sc;
		sc.id = key;
		sc.key = shortcutObj.value("key").toString();
		sc.description = shortcutObj.value("description").toString();
		sc.keySequence = QKeySequence(sc.key);

		m_shortcuts[key] = sc;
	}

	emit shortcutsLoaded();
	return true;
}

bool ShortcutManager::saveToFile(const QString& filePath) {
	QJsonObject shortcutsObj;

	for (const auto& sc : m_shortcuts.values()) {
		QJsonObject scObj;
		scObj.insert("key", sc.key);
		scObj.insert("description", sc.description);
		shortcutsObj.insert(sc.id, scObj);
	}

	QJsonObject root;
	root.insert("shortcuts", shortcutsObj);

	QJsonDocument doc(root);
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly)) {
		qWarning() << "Failed to open shortcuts file for writing:" << filePath;
		return false;
	}

	file.write(doc.toJson());
	file.close();
	return true;
}

ShortcutManager::Shortcut ShortcutManager::getShortcut(const QString& id) const {
	if (m_shortcuts.contains(id)) {
		return m_shortcuts.value(id);
	}
	return Shortcut();
}

void ShortcutManager::remapShortcut(const QString& id, const QString& newKeySequence) {
	if (m_shortcuts.contains(id)) {
		Shortcut& sc = m_shortcuts[id];
		sc.key = newKeySequence;
		sc.keySequence = QKeySequence(newKeySequence);
		emit shortcutChanged(id, newKeySequence);
	}
}

QVector<ShortcutManager::Shortcut> ShortcutManager::allShortcuts() const {
	QVector<Shortcut> result;
	for (const auto& sc : m_shortcuts.values()) {
		result.append(sc);
	}
	return result;
}

void ShortcutManager::registerCallback(const QString& id, ShortcutCallback callback) {
	m_callbacks[id] = callback;
}

void ShortcutManager::triggerShortcut(const QString& id) {
	if (m_callbacks.contains(id)) {
		m_callbacks[id]();
		emit shortcutTriggered(id);
	}
}

QString ShortcutManager::getShortcutDisplay(const QString& id) const {
	if (m_shortcuts.contains(id)) {
		return m_shortcuts.value(id).key;
	}
	return QString();
}

void ShortcutManager::resetToDefaults() {
	m_shortcuts.clear();
	loadDefaults();
}

bool ShortcutManager::isShortcutAvailable(const QString& id) const {
	return m_shortcuts.contains(id);
}

void ShortcutManager::loadDefaults() {
	// Default shortcuts
	m_shortcuts.insert("undo", {"undo", "Ctrl+Z", "Undo last operation", QKeySequence("Ctrl+Z")});
	m_shortcuts.insert("redo", {"redo", "Ctrl+Shift+Z", "Redo last operation", QKeySequence("Ctrl+Shift+Z")});

	m_shortcuts.insert("select", {"select", "Q", "Select tool", QKeySequence("Q")});
	m_shortcuts.insert("move", {"move", "W", "Move/Translate tool", QKeySequence("W")});
	m_shortcuts.insert("rotate", {"rotate", "E", "Rotate tool", QKeySequence("E")});
	m_shortcuts.insert("scale", {"scale", "R", "Scale tool", QKeySequence("R")});

	m_shortcuts.insert("delete", {"delete", "X", "Delete selected", QKeySequence("X")});
	m_shortcuts.insert("duplicate", {"duplicate", "Shift+D", "Duplicate selected", QKeySequence("Shift+D")});

	m_shortcuts.insert("toggle_grid", {"toggle_grid", "G", "Toggle grid", QKeySequence("G")});
	m_shortcuts.insert("toggle_wireframe", {"toggle_wireframe", "Z", "Toggle wireframe", QKeySequence("Z")});
	m_shortcuts.insert("toggle_proportional", {"toggle_proportional", "O", "Toggle proportional editing", QKeySequence("O")});

	m_shortcuts.insert("focus_selected", {"focus_selected", ".", "Focus on selected", QKeySequence(".")});
	m_shortcuts.insert("frame_all", {"frame_all", "Home", "Frame all", QKeySequence("Home")});
}

} // namespace ks
