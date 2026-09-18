#pragma once

#include <QString>
#include <QObject>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <QTextEdit>

namespace ks {

class ModelerPythonAPI;

/**
 * @brief Console per script output e error logging
 */
class PythonScriptConsole : public QObject {
	Q_OBJECT

public:
	explicit PythonScriptConsole(QObject* parent = nullptr);

	/**
	 * @brief Scrivi messaggio di log
	 */
	void log(const QString& message);

	/**
	 * @brief Scrivi messaggio di errore
	 */
	void logError(const QString& message);

	/**
	 * @brief Scrivi messaggio di warning
	 */
	void logWarning(const QString& message);

	/**
	 * @brief Ripulisci console
	 */
	void clear();

	/**
	 * @brief Ottieni output completo
	 */
	QString getOutput() const { return m_output; }

signals:
	void messageLogged(const QString& message);
	void errorLogged(const QString& error);

private:
	QString m_output;
};

/**
 * @brief Manager per script Python
 */
class PythonScriptManager : public QObject {
	Q_OBJECT

public:
	explicit PythonScriptManager(QObject* parent = nullptr);
	~PythonScriptManager();

	/**
	 * @brief Inizializza Python environment
	 */
	bool initialize();

	/**
	 * @brief Esegui script Python
	 */
	bool executeScript(const QString& scriptCode, bool async = false);

	/**
	 * @brief Esegui script da file
	 */
	bool executeScriptFromFile(const QString& filePath, bool async = false);

	/**
	 * @brief Annulla l'esecuzione dello script
	 */
	void cancelExecution();

	/**
	 * @brief Checks if script is running
	 */
	bool isExecuting() const { return m_isExecuting; }

	/**
	 * @brief Ottieni console
	 */
	PythonScriptConsole* getConsole() const { return m_console; }

	/**
	 * @brief Esporta cronologia script eseguiti
	 */
	QVector<QString> getScriptHistory() const { return m_history; }

	/**
	 * @brief Ottieni ultimo errore
	 */
	QString lastError() const { return m_lastError; }

	/**
	 * @brief Ottieni API Python esposta
	 */
	ModelerPythonAPI* pythonAPI() const { return m_api; }

	/**
	 * @brief Carica script da file di libreria
	 */
	QString loadLibraryScript(const QString& name) const;

	/**
	 * @brief Espone API Modeler a Python
	 */
	void exposePythonAPI();

private:
	bool executeScriptInternal(const QString& scriptCode);
	bool parseAndExecuteLine(const QString& line);
	QVariant evaluateExpression(const QString& expr) const;
	QVariant callAPIFunction(const QString& funcName, const QString& argsStr) const;
	QStringList parseArguments(const QString& argsStr) const;

signals:
	/**
	 * @brief Emesso quando script inizia
	 */
	void scriptExecutionStarted();

	/**
	 * @brief Emesso quando script completa
	 */
	void scriptExecutionCompleted();

	/**
	 * @brief Emesso quando script genera errore
	 */
	void scriptError(const QString& error);

	/**
	 * @brief Emesso durante esecuzione asincrona
	 */
	void scriptOutput(const QString& output);

private:
	PythonScriptConsole* m_console = nullptr;
	ModelerPythonAPI* m_api = nullptr;
	bool m_isExecuting = false;
	QVector<QString> m_history;
	QString m_lastScriptCode;
	QString m_lastError;
	QMap<QString, QVariant> m_variables;
};

/**
 * @brief API Python exposed a KS Modeler
 * Wraps KSModelerQml per uso da Python
 */
class ModelerPythonAPI : public QObject {
	Q_OBJECT

public:
	explicit ModelerPythonAPI(QObject* parent = nullptr);

	// Selection API
	Q_INVOKABLE QVariant getSelection() const;
	Q_INVOKABLE bool setSelection(int objectId);
	Q_INVOKABLE void clearSelection();

	// Objects API
	Q_INVOKABLE QVariantList getObjects() const;
	Q_INVOKABLE QVariant getObject(int objectId) const;
	Q_INVOKABLE int createObject(const QString& name, const QString& type);
	Q_INVOKABLE bool deleteObject(int objectId);
	Q_INVOKABLE bool duplicateObject(int objectId);

	// Transform API
	Q_INVOKABLE bool translate(int objectId, float x, float y, float z);
	Q_INVOKABLE bool rotate(int objectId, float x, float y, float z);
	Q_INVOKABLE bool scale(int objectId, float x, float y, float z);

	// Mesh operations
	Q_INVOKABLE bool extrude(const QVariantList& faceIndices, float distance);
	Q_INVOKABLE bool inset(const QVariantList& faceIndices, float amount);
	Q_INVOKABLE bool bevel(const QVariantList& edgeIndices, float amount, int segments);
	Q_INVOKABLE bool subdivide(const QVariantList& faceIndices, int cuts);

	// Material API
	Q_INVOKABLE bool setMaterialColor(int objectId, float r, float g, float b, float a);
	Q_INVOKABLE bool setMaterialMetallic(int objectId, float value);
	Q_INVOKABLE bool setMaterialRoughness(int objectId, float value);
	Q_INVOKABLE QVariantMap getMaterial(int objectId) const;

	// Export API
	Q_INVOKABLE bool exportKN5(const QString& path);
	Q_INVOKABLE bool exportFBX(const QString& path);
	Q_INVOKABLE bool exportOBJ(const QString& path);

	// Utility
	Q_INVOKABLE int getObjectCount() const;
	Q_INVOKABLE bool undo();
	Q_INVOKABLE bool redo();

	void setModeler(class KSModelerQml* modeler) { m_modeler = modeler; }
	class KSModelerQml* modeler() const { return m_modeler; }

private:
	class KSModelerQml* m_modeler = nullptr;
};

} // namespace ks
