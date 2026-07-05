#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include "KsPaths.h"
using KsPaths = ks::SimInstallDetector;

namespace ks {

class KsPathDetector : public QObject {
    Q_OBJECT

public:
    explicit KsPathDetector(QObject* parent = nullptr);
    ~KsPathDetector();

    QString detect();
    bool isDetected() const { return !m_simPath.isEmpty(); }
    QString simPath() const { return m_simPath; }
    QString contentPath() const { return m_contentPath; }

    QStringList carList() const { return m_cars; }
    QStringList trackList() const { return m_tracks; }

signals:
    void detected(const QString& path);
    void error(const QString& message);

private:
    QString m_simPath;
    QString m_contentPath;
    QStringList m_cars;
    QStringList m_tracks;
};

} // namespace ks