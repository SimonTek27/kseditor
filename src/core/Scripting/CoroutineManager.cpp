#include "ScriptDebugger.h"
#include <QMap>
#include <QUuid>
#include <functional>
#include <queue>

namespace ks {
namespace scripting {

struct Coroutine {
    QUuid id;
    QString name;
    std::function<QVariant()> generator;
    QVariant lastValue;
    bool running = false;
    bool finished = false;
    bool yielded = false;
};

static QMap<QUuid, Coroutine> g_coroutines;

CoroutineManager::CoroutineManager(QObject* parent) : QObject(parent)
{
}

CoroutineManager::~CoroutineManager() = default;

CoroutineManager* CoroutineManager::instance()
{
    static CoroutineManager* s_instance = nullptr;
    if (!s_instance) {
        s_instance = new CoroutineManager();
    }
    return s_instance;
}

QUuid CoroutineManager::startCoroutine(std::function<QVariant()> generator, const QString& name)
{
    QUuid id = QUuid::createUuid();
    Coroutine co;
    co.id = id;
    co.name = name.isEmpty() ? id.toString(QUuid::WithoutBraces) : name;
    co.generator = generator;
    co.running = true;
    co.finished = false;
    co.yielded = false;
    g_coroutines[id] = co;
    emit coroutineStarted(id);
    return id;
}

void CoroutineManager::resumeCoroutine(const QUuid& id)
{
    auto it = g_coroutines.find(id);
    if (it == g_coroutines.end() || it->finished || !it->running) return;

    Coroutine& co = it.value();
    co.yielded = false;
    if (co.generator) {
        QVariant result = co.generator();
        co.lastValue = result;
        if (co.finished) {
            co.running = false;
            emit coroutineFinished(id);
        } else {
            emit coroutineYielded(id, result);
        }
    } else {
        co.finished = true;
        co.running = false;
        emit coroutineFinished(id);
    }
}

void CoroutineManager::yieldCoroutine(const QUuid& id, const QVariant& value)
{
    auto it = g_coroutines.find(id);
    if (it == g_coroutines.end()) return;
    it->yielded = true;
    it->lastValue = value;
    emit coroutineYielded(id, value);
}

void CoroutineManager::stopCoroutine(const QUuid& id)
{
    auto it = g_coroutines.find(id);
    if (it == g_coroutines.end()) return;
    it->running = false;
    it->finished = true;
    emit coroutineFinished(id);
}

bool CoroutineManager::isCoroutineRunning(const QUuid& id) const
{
    auto it = g_coroutines.find(id);
    return it != g_coroutines.end() && it->running && !it->finished;
}

} // namespace scripting
} // namespace ks