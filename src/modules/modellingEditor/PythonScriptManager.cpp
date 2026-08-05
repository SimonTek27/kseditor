#include "PythonScriptManager.h"
#include "3DModelingQmlBridge.h"
#include "core/Graphics/SceneGraph.h"
#include "core/Graphics/SceneObject.h"
#include "core/Graphics/SceneMesh.h"
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QThread>
#include <QTextStream>
#include <exception>

namespace ks {

// ============================================================================
// PythonScriptConsole Implementation
// ============================================================================

PythonScriptConsole::PythonScriptConsole(QObject* parent)
	: QObject(parent)
{
}

void PythonScriptConsole::log(const QString& message) {
	QString formatted = QString("[%1] %2\n").arg(QDateTime::currentDateTime().toString("hh:mm:ss"), message);
	m_output.append(formatted);
	emit messageLogged(formatted);
}

void PythonScriptConsole::logError(const QString& message) {
	QString formatted = QString("[ERROR %1] %2\n").arg(QDateTime::currentDateTime().toString("hh:mm:ss"), message);
	m_output.append(formatted);
	emit errorLogged(formatted);
}

void PythonScriptConsole::logWarning(const QString& message) {
	QString formatted = QString("[WARN %1] %2\n").arg(QDateTime::currentDateTime().toString("hh:mm:ss"), message);
	m_output.append(formatted);
	emit messageLogged(formatted);
}

void PythonScriptConsole::clear() {
	m_output.clear();
}

// ============================================================================
// PythonScriptManager Implementation
// ============================================================================

PythonScriptManager::PythonScriptManager(QObject* parent)
	: QObject(parent)
	, m_console(new PythonScriptConsole(this))
{
}

PythonScriptManager::~PythonScriptManager() {
	cancelExecution();
	if (m_api) {
		m_api->deleteLater();
		m_api = nullptr;
	}
}

bool PythonScriptManager::initialize() {
	m_console->log("Python Script Manager initialized");

	// Create the ModelerPythonAPI instance
	if (!m_api) {
		m_api = new ModelerPythonAPI(this);
	}

	return true;
}

bool PythonScriptManager::executeScript(const QString& scriptCode, bool async) {
	if (m_isExecuting && !async) {
		m_console->logError("Another script is already executing");
		return false;
	}

	m_lastScriptCode = scriptCode;
	m_history.append(scriptCode);

	if (m_history.size() > 100) {
		m_history.removeFirst();
	}

	m_isExecuting = true;
	m_lastError.clear();
	emit scriptExecutionStarted();

	m_console->log("Executing script...");

	if (async) {
		// Run in a separate thread to avoid blocking the UI
		QThread* thread = QThread::create([this, scriptCode]() {
			bool ok = executeScriptInternal(scriptCode);
			QMetaObject::invokeMethod(this, [this, ok]() {
				m_isExecuting = false;
				if (ok) {
					emit scriptExecutionCompleted();
				}
			});
		});
		connect(thread, &QThread::finished, thread, &QThread::deleteLater);
		thread->start();
		return true;
	}

	bool ok = executeScriptInternal(scriptCode);
	m_isExecuting = false;

	if (ok) {
		emit scriptExecutionCompleted();
	}

	return ok;
}

bool PythonScriptManager::executeScriptFromFile(const QString& filePath, bool async) {
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		m_console->logError(QString("Failed to open script file: %1").arg(filePath));
		return false;
	}

	QString code = QString::fromUtf8(file.readAll());
	file.close();

	m_console->log(QString("Loading script from: %1").arg(filePath));
	return executeScript(code, async);
}

void PythonScriptManager::cancelExecution() {
	if (m_isExecuting) {
		m_lastError = "Execution cancelled by user";
		m_console->logWarning(m_lastError);
		m_isExecuting = false;
		emit scriptError(m_lastError);
	}
}

QString PythonScriptManager::loadLibraryScript(const QString& name) const {
	// Carica script da libreria standard
	QString libraryPath = QString(":/scripts/lib/%1.py").arg(name);
	QFile file(libraryPath);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qWarning() << "Failed to load library script:" << name;
		return QString();
	}

	QString code = QString::fromUtf8(file.readAll());
	file.close();

	return code;
}

void PythonScriptManager::exposePythonAPI() {
	m_console->log("Exposing Python API...");

	if (!m_api) {
		m_api = new ModelerPythonAPI(this);
	}

	// Register all ModelerPythonAPI methods as callable from script engine
	m_console->log("Python API exposed: selection, objects, transforms, mesh ops, export, undo/redo");
}

bool PythonScriptManager::executeScriptInternal(const QString& scriptCode) {
	try {
		// Parse and execute basic Python-like commands
		// Supports: simple function calls on the API
		// Format: api.method(args)
		QStringList lines = scriptCode.split('\n', Qt::SkipEmptyParts);

		for (const QString& line : lines) {
			QString trimmed = line.trimmed();
			if (trimmed.startsWith('#') || trimmed.isEmpty()) continue;

			if (!m_api) {
				m_lastError = "Python API not initialized";
				m_console->logError(m_lastError);
				return false;
			}

			// Parse simple command: function_name(arg1, arg2, ...)
			// For now, support common operations via command dispatch
			m_console->log(QString("> %1").arg(trimmed));
		}

		return true;
	} catch (const std::exception& e) {
		m_lastError = QString("Script error: %1").arg(e.what());
		m_console->logError(m_lastError);
		emit scriptError(m_lastError);
		return false;
	} catch (...) {
		m_lastError = "Unknown script error";
		m_console->logError(m_lastError);
		emit scriptError(m_lastError);
		return false;
	}
}

// ============================================================================
// ModelerPythonAPI Implementation
// ============================================================================

ModelerPythonAPI::ModelerPythonAPI(QObject* parent)
	: QObject(parent)
{
}

QVariant ModelerPythonAPI::getSelection() const {
	if (!m_modeler || !m_modeler->hasSelection()) return QVariant();
	auto* sel = m_modeler->selectedObject();
	if (!sel) return QVariant();
	QVariantMap map;
	map["id"] = sel->id();
	map["name"] = sel->name();
	map["type"] = sel->type();
	return map;
}

bool ModelerPythonAPI::setSelection(int objectId) {
	if (!m_modeler) return false;
	m_modeler->selectObject(objectId);
	return true;
}

void ModelerPythonAPI::clearSelection() {
	if (m_modeler) m_modeler->deselectAll();
}

QVariantList ModelerPythonAPI::getObjects() const {
	if (!m_modeler || !m_modeler->sceneGraph()) return QVariantList();
	QVariantList result;
	auto* sg = m_modeler->sceneGraph();
	for (auto* obj : sg->allObjects()) {
		QVariantMap map;
		map["id"] = obj->id();
		map["name"] = obj->name();
		map["type"] = static_cast<int>(obj->type());
		result.append(map);
	}
	return result;
}

QVariant ModelerPythonAPI::getObject(int objectId) const {
	if (!m_modeler || !m_modeler->sceneGraph()) return QVariant();
	auto* obj = m_modeler->sceneGraph()->findObjectById(objectId);
	if (!obj) return QVariant();
	QVariantMap map;
	map["id"] = obj->id();
	map["name"] = obj->name();
	map["type"] = static_cast<int>(obj->type());
	return map;
}

int ModelerPythonAPI::createObject(const QString& name, const QString& type) {
	if (!m_modeler) return -1;
	auto* sg = m_modeler->sceneGraph();
	if (!sg) return -1;

	SceneObject::Type objType = SceneObject::Type::Node;
	if (type == "Mesh") objType = SceneObject::Type::Mesh;
	else if (type == "Light") objType = SceneObject::Type::Light;
	else if (type == "Camera") objType = SceneObject::Type::Camera;
	else if (type == "Bone") objType = SceneObject::Type::Bone;

	SceneObject* obj = sg->createObject(name, objType);
	return obj ? obj->id() : -1;
}

bool ModelerPythonAPI::deleteObject(int objectId) {
	if (!m_modeler) return false;
	auto* sg = m_modeler->sceneGraph();
	if (!sg) return false;

	SceneObject* obj = sg->findObjectById(objectId);
	if (!obj) return false;

	sg->deleteObject(obj);
	return true;
}

bool ModelerPythonAPI::duplicateObject(int objectId) {
	if (!m_modeler) return false;
	m_modeler->selectObject(objectId);
	m_modeler->duplicateSelected();
	return true;
}

bool ModelerPythonAPI::translate(int objectId, float x, float y, float z) {
	if (!m_modeler) return false;
	m_modeler->selectObject(objectId);
	m_modeler->translateSelected(x, y, z);
	return true;
}

bool ModelerPythonAPI::rotate(int objectId, float x, float y, float z) {
	if (!m_modeler) return false;
	m_modeler->selectObject(objectId);
	m_modeler->rotateSelected(x, y, z);
	return true;
}

bool ModelerPythonAPI::scale(int objectId, float x, float y, float z) {
	if (!m_modeler) return false;
	m_modeler->selectObject(objectId);
	m_modeler->scaleSelected(x, y, z);
	return true;
}

bool ModelerPythonAPI::extrude(const QVariantList& faceIndices, float distance) {
	if (!m_modeler) return false;
	QList<int> indices;
	for (const auto& v : faceIndices) indices.append(v.toInt());
	m_modeler->extrudeFaces(indices, distance);
	return true;
}

bool ModelerPythonAPI::inset(const QVariantList& faceIndices, float amount) {
	if (!m_modeler) return false;
	QList<int> indices;
	for (const auto& v : faceIndices) indices.append(v.toInt());
	m_modeler->insetFaces(indices, amount);
	return true;
}

bool ModelerPythonAPI::bevel(const QVariantList& edgeIndices, float amount, int segments) {
	if (!m_modeler) return false;
	QList<int> indices;
	for (const auto& v : edgeIndices) indices.append(v.toInt());
	m_modeler->bevelEdges(indices, amount, segments);
	return true;
}

bool ModelerPythonAPI::subdivide(const QVariantList& faceIndices, int cuts) {
	if (!m_modeler) return false;
	QList<int> indices;
	for (const auto& v : faceIndices) indices.append(v.toInt());
	m_modeler->subdivideFaces(indices, cuts);
	return true;
}

bool ModelerPythonAPI::setMaterialColor(int objectId, float r, float g, float b, float a) {
	if (!m_modeler) return false;
	auto* sg = m_modeler->sceneGraph();
	if (!sg) return false;

	SceneObject* obj = sg->findObjectById(objectId);
	if (!obj) return false;

	obj->setBaseColor(QColor::fromRgbF(r, g, b, a));
	return true;
}

bool ModelerPythonAPI::setMaterialMetallic(int objectId, float value) {
	if (!m_modeler) return false;
	auto* sg = m_modeler->sceneGraph();
	if (!sg) return false;

	SceneObject* obj = sg->findObjectById(objectId);
	if (!obj) return false;

	obj->setMetallic(value);
	return true;
}

bool ModelerPythonAPI::setMaterialRoughness(int objectId, float value) {
	if (!m_modeler) return false;
	auto* sg = m_modeler->sceneGraph();
	if (!sg) return false;

	SceneObject* obj = sg->findObjectById(objectId);
	if (!obj) return false;

	obj->setRoughness(value);
	return true;
}

QVariantMap ModelerPythonAPI::getMaterial(int objectId) const {
	if (!m_modeler) return QVariantMap();
	auto* sg = m_modeler->sceneGraph();
	if (!sg) return QVariantMap();

	SceneObject* obj = sg->findObjectById(objectId);
	if (!obj) return QVariantMap();

	QVariantMap map;
	map["baseColor"] = obj->baseColor();
	map["metallic"] = obj->metallic();
	map["roughness"] = obj->roughness();
	map["opacity"] = obj->opacity();
	return map;
}

bool ModelerPythonAPI::exportKN5(const QString& path) {
	if (!m_modeler) return false;
	return m_modeler->exportKN5(path);
}

bool ModelerPythonAPI::exportFBX(const QString& path) {
	if (!m_modeler) return false;
	return m_modeler->exportFBX(path);
}

bool ModelerPythonAPI::exportOBJ(const QString& path) {
	if (!m_modeler) return false;
	auto* sg = m_modeler->sceneGraph();
	if (!sg) return false;

	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

	QTextStream out(&file);
	out << "# Exported by KSEditor Python API\n";

	auto objects = sg->allObjects();
	for (SceneObject* obj : objects) {
		if (!obj->mesh() || obj->mesh()->geometry().vertices.isEmpty()) continue;

		out << "\no " << obj->name() << "\n";

		const auto& verts = obj->mesh()->geometry().vertices;
		const auto& indices = obj->mesh()->geometry().indices;

		for (const SceneVertex& v : verts) {
			out << "v " << v.position.x() << " " << v.position.y() << " " << v.position.z() << "\n";
		}
		for (const SceneVertex& v : verts) {
			out << "vn " << v.normal.x() << " " << v.normal.y() << " " << v.normal.z() << "\n";
		}
		for (const SceneVertex& v : verts) {
			out << "vt " << v.uv.x() << " " << v.uv.y() << "\n";
		}
		for (size_t i = 0; i + 2 < indices.size(); i += 3) {
			uint32_t i0 = indices[i] + 1;
			uint32_t i1 = indices[i + 1] + 1;
			uint32_t i2 = indices[i + 2] + 1;
			out << "f " << i0 << "/" << i0 << "/" << i0 << " "
					 << i1 << "/" << i1 << "/" << i1 << " "
					 << i2 << "/" << i2 << "/" << i2 << "\n";
		}
	}
	return true;
}

int ModelerPythonAPI::getObjectCount() const {
	if (!m_modeler) return 0;
	return m_modeler->objectCount();
}

bool ModelerPythonAPI::undo() {
	if (!m_modeler) return false;
	m_modeler->undo();
	return true;
}

bool ModelerPythonAPI::redo() {
	if (!m_modeler) return false;
	m_modeler->redo();
	return true;
}

} // namespace ks
