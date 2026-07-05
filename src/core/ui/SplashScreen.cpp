#include "SplashScreen.h"
#include <QSplashScreen>
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QColor>
#include <QTimer>
#include <QCoreApplication>

static QString getSplashPath() { return ":/assets/splash.png"; }

void SplashScreen::showSplash(QApplication& app) {
    QPixmap splashPix(getSplashPath());
    if (splashPix.isNull()) {
        splashPix = QPixmap(600, 400);
        splashPix.fill(QColor(30, 30, 32));
        QPainter p(&splashPix);
        p.setPen(Qt::white);
        p.setFont(QFont("Segoe UI", 24, QFont::Bold));
        p.drawText(splashPix.rect(), Qt::AlignCenter, "ksEditor 1.16");
        p.setFont(QFont("Segoe UI", 12));
        p.drawText(splashPix.rect().translated(0, 40), Qt::AlignCenter, "Loading...");
        p.end();
    }

    QSplashScreen* splash = new QSplashScreen(splashPix, Qt::WindowStaysOnTopHint);
    splash->setWindowFlag(Qt::FramelessWindowHint);
    splash->show();
    app.processEvents();

    QTimer::singleShot(500, splash, [splash]() {
        splash->hide();
        splash->deleteLater();
    });
}
