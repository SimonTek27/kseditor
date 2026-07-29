#pragma once

#include <QWidget>
#include <QTreeView>
#include <QLineEdit>
#include <QPushButton>
#include <QFileSystemWatcher>
#include <QComboBox>

class QFileSystemModel;

namespace ks {

class IdeEditorFileBrowser : public QWidget
{
    Q_OBJECT
public:
    explicit IdeEditorFileBrowser(QWidget* parent = nullptr);

    void setRootPath(const QString& path);
    QString rootPath() const { return m_rootPath; }
    void refresh();

signals:
    void fileSelected(const QString& filePath);
    void fileDoubleClicked(const QString& filePath);

private slots:
    void onTreeClicked(const QModelIndex& index);
    void onTreeDoubleClicked(const QModelIndex& index);
    void onFilterChanged(const QString& text);
    void onDirectoryChanged(const QString& path);

private:
    void setupUI();

    QFileSystemModel* m_model = nullptr;
    QTreeView* m_tree = nullptr;
    QLineEdit* m_filterEdit = nullptr;
    QLineEdit* m_pathEdit = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_collapseBtn = nullptr;
    QFileSystemWatcher* m_watcher = nullptr;
    QString m_rootPath;
};

} // namespace ks
