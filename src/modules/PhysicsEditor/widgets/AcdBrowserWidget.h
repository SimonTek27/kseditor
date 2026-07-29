#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QString>

namespace ks {

class AcdBrowserWidget : public QWidget {
    Q_OBJECT
public:
    explicit AcdBrowserWidget(QWidget* parent = nullptr);

    void setExtractedPath(const QString& path);
    void refresh();

signals:
    void fileSelected(const QString& path);

private slots:
    void onItemClicked(QTreeWidgetItem* item, int col);
    void onExportSelected();

private:
    void buildUI();
    QString detectFileType(const QString& ext) const;

    QTreeWidget*      m_tree        = nullptr;
    QTextEdit*        m_preview     = nullptr;
    QPushButton*      m_exportBtn   = nullptr;
    QString           m_extractPath;
};

} // namespace ks