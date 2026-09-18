#pragma once

#include <QString>
#include <QMap>
#include <QVariant>
#include <QSettings>
#include <QDir>

namespace ks { namespace kunos {

struct KsGameSettings {
    QString carId;
    QMap<QString, QString> values;

    static QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) {
        QSettings settings(QDir::homePath() + "/Documents/Assetto Corsa/cfg/settings.ini", QSettings::IniFormat);
        settings.beginGroup("GENERAL");
        return settings.value(key, defaultValue);
    }

    static void setValue(const QString& key, const QVariant& value) {
        QSettings settings(QDir::homePath() + "/Documents/Assetto Corsa/cfg/settings.ini", QSettings::IniFormat);
        settings.beginGroup("GENERAL");
        settings.setValue(key, value);
    }
};

}} // namespace ks::kunos
