#pragma once
#include <QJsonObject>

class ProfileManager {
public:
    static QJsonObject loadProfile();
    static void saveProfile(const QJsonObject& obj);
};
