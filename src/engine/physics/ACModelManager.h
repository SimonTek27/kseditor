#pragma once

/**
 * @file ACModelManager.h
 * @brief Manages lifetime and ownership of AC model instances
 * @copyright KS Physics Engine
 */

#include <memory>
#include <QMap>
#include <QString>

// Forward declarations for AC models (global namespace)
class PacejkaTireModel;
class EngineModel;
class AeroModel;
class DifferentialModel;
class SuspensionModel;
class BrakeThermalModel;
class HybridSystem;

namespace ks {
namespace physics {

// ============================================================================
// AC Model Manager
// ============================================================================

class ACModelManager {
public:
    ACModelManager();
    ~ACModelManager();
    
    // Non-copyable, movable
    ACModelManager(const ACModelManager&) = delete;
    ACModelManager& operator=(const ACModelManager&) = delete;
    ACModelManager(ACModelManager&&) = default;
    ACModelManager& operator=(ACModelManager&&) = default;
    
    // Model access (creates if not exists)
    PacejkaTireModel* tireModel();
    EngineModel* engineModel();
    AeroModel* aeroModel();
    DifferentialModel* differentialModel();
    SuspensionModel* suspensionModel();
    BrakeThermalModel* brakeModel();
    HybridSystem* hybridSystem();
    
    // Model access (const versions)
    const PacejkaTireModel* tireModel() const;
    const EngineModel* engineModel() const;
    const AeroModel* aeroModel() const;
    const DifferentialModel* differentialModel() const;
    const SuspensionModel* suspensionModel() const;
    const BrakeThermalModel* brakeModel() const;
    const HybridSystem* hybridSystem() const;
    
    // Check if models are loaded
    bool areModelsLoaded() const { return m_modelsLoaded; }
    
    // Load models from path
    bool loadModels(const QString& basePath);
    
    // Reset all models
    void reset();
    
    // Clear all models (release memory)
    void clear();

private:
    // AC model instances (owned by this manager)
    std::unique_ptr<PacejkaTireModel> m_tireModel;
    std::unique_ptr<EngineModel> m_engineModel;
    std::unique_ptr<AeroModel> m_aeroModel;
    std::unique_ptr<DifferentialModel> m_differentialModel;
    std::unique_ptr<SuspensionModel> m_suspensionModel;
    std::unique_ptr<BrakeThermalModel> m_brakeModel;
    std::unique_ptr<HybridSystem> m_hybridSystem;
    
    bool m_modelsLoaded = false;
};

// ============================================================================
// Singleton Access (for global model management)
// ============================================================================

class ACModelRegistry {
public:
    static ACModelRegistry& instance();
    
    // Register/unregister model managers
    void registerManager(const QString& name, ACModelManager* manager);
    void unregisterManager(const QString& name);
    
    // Get manager by name
    ACModelManager* getManager(const QString& name);
    
    // Get default manager
    ACModelManager* defaultManager();
    
    // Clear all managers
    void clearAll();

private:
    ACModelRegistry() = default;
    
    QMap<QString, ACModelManager*> m_managers;
    QString m_defaultManagerName;
};

} // namespace physics
} // namespace ks