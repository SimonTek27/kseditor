#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QStandardPaths>
#include <QQmlEngine>
#include <QJSEngine>
#include <QQuickStyle>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include <QDebug>
#include <exception>

#ifdef _WIN32
#include <windows.h>
#endif

#include "MainWindow.h"
#include "core/sys/ModuleManager.h"
#include "core/sys/PluginManager.h"
#include "core/ui/SplashScreen.h"
#include "core/ui/WelcomeScreen.h"
#include "core/ui/NewProjectDialog.h"
#include "core/ui/FontEditorDialog.h"

// QML Bridge registrations
#include "modules/modellingEditor/3DModelingQmlBridge.h"
#include "modules/modellingEditor/BoolOpQmlBridge.h"
#include "modules/modellingEditor/SymmetryQmlBridge.h"
#include "modules/PhysicsEditor/PhysicsQmlBridge.h"
#include "core/Audio/AudioQMLBridge.h"
#include "core/Audio/AudioEngineQML.h"
#include "core/Audio/KSAudioModuleBridge.h"
#include "core/AIEditor/AIEditorQmlBridge.h"
#include "core/tools/FormatToolsQmlBridge.h"
#include "core/modmanager/ModManagerQmlBridge.h"
#include "core/ppfiltersEditor/PPFiltersQmlBridge.h"
#include "core/3dprint/ThreeDPrintQmlBridge.h"
#include "core/FfbEditor/FfbEditorQmlBridge.h"
#include "modules/displayEditor/DisplayEditorQmlBridge.h"
#include "modules/LicensePlatesEditor/LicensePlatesQmlBridge.h"
#include "modules/fontEditor/FontCreatorQmlBridge.h"
#include "modules/soundEditor/AudioEffectsQmlBridge.h"
#include "modules/ShowroomEditor/ShowroomEditorQmlBridge.h"
#include "modules/PhysicsEditor/telemetry/TelemetryQmlBridge.h"
#include "modules/PhysicsEditor/telemetry/TelemetryFeedbackBridge.h"
#include "modules/PhysicsEditor/SetupEditor/SetupEditorQmlBridge.h"
#include "modules/PhysicsEditor/PhysicsProfiler.h"
#include "modules/PhysicsEditor/LapTimeValidation.h"
#include "modules/modellingEditor/TrackBuilder/TerrainEditorQmlBridge.h"
#include "core/assets/AssetsLibraryQmlBridge.h"
#include "core/mesh/MeshDataBridge.h"
#include "core/material/TexturePaintQmlBridge.h"
#include "core/3dprint/ThreeDPrintQmlBridge.h"
#include "core/mesh/MeshLoaderQML.h"
#include "modules/modellingEditor/SceneMeshGeometry.h"
#include "qml/modules/CspConfigQmlBridge.h"
#include "qml/modules/KsContentQMLBridge.h"
#include "modules/modellingEditor/CharacterBuilder/CharacterEditorQmlBridge.h"
#include "core/network/CollabEditorQmlBridge.h"


static int runMainWindow(QApplication& app, const QString& projectPath)
{
    qDebug() << "Creating MainWindow with projectPath:" << projectPath;
    MainWindow window(projectPath);
    window.show();

    QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
        ks::PluginManager::instance()->saveLoadedList();
    });

    return app.exec();
}

static int appMain(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("ksEditor");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("ksEditor");

    // Load translations
    QTranslator qtTranslator;
    if (qtTranslator.load("qt_" + QLocale::system().name(),
            QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        app.installTranslator(&qtTranslator);
    }

    QTranslator appTranslator;
    QString lang = QLocale::system().name(); // e.g. "de", "it", "ja"
    QSettings s;
    if (s.contains("ui/language")) {
        QString savedLang = s.value("ui/language").toString();
        if (savedLang != "system")
            lang = savedLang;
    }
    QStringList searchPaths = {
        QCoreApplication::applicationDirPath() + "/i18n",
        QCoreApplication::applicationDirPath() + "/../i18n",
        ":/i18n"
    };
    for (const QString& path : searchPaths) {
        if (appTranslator.load(path + "/kseditor_" + lang)) {
            app.installTranslator(&appTranslator);
            break;
        }
    }

    // Register QML bridge types
    qmlRegisterSingletonType<ks::KSModelerQml>("ksEditor.Modeler", 1, 0, "Modeler",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return &ks::KSModelerQml::instance(); });
    qmlRegisterSingletonType<ks::PhysicsQmlBridge>("ksEditor.Physics", 1, 0, "Physics",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::PhysicsQmlBridge::instance(); });
    qmlRegisterSingletonType<AudioQMLBridge>("ksEditor.Audio", 1, 0, "AudioBridge",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return AudioQMLBridge::instance(); });
    qmlRegisterSingletonType<ks::AudioEngineQML>("ksEditor.AudioEngine", 1, 0, "AudioEngine",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::AudioEngineQML::instance(); });
    qmlRegisterSingletonType<ks::MeshDataBridge>("ksEditor.MeshData", 1, 0, "MeshData",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::MeshDataBridge::instance(); });
    qmlRegisterSingletonType<ks::AIEditorQmlBridge>("ksEditor.AIEditor", 1, 0, "AIEditor",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::AIEditorQmlBridge::instance(); });
    qmlRegisterSingletonType<ks::FormatToolsQmlBridge>("ksEditor.FormatTools", 1, 0, "FormatTools",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::FormatToolsQmlBridge::instance(); });
    // Plugin registers "ksEditor.Workshop" singleton
    qmlRegisterSingletonType<ks::ModManagerQmlBridge>("ksEditor.ModManager", 1, 0, "ModManager",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::ModManagerQmlBridge::instance(); });
    qmlRegisterSingletonType<ks::TelemetryQmlBridge>("ksEditor.Telemetry", 1, 0, "Telemetry",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::TelemetryQmlBridge::instance(); });
    qmlRegisterSingletonType<ks::TelemetryFeedbackBridge>("ksEditor.TelemetryFeedback", 1, 0, "TelemetryFeedback",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::TelemetryFeedbackBridge::instance(); });
    qmlRegisterSingletonType<ks::TexturePaintQmlBridge>("ksEditor.TexturePainter", 1, 0, "TexturePainter",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::TexturePaintQmlBridge::instance(); });
    qmlRegisterSingletonType<ks::SetupEditorQmlBridge>("ksEditor.SetupEditor", 1, 0, "SetupEditor",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::SetupEditorQmlBridge::instance(); });
    qmlRegisterSingletonType<ks::PPFiltersQmlBridge>("ksEditor.PPFilters", 1, 0, "PPFilters",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::PPFiltersQmlBridge::instance(); });
    qmlRegisterSingletonType<ks::DisplayEditorQmlBridge>("ksEditor.DisplayEditor", 1, 0, "DisplayEditor",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::DisplayEditorQmlBridge::instance(); });
    qmlRegisterSingletonType<ks::LicensePlatesQmlBridge>("ksEditor.LicensePlates", 1, 0, "LicensePlates",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::LicensePlatesQmlBridge::instance(); });
    qmlRegisterSingletonType<ks::FontCreatorQmlBridge>("ksEditor.FontCreator", 1, 0, "FontCreator",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::FontCreatorQmlBridge::instance(); });
    qmlRegisterSingletonType<ks::TerrainEditorQmlBridge>("ksEditor.TerrainEditor", 1, 0, "TerrainEditor",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::TerrainEditorQmlBridge::instance(); });
    qmlRegisterSingletonType<ks::FfbEditorQmlBridge>("ksEditor.FfbEditor", 1, 0, "FfbEditor",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::FfbEditorQmlBridge::instance(); });
    qmlRegisterSingletonType<ks::ShowroomEditorQmlBridge>("ksEditor.ShowroomEditor", 1, 0, "ShowroomEditor",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::ShowroomEditorQmlBridge::instance(); });
    qmlRegisterSingletonType<ks::AssetsLibraryQmlBridge>("ksEditor.AssetsLibrary", 1, 0, "AssetsLibrary",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::AssetsLibraryQmlBridge::instance(); });
    qmlRegisterSingletonType<ks::audio::AudioEffectsQmlBridge>("ksEditor.AudioEffects", 1, 0, "AudioEffects",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::audio::AudioEffectsQmlBridge::instance(); });
    qmlRegisterSingletonType<ks::PhysicsProfiler>("ksEditor.PhysicsProfiler", 1, 0, "PhysicsProfiler",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::PhysicsProfiler::instance(); });
    qmlRegisterSingletonType<ks::LapTimeValidation>("ksEditor.LapTimeValidation", 1, 0, "LapTimeValidation",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::LapTimeValidation::instance(); });

    // Register non-singleton QML types
    qmlRegisterType<ks::audio::KSAudioModuleBridge>("ksEditor.AudioModule", 1, 0, "AudioModule");
    qmlRegisterSingletonType<ks::CollabEditorQmlBridge>("ksEditor.Collaboration", 1, 0, "CollabEditor",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::CollabEditorQmlBridge::instance(); });
    qmlRegisterSingletonType<ks::CspConfigQmlBridge>("ksEditor.CspConfig", 1, 0, "CspConfig",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return ks::CspConfigQmlBridge::instance(); });
    qmlRegisterType<KsContentQMLBridge>("ksEditor.Content", 1, 0, "Content");
    qmlRegisterType<MeshLoaderQML>("ksEditor.MeshLoader", 1, 0, "MeshLoader");
    qmlRegisterType<ks::SceneMeshGeometry>("ksEditor.Modeler", 1, 0, "SceneMeshGeometry");
    qmlRegisterSingletonType<ks::editor::BoolOpQmlBridge>("ksEditor.BoolOp", 1, 0, "BoolOp",
        [](QQmlEngine*, QJSEngine*) -> QObject* {
            static ks::editor::BoolOpQmlBridge* bridge = new ks::editor::BoolOpQmlBridge();
            return bridge;
        });
    qmlRegisterSingletonType<ks::SymmetryQmlBridge>("ksEditor.Symmetry", 1, 0, "Symmetry",
        [](QQmlEngine*, QJSEngine*) -> QObject* {
            return &ks::SymmetryQmlBridge::instance();
        });
    qmlRegisterSingletonType<ks::CharacterEditorQmlBridge>("ksEditor.Character", 1, 0, "CharacterEditor",
        [](QQmlEngine*, QJSEngine*) -> QObject* {
            return ks::CharacterEditorQmlBridge::instance();
        });

    // Register 3D Printing module
    qmlRegisterType<ks::printing::ThreeDPrintQmlBridge>("ksEditor.Printing", 1, 0, "PrintManager");

    // Initialize plugin system
    ks::PluginManager::instance()->scan();

    // Load modern dark theme
    QFile qssFile(":/ui/styles/dark.qss");
    if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(qssFile.readAll()));
        qssFile.close();
    }

    QStringList args;
    for (int i = 0; i < argc; ++i) args << QString(argv[i]);

    if (args.size() > 1) {
        QString cmd = args[1].toLower();
        if (cmd == "-font" || cmd == "--font") {
            SplashScreen::showSplash(app);
            FontEditorDialog fe(nullptr);
            fe.exec();
            return 0;
        }
        if (cmd == "-h" || cmd == "--help") {
            QMessageBox::information(nullptr, "ksEditor Help",
                "ksEditor 1.0.0 - Assetto Corsa Modding Suite\n\n"
                "Usage: kseditor.exe [options]\n\n"
                "Options:\n"
                "  -font, --font    Open font editor directly\n"
                "  -nohw, --nohw    Disable hardware acceleration\n"
                "  -h, --help       Show this help message\n"
                "  <file.ksproj>    Open a project file");
            return 0;
        }
        if (args[1].endsWith(".ksproj", Qt::CaseInsensitive)) {
            QFile file(args[1]);
            if (file.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                file.close();
                if (!doc.isNull() && doc.isObject()) {
                    QString projectPath = doc.object()["path"].toString();
                    if (!projectPath.isEmpty() && QDir(projectPath).exists()) {
                        SplashScreen::showSplash(app);
                        MainWindow window(projectPath);
                        window.show();
                        return app.exec();
                    }
                }
            }
        }
    }

    SplashScreen::showSplash(app);

    qDebug() << "Showing WelcomeScreen...";
    WelcomeScreen welcome;
    bool accepted = false;
    try {
        accepted = (welcome.exec() == QDialog::Accepted);
    } catch (...) {
        qDebug() << "Exception during WelcomeScreen::exec()";
        accepted = false;
    }

    if (!accepted) {
        qDebug() << "WelcomeScreen rejected/closed, exiting";
        return 0;
    }

    qDebug() << "WelcomeScreen accepted, action:" << welcome.selectedAction;

    QString projectPath;
    try {
        if (welcome.selectedAction == WelcomeScreen::New) {
            NewProjectDialog newDlg;
            if (newDlg.exec() == QDialog::Accepted && !newDlg.projectPath.isEmpty()) {
                projectPath = newDlg.projectPath;
            }
        } else if (welcome.selectedAction == WelcomeScreen::NewBlank) {
            // Start with blank project - MainWindow will show with empty project
        } else if (welcome.selectedAction == WelcomeScreen::Open) {
            projectPath = QFileDialog::getExistingDirectory(nullptr,
                "Open Project Folder",
                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        } else if (welcome.selectedAction == WelcomeScreen::Recent) {
            if (!welcome.recentPath.isEmpty() && QDir(welcome.recentPath).exists()) {
                projectPath = welcome.recentPath;
            }
        } else if (welcome.selectedAction == WelcomeScreen::Help) {
            // Show help - for now just start with empty project
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Error",
            QString("An error occurred while processing your selection:\n%1").arg(e.what()));
        return 1;
    } catch (...) {
        QMessageBox::critical(nullptr, "Error",
            "An unknown error occurred while processing your selection.");
        return 1;
    }

    try {
        return runMainWindow(app, projectPath);
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Fatal Error",
            QString("A fatal error occurred during startup:\n\n%1\n\n"
                    "Please check the application log for details.").arg(e.what()));
        return 1;
    } catch (...) {
        QMessageBox::critical(nullptr, "Fatal Error",
            "An unknown fatal error occurred during startup.\n\n"
            "Please check the application log for details.");
        return 1;
    }
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    __try {
        return appMain(argc, argv);
    } __except(1) {
        QMessageBox::critical(nullptr, "Fatal Error",
            "A system error (access violation) occurred.\n\n"
            "The application encountered a critical error and must close.");
        return 1;
    }
#else
    return appMain(argc, argv);
#endif
}

#ifdef _WIN32
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    extern int __argc;
    extern char** __argv;
    return main(__argc, __argv);
}
#endif
