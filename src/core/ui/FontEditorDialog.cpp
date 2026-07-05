#include "FontEditorDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFontDatabase>
#include <QStandardPaths>
#include <QFont>

FontEditorDialog::FontEditorDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Font Editor");
    setMinimumSize(800, 600);
    resize(800, 600);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setupUI();
    updatePreview();
}

void FontEditorDialog::updatePreview() {
    QFont font(m_familyCombo->currentText(), m_sizeSpin->value());
    font.setBold(m_boldCheck->isChecked());
    font.setItalic(m_italicCheck->isChecked());
    m_sampleLabel->setFont(font);
}

void FontEditorDialog::onExportClicked() {
    fontName = m_nameEdit->text();
    fontFamily = m_familyCombo->currentText();
    fontSize = m_sizeSpin->value();
    isBold = m_boldCheck->isChecked();
    isItalic = m_italicCheck->isChecked();

    if (fontName.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter a font name.");
        return;
    }

    QString outputPath = QFileDialog::getSaveFileName(this, "Export Font",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + fontName + ".json",
        "JSON Files (*.json)");

    if (!outputPath.isEmpty()) {
        QJsonObject obj;
        obj["name"] = fontName;
        obj["family"] = fontFamily;
        obj["size"] = fontSize;
        obj["bold"] = isBold;
        obj["italic"] = isItalic;

        QFile file(outputPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
            file.close();
            QMessageBox::information(this, "Success", "Font exported successfully!");
            accept();
        }
    }
}

void FontEditorDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* header = new QLabel("Font Editor");
    header->setStyleSheet("background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #007acc,stop:1 #005a9e); color: white; font-size: 18px; font-weight: bold; padding: 15px;");
    header->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(header);

    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->setContentsMargins(10, 10, 10, 10);

    QWidget* leftPanel = new QWidget();
    leftPanel->setFixedWidth(250);
    leftPanel->setStyleSheet("background: #1e1e1e;");
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(10, 10, 10, 10);

    QLabel* propsLabel = new QLabel("PROPERTIES");
    propsLabel->setStyleSheet("color: #007acc; font-weight: bold; font-size: 11px;");
    leftLayout->addWidget(propsLabel);

    QLabel* nameLabel = new QLabel("Font Name:");
    nameLabel->setStyleSheet("color: #cccccc; font-size: 11px;");
    leftLayout->addWidget(nameLabel);

    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("MyFont");
    m_nameEdit->setStyleSheet("QLineEdit { background: #3c3c3c; color: white; border: 1px solid #555; padding: 5px; border-radius: 3px; }");
    leftLayout->addWidget(m_nameEdit);

    leftLayout->addSpacing(10);

    QLabel* familyLabel = new QLabel("Font Family:");
    familyLabel->setStyleSheet("color: #cccccc; font-size: 11px;");
    leftLayout->addWidget(familyLabel);

    m_familyCombo = new QComboBox();
    m_familyCombo->addItems(QFontDatabase().families());
    m_familyCombo->setStyleSheet("QComboBox { background: #3c3c3c; color: white; border: 1px solid #555; padding: 5px; border-radius: 3px; }");
    connect(m_familyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FontEditorDialog::updatePreview);
    leftLayout->addWidget(m_familyCombo);

    leftLayout->addSpacing(10);

    QLabel* sizeLabel = new QLabel("Size:");
    sizeLabel->setStyleSheet("color: #cccccc; font-size: 11px;");
    leftLayout->addWidget(sizeLabel);

    m_sizeSpin = new QSpinBox();
    m_sizeSpin->setRange(8, 128);
    m_sizeSpin->setValue(32);
    m_sizeSpin->setStyleSheet("QSpinBox { background: #3c3c3c; color: white; border: 1px solid #555; padding: 5px; border-radius: 3px; }");
    connect(m_sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &FontEditorDialog::updatePreview);
    leftLayout->addWidget(m_sizeSpin);

    leftLayout->addSpacing(10);

    m_boldCheck = new QCheckBox("Bold");
    m_boldCheck->setStyleSheet("color: #cccccc;");
    connect(m_boldCheck, &QCheckBox::toggled, this, &FontEditorDialog::updatePreview);
    leftLayout->addWidget(m_boldCheck);

    m_italicCheck = new QCheckBox("Italic");
    m_italicCheck->setStyleSheet("color: #cccccc;");
    connect(m_italicCheck, &QCheckBox::toggled, this, &FontEditorDialog::updatePreview);
    leftLayout->addWidget(m_italicCheck);

    leftLayout->addStretch();

    contentLayout->addWidget(leftPanel);

    QWidget* rightPanel = new QWidget();
    rightPanel->setStyleSheet("background: #252526;");
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);

    QLabel* previewLabel = new QLabel("PREVIEW");
    previewLabel->setStyleSheet("color: #007acc; font-weight: bold; font-size: 11px; padding: 5px;");
    rightLayout->addWidget(previewLabel);

    QWidget* previewArea = new QWidget();
    previewArea->setStyleSheet("background: #1e1e1e; border: 1px solid #3e3e42;");
    QVBoxLayout* previewAreaLayout = new QVBoxLayout(previewArea);

    m_sampleLabel = new QLabel("ABCDEFGHIJKLMNOPQRSTUVWXYZ\nabcdefghijklmnopqrstuvwxyz\n0123456789\n!@#$%^&*()");
    m_sampleLabel->setAlignment(Qt::AlignCenter);
    m_sampleLabel->setStyleSheet("color: white; padding: 20px;");
    previewAreaLayout->addWidget(m_sampleLabel);

    rightLayout->addWidget(previewArea);
    contentLayout->addWidget(rightPanel);

    mainLayout->addLayout(contentLayout);

    QHBoxLayout* footerLayout = new QHBoxLayout();
    footerLayout->addStretch();

    QPushButton* cancelBtn = new QPushButton("Cancel");
    cancelBtn->setStyleSheet("QPushButton { background: #3e3e42; color: white; border: 1px solid #555; padding: 8px 20px; border-radius: 4px; }");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    footerLayout->addWidget(cancelBtn);

    QPushButton* exportBtn = new QPushButton("Export");
    exportBtn->setStyleSheet("QPushButton { background: #007acc; color: white; border: none; padding: 8px 20px; border-radius: 4px; font-weight: bold; }");
    connect(exportBtn, &QPushButton::clicked, this, &FontEditorDialog::onExportClicked);
    footerLayout->addWidget(exportBtn);

    mainLayout->addLayout(footerLayout);
}
