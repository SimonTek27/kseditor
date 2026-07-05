#include "ReplaceDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

ReplaceDialog::ReplaceDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Replace Object");
    setMinimumSize(520, 350);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setupUI();
}

QString ReplaceDialog::selectedSource() const { return m_sourceLabel->text(); }
QString ReplaceDialog::selectedTarget() const { return m_targetLabel->text(); }

void ReplaceDialog::onBrowseSource() {
    QString file = QFileDialog::getOpenFileName(this, "Select Object to Replace",
        QString(), "3D Files (*.kn5 *.fbx *.obj *.dae *.glb *.gltf);;All Files (*)");
    if (!file.isEmpty()) {
        sourcePath = file;
        m_sourceLabel->setText(QFileInfo(file).fileName());
        m_sourceLabel->setStyleSheet("color: #cccccc; font-size: 12px;");
    }
}

void ReplaceDialog::onBrowseTarget() {
    QString file = QFileDialog::getOpenFileName(this, "Select Replacement Object",
        QString(), "3D Files (*.kn5 *.fbx *.obj *.dae *.glb *.gltf);;All Files (*)");
    if (!file.isEmpty()) {
        targetPath = file;
        m_targetLabel->setText(QFileInfo(file).fileName());
        m_targetLabel->setStyleSheet("color: #cccccc; font-size: 12px;");
    }
}

void ReplaceDialog::onReplace() {
    if (sourcePath.isEmpty() || targetPath.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select both source and replacement objects.");
        return;
    }
    accept();
}

void ReplaceDialog::onReplaceAll() {
    if (sourcePath.isEmpty() || targetPath.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select both source and replacement objects.");
        return;
    }
    m_replaceAll = true;
    accept();
}

void ReplaceDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* header = new QLabel("Replace Object");
    header->setStyleSheet("background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #0078d4,stop:1 #005a9e); color: white; font-size: 16px; font-weight: bold; padding: 16px;");
    header->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(header);

    QWidget* content = new QWidget();
    content->setStyleSheet("background: #252526; padding: 20px;");
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setSpacing(12);

    QLabel* srcTitle = new QLabel("Object to Replace:");
    srcTitle->setStyleSheet("color: #aaaaaa; font-size: 12px;");
    contentLayout->addWidget(srcTitle);

    QHBoxLayout* srcRow = new QHBoxLayout();
    m_sourceLabel = new QLabel("No file selected");
    m_sourceLabel->setStyleSheet("color: #666666; font-size: 12px; padding: 6px; background: #1e1e1e; border: 1px solid #3e3e42; border-radius: 3px;");
    srcRow->addWidget(m_sourceLabel, 1);

    QPushButton* srcBtn = new QPushButton("Browse...");
    srcBtn->setStyleSheet("QPushButton { background: #3e3e42; color: white; border: 1px solid #555; padding: 6px 14px; border-radius: 3px; } QPushButton:hover { background: #4e4e52; }");
    connect(srcBtn, &QPushButton::clicked, this, &ReplaceDialog::onBrowseSource);
    srcRow->addWidget(srcBtn);
    contentLayout->addLayout(srcRow);

    QLabel* tgtTitle = new QLabel("Replace With:");
    tgtTitle->setStyleSheet("color: #aaaaaa; font-size: 12px;");
    contentLayout->addWidget(tgtTitle);

    QHBoxLayout* tgtRow = new QHBoxLayout();
    m_targetLabel = new QLabel("No file selected");
    m_targetLabel->setStyleSheet("color: #666666; font-size: 12px; padding: 6px; background: #1e1e1e; border: 1px solid #3e3e42; border-radius: 3px;");
    tgtRow->addWidget(m_targetLabel, 1);

    QPushButton* tgtBtn = new QPushButton("Browse...");
    tgtBtn->setStyleSheet("QPushButton { background: #3e3e42; color: white; border: 1px solid #555; padding: 6px 14px; border-radius: 3px; } QPushButton:hover { background: #4e4e52; }");
    connect(tgtBtn, &QPushButton::clicked, this, &ReplaceDialog::onBrowseTarget);
    tgtRow->addWidget(tgtBtn);
    contentLayout->addLayout(tgtRow);

    contentLayout->addSpacing(8);

    m_keepAttrCheck = new QCheckBox("Keep User Attributes");
    m_keepAttrCheck->setChecked(true);
    m_keepAttrCheck->setStyleSheet("color: #cccccc; font-size: 11px;");
    connect(m_keepAttrCheck, &QCheckBox::toggled, this, [this](bool checked) { keepUserAttributes = checked; });
    contentLayout->addWidget(m_keepAttrCheck);

    QLabel* hint = new QLabel("Tip: Replace all works with single objects, not hierarchies.");
    hint->setStyleSheet("color: #666666; font-size: 10px; font-style: italic;");
    contentLayout->addWidget(hint);

    mainLayout->addWidget(content);

    QWidget* footer = new QWidget();
    footer->setStyleSheet("background: #1e1e1e; padding: 12px;");
    QHBoxLayout* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(16, 0, 16, 0);

    QPushButton* cancelBtn = new QPushButton("Cancel");
    cancelBtn->setStyleSheet("QPushButton { background: #3e3e42; color: white; border: 1px solid #555; padding: 8px 18px; border-radius: 3px; } QPushButton:hover { background: #4e4e52; }");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    footerLayout->addWidget(cancelBtn);

    footerLayout->addStretch();

    QPushButton* replaceAllBtn = new QPushButton("Replace All");
    replaceAllBtn->setStyleSheet("QPushButton { background: #3e3e42; color: white; border: 1px solid #888; padding: 8px 18px; border-radius: 3px; } QPushButton:hover { background: #4e4e52; }");
    connect(replaceAllBtn, &QPushButton::clicked, this, &ReplaceDialog::onReplaceAll);
    footerLayout->addWidget(replaceAllBtn);

    QPushButton* replaceBtn = new QPushButton("Replace");
    replaceBtn->setStyleSheet("QPushButton { background: #0078d4; color: white; border: none; padding: 8px 18px; border-radius: 3px; font-weight: bold; } QPushButton:hover { background: #1a86d9; }");
    connect(replaceBtn, &QPushButton::clicked, this, &ReplaceDialog::onReplace);
    footerLayout->addWidget(replaceBtn);

    mainLayout->addWidget(footer);
}
