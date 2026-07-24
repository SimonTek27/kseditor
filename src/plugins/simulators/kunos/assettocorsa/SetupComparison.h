#pragma once
#include <QObject>
#include <QString>
#include <QMap>

namespace ks {

struct CarSetup {
    QString name;
    QString track;
    QString timestamp;
    
    // Suspension
    double frontRideHeight;
    double rearRideHeight;
    double frontSpringRate;
    double rearSpringRate;
    double frontComp;
    double rearComp;
    double frontReb;
    double rearReb;
    
    // Aero
    double frontWing;
    double rearWing;
    int diffPreload;
    int diffAccel;
    int diffDecel;
    
    // Brakes
    double brakeBias;
    double frontBrakePressure;
    double rearBrakePressure;
    
    // Tires
    double frontPressureL;
    double frontPressureR;
    double rearPressureL;
    double rearPressureR;
    double frontCamber;
    double rearCamber;
    double frontToe;
    double rearToe;
    
    // Electronics
    int tractionControl;
    int absLevel;
    int engineMap;

    // Extra fields not captured by named properties
    QStringList extra;
};

struct SetupComparisonData {
    QString setupA;
    QString setupB;
    QMap<QString, QString> differences;
    QString bestSetup;
    double deltaTime;
};

class SetupComparison : public QObject {
    Q_OBJECT

public:
    static SetupComparison* instance();

    void loadSetupA(const QString& filePath);
    void loadSetupB(const QString& filePath);
    void saveSetup(const QString& name, const CarSetup& setup);
    
    SetupComparisonData compare();
    QStringList getSavedSetups() const;
    CarSetup getSetup(const QString& name) const;
    
    CarSetup getCurrentSetup() const { return m_currentSetup; }
    void setCurrentSetup(const CarSetup& setup) { m_currentSetup = setup; }

signals:
    void setupLoaded(const QString& name);
    void comparisonReady(const SetupComparisonData& result);

private:
    explicit SetupComparison(QObject* parent = nullptr);
    static SetupComparison* s_instance;
    
    CarSetup m_currentSetup;
    CarSetup m_setupA;
    CarSetup m_setupB;
    QMap<QString, CarSetup> m_savedSetups;
};

}