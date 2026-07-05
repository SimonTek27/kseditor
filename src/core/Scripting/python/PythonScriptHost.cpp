#include "PythonScriptHost.h"
#include "core/Graphics/SceneGraph.h"
#include "core/Graphics/SceneObject.h"
#include <QDebug>
#include <QFile>

#if HAS_PYTHON
#include <Python.h>
#endif

namespace ks {

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

    switch (value.type()) {
        case QVariant::Bool:
            pyValue = value.toBool() ? Py_True : Py_False;
            Py_INCREF(pyValue);
            break;
        case QVariant::Int:
        case QVariant::LongLong:
            pyValue = PyLong_FromLong(value.toInt());
            break;
        case QVariant::Double:
        case QVariant::Float:
            pyValue = PyFloat_FromDouble(value.toDouble());
            break;
        case QVariant::String:
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
    // Python builtins are registered via the __builtins__ module
    // Custom scene functions would be registered here as Python callables
    // For now, the Python host provides basic execution capability
#endif
}

bool PythonScriptHost::pushQVariant(PyObject* module, const QString& name, const QVariant& value) {
#if HAS_PYTHON
    PyObject* dict = PyModule_GetDict(module);
    PyObject* pyValue = nullptr;

    switch (value.type()) {
        case QVariant::Bool:
            pyValue = value.toBool() ? Py_True : Py_False;
            Py_INCREF(pyValue);
            break;
        case QVariant::Int:
        case QVariant::LongLong:
            pyValue = PyLong_FromLong(value.toInt());
            break;
        case QVariant::Double:
        case QVariant::Float:
            pyValue = PyFloat_FromDouble(value.toDouble());
            break;
        case QVariant::String:
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
