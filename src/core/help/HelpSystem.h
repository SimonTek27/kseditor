#pragma once
#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include <QBitArray>
#include <QWidget>
#include <QRect>
#include <QMouseEvent>
#include <QPair>
#include <QCheckBox>

namespace ks {

class HelpToolTipManager : public QObject {
    Q_OBJECT
public:
    explicit HelpToolTipManager(QObject* parent = nullptr);
    static HelpToolTipManager* instance();
    ~HelpToolTipManager() override;

    void initialize(QWidget* parent = nullptr);
    void addToolTip(const QString& context, const QString& element,
                    const QString& description, const QString& shortcut = "");
    QMap<QString, QMap<QString, QString>> getToolTips() const;
    QJsonObject createToolTip(const QString& context, const QString& element) const;

signals:
    void toolTipRequested(const QJsonObject& tip);

private:
    static HelpToolTipManager* s_instance;
    QMap<QString, QMap<QString, QPair<QString, QString>>> m_toolTips;
};

class HelpToolTipContextTracker : public QWidget {
    Q_OBJECT
public:
    explicit HelpToolTipContextTracker(QWidget* parent = nullptr);
    ~HelpToolTipContextTracker() override;

    void setCurrentContext(const QString& context);
    QString currentContext() const;

    void registerWidget(QWidget* widget, const QString& element,
                        const QString& context = "",
                        const QRect& position = QRect());

    QJsonObject getToolTipForWidget(QWidget* widget) const;

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    static HelpToolTipContextTracker* s_instance;
    QString m_currentContext;
    QMap<QWidget*, QPair<QString, QString>> m_widgetMap;
};

class TutorialSystem : public QObject {
    Q_OBJECT
public:
    explicit TutorialSystem(QObject* parent = nullptr);
    static TutorialSystem* instance();
    ~TutorialSystem() override;

    enum TutorialPage {
        Welcome,
        QuickStart,
        CreatingProject,
        Importing3D,
        WorkingWith3D,
        Exporting,
        Settings,
        AdvancedFeatures,
        Troubleshooting
    };

    void startTutorial(TutorialPage page);
    void showHelpTopic(const QString& topicId);
    bool hasTutorialStarted() const;
    TutorialPage currentTutorial() const;
    QVector<QString> getSuggestedActions() const;

    struct TutorialStep {
        QString title;
        QString description;
        QString targetElement;
        QVector<QString> actions;
    };

    void setTutorialSteps(const QVector<TutorialStep>& steps);

signals:
    void tutorialStarted(TutorialPage page);
    void tutorialCompleted(TutorialPage page);
    void helpRequested(const QString& topicId);

private:
    void showTutorialUI();
    void setupPageContent(TutorialPage page);

    static TutorialSystem* s_instance;
    QVector<TutorialStep> m_tutorialSteps;
    TutorialPage m_currentTutorial = Welcome;
    int m_currentStep = 0;
    bool m_tutorialStarted = false;
};

class QuickStartGuide : public QObject {
    Q_OBJECT
public:
    explicit QuickStartGuide(QObject* parent = nullptr);
    static QuickStartGuide* instance();
    ~QuickStartGuide() override;

    enum QuickStartSteps {
        WelcomeScreen,
        CreateNewProject,
        ImportModel,
        NavigateViewport,
        UsePropertyPanel,
        ExportProject,
        SaveProject,
        BuildProject
    };

    void startQuickStart();
    void completeStep(int stepIndex);
    bool isStepCompleted(int stepIndex) const;
    QVector<QString> getQuickStartSteps() const;
    QString getStepDescription(int stepIndex) const;
    QString getStepTip(int stepIndex) const;
    bool requiresToolTip(int stepIndex) const;

signals:
    void stepCompleted(int stepIndex);

private:
    static QuickStartGuide* s_instance;
    QBitArray m_completedSteps{8};
};

class HelpSystem : public QObject {
    Q_OBJECT
public:
    explicit HelpSystem(QObject* parent = nullptr);
    static HelpSystem* instance();
    ~HelpSystem() override;

    void initialize(QWidget* parent);
    void registerHelp(const QString& context, const QString& element,
                      const QString& description, const QString& shortcut = "");
    void showHelpForWidget(QWidget* widget);
    void showHelp();
    void startTutorial(TutorialSystem::TutorialPage page);
    void startQuickStartGuide();

    bool hasHelpEnabled() const;
    void enableHelp(bool enabled);

private:
    static HelpSystem* s_instance;
    bool m_helpEnabled = true;
    HelpToolTipManager* m_toolTipManager = nullptr;
    TutorialSystem* m_tutorialSystem = nullptr;
    QuickStartGuide* m_quickStart = nullptr;
};

} // namespace ks
