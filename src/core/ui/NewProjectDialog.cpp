#include "NewProjectDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QSettings>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>

NewProjectDialog::NewProjectDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("New Project");
    setMinimumSize(550, 500);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setupUI();
    showStep1();
}

void NewProjectDialog::onProjectTypeSelected() {
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        selectedType = static_cast<ProjectType>(btn->property("projectType").toInt());
        selectedSubType = Car;
        m_nextBtn->setEnabled(!m_nameEdit->text().isEmpty());
    }
}

void NewProjectDialog::onNameTextChanged(const QString& text) {
    projectName = text;
    m_nextBtn->setEnabled(!text.isEmpty() && selectedType != None);
}

void NewProjectDialog::onPreviousClicked() {
    if (m_currentStep == 2) {
        showStep1();
    }
}

void NewProjectDialog::onCreateClicked() {
    if (projectName.isEmpty()) return;

    QString acRoot = detectAcRoot();
    QString basePath;
    QString moddevRoot;

    if (!acRoot.isEmpty()) {
        basePath = acRoot + "/moddev/content";
        moddevRoot = acRoot + "/moddev";
        QDir().mkpath(basePath);
    } else {
        QMessageBox::StandardButton btn = QMessageBox::question(this,
            "Assetto Corsa Not Found",
            "Could not detect Assetto Corsa installation.\n\nDo you want to select a custom location?",
            QMessageBox::Yes | QMessageBox::No);

        if (btn == QMessageBox::Yes) {
            basePath = QFileDialog::getExistingDirectory(this,
                "Select Project Location",
                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
            if (basePath.isEmpty()) return;
            moddevRoot = QFileInfo(basePath).absolutePath();
        } else {
            basePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/ksProjects";
            QDir().mkpath(basePath);
            moddevRoot = basePath;
        }
    }

    QString contentSubfolder;
    if (selectedType == NewObject3D) {
        contentSubfolder = "objects3d";
    } else if (selectedType == NewShowroom) {
        contentSubfolder = "showroom";
    } else if (selectedSubType == Car) {
        contentSubfolder = "cars";
    } else if (selectedSubType == Track) {
        contentSubfolder = "tracks";
    } else if (selectedSubType == Character) {
        contentSubfolder = "drivers";
    } else if (selectedSubType == Object3D) {
        contentSubfolder = "objects3d";
    } else if (selectedSubType == Showroom) {
        contentSubfolder = "showroom";
    } else if (selectedType == NewFont) {
        contentSubfolder = "fonts";
    }

    if (!contentSubfolder.isEmpty()) {
        projectPath = basePath + "/" + contentSubfolder + "/" + projectName;
    } else {
        projectPath = basePath + "/" + projectName;
    }

    if (QDir(projectPath).exists()) {
        QMessageBox::StandardButton btn = QMessageBox::question(this,
            "Project Exists",
            "A project named '" + projectName + "' already exists.\n\nOverwrite?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (btn == QMessageBox::Yes) {
            QDir(projectPath).removeRecursively();
        } else if (btn == QMessageBox::No) {
            projectName += "_copy";
            projectPath = basePath + "/" + contentSubfolder + "/" + projectName;
        } else {
            return;
        }
    }

    QDir().mkpath(projectPath);
    createProjectStructure();

    QString projectFilePath = moddevRoot + "/" + projectName + ".ksproj";
    QJsonObject projectData;
    projectData["name"] = projectName;
    projectData["version"] = "1.0";
    projectData["type"] = contentSubfolder;
    projectData["path"] = projectPath;
    projectData["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    projectData["author"] = "";

    QFile projectFile(projectFilePath);
    if (projectFile.open(QIODevice::WriteOnly)) {
        projectFile.write(QJsonDocument(projectData).toJson(QJsonDocument::Indented));
        projectFile.close();
        qDebug() << "Project file created:" << projectFilePath;
    }

    QMessageBox::information(this, "Project Created",
        "Project created successfully at:\n" + projectPath);
    accept();
}

void NewProjectDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);

    m_header = new QLabel("Create New Project");
    m_header->setStyleSheet("background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #007acc,stop:1 #005a9e); color: white; font-size: 18px; font-weight: bold; padding: 20px;");
    m_header->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_header);

    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet("background: #252526;");
    m_mainLayout->addWidget(m_contentWidget);

    QWidget* footer = new QWidget();
    footer->setStyleSheet("background: #1e1e1e; padding: 15px;");
    QHBoxLayout* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(20, 0, 20, 0);

    m_prevBtn = new QPushButton("< BACK");
    m_prevBtn->setEnabled(false);
    m_prevBtn->setStyleSheet("QPushButton { background: #3e3e42; color: white; border: 1px solid #555; padding: 10px 20px; border-radius: 4px; } QPushButton:hover { background: #4e4e52; } QPushButton:disabled { color: #666; }");
    connect(m_prevBtn, &QPushButton::clicked, this, &NewProjectDialog::onPreviousClicked);
    footerLayout->addWidget(m_prevBtn);

    footerLayout->addStretch();

    m_nextBtn = new QPushButton("CREATE");
    m_nextBtn->setEnabled(false);
    m_nextBtn->setStyleSheet("QPushButton { background: #007acc; color: white; border: none; padding: 10px 20px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background: #005a9e; } QPushButton:disabled { background: #3e3e42; color: #666; }");
    connect(m_nextBtn, &QPushButton::clicked, this, &NewProjectDialog::onCreateClicked);
    footerLayout->addWidget(m_nextBtn);

    QPushButton* cancelBtn = new QPushButton("CANCEL");
    cancelBtn->setStyleSheet("QPushButton { background: #3e3e42; color: white; border: 1px solid #555; padding: 10px 20px; border-radius: 4px; } QPushButton:hover { background: #4e4e52; }");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    footerLayout->addWidget(cancelBtn);

    m_mainLayout->addWidget(footer);
}

void NewProjectDialog::createProjectStructure() {
    QDir().mkpath(projectPath);

    if (selectedType == NewModel || selectedType == NewPhysics || selectedType == NewSound || selectedType == NewSkin) {
        if (selectedSubType == Car) {
            QDir().mkpath(projectPath + "/animations");
            QDir().mkpath(projectPath + "/sfx");
            QDir().mkpath(projectPath + "/skins");
            QDir().mkpath(projectPath + "/texture");
            QDir().mkpath(projectPath + "/ui");
            QDir().mkpath(projectPath + "/models");
            createIniFile(projectPath + "/data.ini", "[general]\nname=" + projectName + "\ntype=car\n");
        } else if (selectedSubType == Track) {
            QDir().mkpath(projectPath + "/ai");
            QDir().mkpath(projectPath + "/data");
            QDir().mkpath(projectPath + "/models");
            QDir().mkpath(projectPath + "/skins");
            QDir().mkpath(projectPath + "/texture");
            QDir().mkpath(projectPath + "/ui");
            createIniFile(projectPath + "/data.ini", "[general]\nname=" + projectName + "\ntype=track\n");
        } else if (selectedSubType == Character) {
            QDir().mkpath(projectPath + "/meshes");
            QDir().mkpath(projectPath + "/texture");
            QDir().mkpath(projectPath + "/ui");
            createIniFile(projectPath + "/data.ini", "[general]\nname=" + projectName + "\ntype=driver\n");
        } else if (selectedSubType == Object3D) {
            QDir().mkpath(projectPath + "/meshes");
            QDir().mkpath(projectPath + "/texture");
            QDir().mkpath(projectPath + "/materials");
            QDir().mkpath(projectPath + "/animations");
            createIniFile(projectPath + "/data.ini", "[general]\nname=" + projectName + "\ntype=objects3d\n");
        }
    } else if (selectedType == NewSkin) {
        if (selectedSubType == Car) {
            QDir().mkpath(projectPath + "/skins/default");
            createIniFile(projectPath + "/skins/default/skin.ini", "[skin]\nname=" + projectName + "\n");
        }
    } else if (selectedType == NewFont) {
        QDir().mkpath(projectPath);
        QDir().mkpath(projectPath + "/textures");
        createIniFile(projectPath + "/fonts.ini", "[fonts]\nname=" + projectName + "\n");
    } else if (selectedType == NewObject3D) {
        QDir().mkpath(projectPath + "/meshes");
        QDir().mkpath(projectPath + "/texture");
        QDir().mkpath(projectPath + "/materials");
        QDir().mkpath(projectPath + "/animations");
        createIniFile(projectPath + "/data.ini", "[general]\nname=" + projectName + "\ntype=objects3d\n");
    } else if (selectedType == NewShowroom) {
        QDir().mkpath(projectPath + "/meshes");
        QDir().mkpath(projectPath + "/texture");
        QDir().mkpath(projectPath + "/materials");
        QDir().mkpath(projectPath + "/environment");
        createIniFile(projectPath + "/data.ini", "[general]\nname=" + projectName + "\ntype=showroom\n");
    }
}

void NewProjectDialog::createIniFile(const QString& path, const QString& content) {
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << content;
        file.close();
    }
}

void NewProjectDialog::showStep1() {
    m_currentStep = 1;
    m_header->setText("Create New Project");
    m_prevBtn->setEnabled(false);
    m_nextBtn->setEnabled(false);

    delete m_contentWidget->layout();
    QVBoxLayout* layout = new QVBoxLayout(m_contentWidget);
    layout->setContentsMargins(30, 20, 30, 20);
    layout->setSpacing(12);

    QLabel* nameLabel = new QLabel("Project Name:");
    nameLabel->setStyleSheet("color: #aaaaaa; font-size: 14px;");
    layout->addWidget(nameLabel);

    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("Enter project name...");
    m_nameEdit->setStyleSheet("QLineEdit { background: #1e1e1e; color: white; border: 1px solid #3e3e42; padding: 10px; border-radius: 4px; font-size: 14px; } QLineEdit:focus { border: 1px solid #007acc; }");
    connect(m_nameEdit, &QLineEdit::textChanged, this, &NewProjectDialog::onNameTextChanged);
    layout->addWidget(m_nameEdit);

    layout->addSpacing(20);

    QLabel* desc = new QLabel("Select project type:");
    desc->setStyleSheet("color: #aaaaaa; font-size: 14px;");
    layout->addWidget(desc);

    struct ProjectOption {
        QString name;
        QString desc;
        ProjectType type;
        SubType defaultSub;
    };

    QList<ProjectOption> options = {
        {"New Model", "3D model (car, track, driver)", NewModel, Car},
        {"New Physics", "Physics configuration (car, track)", NewPhysics, Car},
        {"New Sound", "Audio configuration (car, track)", NewSound, Car},
        {"New Skin", "Liveries & skins (car, track)", NewSkin, Car},
        {"New Font", "Custom fonts for UI", NewFont, Character},
        {"New 3D Object", "3D objects (scenery, props)", NewObject3D, Object3D},
        {"New Showroom", "Showroom / Display", NewShowroom, Showroom}
    };

    for (const auto& opt : options) {
        QPushButton* btn = new QPushButton(opt.name);
        btn->setToolTip(opt.desc);
        btn->setStyleSheet("QPushButton { background: #2d2d30; color: white; border: 1px solid #3e3e42; padding: 12px; border-radius: 6px; text-align: left; font-size: 14px; } QPushButton:hover { background: #3e3e42; border-color: #007acc; }");
        btn->setProperty("projectType", (int)opt.type);
        connect(btn, &QPushButton::clicked, this, &NewProjectDialog::onProjectTypeSelected);
        layout->addWidget(btn);
    }

    layout->addStretch();
}

QString NewProjectDialog::detectAcRoot() {
#ifdef Q_OS_WIN
    QSettings steamReg("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Valve\\Steam", QSettings::NativeFormat);
    QString steamPath = steamReg.value("InstallPath").toString();
    if (!steamPath.isEmpty()) {
        QString acPath = steamPath + "/steamapps/core/assettocorsa";
        if (QDir(acPath).exists()) return acPath;
    }
#endif
    QStringList possiblePaths = {
        QCoreApplication::applicationDirPath() + "/../assettocorsa",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/assettocorsa",
        "C:/Program Files/Steam/steamapps/core/assettocorsa",
        "D:/SteamLibrary/steamapps/core/assettocorsa"
    };
    for (const QString& path : possiblePaths) {
        if (QDir(path).exists("acs.exe") || QDir(path).exists("AssettoCorsa.exe")) {
            return path;
        }
    }
    return QString();
}
