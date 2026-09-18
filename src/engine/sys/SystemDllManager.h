#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QDir>

namespace ks::engine::sys {

// ============================================================================
// SystemDllManager - Path resolution and availability checks for system DLLs
// DLLs are now statically linked via .lib import libraries.
// This class only tracks paths and availability for deployment purposes.
// ============================================================================

enum class DllId {
    // Audio Runtime (FMOD) - statically linked
    FmodCore,           // fmod64.dll
    FmodStudio,         // fmodstudio64.dll

    // Video/Display - statically linked
    VideoModes,         // ksengineVideoModes.dll
    DirectWrite,        // dwrite.dll
    Dxgi,               // dxgi.dll

    // NVIDIA Streamline - statically linked
    StreamlineCommon,   // sl.common.dll
    StreamlineDlss,     // sl.dlss.dll
    StreamlineDlssG,    // sl.dlss_g.dll
    StreamlineDlssNr,   // sl.dlss_nr.dll
    StreamlineInterposer, // sl.interposer.dll
    StreamlineNis,      // sl.nis.dll
    StreamlinePcl,      // sl.pcl.dll
    StreamlineReflex,   // sl.reflex.dll

    // NVIDIA DLSS (loaded by Streamline, not directly)
    Nvngx_dlss,         // nvngx_dlss.dll
    Nvngx_dlssg,        // nvngx_dlssg.dll
    Nvngx_dlssnr,       // nvngx_dlssnr.dll

    Count
};

struct DllInfo {
    DllId id;
    QString filename;
    QString displayName;
    bool required = false;
    bool found = false;     // DLL file exists in search paths
    QString fullPath;       // Full path to the DLL
};

class SystemDllManager : public QObject {
    Q_OBJECT

public:
    static SystemDllManager* instance();

    explicit SystemDllManager(QObject* parent = nullptr);
    ~SystemDllManager();

    // --- Search paths ---
    void addSearchPath(const QString& path);
    QStringList searchPaths() const { return m_searchPaths; }

    // --- Availability ---
    bool isAvailable(DllId id) const;

    // --- Info ---
    DllInfo info(DllId id) const;
    QString dllPath(DllId id) const;

    // --- System DLL resolution (kernel32, user32, etc.) ---
    static void* resolveSystemDll(const QString& dllName, const char* symbolName);

signals:
    void initialized();

private:
    void initializeDefaults();
    QString findDll(const QString& filename) const;
    static QString dllFilename(DllId id);
    static QString dllDisplayName(DllId id);

    QMap<DllId, DllInfo> m_dlls;
    QStringList m_searchPaths;
};

} // namespace ks::engine::sys
