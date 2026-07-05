#pragma once
#include <QSplitter>
#include <QSettings>
#include <QList>

namespace ks {

class WindowSplitter : public QSplitter {
    Q_OBJECT
public:
    explicit WindowSplitter(Qt::Orientation orientation = Qt::Horizontal, QWidget* parent = nullptr);
    void saveSettings(QSettings& settings, const QString& key) const;
    bool restoreSettings(QSettings& settings, const QString& key);
    QList<int> getWidgetSizes() const;
    void setWidgetSizes(const QList<int>& sizes);

private:
    void saveSizesToSettings(QSettings& settings, const QString& key) const;
    bool loadSizesFromSettings(QSettings& settings, const QString& key);
};

} // namespace ks
