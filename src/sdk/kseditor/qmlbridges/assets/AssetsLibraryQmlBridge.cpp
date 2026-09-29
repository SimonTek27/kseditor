#include "AssetsLibraryQmlBridge.h"

namespace ks {

AssetsLibraryQmlBridge* AssetsLibraryQmlBridge::instance()
{
    if (!s_instance)
        s_instance = new AssetsLibraryQmlBridge(nullptr);
    return s_instance;
}

AssetsLibraryQmlBridge::AssetsLibraryQmlBridge(QObject* parent)
    : QObject(parent)
{
    s_instance = this;
}

AssetsLibraryQmlBridge::~AssetsLibraryQmlBridge() = default;

QVariantList AssetsLibraryQmlBridge::getAssets(const QString&, const QString&, const QString&)
{
    return {};
}

QVariantList AssetsLibraryQmlBridge::getCategories()
{
    return { QStringLiteral("All") };
}

QVariantMap AssetsLibraryQmlBridge::getStorageStats()
{
    return {
        { QStringLiteral("formattedSize"), QStringLiteral("0 B") },
        { QStringLiteral("assetCount"), 0 },
        { QStringLiteral("usedPercent"), 0 },
    };
}

QVariantMap AssetsLibraryQmlBridge::getAsset(const QString&)
{
    return {};
}

bool AssetsLibraryQmlBridge::importAsset(const QString&, const QString&)
{
    emit assetsChanged();
    return false;
}

bool AssetsLibraryQmlBridge::exportAsset(const QString&, const QString&)
{
    return false;
}

void AssetsLibraryQmlBridge::statusMessage(const QString&)
{
}

void AssetsLibraryQmlBridge::openInModeler(const QString&)
{
}

bool AssetsLibraryQmlBridge::removeAsset(const QString&)
{
    emit assetsChanged();
    return false;
}

AssetsLibraryQmlBridge* AssetsLibraryQmlBridge::s_instance = nullptr;

} // namespace ks
