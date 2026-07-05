#include "WindowSplitter.h"

namespace ks {

WindowSplitter::WindowSplitter(Qt::Orientation orientation, QWidget* parent)
    : QSplitter(orientation, parent)
{
}

void WindowSplitter::saveSettings(QSettings& settings, const QString& key) const
{
    saveSizesToSettings(settings, key);
}

bool WindowSplitter::restoreSettings(QSettings& settings, const QString& key)
{
    return loadSizesFromSettings(settings, key);
}

QList<int> WindowSplitter::getWidgetSizes() const
{
    QList<int> sizes;
    for (int i = 0; i < count(); ++i) {
        if (QSplitter* splitter = qobject_cast<QSplitter*>(widget(i))) {
            sizes << (orientation() == Qt::Horizontal
                      ? splitter->width()
                      : splitter->height());
        } else {
            sizes << (orientation() == Qt::Horizontal
                      ? widget(i)->size().width()
                      : widget(i)->size().height());
        }
    }
    return sizes;
}

void WindowSplitter::setWidgetSizes(const QList<int>& sizes)
{
    if (sizes.size() != count()) return;

    for (int i = 0; i < sizes.size(); ++i) {
        if (orientation() == Qt::Horizontal) {
            widget(i)->resize(sizes[i], height());
        } else {
            widget(i)->resize(width(), sizes[i]);
        }
    }
}

void WindowSplitter::saveSizesToSettings(QSettings& settings, const QString& key) const
{
    if (count() == 0) return;

    QString stateKey = key + "/splitter";

    settings.setValue(stateKey + "/totalWidth", width());
    settings.setValue(stateKey + "/totalHeight", height());

    for (int i = 0; i < count(); ++i) {
        QString widgetKey = stateKey + QString("/widget_%1").arg(i);
        settings.setValue(widgetKey, orientation() == Qt::Horizontal
                                      ? widget(i)->size().width()
                                      : widget(i)->size().height());
    }
}

bool WindowSplitter::loadSizesFromSettings(QSettings& settings, const QString& key)
{
    QString stateKey = key + "/splitter";

    if (!settings.contains(stateKey + "/totalWidth") &&
        !settings.contains(stateKey + "/totalHeight")) {
        return false;
    }

    QList<int> sizes;
    for (int i = 0; i < count(); ++i) {
        QString widgetKey = stateKey + QString("/widget_%1").arg(i);
        sizes << settings.value(widgetKey, count() > 0
                                           ? (orientation() == Qt::Horizontal ? width() : height()) / count()
                                           : 100).toInt();
    }

    setWidgetSizes(sizes);
    return true;
}

} // namespace ks
