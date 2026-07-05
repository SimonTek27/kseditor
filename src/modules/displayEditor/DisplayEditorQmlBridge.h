#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <memory>
#include "DisplayEditor.h"

namespace ks {

class DisplayEditorQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName NOTIFY displayNameChanged)
    Q_PROPERTY(int elementCount READ elementCount NOTIFY elementCountChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged)

public:
    static DisplayEditorQmlBridge* instance();

    QString displayName() const { return m_displayName; }
    int elementCount() const { return m_elementCount; }
    QString currentFile() const { return m_currentFile; }

    Q_INVOKABLE bool loadFromFile(const QString& path);
    Q_INVOKABLE bool saveToFile(const QString& path);
    Q_INVOKABLE bool loadFromLua(const QString& path);
    Q_INVOKABLE bool saveToLua(const QString& path);
    Q_INVOKABLE QVariantList getElements();
    Q_INVOKABLE QVariantMap getElement(const QString& id);
    Q_INVOKABLE void addElement(const QVariantMap& element);
    Q_INVOKABLE void removeElement(const QString& id);
    Q_INVOKABLE void updateElement(const QString& id, const QVariantMap& data);
    Q_INVOKABLE void clearElements();
    Q_INVOKABLE void setDisplayName(const QString& name);
    Q_INVOKABLE void setDisplaySize(int width, int height);
    Q_INVOKABLE QVariantMap getDisplaySettings();
    Q_INVOKABLE QStringList getAvailableDataSources();
    Q_INVOKABLE QStringList getAvailableElementTypes();
    Q_INVOKABLE void setBackgroundImage(const QString& path);
    Q_INVOKABLE QString exportAsImage(const QString& path);
    Q_INVOKABLE QVariantList getElementTemplates();
    Q_INVOKABLE bool addElementFromTemplate(const QString& templateName);
    Q_INVOKABLE void updatePhysicsValue(const QString& source, double value);

signals:
    void displayNameChanged();
    void elementCountChanged();
    void currentFileChanged();
    void elementAdded(const QString& id);
    void elementRemoved(const QString& id);
    void elementModified(const QString& id);
    void editorRequested();

private:
    static DisplayEditorQmlBridge* s_instance;
    DisplayEditorQmlBridge(QObject* parent = nullptr) : QObject(parent) {}

    QString m_displayName;
    int m_elementCount = 0;
    QString m_currentFile;
    std::unique_ptr<ksDisplayEditor> m_editor;
};

} // namespace ks
