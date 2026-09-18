#pragma once
#include "../../base/PluginBase.h"
#include <QObject>

namespace ks {
namespace plugins {
namespace kunos {

class KsPlugin : public QObject, public PluginBase {
    Q_OBJECT

public:
    static KsPlugin* instance();

    QString id() const override { return "ks"; }
    QString name() const override { return "Kunos"; }
    QString version() const override { return "1.4"; }
    QString description() const override { return "Support for Kunos Simulazioni"; }

    bool initialize() override;
    void shutdown() override;

    bool isInitialized() const override { return m_initialized; }
    bool isAvailable() const override;

    QString installPath() const override { return m_installPath; }
    void setInstallPath(const QString& path) override;

    QStringList supportedFileExtensions() const override;
    QStringList supportedContentTypes() const override;

    QString getContentDirectory() const;
    QString getCarsDirectory() const;
    QString getTracksDirectory() const;
    QString getDriversDirectory() const;
    QString getSkinsDirectory() const;
    QString getShadersDirectory() const;

    QStringList getCarList() const;
    QStringList getTrackList() const;

    QString findCarPath(const QString& carId) const;
    QString findTrackPath(const QString& trackId) const;

    bool detectInstallation();
    void setDefaultInstallationPath(const QString& path);

signals:
    void installationDetected(const QString& path);
    void installationChanged(const QString& path);

private:
    explicit KsPlugin(QObject* parent = nullptr);
    ~KsPlugin() override;

    QStringList getDefaultInstallationPaths() const;

    static KsPlugin* s_instance;
};

}
}
}