#pragma once

#include <QTreeWidget>
#include <QString>
#include <QStringList>

namespace ks {

// ─────────────────────────────────────────────────────────────────────────────
// FileTreeWidget — shows car folder contents (data/ + .ini files)
// ─────────────────────────────────────────────────────────────────────────────
class FileTreeWidget : public QTreeWidget {
    Q_OBJECT
public:
    explicit FileTreeWidget(QWidget* parent = nullptr);

    void setRootPath(const QString& path);
    void refresh();

signals:
    void fileSelected(const QString& absolutePath);

private slots:
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);

private:
    QString m_rootPath;
    static const QStringList s_carDataExtensions;
};

} // namespace ks