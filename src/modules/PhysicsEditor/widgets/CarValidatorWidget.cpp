#include "CarValidatorWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QListWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <ks/plugins/simulators/kunos/assettocorsa/ksAssettoCorsaIni.h>
#include <ks/plugins/simulators/kunos/assettocorsa/ksAssettoCorsaSetup.h>

namespace ks {

CarValidatorWidget::CarValidatorWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUI();
}

void CarValidatorWidget::buildUI() {
    auto* layout = new QVBoxLayout(this);

    m_reportEdit = new QTextEdit(this);
    m_reportEdit->setReadOnly(true);
    m_reportEdit->setFont(QFont("Consolas", 9));

    m_issuesList = new QListWidget(this);

    m_summaryTable = new QTableWidget(0, 3, this);
    m_summaryTable->setHorizontalHeaderLabels({"Category", "Severity", "Count"});
    m_summaryTable->horizontalHeader()->setStretchLastSection(true);

    m_runBtn = new QPushButton("Run Validation", this);
    m_exportBtn = new QPushButton("Export Report", this);

    auto* btnLayout = new QHBoxLayout;
    btnLayout->addWidget(m_runBtn);
    btnLayout->addWidget(m_exportBtn);
    btnLayout->addStretch();

    layout->addWidget(new QLabel("Validation Report:", this));
    layout->addWidget(m_reportEdit, 1);
    layout->addWidget(new QLabel("Issues:", this));
    layout->addWidget(m_issuesList);
    layout->addWidget(new QLabel("Summary:", this));
    layout->addWidget(m_summaryTable);
    layout->addLayout(btnLayout);

    connect(m_runBtn, &QPushButton::clicked, this, &CarValidatorWidget::onRunValidation);
    connect(m_exportBtn, &QPushButton::clicked, this, &CarValidatorWidget::onExportReport);
    connect(m_issuesList, &QListWidget::itemDoubleClicked, this, &CarValidatorWidget::onFixSuggested);
}

void CarValidatorWidget::validateCar(const QString& carFolder) {
    m_lastCarFolder = carFolder;
    m_issues.clear();

    QDir dataDir(carFolder + "/data");
    if (!dataDir.exists()) {
        m_issues.append({"Structure", "No data folder found", "Error", "Create data folder"});
        populateIssues({"No data folder found"}, {});
        return;
    }

    QStringList iniFiles = dataDir.entryList(QStringList() << "*.ini", QDir::Files);
    if (iniFiles.isEmpty()) {
        m_issues.append({"Structure", "No INI files in data folder", "Error", "Add car.ini"});
        populateIssues({"No INI files in data folder"}, {});
        return;
    }

    QStringList errors;
    QStringList warnings;

    // Load car.ini
    QString carIniPath = dataDir.absoluteFilePath("car.ini");
    KsIniDocument carDoc;
    if (carDoc.load(carIniPath)) {
        QString e = checkTyrePressures(carDoc);
        if (!e.isEmpty()) errors << e;
    }

    // Load setup
    KsSetupData setup = KsSetupManager::load(dataDir.absoluteFilePath("setup.ini"));

    errors << checkSuspensionGeometry(setup);
    errors << checkEngineData(carDoc);
    errors << checkAeroBalance(setup);
    errors << checkMassDistribution(setup);

    populateIssues(errors, warnings);
    m_reportEdit->setPlainText(QString("Validation complete: %1 errors, %2 warnings").arg(errors.size()).arg(warnings.size()));
    emit validationComplete(errors.size(), warnings.size());
}

void CarValidatorWidget::validateIni(const QString& iniPath) {
    KsIniDocument doc;
    if (!doc.load(iniPath)) {
        m_reportEdit->setPlainText("Failed to load: " + iniPath);
        return;
    }
    validateCar(QFileInfo(iniPath).absolutePath());
}

void CarValidatorWidget::onRunValidation() {
    if (!m_lastCarFolder.isEmpty()) {
        validateCar(m_lastCarFolder);
    }
}

void CarValidatorWidget::onExportReport() {
    QString path = QFileDialog::getSaveFileName(this, "Export Report", QString(), "Text files (*.txt);;HTML (*.html)");
    if (!path.isEmpty()) {
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(m_reportEdit->toPlainText().toUtf8());
        }
    }
}

void CarValidatorWidget::onFixSuggested(int issueIndex) {
    if (issueIndex >= 0 && issueIndex < m_issues.size()) {
        QMessageBox::information(this, "Suggested Fix", m_issues[issueIndex].suggestedFix);
    }
}

QString CarValidatorWidget::checkTyrePressures(const KsIniDocument& doc) const {
    const KsIniSection* tyres = doc.section("TYRES");
    if (!tyres) return "";
    // Check pressures
    return "";
}

QString CarValidatorWidget::checkSuspensionGeometry(const KsSetupData& setup) const {
    if (setup.frontRideHeight < 10 || setup.frontRideHeight > 200) {
        return "Front ride height out of range";
    }
    if (setup.rearRideHeight < 10 || setup.rearRideHeight > 200) {
        return "Rear ride height out of range";
    }
    return "";
}

QString CarValidatorWidget::checkEngineData(const KsIniDocument& doc) const {
    Q_UNUSED(doc);
    return "";
}

QString CarValidatorWidget::checkAeroBalance(const KsSetupData& setup) const {
    if (setup.frontWing == 0 && setup.rearWing == 0) {
        return "No aero configured (both wings zero)";
    }
    return "";
}

QString CarValidatorWidget::checkMassDistribution(const KsSetupData& setup) const {
    Q_UNUSED(setup);
    return "";
}

void CarValidatorWidget::populateIssues(const QStringList& errors, const QStringList& warnings) {
    m_issuesList->clear();
    m_summaryTable->setRowCount(0);

    QMap<QString, int> errorCounts, warningCounts;

    for (const QString& e : errors) {
        m_issuesList->addItem("[ERROR] " + e);
        errorCounts["General"]++;
    }
    for (const QString& w : warnings) {
        m_issuesList->addItem("[WARN] " + w);
        warningCounts["General"]++;
    }

    int row = 0;
    for (auto it = errorCounts.begin(); it != errorCounts.end(); ++it) {
        m_summaryTable->insertRow(row);
        m_summaryTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        m_summaryTable->setItem(row, 1, new QTableWidgetItem("Error"));
        m_summaryTable->setItem(row, 2, new QTableWidgetItem(QString::number(it.value())));
        row++;
    }
    for (auto it = warningCounts.begin(); it != warningCounts.end(); ++it) {
        m_summaryTable->insertRow(row);
        m_summaryTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        m_summaryTable->setItem(row, 1, new QTableWidgetItem("Warning"));
        m_summaryTable->setItem(row, 2, new QTableWidgetItem(QString::number(it.value())));
        row++;
    }
}

} // namespace ks