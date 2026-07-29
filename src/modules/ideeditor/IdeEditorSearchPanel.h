#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QComboBox>

namespace ks {

class IdeEditorSearchPanel : public QWidget
{
    Q_OBJECT
public:
    explicit IdeEditorSearchPanel(QWidget* parent = nullptr);

    void setRootPath(const QString& path);

signals:
    void navigateToResult(const QString& filePath, int line);

private slots:
    void onSearch();
    void onResultClicked(QListWidgetItem* item);
    void onClear();

private:
    void setupUI();
    QStringList searchFile(const QString& filePath, const QString& term, bool caseSensitive);

    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_searchBtn = nullptr;
    QPushButton* m_clearBtn = nullptr;
    QCheckBox* m_caseSensitiveChk = nullptr;
    QComboBox* m_filePatternCombo = nullptr;
    QListWidget* m_resultsList = nullptr;
    QLabel* m_statusLabel = nullptr;
    QString m_rootPath;
};

} // namespace ks
