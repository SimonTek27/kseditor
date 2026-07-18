#include "HelpSystem.h"
#include "HelpBrowser.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QApplication>
#include <QToolTip>
#include <QCheckBox>
#include <QSettings>

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
    m_toolTips[context][element] = std::make_pair(description, shortcut);
}

QMap<QString, QMap<QString, QString>> HelpToolTipManager::getToolTips() const
{
    QMap<QString, QMap<QString, QString>> result;
    for (auto it = m_toolTips.constBegin(); it != m_toolTips.constEnd(); ++it) {
        QMap<QString, QString> inner;
        for (auto jt = it.value().constBegin(); jt != it.value().constEnd(); ++jt) {
            inner[jt.key()] = jt.value().first;
        }
        result[it.key()] = inner;
    }
    return result;
}

QJsonObject HelpToolTipManager::createToolTip(const QString& context, const QString& element) const
{
    auto ctxIt = m_toolTips.constFind(context);
    if (ctxIt == m_toolTips.constEnd()) return {};
    auto elemIt = ctxIt.value().constFind(element);
    if (elemIt == ctxIt.value().constEnd()) return {};
    QJsonObject obj;
    obj["context"] = context;
    obj["element"] = element;
    obj["description"] = elemIt.value().first;
    if (!elemIt.value().second.isEmpty())
        obj["shortcut"] = elemIt.value().second;
    return obj;
}

// ============================================================================
// HelpToolTipContextTracker
// ============================================================================

HelpToolTipContextTracker* HelpToolTipContextTracker::s_instance = nullptr;

HelpToolTipContextTracker::HelpToolTipContextTracker(QWidget* parent)
    : QWidget(parent)
{
    s_instance = this;
}

HelpToolTipContextTracker::~HelpToolTipContextTracker()
{
    if (s_instance == this) s_instance = nullptr;
}

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
    m_widgetMap[widget] = std::make_pair(element, context.isEmpty() ? m_currentContext : context);
}

QJsonObject HelpToolTipContextTracker::getToolTipForWidget(QWidget* widget) const
{
    if (!widget) return {};
    auto it = m_widgetMap.constFind(widget);
    if (it == m_widgetMap.constEnd()) return {};
    const QString& element = it.value().first;
    const QString& context = it.value().second;
    return HelpToolTipManager::instance()->createToolTip(context, element);
}

void HelpToolTipContextTracker::mouseMoveEvent(QMouseEvent* event)
{
    QWidget* child = childAt(event->pos());
    if (child) {
        QJsonObject tip = getToolTipForWidget(child);
        if (!tip.isEmpty()) {
            QString text = tip["description"].toString();
            QString shortcut = tip["shortcut"].toString();
            if (!shortcut.isEmpty())
                text += "\nShortcut: " + shortcut;
            QToolTip::showText(event->globalPos(), text, this);
        } else {
            QToolTip::hideText();
        }
    }
    QWidget::mouseMoveEvent(event);
}

void HelpToolTipContextTracker::leaveEvent(QEvent* event)
{
    QToolTip::hideText();
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
    setupPageContent(Welcome);
}

TutorialSystem::~TutorialSystem() {}

TutorialSystem* TutorialSystem::instance()
{
    if (!s_instance) s_instance = new TutorialSystem();
    return s_instance;
}

void TutorialSystem::setupPageContent(TutorialPage page)
{
    m_tutorialSteps.clear();
    m_currentStep = 0;

    switch (page) {
    case Welcome:
        m_tutorialSteps << TutorialStep{
            "Welcome to ksEditor",
            "Welcome to ksEditor! This editor provides professional tools for modding Assetto Corsa vehicles, tracks, and audio.",
            "welcome_screen",
            {"Click 'New Project' to start fresh", "Click 'Open Project' to load an existing project", "Explore the module selector on the left"}
        };
        break;
    case QuickStart:
        m_tutorialSteps << TutorialStep{
            "Quick Start",
            "Let's quickly get you started by creating a simple car project.",
            "project_wizard",
            {"Click 'New Project'", "Select 'Car' from the module list", "Name your project"}
        };
        m_tutorialSteps << TutorialStep{
            "Import 3D Model",
            "Import a 3D model. ksEditor supports FBX, OBJ, GLB, and KN5 formats.",
            "import_button",
            {"Click 'Import 3D...' in the file menu", "Select your 3D model file", "Wait for the model to load"}
        };
        m_tutorialSteps << TutorialStep{
            "Navigate 3D Viewport",
            "Use your mouse to navigate. Left-click to orbit, right-click to pan, scroll to zoom.",
            "viewport",
            {"Left-click and drag to rotate view", "Right-click and drag to pan view", "Scroll wheel to zoom"}
        };
        break;
    case CreatingProject:
        m_tutorialSteps << TutorialStep{
            "Creating a Project",
            "Projects organize your work. Each can contain a car, track, or audio configuration.",
            "new_project_dialog",
            {"Open File -> New Project", "Choose type (Car / Track / Audio)", "Set project name and location", "Click Create"}
        };
        break;
    case Importing3D:
        m_tutorialSteps << TutorialStep{
            "Importing 3D Models",
            "Import models in various formats. ksEditor converts them automatically.",
            "import_dialog",
            {"Open File -> Import -> 3D Model", "Select FBX, OBJ, GLB, KN5, or DAE file", "Adjust import settings", "Click Import"}
        };
        m_tutorialSteps << TutorialStep{
            "Import Settings",
            "Configure scale, smoothing, and material handling during import.",
            "import_settings",
            {"Set unit scale (centimeters/meters)", "Choose smoothing groups", "Select material import behavior"}
        };
        break;
    case WorkingWith3D:
        m_tutorialSteps << TutorialStep{
            "Working with 3D Models",
            "Use the viewport and property panels to edit your model.",
            "viewport",
            {"Select objects in the viewport or Object List", "Use transform gizmo (W/E/R) to move/rotate/scale", "Edit materials in Properties panel", "Use Boolean operations for CSG modeling"}
        };
        m_tutorialSteps << TutorialStep{
            "Material Editor",
            "Edit PBR materials with base color, metallic, roughness, and normal maps.",
            "material_editor",
            {"Select a mesh with a material", "Adjust base color in Properties panel", "Set metallic and roughness values", "Assign texture maps"}
        };
        break;
    case Exporting:
        m_tutorialSteps << TutorialStep{
            "Exporting Your Work",
            "Export your project to game-ready formats.",
            "export_dialog",
            {"Open File -> Export", "Choose format (KN5, FBX, OBJ)", "Configure export options", "Click Export"}
        };
        break;
    case Settings:
        m_tutorialSteps << TutorialStep{
            "Editor Settings",
            "Configure ksEditor to your preferences.",
            "settings_dialog",
            {"Open Edit -> Preferences", "Adjust font size and theme", "Configure rendering settings", "Set hotkeys"}
        };
        break;
    case AdvancedFeatures:
        m_tutorialSteps << TutorialStep{
            "Advanced Features",
            "Explore AI spline editing, physics setup, audio design, and scripting.",
            "advanced_menu",
            {"Open a Track project and edit AI splines", "Use Physics Editor for car setup", "Open Sound Editor for audio design", "Use Script Console (F12) for automation"}
        };
        break;
    case Troubleshooting:
        m_tutorialSteps << TutorialStep{
            "Troubleshooting",
            "Common issues and how to resolve them.",
            "troubleshooting",
            {"Check Output panel for error details", "Verify file paths and permissions", "Ensure all dependencies are installed", "Consult the documentation or community forums"}
        };
        break;
    }
}

void TutorialSystem::startTutorial(TutorialPage page)
{
    if (page < Welcome || page > Troubleshooting) return;

    m_currentTutorial = page;
    m_tutorialStarted = true;
    setupPageContent(page);

    showTutorialUI();

    emit tutorialStarted(page);
}

void TutorialSystem::showHelpTopic(const QString& topicId)
{
    emit helpRequested(topicId);
}

bool TutorialSystem::hasTutorialStarted() const
{
    return m_tutorialStarted;
}

TutorialSystem::TutorialPage TutorialSystem::currentTutorial() const
{
    return m_currentTutorial;
}

QVector<QString> TutorialSystem::getSuggestedActions() const
{
    if (m_currentStep < 0 || m_currentStep >= m_tutorialSteps.size())
        return {};
    return m_tutorialSteps[m_currentStep].actions;
}

void TutorialSystem::setTutorialSteps(const QVector<TutorialStep>& steps)
{
    m_tutorialSteps = steps;
    m_currentStep = 0;
}

void TutorialSystem::showTutorialUI()
{
    if (m_tutorialSteps.isEmpty()) return;

    QDialog dialog;
    dialog.setWindowTitle("Tutorial");
    dialog.setMinimumSize(520, 380);
    dialog.setModal(true);

    auto* layout = new QVBoxLayout(&dialog);

    auto* header = new QLabel(QString("Tutorial: Step %1 of %2")
        .arg(m_currentStep + 1).arg(m_tutorialSteps.size()));
    header->setStyleSheet("font-weight: bold; font-size: 14px; color: #4a90d9;");
    layout->addWidget(header);

    auto* title = new QLabel(m_tutorialSteps[m_currentStep].title);
    title->setStyleSheet("font-size: 18px; font-weight: bold; margin: 8px 0;");
    title->setWordWrap(true);
    layout->addWidget(title);

    auto* desc = new QLabel(m_tutorialSteps[m_currentStep].description);
    desc->setWordWrap(true);
    desc->setStyleSheet("font-size: 13px; margin-bottom: 12px;");
    layout->addWidget(desc);

    auto* actionsLabel = new QLabel("<b>Steps to follow:</b>");
    layout->addWidget(actionsLabel);

    QString actionsHtml;
    const auto& actions = m_tutorialSteps[m_currentStep].actions;
    for (int i = 0; i < actions.size(); ++i) {
        actionsHtml += QString("<li>%1</li>").arg(actions[i]);
    }
    auto* actionsText = new QLabel("<ul>" + actionsHtml + "</ul>");
    actionsText->setWordWrap(true);
    layout->addWidget(actionsText);

    layout->addStretch();

    auto* progressBar = new QProgressBar();
    progressBar->setRange(0, m_tutorialSteps.size());
    progressBar->setValue(m_currentStep + 1);
    progressBar->setTextVisible(true);
    layout->addWidget(progressBar);

    auto* buttonLayout = new QHBoxLayout();
    auto* prevBtn = new QPushButton("Previous");
    prevBtn->setEnabled(m_currentStep > 0);
    auto* nextBtn = new QPushButton(m_currentStep < m_tutorialSteps.size() - 1 ? "Next" : "Finish");
    auto* closeBtn = new QPushButton("Close");

    buttonLayout->addWidget(prevBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);
    buttonLayout->addWidget(nextBtn);
    layout->addLayout(buttonLayout);

    QObject::connect(prevBtn, &QPushButton::clicked, [&]() {
        if (m_currentStep > 0) {
            m_currentStep--;
            dialog.accept();
            showTutorialUI();
        }
    });

    QObject::connect(nextBtn, &QPushButton::clicked, [&]() {
        if (m_currentStep < m_tutorialSteps.size() - 1) {
            m_currentStep++;
            dialog.accept();
            showTutorialUI();
        } else {
            m_tutorialStarted = false;
            emit tutorialCompleted(m_currentTutorial);
            dialog.accept();
        }
    });

    QObject::connect(closeBtn, &QPushButton::clicked, [&]() {
        dialog.accept();
    });

    dialog.exec();
}

// ============================================================================
// QuickStartGuide
// ============================================================================

QuickStartGuide* QuickStartGuide::s_instance = nullptr;
HelpSystem* HelpSystem::s_instance = nullptr;

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
    QSettings settings("ksEditor", "ksEditor");
    if (!settings.value("quickStart/showOnStartup", true).toBool())
        return;

    QDialog dialog;
    dialog.setWindowTitle("Quick Start Guide");
    dialog.setMinimumSize(480, 420);
    dialog.setModal(true);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setSpacing(12);

    auto* title = new QLabel("Quick Start Guide");
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #4a90d9;");
    layout->addWidget(title);

    auto* subtitle = new QLabel("Follow these steps to get started with ksEditor:");
    subtitle->setWordWrap(true);
    layout->addWidget(subtitle);

    const int totalSteps = getQuickStartSteps().size();
    for (int i = 0; i < totalSteps; ++i) {
        auto* stepLabel = new QLabel(QString("<b>%1.</b> %2")
            .arg(i + 1).arg(getStepDescription(i)));
        stepLabel->setWordWrap(true);
        if (isStepCompleted(i))
            stepLabel->setStyleSheet("color: #888;");
        layout->addWidget(stepLabel);
    }

    auto* dontShowAgain = new QCheckBox("Don't show this again");
    dontShowAgain->setChecked(false);
    layout->addWidget(dontShowAgain);

    layout->addStretch();

    auto* skipBtn = new QPushButton("Skip");
    auto* startBtn = new QPushButton("Start Tutorial");

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(skipBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(startBtn);
    layout->addLayout(buttonLayout);

    QObject::connect(startBtn, &QPushButton::clicked, [&]() {
        if (dontShowAgain->isChecked())
            settings.setValue("quickStart/showOnStartup", false);
        TutorialSystem::instance()->startTutorial(TutorialSystem::QuickStart);
        dialog.accept();
    });

    QObject::connect(skipBtn, &QPushButton::clicked, [&]() {
        if (dontShowAgain->isChecked())
            settings.setValue("quickStart/showOnStartup", false);
        for (int i = 0; i < totalSteps; ++i)
            completeStep(i);
        dialog.accept();
    });

    dialog.exec();
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

void HelpSystem::showHelp()
{
    HelpBrowser* browser = new HelpBrowser();
    browser->show();
}

void HelpSystem::showHelpForWidget(QWidget* widget)
{
    if (!widget || !m_helpEnabled) return;

    if (auto* tracker = qobject_cast<HelpToolTipContextTracker*>(widget->parent())) {
        QJsonObject tip = tracker->getToolTipForWidget(widget);
        if (!tip.isEmpty()) {
            QString text = tip["description"].toString();
            QString shortcut = tip["shortcut"].toString();
            if (!shortcut.isEmpty())
                text += "\n\nShortcut: " + shortcut;
            QMessageBox::information(widget, "Help - " + tip["element"].toString(), text);
        }
    } else {
        QWidget* p = widget->parentWidget();
        while (p) {
            if (auto* tracker = qobject_cast<HelpToolTipContextTracker*>(p)) {
                QJsonObject tip = tracker->getToolTipForWidget(widget);
                if (!tip.isEmpty()) {
                    QString text = tip["description"].toString();
                    QMessageBox::information(widget, "Help - " + tip["element"].toString(), text);
                }
                return;
            }
            p = p->parentWidget();
        }
    }
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
