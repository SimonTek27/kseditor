#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QJsonObject>
#include <QVector3D>
#include <memory>
#include <functional>

namespace ks {

/**
 * @brief Interfaccia base per il Command Pattern
 * Ogni operazione di editing implementa questa interfaccia
 */
class Command {
public:
	virtual ~Command() = default;

	/**
	 * @brief Esegue il comando la prima volta
	 */
	virtual void execute() = 0;

	/**
	 * @brief Annulla il comando
	 */
	virtual void undo() = 0;

	/**
	 * @brief Ripete il comando (dopo undo)
	 */
	virtual void redo() = 0;

	/**
	 * @brief Descrizione umana del comando per UI
	 */
	virtual QString description() const = 0;

	/**
	 * @brief Merge con comando precedente (opzionale)
	 * Es: multiple translate -> merge in una sola entry
	 */
	virtual bool canMergeWith(const Command* other) const {
		return false;
	}

	/**
	 * @brief Unisci con comando precedente
	 */
	virtual void mergeWith(const Command* other) {
		// Default: nop
	}
};

using CommandPtr = std::shared_ptr<Command>;

/**
 * @brief Gestore della cronologia comandi con Undo/Redo
 */
class CommandHistory : public QObject {
	Q_OBJECT

public:
	explicit CommandHistory(QObject* parent = nullptr);
	~CommandHistory();

	/**
	 * @brief Esegui e registra un comando
	 */
	void execute(CommandPtr command);

	/**
	 * @brief Annulla l'ultimo comando
	 */
	void undo();

	/**
	 * @brief Ripeti l'ultimo comando annullato
	 */
	void redo();

	/**
	 * @brief Checks if undo is possible
	 */
	bool canUndo() const;

	/**
	 * @brief Checks if redo is possible
	 */
	bool canRedo() const;

	/**
	 * @brief Description of the command that will be undone
	 */
	QString undoDescription() const;

	/**
	 * @brief Description of the command that will be redone
	 */
	QString redoDescription() const;

	/**
	 * @brief Svuota la cronologia
	 */
	void clear();

	/**
	 * @brief Imposta limite massimo di comandi in memoria
	 * Default: 1000
	 */
	void setMaxCommands(int maxCount);

	/**
	 * @brief Ottieni numero di comandi nello stack
	 */
	int commandCount() const { return m_undoStack.size(); }

	/**
	 * @brief Ottieni numero di comandi nel redo stack
	 */
	int redoCount() const { return m_redoStack.size(); }

signals:
	/**
	 * @brief Emesso quando cambia lo stato di undo/redo
	 */
	void historyChanged();

	/**
	 * @brief Emesso quando viene eseguito un comando
	 */
	void commandExecuted(const QString& description);

	/**
	 * @brief Emesso quando viene annullato un comando
	 */
	void commandUndone(const QString& description);

	/**
	 * @brief Emesso quando viene ripetuto un comando
	 */
	void commandRedone(const QString& description);

private:
	QVector<CommandPtr> m_undoStack;
	QVector<CommandPtr> m_redoStack;
	int m_maxCommands = 1000;
};

/**
 * @brief Generic command for scalar properties
 * Saves and restores property values
 */
class ModelPropertyCommand : public Command {
public:
	ModelPropertyCommand(
		const QString& objectName,
		const QString& propertyName,
		const QVariant& oldValue,
		const QVariant& newValue,
		const QString& description = ""
	);

	void execute() override;
	void undo() override;
	void redo() override;
	QString description() const override { return m_description; }
	bool canMergeWith(const Command* other) const override;
	void mergeWith(const Command* other) override;

private:
	void applyValue(const QVariant& value);

	QString m_objectName;
	QString m_propertyName;
	QVariant m_oldValue;
	QVariant m_newValue;
	QString m_description;
};

/**
 * @brief Comando per trasformazione (translate/rotate/scale)
 * Memorizza stato iniziale e finale
 */
class TransformCommand : public Command {
public:
	enum Type {
		Translate,
		Rotate,
		Scale
	};

	TransformCommand(
		int objectId,
		Type type,
		const QVector3D& oldValue,
		const QVector3D& newValue
	);

	void execute() override;
	void undo() override;
	void redo() override;
	QString description() const override;
	bool canMergeWith(const Command* other) const override;
	void mergeWith(const Command* other) override;

private:
	int m_objectId;
	Type m_type;
	QVector3D m_oldValue;
	QVector3D m_newValue;
};

/**
 * @brief Comando per creazione oggetto
 */
class CreateObjectCommand : public Command {
public:
	CreateObjectCommand(
		const QString& name,
		const QString& type,
		int generatedId = -1
	);

	void execute() override;
	void undo() override;
	void redo() override;
	QString description() const override;

	int getCreatedId() const { return m_createdId; }

private:
	QString m_name;
	QString m_type;
	int m_createdId = -1;
};

/**
 * @brief Comando per eliminazione oggetto
 */
class DeleteObjectCommand : public Command {
public:
	explicit DeleteObjectCommand(int objectId);

	void execute() override;
	void undo() override;
	void redo() override;
	QString description() const override;

private:
	int m_objectId;
	QString m_objectName;
	QJsonObject m_savedState;
	int m_createdId = -1;
};

/**
 * @brief Comando per editing mesh (extrude, inset, bevel, etc)
 */
class MeshEditCommand : public Command {
public:
	enum Operation {
		Extrude,
		Inset,
		Bevel,
		Subdivide,
		Other
	};

	MeshEditCommand(
		int objectId,
		Operation op,
		const QByteArray& originalMeshData,
		const QByteArray& modifiedMeshData
	);

	void execute() override;
	void undo() override;
	void redo() override;
	QString description() const override;

private:
	int m_objectId;
	Operation m_operation;
	QByteArray m_originalMeshData;
	QByteArray m_modifiedMeshData;
};

} // namespace ks
