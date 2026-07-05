#include "HelpSystem.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QMessageBox>
#include <QStandardPaths>

namespace ks {

// ============================================================================
// HelpToolTipManager
// ============================================================================

HelpToolTipManager* HelpToolTipManager::s_instance = nullptr;

HelpToolTipManager::HelpToolTipManager(QObject* parent)
    : QObject(parent)
{
}

HelpToolTipManager::~HelpToolTipManager() {}

HelpToolTipManager* HelpToolTipManager::instance()
{
    if (!s_instance) s_instance = new HelpToolTipManager();
    return s_instance;
}

void HelpToolTipManager::initialize(QWidget* /*parent*/)
{
}

void HelpToolTipManager::addToolTip(const QString& context, const QString& element,
                                     const QString& description, const QString& shortcut)
{
    Q_UNUSED(context)
    Q_UNUSED(element)
    Q_UNUSED(description)
    Q_UNUSED(shortcut)
}

QMap<QString, QMap<QString, QString>> HelpToolTipManager::getToolTips() const
{
    return {};
}

QJsonObject HelpToolTipManager::createToolTip(const QString& context, const QString& element) const
{
    Q_UNUSED(context)
    Q_UNUSED(element)
    return {};
}

// ============================================================================
// HelpToolTipContextTracker
// ============================================================================
// HelpToolTipContextTracker
// ============================================================================

HelpToolTipContextTracker::HelpToolTipContextTracker(QWidget* parent)
    : QWidget(parent)
{
}

HelpToolTipContextTracker::~HelpToolTipContextTracker() {}

void HelpToolTipContextTracker::setCurrentContext(const QString& context)
{
    m_currentContext = context;
}

QString HelpToolTipContextTracker::currentContext() const
{
    return m_currentContext;
}

void HelpToolTipContextTracker::registerWidget(QWidget* widget, const QString& element,
                                                const QString& context, const QRect& /*position*/)
{
    if (!widget) return;
    m_widgetMap[widget] = qMakePair(element, context.isEmpty() ? m_currentContext : context);
}

QJsonObject HelpToolTipContextTracker::getToolTipForWidget(QWidget* widget) const
{
    Q_UNUSED(widget)
    return {};
}

void HelpToolTipContextTracker::mouseMoveEvent(QMouseEvent* event)
{
    QWidget::mouseMoveEvent(event);
}

void HelpToolTipContextTracker::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);
}

// ============================================================================
// TutorialSystem
// ============================================================================

TutorialSystem* TutorialSystem::s_instance = nullptr;

TutorialSystem::TutorialSystem(QObject* parent)
    : QObject(parent)
    , m_currentTutorial(Welcome)
    , m_currentStep(0)
{
    m_tutorialSteps << TutorialStep{
        "Welcome to ksEditor",
        "Welcome to ksEditor! This editor provides professional tools for modding Assetto Corsa vehicles, tracks, and audio.",
        "welcome_screen",
        {"Click 'New Project' to start fresh", "Click 'Open Project' to load an existing project"}
    };

    m_tutorialSteps << TutorialStep{
        "Quick Start",
        "Let's quickly get you started by creating a simple car project.",
        "project_wizard",
        {"Click 'New Project'", "Select 'Car' from the module list", "Name your project"}
    };

    m_tutorialSteps << TutorialStep{
        "Import 3D Model",
        "Import a 3D model to work with. ksEditor supports FBX, OBJ, GLB, and KN5 formats.",
        "import_button",
        {"Click 'Import 3D...' in the file menu", "Select your 3D model file", "Wait for the model to load"}
    };

    m_tutorialSteps << TutorialStep{
        "Navigate 3D Viewport",
        "Use your mouse to navigate the 3D viewport. Left-click to orbit, right-click to pan, scroll to zoom.",
        "viewport",
        {"Left-click and drag to rotate view", "Right-click and drag to pan view", "Scroll wheel to zoom"}
    };

    m_tutorialSteps << TutorialStep{
        "Work with Properties",
        "Use the Properties panel to modify the selected object's transform, materials, and settings.",
        "properties_panel",
        {"Select an object in the scene or Object List", "Use the transform controls in Properties panel", "Modify material settings as needed"}
    };

    m_tutorialSteps << TutorialStep{
        "Save Your Project",
        "Save your project regularly. ksEditor uses the .ksep file format.",
        "save_button",
        {"Click 'Save' or press Ctrl+S", "Choose a location for your project", "Enter a project name"}
    };

    m_tutorialSteps << TutorialStep{
        "Build Your Project",
        "Build your project to generate the final game content packages.",
        "build_button",
        {"Click 'Build' in the file menu", "Wait for the build process to complete", "Check the output panel for any errors"}
    };
}

TutorialSystem::~TutorialSystem() {}

TutorialSystem* TutorialSystem::instance()
{
    if (!s_instance) s_instance = new TutorialSystem();
    return s_instance;
}

void TutorialSystem::startTutorial(TutorialPage page)
{
    if (page < Welcome || page > Troubleshooting) return;

    m_currentTutorial = page;
    m_currentStep = 0;

    showTutorialUI();

    emit tutorialStarted(page);
}

void TutorialSystem::showHelpTopic(const QString& topicId)
{
    emit helpRequested(topicId);
}

bool TutorialSystem::hasTutorialStarted() const
{
    return false;
}

TutorialSystem::TutorialPage TutorialSystem::currentTutorial() const
{
    return m_currentTutorial;
}

QVector<QString> TutorialSystem::getSuggestedActions() const
{
    return {};
}

void TutorialSystem::setTutorialSteps(const QVector<TutorialStep>& steps)
{
    m_tutorialSteps = steps;
    m_currentStep = 0;
}

void TutorialSystem::showTutorialUI()
{
    if (m_currentTutorial != Welcome) {
        QString stepDescription = m_tutorialSteps[m_currentStep].description;
        QString title = m_tutorialSteps[m_currentStep].title;

        QMessageBox::information(nullptr, title,
                                 "Tutorial step: " + stepDescription + "\n\n" +
                                 "This is a demo tutorial. In the full implementation, " +
                                 "a step-by-step guide would appear here.");
    }
}

// ============================================================================
// QuickStartGuide
// ============================================================================

QuickStartGuide* QuickStartGuide::s_instance = nullptr;

QuickStartGuide::QuickStartGuide(QObject* parent)
    : QObject(parent)
{
}

QuickStartGuide::~QuickStartGuide() {}

QuickStartGuide* QuickStartGuide::instance()
{
    if (!s_instance) s_instance = new QuickStartGuide();
    return s_instance;
}

void QuickStartGuide::startQuickStart()
{
    if (!m_completedSteps.testBit(WelcomeScreen)) {
        completeStep(WelcomeScreen);
        completeStep(CreateNewProject);
    }
}

void QuickStartGuide::completeStep(int stepIndex)
{
    if (stepIndex >= 0 && stepIndex < m_completedSteps.size()) {
        m_completedSteps.setBit(stepIndex);
        emit stepCompleted(stepIndex);
    }
}

bool QuickStartGuide::isStepCompleted(int stepIndex) const
{
    if (stepIndex < 0 || stepIndex >= m_completedSteps.size()) return false;
    return m_completedSteps.testBit(stepIndex);
}

QVector<QString> QuickStartGuide::getQuickStartSteps() const
{
    QVector<QString> steps;
    steps << "Welcome to ksEditor";
    steps << "Create a new project";
    steps << "Import a 3D model";
    steps << "Navigate the 3D viewport";
    steps << "Work with properties";
    steps << "Save your project";
    steps << "Build your project";
    steps << "Explore advanced features";
    return steps;
}

QString QuickStartGuide::getStepDescription(int stepIndex) const
{
    static const QVector<QString> descriptions = {
        "Welcome to ksEditor! This guide will help you get started quickly.",
        "Create a new project by selecting 'New Project' from the Welcome Screen or File menu.",
        "Import 3D models (FBX, OBJ, GLB, KN5) from the Import menu or by dragging files.",
        "Use the 3D viewport to navigate your models. Left-click to orbit, right-click to pan, scroll to zoom.",
        "Use the Properties panel (Alt+2) to modify object properties like position, rotation, and scale.",
        "Save your project regularly using the Save button (Ctrl+S) or File menu.",
        "Build your project to generate final game content packages. Use the Build button or Ctrl+B.",
        "Explore advanced features like physics setup, audio editing, and more modules."
    };

    if (stepIndex >= 0 && stepIndex < descriptions.size()) {
        return descriptions[stepIndex];
    }
    return "";
}

QString QuickStartGuide::getStepTip(int stepIndex) const
{
    static const QVector<QString> tips = {
        "You can access recent projects from the Welcome Screen or File \u2192 Recent Projects menu.",
        "When creating a car project, ensure you have the original Assetto Corsa cars folder available.",
        "ksEditor supports common 3D formats. FBX is recommended for game assets.",
        "Use the mouse wheel + Alt to zoom without panning. Ctrl+scroll for perspective changes.",
        "Right-click objects in the Object List to access context menu actions like delete, hide, lock.",
        "Auto-save is enabled by default (every 5 minutes). Check the status bar for auto-save notifications.",
        "Build failures may be due to missing dependencies. Check the Output panel for error details.",
        "Shortcut: F1 for context help, F12 for script console, Ctrl+Shift+F for file search."
    };

    if (stepIndex >= 0 && stepIndex < tips.size()) {
        return tips[stepIndex];
    }
    return "";
}

bool QuickStartGuide::requiresToolTip(int stepIndex) const
{
    static const QVector<bool> requiresTip = {true, false, true, true, false, false, true, false};

    if (stepIndex >= 0 && stepIndex < requiresTip.size()) {
        return requiresTip[stepIndex];
    }
    return false;
}

// ============================================================================
// HelpSystem
// ============================================================================

HelpSystem::HelpSystem(QObject* parent)
    : QObject(parent)
{
}

HelpSystem::~HelpSystem() {}

HelpSystem* HelpSystem::instance()
{
    if (!s_instance) s_instance = new HelpSystem();
    return s_instance;
}

void HelpSystem::initialize(QWidget* parent)
{
    m_toolTipManager = HelpToolTipManager::instance();
    m_toolTipManager->initialize(parent);

    m_tutorialSystem = TutorialSystem::instance();
    m_quickStart = QuickStartGuide::instance();
}

void HelpSystem::registerHelp(const QString& context, const QString& element,
                               const QString& description, const QString& shortcut)
{
    if (m_toolTipManager) {
        m_toolTipManager->addToolTip(context, element, description, shortcut);
    }
}

void HelpSystem::showHelpForWidget(QWidget* widget)
{
    Q_UNUSED(widget)
}

void HelpSystem::startTutorial(TutorialSystem::TutorialPage page)
{
    if (m_tutorialSystem) {
        m_tutorialSystem->startTutorial(page);
    }
}

void HelpSystem::startQuickStartGuide()
{
    if (m_quickStart) {
        m_quickStart->startQuickStart();
    }
}

bool HelpSystem::hasHelpEnabled() const
{
    return m_helpEnabled;
}

void HelpSystem::enableHelp(bool enabled)
{
    m_helpEnabled = enabled;
}

} // namespace ks
