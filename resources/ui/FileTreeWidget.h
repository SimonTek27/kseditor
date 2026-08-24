#pragma once

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSortFilterProxyModel>

class FileTreeFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit FileTreeFilterModel(QObject* parent = nullptr);
    void setSearchText(const QString& text);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    QString m_searchText;
};

class FileTreeWidget : public QWidget {
    Q_OBJECT
public:
    explicit FileTreeWidget(QWidget* parent = nullptr);

    void setRootPath(const QString& path);
    QString rootPath() const { return m_rootPath; }
    void refresh();

signals:
    void fileActivated(const QString& filePath);
    void rootPathChanged(const QString& path);

private slots:
    void onDoubleClicked(const QModelIndex& index);
    void onCustomContextMenu(const QPoint& pos);
    void onFilterChanged(int index);
    void onSearchTextChanged(const QString& text);
    void onTreeCollapsed(const QModelIndex& index);
    void onTreeExpanded(const QModelIndex& index);

private:
    void setupUI();
    void setupModel();
    QStringList nameFiltersForIndex(int index) const;
    void updateRootLabel();

    QTreeView* m_treeView;
    QFileSystemModel* m_fileModel;
    FileTreeFilterModel* m_filterProxy;
    QComboBox* m_filterCombo;
    QPushButton* m_refreshBtn;
    QPushButton* m_homeBtn;
    QLabel* m_rootLabel;
    QLineEdit* m_searchEdit;

    QString m_rootPath;
    QStringList m_expandedPaths;
};
