#pragma once
#include <QDialog>

class QLineEdit;
class QTextEdit;
class QComboBox;

class PackageWizard : public QDialog {
    Q_OBJECT
public:
    QString sourcePath;
    QString outputPath;
    QString packageName;
    QString packageVersion;
    QString packageAuthor;
    QString packageDescription;
    QString packageType;

    explicit PackageWizard(QWidget* parent = nullptr);

private slots:
    void onBrowseClicked();
    void onCreateClicked();

private:
    void setupUI();
    void copyDirectory(const QString& sourceDir, const QString& destDir);

    QLineEdit* m_sourceEdit;
    QLineEdit* m_nameEdit;
    QLineEdit* m_versionEdit;
    QLineEdit* m_authorEdit;
    QTextEdit* m_descEdit;
    QComboBox* m_typeCombo;
};
