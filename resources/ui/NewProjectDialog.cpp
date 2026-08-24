#include "NewProjectDialog.h"
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QIcon>
#include <QPixmap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QSettings>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>

NewProjectDialog::NewProjectDialog(QWidget* parent) : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint) {
    setWindowTitle("New Project");
    setMinimumSize(560, 540);
    setupUI();
    applyTheme();
    showStep1();
}

void NewProjectDialog::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_titleBar &&
        m_titleBar->geometry().contains(event->pos())) {
        m_dragPos = event->globalPos() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(event);
}

void NewProjectDialog::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton && !m_dragPos.isNull()) {
        move(event->globalPos() - m_dragPos);
    }
    QDialog::mouseMoveEvent(event);
}

void NewProjectDialog::mouseReleaseEvent(QMouseEvent* event) {
    m_dragPos = QPoint();
    QDialog::mouseReleaseEvent(event);
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
    m_mainLayout->setSpacing(0);

    m_titleBar = new QWidget();
    m_titleBar->setObjectName("titleBar");
    m_titleBar->setFixedHeight(38);
    QHBoxLayout* titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(16, 0, 8, 0);
    titleLayout->setSpacing(4);

    m_header = new QLabel("Create New Project");
    m_header->setObjectName("titleLabel");
    titleLayout->addWidget(m_header);
    titleLayout->addStretch();

    QPushButton* minBtn = new QPushButton();
    minBtn->setObjectName("titleBtn");
    minBtn->setFixedSize(36, 28);
    minBtn->setIcon(QIcon(":/icons/window-minimize.svg"));
    minBtn->setIconSize(QSize(14, 14));
    connect(minBtn, &QPushButton::clicked, this, &QWidget::showMinimized);
    titleLayout->addWidget(minBtn);

    QPushButton* closeBtn = new QPushButton();
    closeBtn->setObjectName("titleBtnClose");
    closeBtn->setFixedSize(36, 28);
    closeBtn->setIcon(QIcon(":/icons/window-close.svg"));
    closeBtn->setIconSize(QSize(14, 14));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    titleLayout->addWidget(closeBtn);

    m_mainLayout->addWidget(m_titleBar);

    m_contentWidget = new QWidget();
    m_mainLayout->addWidget(m_contentWidget, 1);

    QWidget* footer = new QWidget();
    footer->setObjectName("footer");
    QHBoxLayout* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(24, 14, 24, 14);

    m_prevBtn = new QPushButton("< BACK");
    m_prevBtn->setEnabled(false);
    connect(m_prevBtn, &QPushButton::clicked, this, &NewProjectDialog::onPreviousClicked);
    footerLayout->addWidget(m_prevBtn);

    footerLayout->addStretch();

    m_nextBtn = new QPushButton("CREATE");
    m_nextBtn->setEnabled(false);
    connect(m_nextBtn, &QPushButton::clicked, this, &NewProjectDialog::onCreateClicked);
    footerLayout->addWidget(m_nextBtn);

    QPushButton* cancelBtn = new QPushButton("CANCEL");
    cancelBtn->setObjectName("secondaryButton");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    footerLayout->addWidget(cancelBtn);

    m_mainLayout->addWidget(footer);
}

void NewProjectDialog::applyTheme()
{
    setStyleSheet(R"(
        NewProjectDialog {
            background-color: #18181b;
        }
        QWidget#titleBar {
            background-color: #1a1a1e;
            border-bottom: 1px solid #27272a;
        }
        #titleLabel {
            font-size: 14px;
            font-weight: 600;
            color: #d4d4d8;
            padding: 0 4px;
        }
        #sectionLabel {
            font-size: 13px;
            font-weight: 600;
            color: #a1a1aa;
            margin-bottom: 4px;
        }
        #fieldLabel {
            font-size: 13px;
            color: #a1a1aa;
            margin-bottom: 2px;
        }
        QWidget#contentWidget {
            background-color: #1c1c1f;
        }
        QLineEdit {
            background-color: #1c1c1f;
            color: #d4d4d8;
            border: 1px solid #27272a;
            border-radius: 4px;
            padding: 10px 12px;
            font-size: 14px;
        }
        QLineEdit:focus {
            border: 1px solid #3b82f6;
            background-color: #1e1e22;
        }
        QPushButton#projectTypeBtn {
            background-color: #1c1c1f;
            color: #d4d4d8;
            border: 1px solid #27272a;
            border-radius: 8px;
            padding: 14px 16px;
            text-align: left;
            font-size: 14px;
            font-weight: 500;
        }
        QPushButton#projectTypeBtn:hover {
            background-color: #27272a;
            border-color: #3b82f6;
        }
        QPushButton#projectTypeBtn:pressed {
            background-color: #3f3f46;
        }
        QPushButton {
            background-color: #27272a;
            color: #d4d4d8;
            border: 1px solid #3f3f46;
            border-radius: 5px;
            padding: 8px 18px;
            font-size: 12px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #3f3f46;
        }
        QPushButton:pressed {
            background-color: #3b82f6;
            border-color: #3b82f6;
            color: #ffffff;
        }
        QPushButton:disabled {
            background-color: #1c1c1f;
            color: #52525b;
            border-color: #27272a;
        }
        QPushButton#secondaryButton {
            background-color: #27272a;
            color: #d4d4d8;
            border: 1px solid #3f3f46;
            border-radius: 5px;
            padding: 8px 18px;
            font-size: 12px;
            font-weight: 500;
        }
        QPushButton#secondaryButton:hover {
            background-color: #3f3f46;
        }
        QPushButton#titleBtn {
            background: transparent;
            border: none;
            border-radius: 4px;
            padding: 0;
        }
        QPushButton#titleBtn:hover {
            background-color: #27272a;
        }
        QPushButton#titleBtn:pressed {
            background-color: #3f3f46;
        }
        QPushButton#titleBtnClose:hover {
            background-color: #c0392b;
        }
        QWidget#footer {
            background-color: #1a1a1e;
            border-top: 1px solid #27272a;
        }
        QPushButton#primaryButton {
            background-color: #3b82f6;
            color: #ffffff;
            border: 1px solid #3b82f6;
            border-radius: 5px;
            padding: 8px 18px;
            font-size: 12px;
            font-weight: 600;
        }
        QPushButton#primaryButton:hover {
            background-color: #2563eb;
            border-color: #2563eb;
        }
        QPushButton#primaryButton:disabled {
            background-color: #1c1c1f;
            color: #52525b;
            border-color: #27272a;
        }
    )");

    m_header->setObjectName("titleLabel");
    m_contentWidget->setObjectName("contentWidget");
    m_prevBtn->setObjectName("secondaryButton");
    m_nextBtn->setObjectName("primaryButton");
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

QPushButton* NewProjectDialog::createProjectTypeButton(const QString& text, const QString& desc, const QString& iconName, ProjectType type)
{
    QPushButton* btn = new QPushButton();
    btn->setObjectName("projectTypeBtn");
    btn->setMinimumHeight(56);
    btn->setProperty("projectType", (int)type);

    QHBoxLayout* btnLayout = new QHBoxLayout(btn);
    btnLayout->setContentsMargins(4, 4, 12, 4);
    btnLayout->setSpacing(12);

    QLabel* iconLabel = new QLabel(btn);
    QIcon icon(QString(":/icons/%1").arg(iconName));
    if (!icon.isNull()) {
        QPixmap pix = icon.pixmap(28, 28);
        iconLabel->setPixmap(pix);
    }
    iconLabel->setFixedSize(32, 32);
    iconLabel->setAlignment(Qt::AlignCenter);
    btnLayout->addWidget(iconLabel);

    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);
    QLabel* titleLabel = new QLabel(text, btn);
    titleLabel->setStyleSheet("background: transparent; color: #d4d4d8; font-size: 14px; font-weight: 500;");
    QLabel* descLabel = new QLabel(desc, btn);
    descLabel->setStyleSheet("background: transparent; color: #71717a; font-size: 11px;");
    textLayout->addWidget(titleLabel);
    textLayout->addWidget(descLabel);
    btnLayout->addLayout(textLayout, 1);

    connect(btn, &QPushButton::clicked, this, &NewProjectDialog::onProjectTypeSelected);
    return btn;
}

void NewProjectDialog::showStep1() {
    m_currentStep = 1;
    m_header->setText("Create New Project");
    m_prevBtn->setEnabled(false);
    m_nextBtn->setEnabled(false);

    delete m_contentWidget->layout();
    QVBoxLayout* layout = new QVBoxLayout(m_contentWidget);
    layout->setContentsMargins(28, 20, 28, 20);
    layout->setSpacing(8);

    QLabel* nameLabel = new QLabel("Project Name:");
    nameLabel->setObjectName("fieldLabel");
    layout->addWidget(nameLabel);

    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("Enter project name...");
    connect(m_nameEdit, &QLineEdit::textChanged, this, &NewProjectDialog::onNameTextChanged);
    layout->addWidget(m_nameEdit);

    layout->addSpacing(16);

    QLabel* typeLabel = new QLabel("Select project type:");
    typeLabel->setObjectName("fieldLabel");
    layout->addWidget(typeLabel);

    layout->addSpacing(4);

    struct ProjectOption {
        QString name;
        QString desc;
        QString icon;
        ProjectType type;
    };

    QList<ProjectOption> options = {
        {"New Model",       "3D model (car, track, driver)",       "car-model.svg",     NewModel},
        {"New Physics",     "Physics configuration (car, track)",  "physics.svg",       NewPhysics},
        {"New Sound",       "Audio configuration (car, track)",    "audio.svg",         NewSound},
        {"New Skin",        "Liveries & skins (car, track)",       "skin.svg",          NewSkin},
        {"New Font",        "Custom fonts for UI",                 "font.svg",          NewFont},
        {"New 3D Object",   "3D objects (scenery, props)",         "primitive-box.svg", NewObject3D},
        {"New Showroom",    "Showroom / Display",                  "display.svg",       NewShowroom}
    };

    for (const auto& opt : options) {
        QPushButton* btn = createProjectTypeButton(opt.name, opt.desc, opt.icon, opt.type);
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
