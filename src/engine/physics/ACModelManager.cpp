#include "ACModelManager.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

// AC model includes
#include "sdk/kseditor/plugins/simulators/kunos/assettocorsa/physics/PacejkaTireModel.h"
#include "sdk/kseditor/plugins/simulators/kunos/assettocorsa/physics/EngineModel.h"
#include "sdk/kseditor/plugins/simulators/kunos/assettocorsa/physics/AeroModel.h"
#include "sdk/kseditor/plugins/simulators/kunos/assettocorsa/physics/DifferentialModel.h"
#include "sdk/kseditor/plugins/simulators/kunos/assettocorsa/physics/SuspensionModel.h"
#include "sdk/kseditor/plugins/simulators/kunos/assettocorsa/physics/BrakeThermalModel.h"
#include "sdk/kseditor/plugins/simulators/kunos/assettocorsa/physics/HybridSystem.h"

namespace ks {
namespace physics {

// ============================================================================
// ACModelManager Implementation
// ============================================================================

ACModelManager::ACModelManager() {
    // Initialize with default models
    m_tireModel = std::make_unique<PacejkaTireModel>();
    m_engineModel = std::make_unique<EngineModel>();
    m_aeroModel = std::make_unique<AeroModel>();
    m_differentialModel = std::make_unique<DifferentialModel>();
    m_suspensionModel = std::make_unique<SuspensionModel>();
    m_brakeModel = std::make_unique<BrakeThermalModel>();
    m_hybridSystem = std::make_unique<HybridSystem>();
}

ACModelManager::~ACModelManager() {
    clear();
}

PacejkaTireModel* ACModelManager::tireModel() {
    return m_tireModel.get();
}

EngineModel* ACModelManager::engineModel() {
    return m_engineModel.get();
}

AeroModel* ACModelManager::aeroModel() {
    return m_aeroModel.get();
}

DifferentialModel* ACModelManager::differentialModel() {
    return m_differentialModel.get();
}

SuspensionModel* ACModelManager::suspensionModel() {
    return m_suspensionModel.get();
}

BrakeThermalModel* ACModelManager::brakeModel() {
    return m_brakeModel.get();
}

HybridSystem* ACModelManager::hybridSystem() {
    return m_hybridSystem.get();
}

const PacejkaTireModel* ACModelManager::tireModel() const {
    return m_tireModel.get();
}

const EngineModel* ACModelManager::engineModel() const {
    return m_engineModel.get();
}

const AeroModel* ACModelManager::aeroModel() const {
    return m_aeroModel.get();
}

const DifferentialModel* ACModelManager::differentialModel() const {
    return m_differentialModel.get();
}

const SuspensionModel* ACModelManager::suspensionModel() const {
    return m_suspensionModel.get();
}

const BrakeThermalModel* ACModelManager::brakeModel() const {
    return m_brakeModel.get();
}

const HybridSystem* ACModelManager::hybridSystem() const {
    return m_hybridSystem.get();
}

bool ACModelManager::loadModels(const QString& basePath) {
    if (basePath.isEmpty()) {
        qWarning() << "ACModelManager: empty basePath";
        return false;
    }
    QDir baseDir(basePath);
    if (!baseDir.exists()) {
        qWarning() << "ACModelManager: basePath does not exist" << basePath;
        return false;
    }
    auto resolve = [&](const QString& name) -> QString {
        QString p1 = baseDir.filePath("data/" + name);
        if (QFileInfo::exists(p1)) return p1;
        QString p2 = baseDir.filePath(name);
        if (QFileInfo::exists(p2)) return p2;
        return {};
    };
    bool anyLoaded = false;
    QString tyresIni = resolve("tyres.ini");
    if (!tyresIni.isEmpty() && m_tireModel) {
        m_tireModel->loadFromIni(tyresIni);
        anyLoaded = true;
        qDebug() << "ACModelManager: loaded tyres" << tyresIni;
    }
    QString engineIni = resolve("engine.ini");
    if (!engineIni.isEmpty() && m_engineModel) {
        m_engineModel->loadFromIni(engineIni);
        anyLoaded = true;
        qDebug() << "ACModelManager: loaded engine" << engineIni;
    }
    QString aeroIni = resolve("aero.ini");
    if (!aeroIni.isEmpty() && m_aeroModel) {
        m_aeroModel->loadFromIniFile(aeroIni);
        anyLoaded = true;
        qDebug() << "ACModelManager: loaded aero" << aeroIni;
    }
    QString drivetrainIni = resolve("drivetrain.ini");
    if (!drivetrainIni.isEmpty() && m_differentialModel) {
        m_differentialModel->loadFromIni(drivetrainIni);
        anyLoaded = true;
        qDebug() << "ACModelManager: loaded drivetrain" << drivetrainIni;
    }
    QString suspensionIni = resolve("suspension.ini");
    if (!suspensionIni.isEmpty() && m_suspensionModel) {
        m_suspensionModel->loadFromIni(suspensionIni);
        anyLoaded = true;
        qDebug() << "ACModelManager: loaded suspension" << suspensionIni;
    }
    QString brakesIni = resolve("brakes.ini");
    if (!brakesIni.isEmpty() && m_brakeModel) {
        m_brakeModel->loadFromIni(brakesIni);
        anyLoaded = true;
        qDebug() << "ACModelManager: loaded brakes" << brakesIni;
    }
    m_modelsLoaded = true;
    qDebug() << "ACModelManager: Models loaded from" << basePath << "anyLoaded=" << anyLoaded;
    return true;
}

void ACModelManager::reset() {
    if (m_engineModel) m_engineModel->reset();
    if (m_differentialModel) m_differentialModel->reset();
    if (m_suspensionModel) m_suspensionModel->reset();
    if (m_brakeModel) m_brakeModel->reset();
    if (m_hybridSystem) m_hybridSystem->reset();
    
    qDebug() << "ACModelManager: All models reset";
}

void ACModelManager::clear() {
    m_tireModel.reset();
    m_engineModel.reset();
    m_aeroModel.reset();
    m_differentialModel.reset();
    m_suspensionModel.reset();
    m_brakeModel.reset();
    m_hybridSystem.reset();
    
    m_modelsLoaded = false;
    
    qDebug() << "ACModelManager: All models cleared";
}

// ============================================================================
// ACModelRegistry Implementation
// ============================================================================

ACModelRegistry& ACModelRegistry::instance() {
    static ACModelRegistry instance;
    return instance;
}

void ACModelRegistry::registerManager(const QString& name, ACModelManager* manager) {
    m_managers[name] = manager;
    
    if (m_defaultManagerName.isEmpty()) {
        m_defaultManagerName = name;
    }
    
    qDebug() << "ACModelRegistry: Registered manager" << name;
}

void ACModelRegistry::unregisterManager(const QString& name) {
    m_managers.remove(name);
    
    if (m_defaultManagerName == name) {
        m_defaultManagerName = m_managers.isEmpty() ? QString() : m_managers.keys().first();
    }
    
    qDebug() << "ACModelRegistry: Unregistered manager" << name;
}

ACModelManager* ACModelRegistry::getManager(const QString& name) {
    return m_managers.value(name, nullptr);
}

ACModelManager* ACModelRegistry::defaultManager() {
    return m_managers.value(m_defaultManagerName, nullptr);
}

void ACModelRegistry::clearAll() {
    m_managers.clear();
    m_defaultManagerName.clear();
    
    qDebug() << "ACModelRegistry: All managers cleared";
}

} // namespace physics
} // namespace ks