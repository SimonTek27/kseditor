#pragma once
#include <QApplication>
#include <QString>

class SplashScreen {
public:
    static void showSplash(QApplication& app, const QString& mode = "");
};
