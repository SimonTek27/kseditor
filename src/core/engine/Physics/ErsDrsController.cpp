#include "ErsDrsController.h"
#include "HybridSystem.h"
#include <algorithm>

namespace ks {
namespace physics {

ErsDrsController::ErsDrsController() = default;

void ErsDrsController::setHybridSystem(std::unique_ptr<HybridSystem> sys) {
    m_hybridSystem = std::move(sys);
}

HybridSystem& ErsDrsController::hybridSystem() { return *m_hybridSystem; }
const HybridSystem& ErsDrsController::hybridSystem() const { return *m_hybridSystem; }

void ErsDrsController::reset() {
    m_ers = ErsState();
    m_drs = DrsState();
}

void ErsDrsController::activateAttackMode() {
    if (m_hybridSystem) {
        m_hybridSystem->activateAttackMode();
    }
}

void ErsDrsController::setDrsDragReduction(double f) {
    m_drs.dragReduction = std::clamp(f, 0.0, 1.0);
}

void ErsDrsController::update(const ErsDrsInput& input) {
    updateDrs(input.speed);
    updateErs(input.dt, input.rpm, input.throttle, input.brake, input.engineTorque);
}

void ErsDrsController::updateDrs(double speed) {
    if (m_drs.enabled && m_drs.autoActivate && speed > m_drs.speedThreshold / 3.6) {
        m_drs.active = true;
    } else {
        m_drs.active = false;
    }
}

void ErsDrsController::updateErs(double dt, double rpm, double throttle, double brake, double engineTorque) {
    if (m_hybridSystem) {
        m_hybridSystem->update(dt, rpm, throttle, brake,
            0.0f, 1.0f, engineTorque, 0.0f, 0.0f);
        m_ers.deployTorque = m_hybridSystem->getDeployTorque();
        m_ers.regenTorque = m_hybridSystem->getRegenTorque();
        m_ers.batterySoc = m_hybridSystem->getSoc();
        m_ers.batteryTemp = m_hybridSystem->getBatteryTemp();
    } else if (m_ers.enabled) {
        m_ers.deployTorque = throttle > 0.5 ? 100.0 : 0.0;
        m_ers.regenTorque = brake > 0.5 ? 50.0 : 0.0;
    }
}

} // namespace physics
} // namespace ks
