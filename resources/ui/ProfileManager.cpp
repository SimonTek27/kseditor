#include "ProfileManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QCoreApplication>

static QString getProfilePath() {
    return QCoreApplication::applicationDirPath() + "/profile.json";
}

QJsonObject ProfileManager::loadProfile() {
    QFile file(getProfilePath());
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull()) {
            return doc.object();
        }
    }
    QJsonObject defaultProfile;
    defaultProfile["version"] = "1.16";
    defaultProfile["lastModified"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    return defaultProfile;
}

void ProfileManager::saveProfile(const QJsonObject& obj) {
    QFile file(getProfilePath());
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(obj);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}
