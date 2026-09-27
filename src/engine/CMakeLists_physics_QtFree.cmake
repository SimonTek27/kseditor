# Qt-free physics sources for ksengine (include from engine CMake or SimulatorApp)
# Paths relative to src/engine/physics or flat physics/ depending on layout.

set(KSENGINE_PHYSICS_QT_FREE_SOURCES
    PhysicsCoreTypes.h
    PhysicsTypes.h
    PhysicsEngine.h
    PhysicsEngine.cpp
    PhysicsLogger.h
    PhysicsLogger.cpp
    PhysicsProfiler.h
    PhysicsProfiler.cpp
    AeroDraft.h
    AeroModel.h
    AeroModel.cpp
    AeroSimulator.h
    AeroSimulator.cpp
    PacejkaTireModel.h
    PacejkaTireModel.cpp
    TireFlatSpot.h
    TireSimulator.h
    TireSimulator.cpp
    EngineModel.h
    EngineModel.cpp
    WeatherPhysics.h
    WeatherPhysics.cpp
    SuspensionModel.h
    SuspensionModel.cpp
    SuspensionKinematics.h
    DifferentialModel.h
    DifferentialModel.cpp
    DamageSystem.h
    DamageSystem.cpp
    BrakeThermalModel.h
    BrakeThermalModel.cpp
    TrackSurface.h
    ChassisSimulator.h
    HybridSystem.h
    phys_Simulator.h
    phys_Simulator.cpp
)

# Optional note for SimulatorApp:
# target_link_libraries(SimulatorApp PRIVATE ksengine)
# target_include_directories(SimulatorApp PRIVATE ${KSENGINE_ROOT}/physics)
