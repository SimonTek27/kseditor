#include <QtTest/QtTest>

#include "core/Graphics/SceneGraph.h"
#include "modules/modellingEditor/WireParameterSystem.h"

using namespace ks;

namespace {
struct SceneFixture {
    SceneGraph graph;
    QVector<SceneObject*> objects;

    SceneObject* create(const QString& name)
    {
        SceneObject* o = graph.createObject(name);
        objects.append(o);
        return o;
    }

    ~SceneFixture()
    {
        for (SceneObject* o : objects)
            graph.deleteObject(o);
        objects.clear();
    }

    SceneFixture() = default;
    SceneFixture(SceneFixture&&) = delete;
    SceneFixture& operator=(SceneFixture&&) = delete;
};
} // namespace

class TestWireParameterSystem : public QObject
{
    Q_OBJECT

private slots:
    void wiresPositionChannel();
    void scaleAndOffset();
    void disabledBindingIgnored();
    void wireListAndRemove();
    void serializationRoundTrip();
};

void TestWireParameterSystem::wiresPositionChannel()
{
    SceneFixture fx;
    SceneObject* driver = fx.create("driver");
    SceneObject* driven = fx.create("driven");
    fx.graph.updateAllTransforms();

    WireParameterSystem sys;
    QVERIFY(sys.add(driver->id(), "driver", "position.x",
                    driven->id(), "driven", "position.y", 1.0f, 0.0f));
    driver->setPosition(QVector3D(7, 0, 0));
    QCOMPARE(sys.evaluate(&fx.graph), 1);
    // driven.position.y follows driver.position.x because the scale map is 1:1.
    QVERIFY(qAbs(driven->position().y() - 7.0f) < 1e-4f);
}

void TestWireParameterSystem::scaleAndOffset()
{
    SceneFixture fx;
    SceneObject* driver = fx.create("driver");
    SceneObject* driven = fx.create("driven");
    fx.graph.updateAllTransforms();

    WireParameterSystem sys;
    QVERIFY(sys.add(driver->id(), "driver", "position.x",
                    driven->id(), "driven", "scale.z", 2.0f, 0.5f));
    driver->setPosition(QVector3D(3, 0, 0));
    sys.evaluate(&fx.graph);
    QVERIFY(qAbs(driven->scale().z() - (3.0f * 2.0f + 0.5f)) < 1e-4f);
}

void TestWireParameterSystem::disabledBindingIgnored()
{
    SceneFixture fx;
    SceneObject* driver = fx.create("driver");
    SceneObject* driven = fx.create("driven");
    fx.graph.updateAllTransforms();

    WireParameterSystem sys;
    QVERIFY(sys.add(driver->id(), "driver", "position.x",
                    driven->id(), "driven", "position.y", 1.0f, 0.0f));
    QVERIFY(sys.setEnabled(driven->id(), 0, false));
    driver->setPosition(QVector3D(9, 0, 0));
    sys.evaluate(&fx.graph);
    QVERIFY(qAbs(driven->position().y() - 0.0f) < 1e-4f);
}

void TestWireParameterSystem::wireListAndRemove()
{
    SceneFixture fx;
    SceneObject* driver = fx.create("driver");
    SceneObject* driven = fx.create("driven");
    SceneObject* other = fx.create("other");
    fx.graph.updateAllTransforms();

    WireParameterSystem sys;
    QVERIFY(sys.add(driver->id(), "driver", "position.x", driven->id(), "driven", "position.z", 1.0f, 0.0f));
    QVERIFY(sys.add(other->id(), "other", "position.y", driven->id(), "driven", "position.x", 1.0f, 0.0f));

    // Two bindings both drive `driven`.
    QCOMPARE(sys.forObject(driven->id()).size(), 2);
    // `driver` drives one binding.
    QCOMPARE(sys.drivenBy(driver->id()).size(), 1);

    QVERIFY(sys.remove(driven->id(), 0));
    QCOMPARE(sys.forObject(driven->id()).size(), 1);

    sys.clearDriver(other->id());
    QCOMPARE(sys.forObject(driven->id()).size(), 0);
}

void TestWireParameterSystem::serializationRoundTrip()
{
    SceneFixture fx;
    SceneObject* driver = fx.create("driver");
    SceneObject* driven = fx.create("driven");
    fx.graph.updateAllTransforms();

    WireParameterSystem sys;
    QVERIFY(sys.add(driver->id(), "driver", "position.x",
                    driven->id(), "driven", "scale.z", 3.0f, 1.25f));
    const auto b = sys.forObject(driven->id()).constFirst();
    WireBinding restored;
    restored.fromVariant(b.toVariant());
    QCOMPARE(restored.driverProp, QStringLiteral("position.x"));
    QCOMPARE(restored.drivenProp, QStringLiteral("scale.z"));
    QVERIFY(qAbs(restored.scale - 3.0f) < 1e-4f);
    QVERIFY(qAbs(restored.offset - 1.25f) < 1e-4f);
    QVERIFY(restored.enabled);
}

QTEST_MAIN(TestWireParameterSystem)

#include "test_WireParameterSystem.moc"
