#include "simulator/SimulationLoop.h"
#include "simulator/InputManager.h"
#include "simulator/NetworkManager.h"
#include "simulator/RaceSessionManager.h"
#include "devices/DeviceManager.h"
#include "Graphics/RenderSystem.h"
#include "Graphics/VulkanRenderer.h"
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QElapsedTimer>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Initialize the simulation loop
    ks::sim::SimulationLoop simulation;

    // Initialize the network manager for server functionality
    ks::sim::NetworkManager networkManager;

    // Initialize the race session manager
    ks::sim::RaceSessionManager sessionManager;

    // Initialize device manager
    ks::sim::DeviceManager deviceManager;

    qInfo() << "SimulatorServerApp: initialization started";

    // Simple setup - register core systems
    sessionManager.initialize();
    networkManager.setupServer();

    qInfo() << "SimulatorServerApp: systems initialized";

    // Run the simulation iteration
    simulation.update(1.0f / 120.0f);

    qInfo() << "SimulatorServerApp: running...";

    return app.exec();
}