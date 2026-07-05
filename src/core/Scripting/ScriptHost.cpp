#include "ScriptHost.h"
#include "core/Graphics/SceneGraph.h"
#include "core/Graphics/SceneObject.h"
#include <QDebug>

namespace ks {

ScriptHost::ScriptHost(Language lang, QObject* parent)
    : QObject(parent)
    , m_language(lang)
{
}

ScriptHost::~ScriptHost() {
    shutdown();
}

void ScriptHost::registerFunction(const QString& name, ScriptFunction func) {
    m_functions[name] = func;
}

void ScriptHost::registerBuiltins() {
    // Scene graph functions
    registerFunction("scene_object_count", [this](const QVariantList&) -> QVariant {
        if (!m_sceneGraph) return 0;
        return m_sceneGraph->objectCount();
    });

    registerFunction("scene_find_by_name", [this](const QVariantList& args) -> QVariant {
        if (!m_sceneGraph || args.isEmpty()) return -1;
        auto* obj = m_sceneGraph->findObjectByName(args[0].toString());
        return obj ? obj->id() : -1;
    });

    registerFunction("scene_find_by_type", [this](const QVariantList& args) -> QVariant {
        if (!m_sceneGraph || args.isEmpty()) return QVariantList{};
        auto type = static_cast<SceneObject::Type>(args[0].toInt());
        auto objects = m_sceneGraph->findObjectsByType(type);
        QVariantList ids;
        for (auto* obj : objects) ids.append(obj->id());
        return ids;
    });

    registerFunction("scene_create_object", [this](const QVariantList& args) -> QVariant {
        if (!m_sceneGraph || args.isEmpty()) return -1;
        QString name = args[0].toString();
        SceneObject::Type type = args.size() > 1 ?
            static_cast<SceneObject::Type>(args[1].toInt()) : SceneObject::Type::Node;
        auto* obj = m_sceneGraph->createObject(name, type);
        return obj ? obj->id() : -1;
    });

    registerFunction("scene_delete_object", [this](const QVariantList& args) -> QVariant {
        if (!m_sceneGraph || args.isEmpty()) return false;
        auto* obj = m_sceneGraph->findObjectById(args[0].toInt());
        if (obj) {
            m_sceneGraph->deleteObject(obj);
            return true;
        }
        return false;
    });

    registerFunction("scene_set_translation", [this](const QVariantList& args) -> QVariant {
        if (!m_sceneGraph || args.size() < 4) return false;
        auto* obj = m_sceneGraph->findObjectById(args[0].toInt());
        if (obj) {
            obj->setTranslation(QVector3D(args[1].toFloat(), args[2].toFloat(), args[3].toFloat()));
            return true;
        }
        return false;
    });

    registerFunction("scene_get_translation", [this](const QVariantList& args) -> QVariant {
        if (!m_sceneGraph || args.isEmpty()) return QVariantList{};
        auto* obj = m_sceneGraph->findObjectById(args[0].toInt());
        if (obj) {
            auto t = obj->translation();
            return QVariantList{t.x(), t.y(), t.z()};
        }
        return QVariantList{};
    });

    registerFunction("scene_set_material", [this](const QVariantList& args) -> QVariant {
        if (!m_sceneGraph || args.size() < 5) return false;
        auto* obj = m_sceneGraph->findObjectById(args[0].toInt());
        if (obj) {
            obj->setBaseColor(QColor::fromRgbF(args[1].toFloat(), args[2].toFloat(), args[3].toFloat()));
            obj->setMetallic(args[4].toFloat());
            if (args.size() > 5) obj->setRoughness(args[5].toFloat());
            return true;
        }
        return false;
    });

    registerFunction("scene_update_transforms", [this](const QVariantList&) -> QVariant {
        if (m_sceneGraph) {
            m_sceneGraph->updateAllTransforms();
            return true;
        }
        return false;
    });

    registerFunction("scene_save", [this](const QVariantList& args) -> QVariant {
        if (!m_sceneGraph || args.isEmpty()) return false;
        return m_sceneGraph->saveToFile(args[0].toString());
    });

    registerFunction("scene_load", [this](const QVariantList& args) -> QVariant {
        if (!m_sceneGraph || args.isEmpty()) return false;
        return m_sceneGraph->loadFromFile(args[0].toString());
    });

    // Logging
    registerFunction("print", [this](const QVariantList& args) -> QVariant {
        QString msg;
        for (const auto& arg : args) msg += arg.toString() + " ";
        emit output(msg.trimmed());
        return true;
    });

    registerFunction("log", [this](const QVariantList& args) -> QVariant {
        QString msg;
        for (const auto& arg : args) msg += arg.toString() + " ";
        qDebug() << "[Script]" << msg.trimmed();
        return true;
    });
}

} // namespace ks
