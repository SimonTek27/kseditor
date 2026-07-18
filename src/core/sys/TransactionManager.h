#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include <QUuid>
#include <QMutex>
#include <QStack>

namespace ks {

class TransactionManager : public QObject
{
    Q_OBJECT

public:
    static TransactionManager* instance();

    explicit TransactionManager(QObject* parent = nullptr);
    ~TransactionManager();

    // Transaction lifecycle
    QString beginTransaction(const QString& description = QString());
    void commitTransaction(const QString& transactionId);
    void rollbackTransaction(const QString& transactionId);
    
    // Check if there's an active transaction
    bool hasActiveTransaction() const { return !m_transactionStack.isEmpty(); }
    QString currentTransaction() const { return m_transactionStack.isEmpty() ? QString() : m_transactionStack.top(); }

    // Cross-module operations
    void registerModule(const QString& moduleId, QObject* module);
    void unregisterModule(const QString& moduleId);
    
    // Get all changes for a transaction
    struct Change {
        QString transactionId;
        QString moduleId;
        QString property;
        QJsonValue oldValue;
        QJsonValue newValue;
        QString description;
        QDateTime timestamp;
    };
    
    // Record a change that can be rolled back
    template<typename T>
    void recordChange(const QString& moduleId, const QString& property, 
                      const T& oldValue, const T& newValue,
                      const QString& description = QString()) {
        if (!hasActiveTransaction()) return;
        
        Change change;
        change.transactionId = currentTransaction();
        change.moduleId = moduleId;
        change.property = property;
        change.oldValue = QJsonValue::fromVariant(oldValue);
        change.newValue = QJsonValue::fromVariant(newValue);
        change.description = description;
        change.timestamp = QDateTime::currentDateTime();
        
        m_changes[change.transactionId].append(change);
    }

    QVector<Change> getChanges(const QString& transactionId) const;
    QVector<Change> getAllChanges() const;

    // Serialization for crash recovery
    QJsonObject serializeTransaction(const QString& transactionId) const;
    void deserializeTransaction(const QJsonObject& data);

    // Callbacks for module state changes
    using ModuleSnapshotCallback = std::function<QJsonObject(const QString&)>;
    using ModuleRestoreCallback = std::function<void(const QString&, const QJsonObject&)>;

    void setSnapshotCallback(const QString& moduleId, ModuleSnapshotCallback cb);
    void setRestoreCallback(const QString& moduleId, ModuleRestoreCallback cb);

signals:
    void transactionStarted(const QString& transactionId, const QString& description);
    void transactionCommitted(const QString& transactionId);
    void transactionRolledBack(const QString& transactionId, const QString& reason);
    void changeRecorded(const Change& change);

private:
    void applyRollback(const QString& transactionId);
    
    struct Transaction {
        QString id;
        QString description;
        QDateTime startTime;
        QVector<Change> changes;
    };

    QMap<QString, Transaction> m_transactions;
    QMap<QString, QVector<Change>> m_changes;
    QStack<QString> m_transactionStack;
    QMap<QString, ModuleSnapshotCallback> m_snapshotCallbacks;
    QMap<QString, ModuleRestoreCallback> m_restoreCallbacks;
    mutable QMutex m_mutex;
};

} // namespace ks