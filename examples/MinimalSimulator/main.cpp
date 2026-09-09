#include "../../src/core/engine/Engine.h"
#include "../../src/core/engine/Graphics/RenderSystem.h"
#include "../../src/core/engine/Physics/TrackSurface.h"
#include "../../src/core/engine/Devices/InputSystem.h"
#include "../../src/core/engine/Network/NetSystem.h"
#include "../../src/core/engine/Assets/Packaging.h"
#include <QCoreApplication>
#include <QDebug>

using namespace ks::engine;
using namespace ks::engine::graphics;
using namespace ks::engine::physics;
using namespace ks::engine::devices;
using namespace ks::engine::network;
using namespace ks::engine::assets;

struct Car { QVector3D pos; float speed=0; };

int main(int argc, char *argv[]){
    QCoreApplication app(argc, argv);
    Engine &e = Engine::instance();
    e.registerModule(&RenderSystem::instance());
    e.registerModule(&TrackSurface::instance());
    e.registerModule(&InputSystem::instance());
    e.registerModule(&NetSystem::instance());
    e.registerModule(&Packaging::instance());
    if (!e.initialize()) return 1;
    e.setFixedDt(1.0/120.0);

    Entity car = e.registry().create();
    e.registry().emplace<Car>(car, Car{{0,0,0}, 0});

    TrackSurface::instance().setBaseGrip(1.0f);
    RenderSystem::instance().enableDeferred(true);

    e.addSystem<double>([](double dt){
        auto &reg = Engine::instance().registry();
        for (Entity en : reg.view<Car>()){
            Car *c = reg.get<Car>(en);
            float grip = TrackSurface::instance().getGrip(c->pos);
            c->pos += QVector3D(c->speed * grip * float(dt),0,0);
        }
        RenderSystem::instance().beginFrame();
        RenderSystem::instance().endFrame();
    });

    e.start();
    qInfo() << "MinimalSimulator: running standalone on ksengine" << e.fixedDt();
    return app.exec();
}
