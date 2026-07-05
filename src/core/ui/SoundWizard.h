#pragma once
#include <QDialog>

class QLineEdit;
class QComboBox;

class SoundWizard : public QDialog {
    Q_OBJECT
public:
    QString carName;
    QString engineType;
    QString soundCategory;
    int cylinders = 4;
    bool isTurbo = false;

    explicit SoundWizard(QWidget* parent = nullptr);

private slots:
    void onCreateClicked();

private:
    void setupUI();

    QLineEdit* m_nameEdit;
    QComboBox* m_engineCombo;
    QComboBox* m_cylindersCombo;
    QComboBox* m_categoryCombo;
};
