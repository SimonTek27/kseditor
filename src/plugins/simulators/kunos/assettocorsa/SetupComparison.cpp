#include "SetupComparison.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace ks {

SetupComparison* SetupComparison::s_instance = nullptr;

SetupComparison::SetupComparison(QObject* parent)
    : QObject(parent)
{}

SetupComparison* SetupComparison::instance() {
    if (!s_instance) {
        s_instance = new SetupComparison();
    }
    return s_instance;
}

void SetupComparison::loadSetupA(const QString& filePath) {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            m_setupA.name = obj["name"].toString();
            m_setupA.frontWing = obj["frontWing"].toDouble();
            m_setupA.rearWing = obj["rearWing"].toDouble();
            emit setupLoaded("A");
        }
        file.close();
    }
}

void SetupComparison::loadSetupB(const QString& filePath) {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            m_setupB.name = obj["name"].toString();
            m_setupB.frontWing = obj["frontWing"].toDouble();
            m_setupB.rearWing = obj["rearWing"].toDouble();
            emit setupLoaded("B");
        }
        file.close();
    }
}

void SetupComparison::saveSetup(const QString& name, const CarSetup& setup) {
    m_savedSetups.insert(name, setup);
}

SetupComparisonData SetupComparison::compare() {
    SetupComparisonData result;
    result.setupA = m_setupA.name;
    result.setupB = m_setupB.name;
    
    if (qAbs(m_setupA.frontWing - m_setupB.frontWing) > 0.1) {
        result.differences["frontWing"] = "Different";
    }
    if (qAbs(m_setupA.rearWing - m_setupB.rearWing) > 0.1) {
        result.differences["rearWing"] = "Different";
    }
    
    emit comparisonReady(result);
    return result;
}

QStringList SetupComparison::getSavedSetups() const {
    return m_savedSetups.keys();
}

CarSetup SetupComparison::getSetup(const QString& name) const {
    return m_savedSetups.value(name);
}

}