#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include "ContentBrowser.h"

namespace ks {

class ContentBrowserWidget : public QWidget {
    Q_OBJECT
public:
    explicit ContentBrowserWidget(const QString& contentPath, QWidget* parent = nullptr);

    void setContentPath(const QString& path);
    QString contentPath() const { return m_contentPath; }

signals:
    void contentInstalled(const QString& name);
    void contentUninstalled(const QString& name);

private slots:
    void refreshContent();
    void onItemSelected();
    void onInstall();
    void onUninstall();
    void onValidate();

private:
    void updateStats();
    void showItemDetails(const ContentBrowser::ContentItem& item);
    QString formatSize(qint64 bytes) const;

    QString m_contentPath;

    // UI
    QTreeWidget* m_categoryTree;
    QTableWidget* m_contentTable;
    QLineEdit* m_searchEdit;
    QComboBox* m_typeFilter;
    QLabel* m_statsLabel;

    QLabel* m_previewLabel;
    QLabel* m_detailName;
    QLabel* m_detailType;
    QLabel* m_detailAuthor;
    QLabel* m_detailVersion;
    QLabel* m_detailRating;
    QLabel* m_detailSize;
    QLabel* m_detailPath;
    QPushButton* m_installBtn;
    QPushButton* m_uninstallBtn;
    QPushButton* m_validateBtn;

    ContentBrowser::ContentFilter m_currentFilter;
    QVector<ContentBrowser::ContentItem> m_currentItems;
};

} // namespace ks
