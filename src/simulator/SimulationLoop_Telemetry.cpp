#include "SimulationLoop.h"
#include "UdpTelemetryBridge.h"
#include "TcpTelemetryBridge.h"
#include "adapters/assetto_corsa/AcSharedMemoryPublisher.h"
#if HAS_VEHICLE_SIM
#include "engine/physics/VehicleSimulator.h"
#endif
#include <cstdio>

namespace ks::sim {

void SimulationLoop::publishSharedMemory() {
    if (!m_shm || !m_shmEnabled) return;
#if HAS_VEHICLE_SIM
    if (!m_vehicle) return;
    const auto st = m_vehicle->getState();
    const auto& ffb = m_vehicle->ffbSample();
    ks::ac::AcLiveInput live;
    live.throttle = static_cast<float>(st.throttle);
    live.brake = static_cast<float>(st.brake);
    live.steer = static_cast<float>(st.steering);
    live.speedMs = static_cast<float>(st.speed);
    live.rpm = static_cast<float>(m_vehicle->rpm());
    live.gear = m_vehicle->currentGear();
    live.fuel = static_cast<float>(st.fuel);
    live.velocity[0] = st.velocity.x; live.velocity[1] = st.velocity.y; live.velocity[2] = st.velocity.z;
    live.accG[0] = st.acceleration.x / 9.81f; live.accG[1] = st.acceleration.y / 9.81f; live.accG[2] = st.acceleration.z / 9.81f;
    live.heading = st.heading;
    live.wheelSlip[0] = ffb.slipAngleFL; live.wheelSlip[1] = ffb.slipAngleFR;
    live.wheelLoad[0] = ffb.loadFL; live.wheelLoad[1] = ffb.loadFR;
    live.finalFF = ffb.aligningMomentNm;
    for (int i = 0; i < 4; ++i) {
        live.tyreTemp[i] = static_cast<float>(st.tyreTemp[i]);
        live.tyreWear[i] = static_cast<float>(st.tyreWear[i]);
        live.tyrePressure[i] = static_cast<float>(st.tyrePressure[i]);
    }
    live.carX = st.position.x; live.carY = st.position.y; live.carZ = st.position.z;
    live.normalizedSpline = m_normalizedSpline;
    live.distanceTraveled = static_cast<float>(m_lapDistance);
    live.airTemp = m_weather.ambientTemp; live.roadTemp = m_weather.trackTemp;
    live.completedLaps = m_lapTimer.completedLaps();
    live.currentSector = m_lapTimer.sectorIndex();
    live.iCurrentTimeMs = m_lapTimer.currentTimeMs();
    live.iLastTimeMs = m_lapTimer.lastTimeMs();
    live.iBestTimeMs = m_lapTimer.bestTimeMs();
    live.sessionType = static_cast<int>(m_sessionType);
    live.status = m_running ? 2 : 0;
    live.inPit = st.inPitLane; live.pitLimiter = st.pitLimiterActive;
    live.carModel = m_carName; live.trackName = m_trackData.name;
    live.maxRpm = 8500; live.totalLaps = m_totalLaps;
    live.sectorCount = m_lapTimer.sectorCount();
    live.sessionTimeLeft = static_cast<float>(m_timeRemaining);
    live.trackSplineLength = m_trackData.splineLength;
    m_shm->publish(live);
#endif
}

void SimulationLoop::publishUdpTelemetry() {
    if (!m_udp || !m_udpEnabled) return;
#if HAS_VEHICLE_SIM
    if (!m_vehicle) return;
    const auto st = m_vehicle->getState();
    UdpTelemSample s;
    s.timeSec = m_simTime;
    s.speedMs = static_cast<float>(st.speed);
    s.rpm = static_cast<float>(m_vehicle->rpm());
    s.throttle = static_cast<float>(st.throttle);
    s.brake = static_cast<float>(st.brake);
    s.steer = static_cast<float>(st.steering);
    s.gear = m_vehicle->currentGear();
    s.fuelL = static_cast<float>(st.fuel);
    s.posX = st.position.x; s.posY = st.position.y; s.posZ = st.position.z;
    s.velX = st.velocity.x; s.velY = st.velocity.y; s.velZ = st.velocity.z;
    s.heading = st.heading;
    s.completedLaps = m_lapTimer.completedLaps();
    s.currentSector = m_lapTimer.sectorIndex();
    s.currentTimeMs = m_lapTimer.currentTimeMs();
    s.lastTimeMs = m_lapTimer.lastTimeMs();
    s.bestTimeMs = m_lapTimer.bestTimeMs();
    s.sessionType = static_cast<int>(m_sessionType);
    s.status = m_running ? 2 : 0;
    s.normalizedSpline = m_normalizedSpline;
    s.airTemp = m_weather.ambientTemp;
    s.roadTemp = m_weather.trackTemp;
    s.inPit = st.inPitLane;
    s.pitLimiter = st.pitLimiterActive;
    m_udp->publish(s);
#endif
}

void SimulationLoop::publishTcpTelemetry() {
    if (!m_tcp || !m_tcpEnabled) return;
#if HAS_VEHICLE_SIM
    if (!m_vehicle) return;
    const auto st = m_vehicle->getState();
    UdpTelemSample s;
    s.timeSec = m_simTime;
    s.speedMs = static_cast<float>(st.speed);
    s.rpm = static_cast<float>(m_vehicle->rpm());
    s.throttle = static_cast<float>(st.throttle);
    s.brake = static_cast<float>(st.brake);
    s.steer = static_cast<float>(st.steering);
    s.gear = m_vehicle->currentGear();
    s.fuelL = static_cast<float>(st.fuel);
    s.posX = st.position.x; s.posY = st.position.y; s.posZ = st.position.z;
    s.completedLaps = m_lapTimer.completedLaps();
    s.currentTimeMs = m_lapTimer.currentTimeMs();
    s.lastTimeMs = m_lapTimer.lastTimeMs();
    s.bestTimeMs = m_lapTimer.bestTimeMs();
    s.sessionType = static_cast<int>(m_sessionType);
    s.status = m_running ? 2 : 0;
    s.normalizedSpline = m_normalizedSpline;
    m_tcp->publish(s);
#endif
}

} // namespace ks::sim
