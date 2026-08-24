#include "PackageWizard.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>

PackageWizard::PackageWizard(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Package Wizard - Create ksPackage");
    setMinimumSize(600, 550);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setupUI();
}

void PackageWizard::onBrowseClicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Source Folder",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    if (!dir.isEmpty()) {
        m_sourceEdit->setText(dir);
        sourcePath = dir;
        packageName = QFileInfo(dir).baseName();
        m_nameEdit->setText(packageName);

        QSettings ini(dir + "/data.ini", QSettings::IniFormat);
        QString type = ini.value("general/type", "").toString();
        if (!type.isEmpty()) {
            int index = m_typeCombo->findText(type);
            if (index >= 0) m_typeCombo->setCurrentIndex(index);
        }
    }
}

void PackageWizard::onCreateClicked() {
    sourcePath = m_sourceEdit->text();
    packageName = m_nameEdit->text();
    packageVersion = m_versionEdit->text();
    packageAuthor = m_authorEdit->text();
    packageDescription = m_descEdit->toPlainText();
    packageType = m_typeCombo->currentText();

    if (sourcePath.isEmpty() || packageName.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please fill in all required fields.");
        return;
    }

    QString outputDir = QFileDialog::getExistingDirectory(this,
        "Select Output Location",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));

    if (outputDir.isEmpty()) return;

    outputPath = outputDir + "/" + packageName + "_v" + packageVersion;

    if (QDir(outputPath).exists()) {
        QMessageBox::StandardButton btn = QMessageBox::question(this,
            "Package Exists",
            "A package named '" + packageName + "' already exists.\n\nOverwrite?",
            QMessageBox::Yes | QMessageBox::Cancel);
        if (btn == QMessageBox::Yes) {
            QDir(outputPath).removeRecursively();
        } else {
            return;
        }
    }

    QDir().mkpath(outputPath);

    QJsonObject manifest;
    manifest["name"] = packageName;
    manifest["version"] = packageVersion;
    manifest["author"] = packageAuthor;
    manifest["description"] = packageDescription;
    manifest["type"] = packageType;
    manifest["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    manifest["gameVersion"] = "1.8";

    QFile manifestFile(outputPath + "/manifest.json");
    if (manifestFile.open(QIODevice::WriteOnly)) {
        manifestFile.write(QJsonDocument(manifest).toJson(QJsonDocument::Indented));
        manifestFile.close();
    }

    copyDirectory(sourcePath, outputPath + "/content");

    QMessageBox::information(this, "Success",
        "Package created successfully!\n\n" + outputPath);
    accept();
}

void PackageWizard::copyDirectory(const QString& sourceDir, const QString& destDir) {
    QDir().mkpath(destDir);
    QDir source(sourceDir);
    QFileInfoList entries = source.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    for (const QFileInfo& entry : entries) {
        QString destPath = destDir + "/" + entry.fileName();
        if (entry.isDir()) {
            copyDirectory(entry.absoluteFilePath(), destPath);
        } else {
            QFile::copy(entry.absoluteFilePath(), destPath);
        }
    }
}

void PackageWizard::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* header = new QLabel("Create ksPackage");
    header->setStyleSheet("background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #28a745,stop:1 #20c997); color: white; font-size: 18px; font-weight: bold; padding: 20px;");
    header->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(header);

    QWidget* content = new QWidget();
    content->setStyleSheet("background: #252526; padding: 20px;");
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setSpacing(15);

    QLabel* sourceLabel = new QLabel("Source Folder:");
    sourceLabel->setStyleSheet("color: #aaaaaa;");
    contentLayout->addWidget(sourceLabel);

    QHBoxLayout* sourceRow = new QHBoxLayout();
    m_sourceEdit = new QLineEdit();
    m_sourceEdit->setPlaceholderText("Select folder to package...");
    m_sourceEdit->setStyleSheet("QLineEdit { background: #1e1e1e; color: white; border: 1px solid #3e3e42; padding: 8px; border-radius: 4px; }");
    sourceRow->addWidget(m_sourceEdit);

    QPushButton* browseBtn = new QPushButton("Browse");
    browseBtn->setStyleSheet("QPushButton { background: #3e3e42; color: white; border: 1px solid #555; padding: 8px 16px; border-radius: 4px; } QPushButton:hover { background: #4e4e52; }");
    connect(browseBtn, &QPushButton::clicked, this, &PackageWizard::onBrowseClicked);
    sourceRow->addWidget(browseBtn);
    contentLayout->addLayout(sourceRow);

    QLabel* nameLabel = new QLabel("Package Name:");
    nameLabel->setStyleSheet("color: #aaaaaa;");
    contentLayout->addWidget(nameLabel);

    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("MyPackage");
    m_nameEdit->setStyleSheet("QLineEdit { background: #1e1e1e; color: white; border: 1px solid #3e3e42; padding: 8px; border-radius: 4px; }");
    contentLayout->addWidget(m_nameEdit);

    QLabel* versionLabel = new QLabel("Version:");
    versionLabel->setStyleSheet("color: #aaaaaa;");
    contentLayout->addWidget(versionLabel);

    m_versionEdit = new QLineEdit("1.0.0");
    m_versionEdit->setStyleSheet("QLineEdit { background: #1e1e1e; color: white; border: 1px solid #3e3e42; padding: 8px; border-radius: 4px; }");
    contentLayout->addWidget(m_versionEdit);

    QLabel* authorLabel = new QLabel("Author:");
    authorLabel->setStyleSheet("color: #aaaaaa;");
    contentLayout->addWidget(authorLabel);

    m_authorEdit = new QLineEdit();
    m_authorEdit->setPlaceholderText("Your Name");
    m_authorEdit->setStyleSheet("QLineEdit { background: #1e1e1e; color: white; border: 1px solid #3e3e42; padding: 8px; border-radius: 4px; }");
    contentLayout->addWidget(m_authorEdit);

    QLabel* descLabel = new QLabel("Description:");
    descLabel->setStyleSheet("color: #aaaaaa;");
    contentLayout->addWidget(descLabel);

    m_descEdit = new QTextEdit();
    m_descEdit->setPlaceholderText("Package description...");
    m_descEdit->setStyleSheet("QTextEdit { background: #1e1e1e; color: white; border: 1px solid #3e3e42; padding: 8px; border-radius: 4px; }");
    m_descEdit->setMaximumHeight(80);
    contentLayout->addWidget(m_descEdit);

    QLabel* typeLabel = new QLabel("Package Type:");
    typeLabel->setStyleSheet("color: #aaaaaa;");
    contentLayout->addWidget(typeLabel);

    m_typeCombo = new QComboBox();
    m_typeCombo->addItems({"car", "track", "driver", "object3d", "showroom", "skin", "fonts", "weather", "sound"});
    m_typeCombo->setStyleSheet("QComboBox { background: #1e1e1e; color: white; border: 1px solid #3e3e42; padding: 8px; border-radius: 4px; } QComboBox::drop-down { border: none; }");
    contentLayout->addWidget(m_typeCombo);

    mainLayout->addWidget(content);

    QWidget* footer = new QWidget();
    footer->setStyleSheet("background: #1e1e1e; padding: 15px;");
    QHBoxLayout* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(20, 0, 20, 0);

    QPushButton* cancelBtn = new QPushButton("Cancel");
    cancelBtn->setStyleSheet("QPushButton { background: #3e3e42; color: white; border: 1px solid #555; padding: 10px 20px; border-radius: 4px; } QPushButton:hover { background: #4e4e52; }");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    footerLayout->addWidget(cancelBtn);

    footerLayout->addStretch();

    QPushButton* createBtn = new QPushButton("Create Package");
    createBtn->setStyleSheet("QPushButton { background: #28a745; color: white; border: none; padding: 10px 20px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background: #20c997; }");
    connect(createBtn, &QPushButton::clicked, this, &PackageWizard::onCreateClicked);
    footerLayout->addWidget(createBtn);

    mainLayout->addWidget(footer);
}
