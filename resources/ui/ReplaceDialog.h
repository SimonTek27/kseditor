#pragma once
#include <QDialog>

class QLabel;
class QCheckBox;

class ReplaceDialog : public QDialog {
    Q_OBJECT
public:
    QString sourcePath;
    QString targetPath;
    bool keepUserAttributes = true;

    explicit ReplaceDialog(QWidget* parent = nullptr);

    QString selectedSource() const;
    QString selectedTarget() const;

private slots:
    void onBrowseSource();
    void onBrowseTarget();
    void onReplace();
    void onReplaceAll();

private:
    void setupUI();

    QLabel* m_sourceLabel;
    QLabel* m_targetLabel;
    QCheckBox* m_keepAttrCheck;
    bool m_replaceAll = false;
};
