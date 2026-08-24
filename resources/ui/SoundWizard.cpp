#include "SoundWizard.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QMessageBox>

SoundWizard::SoundWizard(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Sound Wizard - New Car Sound");
    setMinimumSize(600, 500);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setupUI();
}

void SoundWizard::onCreateClicked() {
    carName = m_nameEdit->text();
    engineType = m_engineCombo->currentText();
    cylinders = m_cylindersCombo->currentText().toInt();
    isTurbo = (engineType != "Naturally Aspirated");
    soundCategory = m_categoryCombo->currentText();

    if (carName.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter a car name.");
        return;
    }
    accept();
}

void SoundWizard::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* header = new QLabel("Configure Car Sound");
    header->setStyleSheet("background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #007acc,stop:1 #005a9e); color: white; font-size: 18px; font-weight: bold; padding: 20px;");
    header->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(header);

    QWidget* content = new QWidget();
    content->setStyleSheet("background: #252526;");
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(30, 20, 30, 20);

    QLabel* nameLabel = new QLabel("Car Name:");
    nameLabel->setStyleSheet("color: #cccccc; font-size: 13px;");
    contentLayout->addWidget(nameLabel);

    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("e.g., Ferrari 488 GT3");
    m_nameEdit->setStyleSheet("QLineEdit { background: #3c3c3c; color: white; border: 1px solid #555; padding: 8px; border-radius: 4px; font-size: 14px; }");
    contentLayout->addWidget(m_nameEdit);

    contentLayout->addSpacing(15);

    QLabel* engineLabel = new QLabel("Engine Type:");
    engineLabel->setStyleSheet("color: #cccccc; font-size: 13px;");
    contentLayout->addWidget(engineLabel);

    m_engineCombo = new QComboBox();
    m_engineCombo->addItems({"Naturally Aspirated", "Turbocharged", "Supercharged"});
    m_engineCombo->setStyleSheet("QComboBox { background: #3c3c3c; color: white; border: 1px solid #555; padding: 8px; border-radius: 4px; }");
    contentLayout->addWidget(m_engineCombo);

    contentLayout->addSpacing(15);

    QLabel* cylindersLabel = new QLabel("Cylinders:");
    cylindersLabel->setStyleSheet("color: #cccccc; font-size: 13px;");
    contentLayout->addWidget(cylindersLabel);

    m_cylindersCombo = new QComboBox();
    m_cylindersCombo->addItems({"4", "6", "8", "10", "12"});
    m_cylindersCombo->setCurrentText("8");
    m_cylindersCombo->setStyleSheet(m_engineCombo->styleSheet());
    contentLayout->addWidget(m_cylindersCombo);

    contentLayout->addSpacing(15);

    QLabel* categoryLabel = new QLabel("Sound Category:");
    categoryLabel->setStyleSheet("color: #cccccc; font-size: 13px;");
    contentLayout->addWidget(categoryLabel);

    m_categoryCombo = new QComboBox();
    m_categoryCombo->addItems({"GT3", "GT2", "Formula", "Touring Car", "Rally", "Endurance", "Road Car"});
    m_categoryCombo->setStyleSheet(m_engineCombo->styleSheet());
    contentLayout->addWidget(m_categoryCombo);

    contentLayout->addStretch();

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton* cancelBtn = new QPushButton("Cancel");
    cancelBtn->setStyleSheet("QPushButton { background: #3e3e42; color: white; border: 1px solid #555; padding: 10px 25px; border-radius: 4px; } QPushButton:hover { background: #4e4e52; }");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    QPushButton* createBtn = new QPushButton("Create Sound");
    createBtn->setStyleSheet("QPushButton { background: #007acc; color: white; border: none; padding: 10px 25px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background: #005a9e; }");
    connect(createBtn, &QPushButton::clicked, this, &SoundWizard::onCreateClicked);
    btnLayout->addWidget(createBtn);

    contentLayout->addLayout(btnLayout);
    mainLayout->addWidget(content);
}
