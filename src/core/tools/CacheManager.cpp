#include "CacheManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDebug>
#include <QTimer>

namespace ks {

CacheManager* CacheManager::s_instance = nullptr;

CacheManager* CacheManager::instance()
{
    if (!s_instance)
        s_instance = new CacheManager();
    return s_instance;
}

CacheManager::CacheManager(QObject* parent)
    : QObject(parent)
{
    m_cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/kseditor";
    QDir().mkpath(m_cacheDir);

    // Periodic eviction
    auto* evictTimer = new QTimer(this);
    connect(evictTimer, &QTimer::timeout, this, &CacheManager::evictExpired);
    evictTimer->start(60000); // every minute
}

CacheManager::~CacheManager()
{
    s_instance = nullptr;
}

MemoryCache::MemoryCache(const QString& id, QObject* parent)
    : QObject(parent), m_id(id) {}
MemoryCache::~MemoryCache() {}

DiskCache::DiskCache(const QString& id, QObject* parent)
    : QObject(parent), m_id(id) {}
DiskCache::~DiskCache() {}

QString CacheManager::cacheKey(const QString& source) const
{
    return QCryptographicHash::hash(source.toUtf8(), QCryptographicHash::Md5).toHex();
}

QString CacheManager::cachePath(const QString& key, const QString& ext) const
{
    return QDir(m_cacheDir).filePath(key + (ext.isEmpty() ? "" : "." + ext));
}

bool CacheManager::has(const QString& key) const
{
    if (m_memCache.contains(key)) {
        const auto& e = m_memCache[key];
        if (e.expiry.isNull() || e.expiry > QDateTime::currentDateTime()) return true;
    }
    return QFile::exists(cachePath(key));
}

QVariant CacheManager::get(const QString& key) const
{
    // Memory cache first
    if (m_memCache.contains(key)) {
        const auto& e = m_memCache[key];
        if (e.expiry.isNull() || e.expiry > QDateTime::currentDateTime())
            return e.data;
        const_cast<CacheManager*>(this)->m_memCache.remove(key);
    }
    // Disk cache
    QFile f(cachePath(key, "cache"));
    if (!f.open(QIODevice::ReadOnly)) return {};
    QDataStream ds(&f);
    QVariant v;
    ds >> v;
    return v;
}

void CacheManager::set(const QString& key, const QVariant& value, int ttlSeconds)
{
    MemEntry e;
    e.data   = value;
    e.expiry = ttlSeconds > 0
               ? QDateTime::currentDateTime().addSecs(ttlSeconds)
               : QDateTime();
    m_memCache.insert(key, e);

    // Persist to disk
    QFile f(cachePath(key, "cache"));
    if (f.open(QIODevice::WriteOnly)) {
        QDataStream ds(&f);
        ds << value;
    }
    emit entryAdded(key);
}

void CacheManager::remove(const QString& key)
{
    m_memCache.remove(key);
    QFile::remove(cachePath(key, "cache"));
    emit entryRemoved(key);
}

void CacheManager::clear()
{
    m_memCache.clear();
    QDir dir(m_cacheDir);
    for (const auto& fi : dir.entryInfoList({"*.cache"}, QDir::Files))
        QFile::remove(fi.absoluteFilePath());
    emit cacheCleared();
}

void CacheManager::evictExpired()
{
    QDateTime now = QDateTime::currentDateTime();
    QStringList toRemove;
    for (auto it = m_memCache.begin(); it != m_memCache.end(); ++it)
        if (!it->expiry.isNull() && it->expiry <= now)
            toRemove << it.key();
    for (const auto& k : toRemove) remove(k);
}

qint64 CacheManager::diskSize() const
{
    qint64 total = 0;
    for (const auto& fi : QDir(m_cacheDir).entryInfoList({"*.cache"}, QDir::Files))
        total += fi.size();
    return total;
}

int CacheManager::entryCount() const
{
    return m_memCache.size();
}

void CacheManager::setMaxDiskSize(qint64 bytes)
{
    m_maxDiskBytes = bytes;
}

void CacheManager::setCacheDirectory(const QString& dir)
{
    m_cacheDir = dir;
    QDir().mkpath(dir);
}

} // namespace ks
