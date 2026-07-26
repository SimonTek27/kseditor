// Assetto Corsa SDK Integration Example
// References only headers that actually exist in the plugin layer.

#include "SDKBackend.h"
#include "plugins/simulators/kunos/assettocorsa/ksAssettoCorsaIni.h"
#include "plugins/simulators/kunos/assettocorsa/KsAssettoCorsaContentPath.h"
#include "plugins/simulators/kunos/assettocorsa/ksAssettoCorsaConfig.h"
#include "plugins/simulators/kunos/assettocorsa/ksAssettoCorsaSetup.h"
#include <QDebug>

using namespace ks;

// Example: listing and loading cars from the AC content directory
void exampleLoadCar()
{
    QStringList cars = SDKBackend::getCarList();
    qDebug() << "Found" << cars.size() << "cars";

    for (const QString& carId : cars) {
        KsCarSpec spec;
        if (SDKBackend::loadCarSpec(carId, spec)) {
            qDebug() << "Car:" << spec.screenName
                     << "Mass:" << spec.totalMass << "kg"
                     << "Steer lock:" << spec.steerLock << "deg";
        }
    }
}

// Example: basic physics helpers exposed by SDKBackend
void examplePhysicsCalc()
{
    const float speedMs = 80.0f;   // ~288 km/h
    const float radius  = 150.0f;  // metres
    const float aoa     = 5.0f;    // degrees
    const float cl      = 1.2f;    // lift coefficient

    float downforce  = SDKBackend::calculateDownforce(speedMs, aoa, cl);
    float cornerG    = SDKBackend::calculateCornerG(speedMs, radius);
    float stoppingDist = SDKBackend::calculateStoppingDistance(speedMs, 1.2f);

    qDebug() << "At" << speedMs * 3.6f << "km/h:";
    qDebug() << "  Downforce:"       << downforce      << "N";
    qDebug() << "  Corner G:"        << cornerG        << "g";
    qDebug() << "  Stopping dist:"   << stoppingDist   << "m";
}
