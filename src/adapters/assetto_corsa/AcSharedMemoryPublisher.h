#pragma once
/**
 * Publishes AcPhysics/Graphics/Static pages for AC-compatible apps.
 * Qt-free. Windows uses CreateFileMapping; other platforms soft-buffer only
 * unless KS_AC_SHM_POSIX is defined.
 */
#include "AcSharedMemory.h"
#include <cstdio>
#include <mutex>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace ks {
namespace ac {

class AcSharedMemoryPublisher {
public:
    bool open() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_ok) return true;
#ifdef _WIN32
        m_physMap = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                       0, sizeof(AcPhysicsPage), L"Local\\acpmf_physics");
        m_gfxMap = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                      0, sizeof(AcGraphicsPage), L"Local\\acpmf_graphics");
        m_statMap = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                       0, sizeof(AcStaticPage), L"Local\\acpmf_static");
        if (!m_physMap || !m_gfxMap || !m_statMap) {
            std::fprintf(stderr, "AcSharedMemory: CreateFileMapping failed\n");
            closeUnlocked();
            return false;
        }
        m_phys = static_cast<AcPhysicsPage*>(MapViewOfFile(m_physMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(AcPhysicsPage)));
        m_gfx = static_cast<AcGraphicsPage*>(MapViewOfFile(m_gfxMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(AcGraphicsPage)));
        m_stat = static_cast<AcStaticPage*>(MapViewOfFile(m_statMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(AcStaticPage)));
        if (!m_phys || !m_gfx || !m_stat) {
            closeUnlocked();
            return false;
        }
        std::memset(m_phys, 0, sizeof(*m_phys));
        std::memset(m_gfx, 0, sizeof(*m_gfx));
        std::memset(m_stat, 0, sizeof(*m_stat));
#else
        // Soft mirror when no POSIX shm — apps on same process can still read via pointers
        m_physSoft = {};
        m_gfxSoft = {};
        m_statSoft = {};
        m_phys = &m_physSoft;
        m_gfx = &m_gfxSoft;
        m_stat = &m_statSoft;
#endif
        m_ok = true;
        std::fprintf(stderr, "AcSharedMemory: publisher ready\n");
        return true;
    }

    void close() {
        std::lock_guard<std::mutex> lock(m_mutex);
        closeUnlocked();
    }

    void publish(const AcLiveInput& in) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_ok || !m_phys || !m_gfx || !m_stat) return;
        fillPhysics(*m_phys, in);
        fillGraphics(*m_gfx, in);
        if (!m_staticFilled) {
            fillStatic(*m_stat, in);
            m_staticFilled = true;
        }
        // Update dynamic static fields occasionally
        m_stat->maxRpm = in.maxRpm;
    }

    void invalidateStatic() { m_staticFilled = false; }

    bool isOpen() const { return m_ok; }

    const AcPhysicsPage* physics() const { return m_phys; }
    const AcGraphicsPage* graphics() const { return m_gfx; }
    const AcStaticPage* staticPage() const { return m_stat; }

private:
    void closeUnlocked() {
#ifdef _WIN32
        if (m_phys) { UnmapViewOfFile(m_phys); m_phys = nullptr; }
        if (m_gfx) { UnmapViewOfFile(m_gfx); m_gfx = nullptr; }
        if (m_stat) { UnmapViewOfFile(m_stat); m_stat = nullptr; }
        if (m_physMap) { CloseHandle(m_physMap); m_physMap = nullptr; }
        if (m_gfxMap) { CloseHandle(m_gfxMap); m_gfxMap = nullptr; }
        if (m_statMap) { CloseHandle(m_statMap); m_statMap = nullptr; }
#else
        m_phys = m_gfx = m_stat = nullptr;
#endif
        m_ok = false;
        m_staticFilled = false;
    }

    std::mutex m_mutex;
    bool m_ok = false;
    bool m_staticFilled = false;
    AcPhysicsPage* m_phys = nullptr;
    AcGraphicsPage* m_gfx = nullptr;
    AcStaticPage* m_stat = nullptr;
#ifdef _WIN32
    HANDLE m_physMap = nullptr, m_gfxMap = nullptr, m_statMap = nullptr;
#else
    AcPhysicsPage m_physSoft{};
    AcGraphicsPage m_gfxSoft{};
    AcStaticPage m_statSoft{};
#endif
};

} // namespace ac
} // namespace ks
