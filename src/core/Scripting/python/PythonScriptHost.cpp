#if HAS_PYTHON
#include <Python.h>
#endif

#include "PythonScriptHost.h"
#include "core/Graphics/SceneGraph.h"
#include "core/Graphics/SceneObject.h"
#include "core/Graphics/SceneMesh.h"
#include <QDebug>
#include <QFile>
#include <QVector3D>

namespace ks {

#if HAS_PYTHON
static PythonScriptHost* s_pyHostInstance = nullptr;

static PyObject* pySceneObjectCount(PyObject* /*self*/, PyObject* /*args*/) {
    if (!s_pyHostInstance || !s_pyHostInstance->sceneGraph())
        return PyLong_FromLong(0);
    return PyLong_FromLong(s_pyHostInstance->sceneGraph()->objectCount());
}

static PyObject* pySceneFindByName(PyObject* /*self*/, PyObject* args) {
    const char* name = nullptr;
    if (!PyArg_ParseTuple(args, "s", &name))
        return PyLong_FromLong(-1);
    if (!s_pyHostInstance || !s_pyHostInstance->sceneGraph() || !name)
        return PyLong_FromLong(-1);
    SceneObject* obj = s_pyHostInstance->sceneGraph()->findObjectByName(QString::fromUtf8(name));
    return PyLong_FromLong(obj ? obj->id() : -1);
}

static PyObject* pySceneFindByType(PyObject* /*self*/, PyObject* args) {
    int typeInt = 0;
    if (!PyArg_ParseTuple(args, "i", &typeInt))
        Py_RETURN_NONE;
    if (!s_pyHostInstance || !s_pyHostInstance->sceneGraph())
        Py_RETURN_NONE;
    auto type = static_cast<SceneObject::Type>(typeInt);
    auto objects = s_pyHostInstance->sceneGraph()->findObjectsByType(type);
    PyObject* list = PyList_New(objects.size());
    for (int i = 0; i < objects.size(); ++i) {
        PyList_SetItem(list, i, PyLong_FromLong(objects[i]->id()));
    }
    return list;
}

static PyObject* pySceneCreateObject(PyObject* /*self*/, PyObject* args) {
    const char* name = nullptr;
    int typeInt = 0;
    if (!PyArg_ParseTuple(args, "si", &name, &typeInt))
        return PyLong_FromLong(-1);
    if (!s_pyHostInstance || !s_pyHostInstance->sceneGraph() || !name)
        return PyLong_FromLong(-1);
    auto type = static_cast<SceneObject::Type>(typeInt);
    SceneObject* obj = s_pyHostInstance->sceneGraph()->createObject(QString::fromUtf8(name), type);
    return PyLong_FromLong(obj->id());
}

static PyObject* pySceneDeleteObject(PyObject* /*self*/, PyObject* args) {
    int id = 0;
    if (!PyArg_ParseTuple(args, "i", &id))
        Py_RETURN_FALSE;
    if (!s_pyHostInstance || !s_pyHostInstance->sceneGraph())
        Py_RETURN_FALSE;
    SceneObject* obj = s_pyHostInstance->sceneGraph()->findObjectById(id);
    if (obj && obj != s_pyHostInstance->sceneGraph()->root()) {
        s_pyHostInstance->sceneGraph()->deleteObject(obj);
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* pySceneSetTranslation(PyObject* /*self*/, PyObject* args) {
    int id = 0;
    double x = 0, y = 0, z = 0;
    if (!PyArg_ParseTuple(args, "iddd", &id, &x, &y, &z))
        Py_RETURN_FALSE;
    if (!s_pyHostInstance || !s_pyHostInstance->sceneGraph())
        Py_RETURN_FALSE;
    SceneObject* obj = s_pyHostInstance->sceneGraph()->findObjectById(id);
    if (obj) {
        obj->setTranslation(QVector3D(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)));
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* pySceneGetTranslation(PyObject* /*self*/, PyObject* args) {
    int id = 0;
    if (!PyArg_ParseTuple(args, "i", &id))
        Py_RETURN_NONE;
    if (!s_pyHostInstance || !s_pyHostInstance->sceneGraph())
        Py_RETURN_NONE;
    SceneObject* obj = s_pyHostInstance->sceneGraph()->findObjectById(id);
    if (obj) {
        auto t = obj->translation();
        PyObject* list = PyList_New(3);
        PyList_SetItem(list, 0, PyFloat_FromDouble(t.x()));
        PyList_SetItem(list, 1, PyFloat_FromDouble(t.y()));
        PyList_SetItem(list, 2, PyFloat_FromDouble(t.z()));
        return list;
    }
    Py_RETURN_NONE;
}

static PyObject* pySceneSetMaterial(PyObject* /*self*/, PyObject* args) {
    int id = 0;
    const char* matName = nullptr;
    if (!PyArg_ParseTuple(args, "is", &id, &matName))
        Py_RETURN_FALSE;
    if (!s_pyHostInstance || !s_pyHostInstance->sceneGraph() || !matName)
        Py_RETURN_FALSE;
    SceneObject* obj = s_pyHostInstance->sceneGraph()->findObjectById(id);
    if (obj && obj->mesh() && !obj->mesh()->geometry().subMeshes.isEmpty()) {
        obj->mesh()->geometry().subMeshes[0].materialName = QString::fromUtf8(matName);
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* pySceneUpdateTransforms(PyObject* /*self*/, PyObject* /*args*/) {
    if (!s_pyHostInstance || !s_pyHostInstance->sceneGraph())
        Py_RETURN_FALSE;
    s_pyHostInstance->sceneGraph()->updateAllTransforms();
    Py_RETURN_TRUE;
}

static PyObject* pySceneSave(PyObject* /*self*/, PyObject* args) {
    const char* path = nullptr;
    if (!PyArg_ParseTuple(args, "s", &path))
        Py_RETURN_FALSE;
    if (!s_pyHostInstance || !s_pyHostInstance->sceneGraph() || !path)
        Py_RETURN_FALSE;
    if (s_pyHostInstance->sceneGraph()->saveToFile(QString::fromUtf8(path)))
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject* pySceneLoad(PyObject* /*self*/, PyObject* args) {
    const char* path = nullptr;
    if (!PyArg_ParseTuple(args, "s", &path))
        Py_RETURN_FALSE;
    if (!s_pyHostInstance || !s_pyHostInstance->sceneGraph() || !path)
        Py_RETURN_FALSE;
    if (s_pyHostInstance->sceneGraph()->loadFromFile(QString::fromUtf8(path)))
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyMethodDef kSceneMethods[] = {
    {"scene_object_count",     pySceneObjectCount,     METH_VARARGS, "Return number of objects in scene"},
    {"scene_find_by_name",     pySceneFindByName,       METH_VARARGS, "Find object by name, return id"},
    {"scene_find_by_type",     pySceneFindByType,       METH_VARARGS, "Find objects by type, return list of ids"},
    {"scene_create_object",    pySceneCreateObject,     METH_VARARGS, "Create object with name and type, return id"},
    {"scene_delete_object",    pySceneDeleteObject,     METH_VARARGS, "Delete object by id"},
    {"scene_set_translation",  pySceneSetTranslation,   METH_VARARGS, "Set object translation (id, x, y, z)"},
    {"scene_get_translation",  pySceneGetTranslation,   METH_VARARGS, "Get object translation as [x, y, z]"},
    {"scene_set_material",     pySceneSetMaterial,      METH_VARARGS, "Set object material by name"},
    {"scene_update_transforms",pySceneUpdateTransforms, METH_VARARGS, "Update all transforms"},
    {"scene_save",             pySceneSave,             METH_VARARGS, "Save scene to file"},
    {"scene_load",             pySceneLoad,             METH_VARARGS, "Load scene from file"},
    {nullptr, nullptr, 0, nullptr}
};
#endif

PythonScriptHost::PythonScriptHost(QObject* parent)
    : ScriptHost(Language::Python, parent)
{
}

PythonScriptHost::~PythonScriptHost() {
    shutdown();
}

bool PythonScriptHost::isAvailable() const {
#if HAS_PYTHON
    return true;
#else
    return false;
#endif
}

bool PythonScriptHost::initialize() {
#if HAS_PYTHON
    if (Py_IsInitialized()) {
        m_pyMain = PyImport_AddModule("__main__");
        m_pyBuiltins = PyEval_GetBuiltins();
        m_initialized = true;
        return true;
    }

    Py_Initialize();
    if (!Py_IsInitialized()) {
        emit error("Failed to initialize Python interpreter");
        return false;
    }

    m_pyMain = PyImport_AddModule("__main__");
    m_pyBuiltins = PyEval_GetBuiltins();

    if (!m_pyMain) {
        emit error("Failed to get __main__ module");
        Py_Finalize();
        return false;
    }

    registerBuiltinFunctions();

    m_initialized = true;
    return true;
#else
    emit error("Python not available (HAS_PYTHON not defined). Install Python development libraries.");
    return false;
#endif
}

void PythonScriptHost::shutdown() {
#if HAS_PYTHON
    if (Py_IsInitialized()) {
        Py_Finalize();
    }
    m_pyMain = nullptr;
    m_pyBuiltins = nullptr;
    m_initialized = false;
#endif
}

bool PythonScriptHost::execute(const QString& code, QString& output, QString& error) {
#if HAS_PYTHON
    if (!m_pyMain) {
        error = "Python not initialized";
        return false;
    }

    PyObject* result = PyRun_String(code.toUtf8().constData(), Py_file_input,
                                     PyModule_GetDict(m_pyMain), PyModule_GetDict(m_pyMain));
    if (!result) {
        PyErr_Print();
        error = "Python execution error";
        emit this->error(error);
        return false;
    }

    Py_DECREF(result);
    return true;
#else
    Q_UNUSED(code);
    output.clear();
    error = "Python not available";
    return false;
#endif
}

bool PythonScriptHost::executeFile(const QString& filePath, QString& output, QString& error) {
#if HAS_PYTHON
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = "Cannot open file: " + filePath;
        return false;
    }

    QByteArray code = file.readAll();
    file.close();

    return execute(QString::fromUtf8(code), output, error);
#else
    Q_UNUSED(filePath);
    output.clear();
    error = "Python not available";
    return false;
#endif
}

QVariant PythonScriptHost::getVariable(const QString& name) {
#if HAS_PYTHON
    if (!m_pyMain) return QVariant();

    PyObject* dict = PyModule_GetDict(m_pyMain);
    PyObject* value = PyDict_GetItemString(dict, name.toUtf8().constData());
    if (!value) return QVariant();

    if (PyLong_Check(value)) {
        return QVariant((int)PyLong_AsLong(value));
    } else if (PyFloat_Check(value)) {
        return QVariant(PyFloat_AsDouble(value));
    } else if (PyUnicode_Check(value)) {
        return QVariant(QString::fromUtf8(PyUnicode_AsUTF8(value)));
    } else if (PyBool_Check(value)) {
        return QVariant((bool)Py_IsTrue(value));
    }
    return QVariant();
#else
    Q_UNUSED(name);
    return QVariant();
#endif
}

void PythonScriptHost::setVariable(const QString& name, const QVariant& value) {
#if HAS_PYTHON
    if (!m_pyMain) return;

    PyObject* dict = PyModule_GetDict(m_pyMain);
    PyObject* pyValue = nullptr;

    switch (value.typeId()) {
        case QMetaType::Bool:
            pyValue = value.toBool() ? Py_True : Py_False;
            Py_INCREF(pyValue);
            break;
        case QMetaType::Int:
        case QMetaType::LongLong:
            pyValue = PyLong_FromLong(value.toInt());
            break;
        case QMetaType::Double:
        case QMetaType::Float:
            pyValue = PyFloat_FromDouble(value.toDouble());
            break;
        case QMetaType::QString:
            pyValue = PyUnicode_FromString(value.toString().toUtf8().constData());
            break;
        default:
            pyValue = Py_None;
            Py_INCREF(pyValue);
            break;
    }

    PyDict_SetItemString(dict, name.toUtf8().constData(), pyValue);
    Py_DECREF(pyValue);
#endif
}

void PythonScriptHost::registerBuiltinFunctions() {
#if HAS_PYTHON
    s_pyHostInstance = this;

    PyObject* builtins = PyEval_GetBuiltins();
    if (!builtins) return;

    for (int i = 0; kSceneMethods[i].ml_name != nullptr; ++i) {
        PyObject* func = PyCFunction_New(&kSceneMethods[i], nullptr);
        if (func) {
            PyDict_SetItemString(builtins, kSceneMethods[i].ml_name, func);
            Py_DECREF(func);
        }
    }
#endif
}

bool PythonScriptHost::pushQVariant(PyObject* module, const QString& name, const QVariant& value) {
#if HAS_PYTHON
    PyObject* dict = PyModule_GetDict(module);
    PyObject* pyValue = nullptr;

    switch (value.typeId()) {
        case QMetaType::Bool:
            pyValue = value.toBool() ? Py_True : Py_False;
            Py_INCREF(pyValue);
            break;
        case QMetaType::Int:
        case QMetaType::LongLong:
            pyValue = PyLong_FromLong(value.toInt());
            break;
        case QMetaType::Double:
        case QMetaType::Float:
            pyValue = PyFloat_FromDouble(value.toDouble());
            break;
        case QMetaType::QString:
            pyValue = PyUnicode_FromString(value.toString().toUtf8().constData());
            break;
        default:
            pyValue = Py_None;
            Py_INCREF(pyValue);
            break;
    }

    int result = PyDict_SetItemString(dict, name.toUtf8().constData(), pyValue);
    Py_DECREF(pyValue);
    return result == 0;
#else
    return false;
#endif
}

QVariant PythonScriptHost::callPythonFunction(const QString& funcName, const QVariantList& args) {
#if HAS_PYTHON
    if (!m_pyMain) return QVariant();

    PyObject* dict = PyModule_GetDict(m_pyMain);
    PyObject* func = PyDict_GetItemString(dict, funcName.toUtf8().constData());
    if (!func || !PyCallable_Check(func)) {
        return QVariant();
    }

    PyObject* pyArgs = PyTuple_New(args.size());
    for (int i = 0; i < args.size(); ++i) {
        const QVariant& arg = args[i];
        PyObject* pyArg = nullptr;
        switch (arg.type()) {
            case QVariant::Bool:
                pyArg = arg.toBool() ? Py_True : Py_False;
                Py_INCREF(pyArg);
                break;
            case QVariant::Int:
                pyArg = PyLong_FromLong(arg.toInt());
                break;
            case QVariant::Double:
                pyArg = PyFloat_FromDouble(arg.toDouble());
                break;
            case QVariant::String:
                pyArg = PyUnicode_FromString(arg.toString().toUtf8().constData());
                break;
            default:
                pyArg = Py_None;
                Py_INCREF(pyArg);
                break;
        }
        PyTuple_SetItem(pyArgs, i, pyArg);
    }

    PyObject* result = PyObject_CallObject(func, pyArgs);
    Py_DECREF(pyArgs);

    if (!result) {
        PyErr_Print();
        return QVariant();
    }

    QVariant retval;
    if (PyLong_Check(result)) {
        retval = QVariant((int)PyLong_AsLong(result));
    } else if (PyFloat_Check(result)) {
        retval = QVariant(PyFloat_AsDouble(result));
    } else if (PyUnicode_Check(result)) {
        retval = QVariant(QString::fromUtf8(PyUnicode_AsUTF8(result)));
    } else if (PyBool_Check(result)) {
        retval = QVariant((bool)Py_IsTrue(result));
    }
    Py_DECREF(result);
    return retval;
#else
    return QVariant();
#endif
}

} // namespace ks
