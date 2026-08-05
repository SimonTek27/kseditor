#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QListWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QString>
#include <QVector>
#include <QStringList>

namespace ks {
namespace kunos { struct KsSetupData; class KsSetupManager; }
namespace plugins { namespace kunos { namespace ks { class KsIniDocument; class KsIniSection; } } }

using KsSetupData     = ::ks::kunos::KsSetupData;
using KsSetupManager  = ::ks::kunos::KsSetupManager;
using KsIniDocument   = ::ks::plugins::kunos::ks::KsIniDocument;
using KsIniSection    = ::ks::plugins::kunos::ks::KsIniSection;
}

namespace ks {

class CarValidatorWidget : public QWidget {
    Q_OBJECT
public:
    explicit CarValidatorWidget(QWidget* parent = nullptr);

    void validateCar(const QString& carFolder);
    void validateIni(const QString& iniPath);

signals:
    void validationComplete(int errors, int warnings);

private slots:
    void onRunValidation();
    void onExportReport();
    void onFixSuggested(int issueIndex);

private:
    void buildUI();
    QString checkTyrePressures(const KsIniDocument& doc) const;
    QString checkSuspensionGeometry(const KsSetupData& setup) const;
    QString checkEngineData(const KsIniDocument& doc) const;
    QString checkAeroBalance(const KsSetupData& setup) const;
    QString checkMassDistribution(const KsSetupData& setup) const;
    void populateIssues(const QStringList& errors, const QStringList& warnings);

    struct ValidationIssue {
        QString category;
        QString message;
        QString severity;
        QString suggestedFix;
    };

    QTextEdit* m_reportEdit = nullptr;
    QListWidget* m_issuesList = nullptr;
    QTableWidget* m_summaryTable = nullptr;
    QPushButton* m_runBtn = nullptr;
    QPushButton* m_exportBtn = nullptr;
    QString m_lastCarFolder;
    QVector<ValidationIssue> m_issues;
};

} // namespace ks