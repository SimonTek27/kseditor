#include "SimulationLoop.h"
#include "MultiCarManager.h"
#include "NetworkManager.h"
#include "engine/physics/VehiclePhysics.h"
#include "Graphics/RenderSystem.h"
#include "Graphics/VulkanRenderer.h"
#include "Graphics/VulkanFunctions.h"
#include "Graphics/SceneGraph.h"
#include "Graphics/SceneMesh.h"
#include "Graphics/SceneObject.h"
#include "Graphics/TerrainSystem.h"
#include "Graphics/GPUParticleSystem.h"
#include "Graphics/WaterSystem.h"
#include "Graphics/PostProcessingPipeline.h"
#include "Graphics/CascadedShadowMaps.h"
#include "Graphics/VegetationSystem.h"
#include "Graphics/DecalSystem.h"
#include "Graphics/SSRSystem.h"
#include "engine/devices/simracing/SimRacingDevices.h"
#include <QFile>
#include <QCoreApplication>
#ifdef _WIN32
#include "devices/xinput/XInputDevice.h"
#endif
#if HAS_KSNET
#include "sdk/kseditor/plugins/simulators/kunos/assettocorsa/acFiles/KN5Parser.h"
#endif
#include <cmath>
#include <cstring>
#include <filesystem>

namespace ks::sim {
using namespace ks::sim::net;

#if !HAS_YOJIMBO
static constexpr uint8_t SESSION_RACE = 2;
static constexpr uint8_t PHASE_COUNTDOWN = 1;
static constexpr uint8_t PHASE_GREEN_FLAG = 2;
static constexpr uint8_t PHASE_CHECKERED_FLAG = 4;
#endif

static constexpr float DEG_TO_RAD = 3.14159265f / 180.0f;

SimulationLoop::SimulationLoop()
    : m_vulkanMode(true)
{
    m_vehicle = std::make_unique<ks::physics::VehicleSimulator>();
    m_camera = std::make_unique<CameraController>();
    m_audio = std::make_unique<SimulatorAudio>();
    m_input = std::make_unique<InputManager>();
    m_multiCar = std::make_unique<MultiCarManager>();
    m_network = std::make_unique<NetworkManager>(this);
}

SimulationLoop::~SimulationLoop()
{
    stop();
    if (m_ffb) {
        m_ffb->shutdown();
        m_ffb.reset();
    }
}

bool SimulationLoop::initialize()
{
    m_audio->initialize();
    m_input->initialize();
    return true;
}

bool SimulationLoop::loadTrack(const std::string& kn5Path)
{
#if HAS_KSNET
    std::string err;
    KN5Parser::KN5File kn5 = KN5Parser::KN5ParserImpl::parse(QString::fromStdString(kn5Path), &err);
    if (!kn5.isValid()) {
        return false;
    }

    if (m_vulkanMode) {
        auto& rs = ::ks::engine::graphics::RenderSystem::instance();

        for (const auto& mesh : kn5.meshes) {
            if (mesh.isSkinnedMesh) continue;
            if (mesh.vertexData.isEmpty()) continue;

            KN5Parser::Mesh mutableMesh = mesh;
            mutableMesh.decodeVertices();

            QVector<::ks::SceneVertex> vertices;
            vertices.reserve(mutableMesh.positions.size());

            for (int i = 0; i < mutableMesh.positions.size(); ++i) {
                ::ks::SceneVertex sv;
                sv.position = mutableMesh.positions[i];
                sv.normal = (i < mutableMesh.normals.size()) ? mutableMesh.normals[i] : QVector3D(0, 1, 0);
                sv.uv = (i < mutableMesh.uv0.size()) ? mutableMesh.uv0[i] : QVector2D(0, 0);
                sv.color = QVector4D(1, 1, 1, 1);
                sv.tangent = (i < mutableMesh.tangents.size()) ? mutableMesh.tangents[i] : QVector3D(1, 0, 0);
                vertices.append(sv);
            }

            QVector<quint32> indices;
            if (mutableMesh.positions.size() <= 65535 && mutableMesh.indexData.size() >= 2) {
                const quint16* src = reinterpret_cast<const quint16*>(mutableMesh.indexData.constData());
                int count = mutableMesh.indexData.size() / sizeof(quint16);
                for (int i = 0; i < count; ++i) indices.append(src[i]);
            } else if (!mutableMesh.indexData.isEmpty()) {
                const quint32* src = reinterpret_cast<const quint32*>(mutableMesh.indexData.constData());
                int count = mutableMesh.indexData.size() / sizeof(quint32);
                for (int i = 0; i < count; ++i) indices.append(src[i]);
            }

            QMatrix4x4 localTransform;
            localTransform(0, 0) = mesh.transform[0]; localTransform(1, 0) = mesh.transform[1];
            localTransform(2, 0) = mesh.transform[2]; localTransform(3, 0) = mesh.transform[3];
            localTransform(0, 1) = mesh.transform[4]; localTransform(1, 1) = mesh.transform[5];
            localTransform(2, 1) = mesh.transform[6]; localTransform(3, 1) = mesh.transform[7];
            localTransform(0, 2) = mesh.transform[8]; localTransform(1, 2) = mesh.transform[9];
            localTransform(2, 2) = mesh.transform[10]; localTransform(3, 2) = mesh.transform[11];

            if (m_vulkanRenderer) {
                QVector<VulkanRenderer::Vertex> vkVerts;
                vkVerts.reserve(vertices.size());
                for (const auto& sv : vertices) {
                    VulkanRenderer::Vertex v;
                    v.position = sv.position;
                    v.normal = sv.normal;
                    v.uv = sv.uv;
                    v.color = sv.color;
                    vkVerts.append(v);
                }
                QString meshName = QString("track_%1").arg(m_renderables.size());
                m_vulkanRenderer->createMesh(meshName, vkVerts, indices);
                m_renderables.append({meshName, localTransform, indices.size(), 0});
            }
        }

        m_trackLoaded = true;
        return true;
    }

    return false;
#else
    (void)kn5Path;
    return false;
#endif
}

bool SimulationLoop::loadTrackFolder(const std::string& trackDirectory)
{
#if HAS_KSNET
    m_trackData = m_trackLoader.loadTrackFolder(QString::fromStdString(trackDirectory));

    if (!m_trackData.kn5Loaded) {
        return false;
    }

    QString kn5Path = m_trackData.kn5Path;
    std::string err;
    KN5Parser::KN5File kn5 = KN5Parser::KN5ParserImpl::parse(kn5Path, &err);

    if (!kn5.isValid()) {
        return false;
    }

    if (m_vulkanMode) {
        auto& rs = ::ks::engine::graphics::RenderSystem::instance();

        for (const auto& mesh : kn5.meshes) {
            if (mesh.isSkinnedMesh) continue;
            if (mesh.vertexData.isEmpty()) continue;

            KN5Parser::Mesh mutableMesh = mesh;
            mutableMesh.decodeVertices();

            QVector<::ks::SceneVertex> vertices;
            vertices.reserve(mutableMesh.positions.size());

            for (int i = 0; i < mutableMesh.positions.size(); ++i) {
                ::ks::SceneVertex sv;
                sv.position = mutableMesh.positions[i];
                sv.normal = (i < mutableMesh.normals.size()) ? mutableMesh.normals[i] : QVector3D(0, 1, 0);
                sv.uv = (i < mutableMesh.uv0.size()) ? mutableMesh.uv0[i] : QVector2D(0, 0);
                sv.color = QVector4D(1, 1, 1, 1);
                sv.tangent = (i < mutableMesh.tangents.size()) ? mutableMesh.tangents[i] : QVector3D(1, 0, 0);
                vertices.append(sv);
            }

            QVector<quint32> indices;
            if (mutableMesh.positions.size() <= 65535 && mutableMesh.indexData.size() >= 2) {
                const quint16* src = reinterpret_cast<const quint16*>(mutableMesh.indexData.constData());
                int count = mutableMesh.indexData.size() / sizeof(quint16);
                for (int i = 0; i < count; ++i) indices.append(src[i]);
            } else if (!mutableMesh.indexData.isEmpty()) {
                const quint32* src = reinterpret_cast<const quint32*>(mutableMesh.indexData.constData());
                int count = mutableMesh.indexData.size() / sizeof(quint32);
                for (int i = 0; i < count; ++i) indices.append(src[i]);
            }

            QMatrix4x4 localTransform;
            localTransform(0, 0) = mesh.transform[0]; localTransform(1, 0) = mesh.transform[1];
            localTransform(2, 0) = mesh.transform[2]; localTransform(3, 0) = mesh.transform[3];
            localTransform(0, 1) = mesh.transform[4]; localTransform(1, 1) = mesh.transform[5];
            localTransform(2, 1) = mesh.transform[6]; localTransform(3, 1) = mesh.transform[7];
            localTransform(0, 2) = mesh.transform[8]; localTransform(1, 2) = mesh.transform[9];
            localTransform(2, 2) = mesh.transform[10]; localTransform(3, 2) = mesh.transform[11];

            if (m_vulkanRenderer) {
                QVector<VulkanRenderer::Vertex> vkVerts;
                vkVerts.reserve(vertices.size());
                for (const auto& sv : vertices) {
                    VulkanRenderer::Vertex v;
                    v.position = sv.position;
                    v.normal = sv.normal;
                    v.uv = sv.uv;
                    v.color = sv.color;
                    vkVerts.append(v);
                }
                QString meshName = QString("track_%1").arg(m_renderables.size());
                m_vulkanRenderer->createMesh(meshName, vkVerts, indices);
                m_renderables.append({meshName, localTransform, indices.size(), 0});
            }
        }

        m_trackLoaded = true;
        return true;
    }

    return false;
#else
    (void)trackDirectory;
    return false;
#endif
}

bool SimulationLoop::loadCar(const std::string& carDir)
{
#if HAS_KSNET
    std::filesystem::path dir(carDir);
    std::string kn5Path;

    std::string dirName = dir.filename().string();
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.path().extension() == ".kn5") {
            if (entry.path().stem().string() == dirName) {
                kn5Path = entry.path().string();
                break;
            }
        }
    }
    if (kn5Path.empty()) {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.path().extension() == ".kn5") {
                kn5Path = entry.path().string();
                break;
            }
        }
    }

    if (kn5Path.empty()) {
        return false;
    }

    std::string err;
    KN5Parser::KN5File kn5 = KN5Parser::KN5ParserImpl::parse(QString::fromStdString(kn5Path), &err);
    if (!kn5.isValid()) {
        return false;
    }

    if (m_vulkanMode) {
        auto& rs = ::ks::engine::graphics::RenderSystem::instance();

        for (const auto& mesh : kn5.meshes) {
            if (mesh.isSkinnedMesh) continue;
            if (mesh.vertexData.isEmpty()) continue;

            KN5Parser::Mesh mutableMesh = mesh;
            mutableMesh.decodeVertices();

            QVector<::ks::SceneVertex> vertices;
            vertices.reserve(mutableMesh.positions.size());

            for (int i = 0; i < mutableMesh.positions.size(); ++i) {
                ::ks::SceneVertex sv;
                sv.position = mutableMesh.positions[i];
                sv.normal = (i < mutableMesh.normals.size()) ? mutableMesh.normals[i] : QVector3D(0, 1, 0);
                sv.uv = (i < mutableMesh.uv0.size()) ? mutableMesh.uv0[i] : QVector2D(0, 0);
                sv.color = QVector4D(1, 1, 1, 1);
                sv.tangent = (i < mutableMesh.tangents.size()) ? mutableMesh.tangents[i] : QVector3D(1, 0, 0);
                vertices.append(sv);
            }

            QVector<quint32> indices;
            if (mutableMesh.positions.size() <= 65535 && mutableMesh.indexData.size() >= 2) {
                const quint16* src = reinterpret_cast<const quint16*>(mutableMesh.indexData.constData());
                int count = mutableMesh.indexData.size() / sizeof(quint16);
                for (int i = 0; i < count; ++i) indices.append(src[i]);
            } else if (!mutableMesh.indexData.isEmpty()) {
                const quint32* src = reinterpret_cast<const quint32*>(mutableMesh.indexData.constData());
                int count = mutableMesh.indexData.size() / sizeof(quint32);
                for (int i = 0; i < count; ++i) indices.append(src[i]);
            }

            QMatrix4x4 localTransform;
            localTransform(0, 0) = mesh.transform[0]; localTransform(1, 0) = mesh.transform[1];
            localTransform(2, 0) = mesh.transform[2]; localTransform(3, 0) = mesh.transform[3];
            localTransform(0, 1) = mesh.transform[4]; localTransform(1, 1) = mesh.transform[5];
            localTransform(2, 1) = mesh.transform[6]; localTransform(3, 1) = mesh.transform[7];
            localTransform(0, 2) = mesh.transform[8]; localTransform(1, 2) = mesh.transform[9];
            localTransform(2, 2) = mesh.transform[10]; localTransform(3, 2) = mesh.transform[11];

            if (m_vulkanRenderer) {
                QVector<VulkanRenderer::Vertex> vkVerts;
                vkVerts.reserve(vertices.size());
                for (const auto& sv : vertices) {
                    VulkanRenderer::Vertex v;
                    v.position = sv.position;
                    v.normal = sv.normal;
                    v.uv = sv.uv;
                    v.color = sv.color;
                    vkVerts.append(v);
                }
                QString meshName = QString("car_%1").arg(m_renderables.size());
                m_vulkanRenderer->createMesh(meshName, vkVerts, indices);
                m_renderables.append({meshName, localTransform, indices.size(), 0});
            }
        }

        m_carLoaded = true;
    } else {
        return false;
    }

    m_vehicle->setMass(1200);
    m_vehicle->setEnginePower(260);
    m_vehicle->setMaxRpm(8500);
    m_vehicle->setDragCoeff(0.35);
    m_vehicle->setFrontalArea(2.2);
    m_vehicle->setWheelBase(2.6);
    m_vehicle->setTrackWidth(1.6);

    auto tryLoad = [&](const std::string& name, auto loader) {
        std::filesystem::path p1 = dir / "data" / name;
        std::filesystem::path p2 = dir / name;
        if (std::filesystem::exists(p1)) loader(p1.string());
        else if (std::filesystem::exists(p2)) loader(p2.string());
    };
    tryLoad("tyres.ini", [&](const std::string& p){ m_vehicle->loadTyresFromIni(QString::fromStdString(p)); });
    tryLoad("engine.ini", [&](const std::string& p){ m_vehicle->loadEngineFromIni(QString::fromStdString(p)); });
    tryLoad("drivetrain.ini", [&](const std::string& p){ m_vehicle->loadDrivetrainFromIni(QString::fromStdString(p)); });
    tryLoad("aero.ini", [&](const std::string& p){ m_vehicle->loadAeroFromIni(QString::fromStdString(p)); });
    tryLoad("suspension.ini", [&](const std::string& p){ m_vehicle->loadSuspensionFromIni(QString::fromStdString(p)); });

    return true;
#else
    (void)carDir;
    return false;
#endif
}

bool SimulationLoop::loadCarAudio(const std::string& carDirectory)
{
    if (!m_audio->loadCarAudio(carDirectory)) {
        return false;
    }
    return true;
}

void SimulationLoop::start()
{
    if (m_running) return;
    m_running = true;
    m_vehicle->startSimulation();
    m_lastTime = std::chrono::steady_clock::now();

    m_sessionType = SESSION_RACE;
    m_sessionPhase = PHASE_COUNTDOWN;
    m_timeRemaining = 5.0;
    m_currentLap = 0;
    m_totalLaps = 5;
}

void SimulationLoop::stop()
{
    if (!m_running) return;
    m_running = false;
    m_vehicle->stopSimulation();
}

void SimulationLoop::reset()
{
    m_vehicle->reset();
    m_camera->setPosition(vec3(0, 2, 5));
}

void SimulationLoop::tick()
{
    if (!m_running) {
        render();
        return;
    }

    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - m_lastTime).count();
    m_lastTime = now;

    m_simAccumulator += elapsed;

    while (m_simAccumulator >= m_physicsDt) {
        m_input->update();
        applyInput();
        m_vehicle->updatePhysics(m_physicsDt);
        m_simAccumulator -= m_physicsDt;
    }

    if (m_ffb && m_vehicle) {
        float torqueNm = 0.0f;
        float speedKph = m_vehicle->getState().speed * 3.6f;
        m_ffb->updateFFB(torqueNm);

        if (m_input->hasXInput() && m_input->isXInputConnected()) {
#ifdef _WIN32
            m_input->xinput()->applyFFB(torqueNm, speedKph);
#endif
        }
    }

    updateWeather();

    if (m_sessionPhase == PHASE_COUNTDOWN) {
        m_timeRemaining -= elapsed;
        if (m_timeRemaining <= 0.0f) {
            m_sessionPhase = PHASE_GREEN_FLAG;
            m_timeRemaining = 0.0;
            if (onSessionStateChanged) onSessionStateChanged(m_sessionType, m_sessionPhase, m_currentLap, m_totalLaps, m_timeRemaining);
        } else {
            if (onSessionStateChanged) onSessionStateChanged(m_sessionType, m_sessionPhase, m_currentLap, m_totalLaps, m_timeRemaining);
        }
    } else if (m_sessionPhase == PHASE_GREEN_FLAG) {
        m_timeRemaining -= elapsed;
        if (m_timeRemaining <= 0.0f) {
            m_sessionPhase = PHASE_CHECKERED_FLAG;
        }
        if (onSessionStateChanged) onSessionStateChanged(m_sessionType, m_sessionPhase, m_currentLap, m_totalLaps, m_timeRemaining);
    }

    auto state = m_vehicle->getState();
    if (m_sessionPhase == PHASE_GREEN_FLAG && state.speed > 0.1f) {
        static double lastZ = 0.0;
        if ((lastZ <= 0.0 && state.position.z() > 0.0) || (lastZ > 0.0 && state.position.z() <= 0.0)) {
            if (m_currentLap < m_totalLaps) {
                m_currentLap++;
                if (onSessionStateChanged) onSessionStateChanged(m_sessionType, m_sessionPhase, m_currentLap, m_totalLaps, m_timeRemaining);
            }
        }
        lastZ = state.position.z();
    }

    if (m_sessionPhase == PHASE_GREEN_FLAG && m_currentLap >= m_totalLaps && m_totalLaps > 0) {
        m_sessionPhase = PHASE_CHECKERED_FLAG;
        if (onSessionStateChanged) onSessionStateChanged(m_sessionType, m_sessionPhase, m_currentLap, m_totalLaps, m_timeRemaining);
    }

    QMatrix4x4 carBodyMatrix;
    carBodyMatrix.translate(state.position);
    carBodyMatrix.rotate(state.rotation.y() * 57.2957795f, 0, 1, 0);
    carBodyMatrix.rotate(state.rotation.x() * 57.2957795f, 1, 0, 0);
    carBodyMatrix.rotate(state.rotation.z() * 57.2957795f, 0, 0, 1);

    mat4 carMat;
    memcpy(carMat.m, carBodyMatrix.constData(), sizeof(float) * 16);

    float speedKmh = state.speed * 3.6f;
    m_camera->update(m_physicsDt, carMat, speedKmh);

    // Per-wheel surface sampling
    SimulatorAudio::SurfaceType surfaceType = SimulatorAudio::SurfaceType::Asphalt;
    float wetness = m_weather.trackWetness;
    float rainIntensity = m_weather.rainIntensity;

    // Sample TrackSurface at car position for surface type
    auto surfaceSample = ks::engine::physics::TrackSurface::instance().sample(state.position);
    float grip = surfaceSample.grip;
    float cellWetness = surfaceSample.wetness;
    
    // Derive surface type
    if (cellWetness > 0.3f) {
        surfaceType = SimulatorAudio::SurfaceType::Wet;
    } else if (grip < 0.4f) {
        surfaceType = SimulatorAudio::SurfaceType::Grass;
    } else if (grip < 0.6f) {
        surfaceType = SimulatorAudio::SurfaceType::Gravel;
    } else {
        surfaceType = SimulatorAudio::SurfaceType::Asphalt;
    }

    // Store last surface type for csp surfaces check
    m_lastSurfaceType = surfaceType;

    m_audio->updatePhysics(
        state.rpm,
        m_input->throttle(),
        m_input->brake(),
        state.speed,
        m_input->steer(),
        state.gear,
        false,
        0.0f,
        0.0f,
        surfaceType,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        wetness,
        rainIntensity,
        m_physicsDt
    );

    m_dashboard.update(state.speed, state.rpm, state.gear, 0, 0,
                       0, 0, 1e9f, 0, 1, 1);

    m_setupGarage.update(m_physicsDt);

    float lateralG = 0.0f;
    float longitudinalG = 0.0f;
    if (state.speed > 0.1f) {
        longitudinalG = (m_input->throttle() - m_input->brake()) * state.speed * 0.01f;
        lateralG = m_input->steer() * state.speed * 0.005f;
    }
    m_telemetry.update(state.speed, state.rpm, m_input->throttle(),
                       m_input->brake(), m_input->steer(), lateralG, longitudinalG);

    m_physicsFrameCount++;
    m_frameTime += elapsed;
    if (m_frameTime >= 1.0) {
        m_fps = m_physicsFrameCount / m_frameTime;
        m_physicsFrameCount = 0;
        m_frameTime = 0.0;
    }

    render();
}

void SimulationLoop::render()
{
    float sunAngle = (m_timeOfDay - 6.0f) / 12.0f * 3.14159f;
    float elevation = std::sin(sunAngle);
    float horizontal = std::cos(sunAngle);
    QVector3D sunDir(horizontal * 0.5f, std::max(elevation, -0.1f), horizontal * 0.3f);

    float nightFactor = (m_timeOfDay < 6.0f || m_timeOfDay > 19.0f) ? 0.0f : 1.0f;
    float cloudDim = 1.0f - m_weather.cloudCover * 0.3f;
    QVector3D sunColor = QVector3D(1.0f, 0.95f, 0.9f) * nightFactor * cloudDim;
    QVector3D ambientColor(0.3f, 0.35f, 0.4f);

    float t = std::abs(m_timeOfDay - 12.0f) / 6.0f;
    if (t > 0.6f && nightFactor > 0.0f) {
        float sunsetBlend = (t - 0.6f) / 0.4f;
        sunColor = sunColor * (1.0f - sunsetBlend) + QVector3D(1.0f, 0.5f, 0.2f) * sunsetBlend;
        ambientColor = ambientColor * (1.0f - sunsetBlend * 0.3f) + QVector3D(0.4f, 0.3f, 0.25f) * sunsetBlend * 0.3f;
    }

    if (m_vulkanMode && m_vulkanRenderer) {
        auto& rs = ::ks::engine::graphics::RenderSystem::instance();
        auto& streamline = ::ks::engine::graphics::StreamlineIntegration::instance();
        QMatrix4x4 viewMat, projMat;
        memcpy(viewMat.data(), m_camera->viewMatrix().m, sizeof(float) * 16);
        memcpy(projMat.data(), m_camera->projectionMatrix().m, sizeof(float) * 16);
        rs.setViewMatrix(viewMat);
        rs.setProjectionMatrix(projMat);
        rs.setSun(sunDir, sunColor);
        float fogDensity = (m_weather.cloudCover > 0.3f) ? 0.0002f * m_weather.cloudCover : 0.0f;
        rs.setFog(fogDensity > 0.0f, ambientColor, fogDensity);
        rs.setRain(m_weather.rainIntensity, m_weather.rainIntensity * 0.5f);

        QVector3D camPos;
        for (int i = 0; i < 3; ++i) camPos[i] = viewMat.inverted().column(3)[i];

        auto& water = ::ks::engine::graphics::WaterSystem::instance();
        if (water.isInitialized()) {
            water.setSeaLevel(0.0f);
            water.update(1.0f / 60.0f, camPos);
        }

        auto& particles = ::ks::engine::graphics::GPUParticleSystem::instance();
        if (particles.isInitialized()) {
            particles.update(1.0f / 60.0f);
        }

        auto& vegetation = ::ks::engine::graphics::VegetationSystem::instance();
        if (vegetation.isInitialized()) {
            vegetation.update(1.0f / 60.0f, camPos, sunDir);
        }

        auto& decals = ::ks::engine::graphics::DecalSystem::instance();
        if (decals.isInitialized()) {
            decals.update(1.0f / 60.0f, camPos);
        }

        rs.beginFrame();
        rs.endFrame();

        if (!m_pipelineInitialized && m_vulkanRenderer->device()) {
            m_scenePass = std::make_unique<VulkanRenderPass>();
            m_scenePass->setDevice(m_vulkanRenderer->device());

            QString shaderDir = QCoreApplication::applicationDirPath() + "/../src/engine/Graphics/shaders";
            QFile vertFile(shaderDir + "/passthrough.vert");
            QFile fragFile(shaderDir + "/passthrough.frag");
            if (!vertFile.exists()) {
                shaderDir = QCoreApplication::applicationDirPath() + "/shaders";
                vertFile.setFileName(shaderDir + "/passthrough.vert");
                fragFile.setFileName(shaderDir + "/passthrough.frag");
            }

            if (vertFile.open(QIODevice::ReadOnly) && fragFile.open(QIODevice::ReadOnly)) {
                m_scenePass->setVertexShader(QString::fromUtf8(vertFile.readAll()));
                m_scenePass->setFragmentShader(QString::fromUtf8(fragFile.readAll()));
                vertFile.close();
                fragFile.close();
            } else {
                m_scenePass->setVertexShader(QStringLiteral(R"glsl(
#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 baseColor; vec4 sunDirection; vec4 sunColor; } pc;
layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragVertexColor;
void main() {
    gl_Position = pc.mvp * vec4(inPosition, 1.0);
    fragNormal = inNormal;
    fragTexCoord = inTexCoord;
    fragVertexColor = inColor;
}
)glsl"));
                m_scenePass->setFragmentShader(QStringLiteral(R"glsl(
#version 450
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragVertexColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 baseColor; vec4 sunDirection; vec4 sunColor; } pc;
layout(location = 0) out vec4 outColor;
void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-pc.sunDirection.xyz);
    float NdotL = max(dot(N, L), 0.0);
    vec3 ambient = pc.baseColor.rgb * 0.3;
    vec3 diffuse = pc.sunColor.rgb * pc.baseColor.rgb * NdotL;
    outColor = vec4(ambient + diffuse, pc.baseColor.a);
}
)glsl"));
            }

            m_scenePass->setPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 112);
            if (m_scenePass->compile()) {
                m_pipelineInitialized = true;
            } else {
                qWarning() << "Scene pipeline compilation failed:" << m_scenePass->errorString();
            }
        }

        if (m_pipelineInitialized && !m_renderables.isEmpty()) {
            m_vulkanRenderer->setProjectionMatrix(projMat);
            m_vulkanRenderer->setViewMatrix(viewMat);
            m_vulkanRenderer->clear(QColor(100, 120, 150));
            m_vulkanRenderer->beginFrame();

            // Evaluate Streamline features (DLSS/Reflex/NIS)
            if (streamline.isAvailable() && m_vulkanRenderer->commandBuffer()) {
                streamline.evaluateFeatures(m_vulkanRenderer->commandBuffer(), m_streamlineFrameIndex);
                m_streamlineFrameIndex++;
            }

            m_vulkanRenderer->setPipeline(m_scenePass->pipeline());

            QMatrix4x4 vp = projMat * viewMat;
            for (const auto& entry : m_renderables) {
                QMatrix4x4 mvp = vp * entry.transform;
                m_vulkanRenderer->setModelMatrix(entry.transform);
                m_vulkanRenderer->setUniform("_currentMesh", entry.meshName);

                struct PushData {
                    float mvp[16];
                    float baseColor[4];
                    float sunDir[4];
                    float sunColor[4];
                } push;
                memcpy(push.mvp, mvp.constData(), sizeof(float) * 16);
                push.baseColor[0] = 0.8f; push.baseColor[1] = 0.8f; push.baseColor[2] = 0.85f; push.baseColor[3] = 1.0f;
                push.sunDir[0] = sunDir.x(); push.sunDir[1] = sunDir.y(); push.sunDir[2] = sunDir.z(); push.sunDir[3] = 0.0f;
                push.sunColor[0] = sunColor.x(); push.sunColor[1] = sunColor.y(); push.sunColor[2] = sunColor.z(); push.sunColor[3] = 1.0f;

                if (m_scenePass->pipelineLayout()) {
                    g_vk.cmdPushConstants(m_vulkanRenderer->commandBuffer(), m_scenePass->pipelineLayout(),
                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushData), &push);
                }

                m_vulkanRenderer->drawIndexedMesh(entry.indexCount, entry.firstIndex);
            }

            m_vulkanRenderer->endFrame();
        }

        return;
    }
}

void SimulationLoop::applyInput()
{
    m_vehicle->setThrottle(m_input->throttle());
    m_vehicle->setBrake(m_input->brake());
    m_vehicle->setSteering(m_input->steer());
}

void SimulationLoop::updateWeather()
{
    m_timeOfDay += (1.0f / 60.0f);
    if (m_timeOfDay >= 24.0f) m_timeOfDay -= 24.0f;

    m_vehicle->setWeatherState(m_weather);
    m_vehicle->setTrackWetness(m_weather.trackWetness);
    m_vehicle->setRainIntensity(m_weather.rainIntensity);
    m_vehicle->setAirDensity(m_weather.airDensity);
}

// ============================================================================
// Multiplayer Networking
// ============================================================================

void SimulationLoop::applyRemoteInput(int clientIndex, const net::InputData& input)
{
#if HAS_YOJIMBO
    if (!m_multiCar) return;

    auto* car = m_multiCar->getCarByClientIndex(clientIndex);
    if (!car) return;

    car->vehicle->setThrottle(input.throttle);
    car->vehicle->setBrake(input.brake);
    car->vehicle->setSteering(input.steering);
#else
    (void)clientIndex;
    (void)input;
#endif
}

void SimulationLoop::broadcastLocalCarState()
{
#if HAS_YOJIMBO
    if (!m_network || !m_network->isConnected() || !m_vehicle) return;

    net::CarStateData state;
    auto vstate = m_vehicle->getState();
    state.posX = vstate.position.x();
    state.posY = vstate.position.y();
    state.posZ = vstate.position.z();
    state.rotX = vstate.rotation.x();
    state.rotY = vstate.rotation.y();
    state.rotZ = vstate.rotation.z();
    state.velX = vstate.velocity.x();
    state.velY = vstate.velocity.y();
    state.velZ = vstate.velocity.z();
    state.speed = vstate.speed;
    state.rpm = vstate.rpm;
    state.gear = vstate.gear;
    state.throttle = vstate.throttle;
    state.brake = vstate.brake;
    state.steering = vstate.steering;

    if (m_network->isHosting()) {
        m_network->server()->broadcastCarState(0, state);
    }
#endif
}

void SimulationLoop::handleRemoteCarState(uint32_t carId, const net::CarStateData& state)
{
#if HAS_YOJIMBO
    if (!m_multiCar) return;

    auto* car = m_multiCar->getCarById(carId);
    if (!car) return;

    QVector3D targetPos(state.posX, state.posY, state.posZ);
    (void)targetPos;
#else
    (void)carId;
    (void)state;
#endif
}

} // namespace ks::sim
