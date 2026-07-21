#include "SplashScreen.h"
#include <QSplashScreen>
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QColor>
#include <QTimer>
#include <QCoreApplication>

static QString getSplashPath(const QString& mode) {
    if (mode == "paint")
        return ":/assets/paint_splash.png";
    return ":/assets/splash.png";
}

void SplashScreen::showSplash(QApplication& app, const QString& mode) {
    QPixmap splashPix(getSplashPath(mode));
    if (splashPix.isNull()) {
        splashPix = QPixmap(600, 400);
        splashPix.fill(QColor(26, 26, 28));

        QPainter p(&splashPix);
        p.setRenderHint(QPainter::Antialiasing);

        if (mode == "paint") {
            // PhotoGIMP-inspired paint splash
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0, 102, 204));
            p.drawRoundedRect(0, 0, 600, 4, 0, 0);

            p.setPen(QColor(0, 136, 255));
            p.setFont(QFont("Segoe UI", 26, QFont::Bold));
            p.drawText(splashPix.rect().adjusted(0, 0, 0, -60), Qt::AlignCenter, "LiveryEditor");

            p.setPen(QColor(153, 153, 153));
            p.setFont(QFont("Segoe UI", 12));
            p.drawText(splashPix.rect().adjusted(0, 50, 0, -30), Qt::AlignCenter, "PhotoGIMP-inspired Paint Mode");

            p.setPen(QColor(102, 102, 102));
            p.setFont(QFont("Segoe UI", 10));
            p.drawText(splashPix.rect().adjusted(0, 0, -20, -10), Qt::AlignBottom | Qt::AlignRight, "Loading...");
        } else {
            p.setPen(Qt::white);
            p.setFont(QFont("Segoe UI", 24, QFont::Bold));
            p.drawText(splashPix.rect(), Qt::AlignCenter, "ksEditor 1.16");
            p.setFont(QFont("Segoe UI", 12));
            p.drawText(splashPix.rect().translated(0, 40), Qt::AlignCenter, "Loading...");
        }
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
