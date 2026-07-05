#pragma once

#include <QString>
#include <QVector>
#include <QObject>
#include <QThread>
#include <QMutex>
#include <memory>

namespace ks {

/**
 * @brief Task singolo per il batch processor
 */
struct ModelBatchTask {
	QString id;
	QString inputPath;
	QString outputPath;
	QString format; // "kn5", "fbx", "glb", "obj"

	bool compressTextures = true;
	bool reduceLODs = false;
	bool generateCollider = true;

	enum class Status {
		Pending,
		Processing,
		Completed,
		Failed,
		Cancelled
	};
	Status status = Status::Pending;

	QString errorMessage;
	float progress = 0.0f; // 0.0 - 1.0
};

/**
 * @brief Worker thread per l'elaborazione batch
 */
class BatchWorker : public QObject {
	Q_OBJECT

public:
	explicit BatchWorker(QObject* parent = nullptr);

	void processTask(const ModelBatchTask& task);

signals:
	void taskProgress(const QString& taskId, float progress);
	void taskCompleted(const QString& taskId);
	void taskFailed(const QString& taskId, const QString& error);

private:
	bool processKN5(const ModelBatchTask& task);
	bool processFBX(const ModelBatchTask& task);
	bool processGLB(const ModelBatchTask& task);
	bool processOBJ(const ModelBatchTask& task);
};

/**
 * @brief Advanced Batch Processor
 * Gestisce processamento parallelo di file multipli
 */
class AdvancedBatchProcessor : public QObject {
	Q_OBJECT

public:
	explicit AdvancedBatchProcessor(QObject* parent = nullptr);
	~AdvancedBatchProcessor();

	/**
	 * @brief Aggiungi task alla queue
	 */
	void addTask(const ModelBatchTask& task);

	/**
	 * @brief Rimuovi task dalla queue
	 */
	bool removeTask(const QString& taskId);

	/**
	 * @brief Ottieni task per ID
	 */
	ModelBatchTask getTask(const QString& taskId) const;

	/**
	 * @brief Inizia processing della queue
	 */
	void startProcessing(int maxParallelWorkers = 2);

	/**
	 * @brief Pausa processing
	 */
	void pauseProcessing();

	/**
	 * @brief Riprendi processing
	 */
	void resumeProcessing();

	/**
	 * @brief Cancella tutto
	 */
	void cancelAll();

	/**
	 * @brief Cancella uno specifico task
	 */
	bool cancelTask(const QString& taskId);

	/**
	 * @brief Ottieni lista di tutti i tasks
	 */
	QVector<ModelBatchTask> getAllTasks() const;

	/**
	 * @brief Ottieni lista di tasks per status
	 */
	QVector<ModelBatchTask> getTasksByStatus(ModelBatchTask::Status status) const;

	/**
	 * @brief Statistiche batch
	 */
	struct BatchStats {
		int totalTasks = 0;
		int completedTasks = 0;
		int failedTasks = 0;
		int processingTasks = 0;
		int pendingTasks = 0;
		float overallProgress = 0.0f;
		QString estimatedTimeRemaining;
	};

	BatchStats getStats() const;

	/**
	 * @brief Esporta report in file
	 */
	bool exportReport(const QString& filePath);

	/**
	 * @brief Importa task list da file
	 */
	bool importTaskList(const QString& filePath);

	/**
	 * @brief Esporta task list a file
	 */
	bool exportTaskList(const QString& filePath);

signals:
	/**
	 * @brief Emesso quando inizia il processamento di un task
	 */
	void taskStarted(const QString& taskId);

	/**
	 * @brief Emesso durante il processamento
	 */
	void taskProgress(const QString& taskId, float progress);

	/**
	 * @brief Emesso quando un task completa
	 */
	void taskCompleted(const QString& taskId);

	/**
	 * @brief Emesso quando un task fallisce
	 */
	void taskFailed(const QString& taskId, const QString& error);

	/**
	 * @brief Emesso quando batch processing completa
	 */
	void batchCompleted();

	/**
	 * @brief Emesso quando viene messo in pausa
	 */
	void processingPaused();

	/**
	 * @brief Emesso quando viene ripreso
	 */
	void processingResumed();

	/**
	 * @brief Emesso quando stats cambiano
	 */
	void statsUpdated();

private:
	QVector<ModelBatchTask> m_tasks;
	QVector<QThread*> m_workerThreads;
	QVector<BatchWorker*> m_workers;
	mutable QMutex m_tasksMutex;
	bool m_isProcessing = false;
	bool m_isPaused = false;
	int m_maxParallelWorkers = 2;
	int m_activeWorkers = 0;

	void scheduleNextTasks();
	void updateStats();

private slots:
	void onWorkerTaskCompleted(const QString& taskId);
	void onWorkerTaskFailed(const QString& taskId, const QString& error);
	void onWorkerProgress(const QString& taskId, float progress);
};

} // namespace ks
