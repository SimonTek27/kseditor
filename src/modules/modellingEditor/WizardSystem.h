#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>
#include <QVariant>

namespace ks {

class WizardPage : public QObject
{
    Q_OBJECT

public:
    explicit WizardPage(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~WizardPage();

    QString getId() const { return m_id; }
    void setId(const QString& id) { m_id = id; }

    QString getTitle() const { return m_title; }
    void setTitle(const QString& title) { m_title = title; }

    QString getDescription() const { return m_description; }
    void setDescription(const QString& desc) { m_description = desc; }

    virtual bool isComplete() const;
    virtual QString getError() const { return QString(); }

    virtual void enter();
    virtual void exit();

    virtual QJsonObject getData() const;
    virtual void setData(const QJsonObject& data);

    void setComplete(bool complete);

signals:
    void completeChanged(bool complete);
    void dataChanged(const QJsonObject& data);
    void entered();
    void exited();

protected:
    QString m_id;
    QString m_title;
    QString m_description;
    bool m_complete = false;
    QJsonObject m_data;
};

class Wizard : public QObject
{
    Q_OBJECT

public:
    explicit Wizard(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~Wizard();

    void setTitle(const QString& title);
    QString getTitle() const { return m_title; }

    void setId(const QString& id) { m_id = id; }
    QString getId() const { return m_id; }

    void addPage(WizardPage* page);
    void removePage(const QString& pageId);

    int pageCount() const;
    QVector<WizardPage*> getPages() const { return m_pages; }

    WizardPage* getPage(const QString& pageId) const;
    WizardPage* currentPage() const;

    bool canGoNext() const;
    bool canGoBack() const;
    bool isLastPage() const;
    bool isFirstPage() const;

    void goNext();
    void goBack();
    void goToPage(const QString& pageId);

    void start();
    void finish();
    void cancel();

    QJsonObject collectData() const;

signals:
    void pageChanged(int pageIndex);
    void canGoNextChanged(bool canGoNext);
    void started();
    void finished(const QJsonObject& data);
    void cancelled();

private:
    QString m_id;
    QString m_title;
    QVector<WizardPage*> m_pages;
    int m_currentIndex = 0;
};

class WizardFactory : public QObject
{
    Q_OBJECT

public:
    explicit WizardFactory(QObject* parent = nullptr);
    ~WizardFactory();

    void registerWizard(const QString& id, Wizard* (*createFunc)());
    void unregisterWizard(const QString& wizardId);

    Wizard* createWizard(const QString& wizardId) const;
    QVector<QString> getAvailableWizards() const;

    bool hasWizard(const QString& wizardId) const;

signals:
    void wizardRegistered(const QString& wizardId);
    void wizardUnregistered(const QString& wizardId);

private:
    QMap<QString, Wizard* (*)()> m_creators;
};

class WizardManager : public QObject
{
    Q_OBJECT

public:
    explicit WizardManager(QObject* parent = nullptr);
    ~WizardManager();

    void registerWizard(const QString& wizardId, Wizard* wizard);
    void unregisterWizard(const QString& wizardId);

    Wizard* getWizard(const QString& wizardId) const;
    QVector<Wizard*> getWizards() const { return m_wizards.values(); }

    bool showWizard(const QString& wizardId);
    void showWizard(Wizard* wizard);

    void closeWizard();

    bool hasActiveWizard() const { return m_activeWizard != nullptr; }
    Wizard* getActiveWizard() const { return m_activeWizard; }

signals:
    void wizardShown(Wizard* wizard);
    void wizardClosed();
    void wizardFinished(bool accepted);

private:
    QMap<QString, Wizard*> m_wizards;
    Wizard* m_activeWizard = nullptr;
};

class ProjectCreationWizard : public Wizard
{
    Q_OBJECT

public:
    explicit ProjectCreationWizard(QObject* parent = nullptr);
    ~ProjectCreationWizard();

    void setTemplates(const QVector<QString>& templates);
    void setRecentProjects(const QVector<QString>& recent);

    QString getCreatedProjectPath() const { return m_projectPath; }

private:
    QString m_projectPath;
    QVector<QString> m_templates;
    QVector<QString> m_recent;
};

class ImportWizard : public Wizard
{
    Q_OBJECT

public:
    explicit ImportWizard(QObject* parent = nullptr);
    ~ImportWizard();

    QString getImportedFile() const { return m_importedFile; }
    QVector<QString> getImportResults() const { return m_results; }

private:
    QString m_importedFile;
    QVector<QString> m_results;
};

class ExportWizard : public Wizard
{
    Q_OBJECT

public:
    explicit ExportWizard(QObject* parent = nullptr);
    ~ExportWizard();

    void setInputFile(const QString& path);
    void setAvailableFormats(const QVector<QString>& formats);

    QString getOutputFile() const { return m_outputFile; }

private:
    QString m_inputFile;
    QString m_outputFile;
    QVector<QString> m_formats;
};

} // namespace ks
