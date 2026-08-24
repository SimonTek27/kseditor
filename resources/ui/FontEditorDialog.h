#pragma once
#include <QDialog>

class QLineEdit;
class QComboBox;
class QSpinBox;
class QCheckBox;
class QLabel;

class FontEditorDialog : public QDialog {
    Q_OBJECT
public:
    QString fontName;
    QString fontFamily;
    int fontSize = 32;
    bool isBold = false;
    bool isItalic = false;

    explicit FontEditorDialog(QWidget* parent = nullptr);

private slots:
    void updatePreview();
    void onExportClicked();

private:
    void setupUI();

    QLineEdit* m_nameEdit;
    QComboBox* m_familyCombo;
    QSpinBox* m_sizeSpin;
    QCheckBox* m_boldCheck;
    QCheckBox* m_italicCheck;
    QLabel* m_sampleLabel;
};
