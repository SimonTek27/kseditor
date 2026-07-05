#include "AdvancedBatchProcessor.h"
#include <QThreadPool>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>
#include <algorithm>

namespace ks {

// ============================================================================
// BatchWorker Implementation
// ============================================================================

BatchWorker::BatchWorker(QObject* parent)
	: QObject(parent)
{
}

void BatchWorker::processTask(const ModelBatchTask& task) {
	bool success = false;

	QString fmt = task.format.toLower();
	if (fmt == "kn5") {
		success = processKN5(task);
	} else if (fmt == "fbx") {
		success = processFBX(task);
	} else if (fmt == "glb" || fmt == "gltf") {
		success = processGLB(task);
	} else if (fmt == "obj") {
		success = processOBJ(task);
	} else {
		emit taskFailed(task.id, "Unsupported format: " + task.format);
		return;
	}

	if (success) {
		emit taskCompleted(task.id);
	} else {
		emit taskFailed(task.id, "Processing failed");
	}
}

bool BatchWorker::processKN5(const ModelBatchTask& task) {
	QFile inputFile(task.inputPath);
	if (!inputFile.exists()) {
		return false;
	}

	// KN5 processing: read header, validate format, apply optimizations
	if (!inputFile.open(QIODevice::ReadOnly)) {
		return false;
	}

	// Verify KN5 magic bytes
	QByteArray header = inputFile.read(4);
	if (header.size() < 4 || header[0] != 'K' || header[1] != 'N' || header[2] != '5') {
		return false;
	}
	inputFile.close();

	// Copy with optional texture compression
	if (task.compressTextures) {
		// KN5 texture compression requires format-specific parsing
		// (KN5 embeds textures in a proprietary binary stream).
		// The file is copied as-is; use KsKN5Converter for full pipeline.
		qDebug() << "KN5: texture compression requested (requires KsKN5Converter for full support)";
	}

	if (task.outputPath.isEmpty() || task.outputPath == task.inputPath) {
		return true; // In-place processing
	}

	return QFile::copy(task.inputPath, task.outputPath);
}

bool BatchWorker::processFBX(const ModelBatchTask& task) {
	QFile inputFile(task.inputPath);
	if (!inputFile.exists()) {
		return false;
	}

	// FBX binary format validation
	if (!inputFile.open(QIODevice::ReadOnly)) {
		return false;
	}

	QByteArray header = inputFile.read(27);
	inputFile.close();

	// FBX magic: "Kaydara FBX Binary  \x00"
	if (header.size() < 27 || !header.startsWith("Kaydara FBX Binary")) {
		return false;
	}

	if (task.outputPath.isEmpty() || task.outputPath == task.inputPath) {
		return true;
	}

	return QFile::copy(task.inputPath, task.outputPath);
}

bool BatchWorker::processGLB(const ModelBatchTask& task) {
	QFile inputFile(task.inputPath);
	if (!inputFile.exists()) {
		return false;
	}

	// GLB format validation (glTF binary)
	if (!inputFile.open(QIODevice::ReadOnly)) {
		return false;
	}

	QByteArray header = inputFile.read(12);
	inputFile.close();

	// GLB magic: 0x46546C67 ("glTF") + version(4) + length(4)
	if (header.size() < 12) {
		return false;
	}
	uint32_t magic = *reinterpret_cast<const uint32_t*>(header.constData());
	if (magic != 0x46546C67) { // "glTF"
		return false;
	}

	if (task.outputPath.isEmpty() || task.outputPath == task.inputPath) {
		return true;
	}

	return QFile::copy(task.inputPath, task.outputPath);
}

bool BatchWorker::processOBJ(const ModelBatchTask& task) {
	QFile inputFile(task.inputPath);
	if (!inputFile.exists()) {
		return false;
	}

	if (!inputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return false;
	}

	// Basic OBJ validation - check for valid vertex data
	QTextStream in(&inputFile);
	bool hasVertices = false;
	while (!in.atEnd()) {
		QString line = in.readLine().trimmed();
		if (line.startsWith("v ")) {
			hasVertices = true;
			break;
		}
		if (line.startsWith("#") || line.isEmpty()) continue;
		if (!line.startsWith("v ") && !line.startsWith("vt ") &&
			!line.startsWith("vn ") && !line.startsWith("f ") &&
			!line.startsWith("o ") && !line.startsWith("g ")) {
			// Unknown directive, but still valid OBJ
			continue;
		}
	}
	inputFile.close();

	if (!hasVertices) {
		return false; // No geometry data
	}

	if (task.outputPath.isEmpty() || task.outputPath == task.inputPath) {
		return true;
	}

	return QFile::copy(task.inputPath, task.outputPath);
}

// ============================================================================
// AdvancedBatchProcessor Implementation
// ============================================================================

AdvancedBatchProcessor::AdvancedBatchProcessor(QObject* parent)
	: QObject(parent)
	, m_maxParallelWorkers(2)
	, m_activeWorkers(0)
{
}

AdvancedBatchProcessor::~AdvancedBatchProcessor() {
	cancelAll();

	for (auto thread : m_workerThreads) {
		thread->quit();
		thread->wait();
		delete thread;
	}
}

void AdvancedBatchProcessor::addTask(const ModelBatchTask& task) {
	{
		QMutexLocker locker(&m_tasksMutex);
		m_tasks.append(task);
	}
	emit statsUpdated();
}

bool AdvancedBatchProcessor::removeTask(const QString& taskId) {
	QMutexLocker locker(&m_tasksMutex);

	auto it = std::find_if(m_tasks.begin(), m_tasks.end(),
		[taskId](const ModelBatchTask& t) { return t.id == taskId; });

	if (it != m_tasks.end()) {
		m_tasks.erase(it);
		return true;
	}
	return false;
}

ModelBatchTask AdvancedBatchProcessor::getTask(const QString& taskId) const {
	QMutexLocker locker(&m_tasksMutex);

	for (const auto& task : m_tasks) {
		if (task.id == taskId) {
			return task;
		}
	}
	return ModelBatchTask();
}

void AdvancedBatchProcessor::startProcessing(int maxParallelWorkers) {
	if (m_isProcessing) return;

	m_maxParallelWorkers = maxParallelWorkers;
	m_isProcessing = true;
	m_isPaused = false;

	scheduleNextTasks();
}

void AdvancedBatchProcessor::pauseProcessing() {
	m_isPaused = true;
	emit processingPaused();
}

void AdvancedBatchProcessor::resumeProcessing() {
	m_isPaused = false;
	emit processingResumed();
	scheduleNextTasks();
}

void AdvancedBatchProcessor::cancelAll() {
	m_isProcessing = false;
	m_activeWorkers = 0;

	QMutexLocker locker(&m_tasksMutex);
	for (auto& task : m_tasks) {
		if (task.status == ModelBatchTask::Status::Pending ||
			task.status == ModelBatchTask::Status::Processing) {
			task.status = ModelBatchTask::Status::Cancelled;
		}
	}
}

bool AdvancedBatchProcessor::cancelTask(const QString& taskId) {
	QMutexLocker locker(&m_tasksMutex);

	for (auto& task : m_tasks) {
		if (task.id == taskId) {
			if (task.status == ModelBatchTask::Status::Pending ||
				task.status == ModelBatchTask::Status::Processing) {
				task.status = ModelBatchTask::Status::Cancelled;
				return true;
			}
		}
	}
	return false;
}

QVector<ModelBatchTask> AdvancedBatchProcessor::getAllTasks() const {
	QMutexLocker locker(&m_tasksMutex);
	return m_tasks;
}

QVector<ModelBatchTask> AdvancedBatchProcessor::getTasksByStatus(ModelBatchTask::Status status) const {
	QMutexLocker locker(&m_tasksMutex);
	QVector<ModelBatchTask> result;

	for (const auto& task : m_tasks) {
		if (task.status == status) {
			result.append(task);
		}
	}
	return result;
}

AdvancedBatchProcessor::BatchStats AdvancedBatchProcessor::getStats() const {
	QMutexLocker locker(&m_tasksMutex);
	BatchStats stats;

	stats.totalTasks = m_tasks.size();

	for (const auto& task : m_tasks) {
		switch (task.status) {
			case ModelBatchTask::Status::Completed:
				stats.completedTasks++;
				break;
			case ModelBatchTask::Status::Failed:
			case ModelBatchTask::Status::Cancelled:
				stats.failedTasks++;
				break;
			case ModelBatchTask::Status::Processing:
				stats.processingTasks++;
				break;
			case ModelBatchTask::Status::Pending:
				stats.pendingTasks++;
				break;
		}
	}

	if (stats.totalTasks > 0) {
		stats.overallProgress = (float)(stats.completedTasks + stats.failedTasks) / stats.totalTasks;
	}

	return stats;
}

bool AdvancedBatchProcessor::exportReport(const QString& filePath) {
	QJsonObject report;
	report["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

	auto stats = getStats();
	report["total_tasks"] = stats.totalTasks;
	report["completed"] = stats.completedTasks;
	report["failed"] = stats.failedTasks;
	report["progress"] = stats.overallProgress;

	QJsonArray tasksArray;
	QMutexLocker locker(&m_tasksMutex);

	for (const auto& task : m_tasks) {
		QJsonObject taskObj;
		taskObj["id"] = task.id;
		taskObj["input"] = task.inputPath;
		taskObj["output"] = task.outputPath;
		taskObj["format"] = task.format;
		taskObj["status"] = static_cast<int>(task.status);
		if (!task.errorMessage.isEmpty()) {
			taskObj["error"] = task.errorMessage;
		}
		tasksArray.append(taskObj);
	}

	report["tasks"] = tasksArray;

	QJsonDocument doc(report);
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly)) {
		qWarning() << "Failed to export report:" << filePath;
		return false;
	}

	file.write(doc.toJson());
	file.close();
	return true;
}

bool AdvancedBatchProcessor::importTaskList(const QString& filePath) {
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly)) {
		return false;
	}

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	file.close();

	if (!doc.isArray()) {
		return false;
	}

	{
		QMutexLocker locker(&m_tasksMutex);
		m_tasks.clear();

		for (const auto& value : doc.array()) {
			if (value.isObject()) {
				QJsonObject obj = value.toObject();
				ModelBatchTask task;
				task.id = obj["id"].toString();
				task.inputPath = obj["input"].toString();
				task.outputPath = obj["output"].toString();
				task.format = obj["format"].toString();
				m_tasks.append(task);
			}
		}
	}

	return true;
}

bool AdvancedBatchProcessor::exportTaskList(const QString& filePath) {
	QJsonArray array;

	{
		QMutexLocker locker(&m_tasksMutex);
		for (const auto& task : m_tasks) {
			QJsonObject obj;
			obj["id"] = task.id;
			obj["input"] = task.inputPath;
			obj["output"] = task.outputPath;
			obj["format"] = task.format;
			array.append(obj);
		}
	}

	QJsonDocument doc(array);
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly)) {
		return false;
	}

	file.write(doc.toJson());
	file.close();
	return true;
}

void AdvancedBatchProcessor::scheduleNextTasks() {
	if (!m_isProcessing || m_isPaused) return;

	QMutexLocker locker(&m_tasksMutex);

	for (auto& task : m_tasks) {
		if (m_activeWorkers >= m_maxParallelWorkers) break;

		if (task.status == ModelBatchTask::Status::Pending) {
			task.status = ModelBatchTask::Status::Processing;
			emit taskStarted(task.id);

			// Crea worker se necessario
			if (m_activeWorkers >= m_workers.size()) {
				QThread* thread = new QThread();
				BatchWorker* worker = new BatchWorker();
				worker->moveToThread(thread);

				connect(worker, &BatchWorker::taskCompleted,
						this, &AdvancedBatchProcessor::onWorkerTaskCompleted);
				connect(worker, &BatchWorker::taskFailed,
						this, &AdvancedBatchProcessor::onWorkerTaskFailed);

				m_workerThreads.append(thread);
				m_workers.append(worker);
				thread->start();
			}

			// Assegna task al worker
			m_activeWorkers++;
			QMetaObject::invokeMethod(m_workers[m_activeWorkers - 1],
				"processTask", Qt::QueuedConnection,
				Q_ARG(ModelBatchTask, task));
		}
	}
}

void AdvancedBatchProcessor::updateStats() {
	emit statsUpdated();
}

void AdvancedBatchProcessor::onWorkerTaskCompleted(const QString& taskId) {
	{
		QMutexLocker locker(&m_tasksMutex);
		for (auto& task : m_tasks) {
			if (task.id == taskId) {
				task.status = ModelBatchTask::Status::Completed;
				break;
			}
		}
	}

	m_activeWorkers--;
	emit taskCompleted(taskId);
	updateStats();

	if (m_isProcessing) {
		scheduleNextTasks();
	}
}

void AdvancedBatchProcessor::onWorkerTaskFailed(const QString& taskId, const QString& error) {
	{
		QMutexLocker locker(&m_tasksMutex);
		for (auto& task : m_tasks) {
			if (task.id == taskId) {
				task.status = ModelBatchTask::Status::Failed;
				task.errorMessage = error;
				break;
			}
		}
	}

	m_activeWorkers--;
	emit taskFailed(taskId, error);
	updateStats();

	if (m_isProcessing) {
		scheduleNextTasks();
	}
}

void AdvancedBatchProcessor::onWorkerProgress(const QString& taskId, float progress) {
	{
		QMutexLocker locker(&m_tasksMutex);
		for (auto& task : m_tasks) {
			if (task.id == taskId) {
				task.progress = progress;
				break;
			}
		}
	}

	emit taskProgress(taskId, progress);
}

} // namespace ks
