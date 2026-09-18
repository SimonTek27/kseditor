#include "SystemDllManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ks::engine::sys {

// ============================================================================
// Singleton
// ============================================================================

SystemDllManager* SystemDllManager::instance() {
    static SystemDllManager inst;
    return &inst;
}

// ============================================================================
// Construction / Destruction
// ============================================================================

SystemDllManager::SystemDllManager(QObject* parent)
    : QObject(parent) {
    initializeDefaults();
}

SystemDllManager::~SystemDllManager() {
}

// ============================================================================
// Default setup
// ============================================================================

void SystemDllManager::initializeDefaults() {
    const int count = static_cast<int>(DllId::Count);
    for (int i = 0; i < count; ++i) {
        DllId id = static_cast<DllId>(i);
        DllInfo info;
        info.id = id;
        info.filename = dllFilename(id);
        info.displayName = dllDisplayName(id);
        info.required = false;
        info.found = false;
        m_dlls[id] = info;
    }

    // Core audio DLLs are critical for the simulator
    m_dlls[DllId::FmodCore].required = true;

    // Default search paths (relative to application dir)
    QString appDir = QCoreApplication::applicationDirPath();
    m_searchPaths = {
        appDir + "/system",
        appDir + "/../system",
        appDir + "/../../bin/simulator/system",
        appDir,
    };

    // Check availability of all DLLs
    for (auto& info : m_dlls) {
        info.fullPath = findDll(info.filename);
        info.found = !info.fullPath.isEmpty();
    }

    qDebug() << "SystemDllManager: initialized with" << m_searchPaths.size() << "search paths";

    // Log which DLLs are available
    for (const auto& info : m_dlls) {
        if (info.found) {
            qDebug() << "  [OK]" << info.displayName << "->" << info.fullPath;
        } else if (info.required) {
            qDebug() << "  [MISSING]" << info.displayName << "(REQUIRED)";
        }
    }

    emit initialized();
}

// ============================================================================
// Search paths
// ============================================================================

void SystemDllManager::addSearchPath(const QString& path) {
    if (!m_searchPaths.contains(path)) {
        m_searchPaths.append(path);
        // Re-check availability with new path
        for (auto& info : m_dlls) {
            if (!info.found) {
                info.fullPath = findDll(info.filename);
                info.found = !info.fullPath.isEmpty();
            }
        }
    }
}

// ============================================================================
// Availability
// ============================================================================

bool SystemDllManager::isAvailable(DllId id) const {
    auto it = m_dlls.find(id);
    if (it == m_dlls.end()) return false;
    return it.value().found;
}

// ============================================================================
// Info
// ============================================================================

DllInfo SystemDllManager::info(DllId id) const {
    auto it = m_dlls.find(id);
    if (it == m_dlls.end()) return DllInfo{};
    return it.value();
}

QString SystemDllManager::dllPath(DllId id) const {
    auto it = m_dlls.find(id);
    if (it == m_dlls.end()) return QString();
    return it.value().fullPath;
}

// ============================================================================
// System DLL resolution (kernel32, user32, etc.)
// ============================================================================

void* SystemDllManager::resolveSystemDll(const QString& dllName, const char* symbolName) {
#ifdef _WIN32
    HMODULE hMod = GetModuleHandleW(reinterpret_cast<LPCWSTR>(dllName.utf16()));
    if (!hMod) {
        hMod = LoadLibraryW(reinterpret_cast<LPCWSTR>(dllName.utf16()));
    }
    if (hMod) {
        return reinterpret_cast<void*>(GetProcAddress(hMod, symbolName));
    }
#endif
    return nullptr;
}

// ============================================================================
// Private helpers
// ============================================================================

QString SystemDllManager::findDll(const QString& filename) const {
    for (const QString& basePath : m_searchPaths) {
        QString fullPath = QDir(basePath).filePath(filename);
        if (QFile::exists(fullPath)) {
            return fullPath;
        }
    }
    return QString();
}

QString SystemDllManager::dllFilename(DllId id) {
    switch (id) {
    case DllId::FmodCore:           return QStringLiteral("fmod64.dll");
    case DllId::FmodStudio:         return QStringLiteral("fmodstudio64.dll");
    case DllId::VideoModes:         return QStringLiteral("ksengineVideoModes.dll");
    case DllId::DirectWrite:        return QStringLiteral("dwrite.dll");
    case DllId::Dxgi:               return QStringLiteral("dxgi.dll");
    case DllId::StreamlineCommon:   return QStringLiteral("sl.common.dll");
    case DllId::StreamlineDlss:     return QStringLiteral("sl.dlss.dll");
    case DllId::StreamlineDlssG:    return QStringLiteral("sl.dlss_g.dll");
    case DllId::StreamlineDlssNr:   return QStringLiteral("sl.dlss_nr.dll");
    case DllId::StreamlineInterposer: return QStringLiteral("sl.interposer.dll");
    case DllId::StreamlineNis:      return QStringLiteral("sl.nis.dll");
    case DllId::StreamlinePcl:      return QStringLiteral("sl.pcl.dll");
    case DllId::StreamlineReflex:   return QStringLiteral("sl.reflex.dll");
    case DllId::Nvngx_dlss:         return QStringLiteral("nvngx_dlss.dll");
    case DllId::Nvngx_dlssg:        return QStringLiteral("nvngx_dlssg.dll");
    case DllId::Nvngx_dlssnr:       return QStringLiteral("nvngx_dlssnr.dll");
    default:                        return QString();
    }
}

QString SystemDllManager::dllDisplayName(DllId id) {
    switch (id) {
    case DllId::FmodCore:           return QStringLiteral("FMOD Core");
    case DllId::FmodStudio:         return QStringLiteral("FMOD Studio");
    case DllId::VideoModes:         return QStringLiteral("Video Modes");
    case DllId::DirectWrite:        return QStringLiteral("DirectWrite");
    case DllId::Dxgi:               return QStringLiteral("DXGI");
    case DllId::StreamlineCommon:   return QStringLiteral("Streamline Common");
    case DllId::StreamlineDlss:     return QStringLiteral("Streamline DLSS");
    case DllId::StreamlineDlssG:    return QStringLiteral("Streamline DLSS-G");
    case DllId::StreamlineDlssNr:   return QStringLiteral("Streamline DLSS-NR");
    case DllId::StreamlineInterposer: return QStringLiteral("Streamline Interposer");
    case DllId::StreamlineNis:      return QStringLiteral("Streamline NIS");
    case DllId::StreamlinePcl:      return QStringLiteral("Streamline PCL");
    case DllId::StreamlineReflex:   return QStringLiteral("Streamline Reflex");
    case DllId::Nvngx_dlss:         return QStringLiteral("NVIDIA DLSS");
    case DllId::Nvngx_dlssg:        return QStringLiteral("NVIDIA DLSS-G");
    case DllId::Nvngx_dlssnr:       return QStringLiteral("NVIDIA DLSS-NR");
    default:                        return QStringLiteral("Unknown");
    }
}

} // namespace ks::engine::sys
