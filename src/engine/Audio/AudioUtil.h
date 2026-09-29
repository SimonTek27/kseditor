#pragma once

// Qt-free helpers for the Audio module: QFile/QDir/QFileInfo/QTextStream,
// QProcess, QLibrary, QCryptographicHash, QtEndian and QRandomGenerator are
// all replaced here. The generic primitives (timestamps, UUIDs, directories,
// plain file IO, env vars) are re-exported from material/MaterialUtil.h so
// that logic exists only once; everything audio/process specific lives in
// this header plus AudioUtil.cpp (the Windows-only parts stay out of the
// header so no <windows.h> macro (min/max) leaks into audio code).

#include "../material/MaterialUtil.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace ks::audio {

namespace fs = std::filesystem;

// --- re-exported from ks::mat (same helpers the material module uses) ---
using mat::appDataLocation;
using mat::envValue;
using mat::fileExists;
using mat::fileName;
using mat::formatIsoDate;
using mat::isoDateNow;
using mat::makeDirectories;
using mat::nowMs;
using mat::nowSecs;
using mat::parentPath;
using mat::parseIsoDate;
using mat::randomInt;
using mat::readTextFile;
using mat::uuidWithBraces;
using mat::writeBinaryFile;
using mat::writeTextFile;

// --- string helpers (QString::split / trimmed style) ------------------------

// QString::split(delimiter): empty pieces are kept, like Qt's default.
inline std::vector<std::string> splitString(const std::string& text, char delimiter)
{
    std::vector<std::string> out;
    std::size_t pos = 0;
    for (;;) {
        const std::size_t next = text.find(delimiter, pos);
        out.push_back(text.substr(pos, next == std::string::npos ? std::string::npos : next - pos));
        if (next == std::string::npos) break;
        pos = next + 1;
    }
    return out;
}

// --- random (QRandomGenerator / QUuid) --------------------------------------

// QRandomGenerator::global()->generateDouble(): [0.0, 1.0)
inline double randomDouble()
{
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(mat::rng());
}

// QUuid::createUuid().toString(QUuid::WithoutBraces):36 chars with dashes.
// (mat::randomUuid() is the32-hex shape without dashes - do not use it
// where the Qt code produced a real UUID string.)
std::string randomUuidNoBraces();

// --- dates (QDateTime::toString(Qt::ISODate)) -------------------------------

// QDateTime::currentDateTimeUtc().toString(Qt::ISODate) -> "...Z".
std::string isoDateNowUtc();

// --- paths (QFileInfo / QDir) -----------------------------------------------

bool dirExists(const std::string& path);

// QFileInfo(path).absoluteFilePath()
std::string absolutePath(const std::string& path);
// QFileInfo(path).absolutePath() - absolute directory containing the file
std::string absoluteParentPath(const std::string& path);
// QFileInfo(path).baseName() - file name minus the last suffix ("a.b.c" -> "a.b")
std::string baseName(const std::string& path);
// QFileInfo(path).suffix() - last suffix without the dot ("a.b.c" -> "c")
std::string suffix(const std::string& path);
// QDir::cleanPath(): forward slashes, no "." / ".." / duplicate separators
std::string cleanPath(const std::string& path);
// QDir::currentPath()
std::string currentPath();
// QCoreApplication::applicationDirPath()
std::string applicationDirPath();
// QDir::temp().absolutePath()
std::string tempDir();
// QDir::temp().absoluteFilePath(name)
std::string tempFilePath(const std::string& name);

// QFile-based whole-file reads (QFile::open(ReadOnly) + readAll)
bool readBinaryFile(const std::string& path, std::string* out, std::string* err = nullptr);
// QFile::remove / QFile::copy
bool removeFile(const std::string& path);
bool copyFile(const std::string& from, const std::string& to);

// QDir::entryList(nameFilters, QDir::Files): returns matching regular files.
// namePattern is a wildcard like "*.json"; recursive walks subdirectories
// (QDirIterator style). Overload takes multiple filters ("*.so", "*.dll").
std::vector<std::string> listFiles(const std::string& dir,
                                   const std::string& namePattern = "*",
                                   bool recursive = false);
std::vector<std::string> listFiles(const std::string& dir,
                                   const std::vector<std::string>& namePatterns,
                                   bool recursive = false);

// --- process (QProcess) ------------------------------------------------------
//
// QProcess::start(program, args) + waitForFinished + readAllStandardOutput/
// Error + exitCode + exitStatus are covered by runProcess(). The async form
// (readyReadStandardOutput/readyReadStandardError/finished signals) maps to
// runProcessAsync(): callbacks run on a detached worker thread, exactly like
// the Qt slots ran on the event-loop thread.

struct ProcessResult {
    int exitCode = -1;      // QProcess::exitCode(); -1 when the process never ran
    bool crashed = false;   // QProcess::CrashExit
    bool timedOut = false;
    std::string out;        // full stdout
    std::string err;        // full stderr
    // QProcess::ExitStatus::NormalExit && exitCode == 0
    bool ok() const { return !crashed && !timedOut && exitCode == 0; }
};

// timeoutMs < 0 waits forever (QProcess::waitForFinished(-1)).
// workingDir maps to QProcess::setWorkingDirectory().
ProcessResult runProcess(const std::string& program,
                         const std::vector<std::string>& args,
                         int timeoutMs = -1,
                         const std::string& workingDir = {});

struct ProcessCallbacks {
    std::function<void(const std::string& chunk)> onStdout;  // readyReadStandardOutput
    std::function<void(const std::string& chunk)> onStderr;  // readyReadStandardError
    std::function<void(int exitCode, bool crashed)> onFinished;  // finished
};

// Detached thread; the callbacks fire on that thread (the Qt versions fired
// on the event loop thread). The AudioUtil.cpp translation unit owns the
// thread; the caller outlives it in practice (fire-and-forget).
void runProcessAsync(const std::string& program,
                     const std::vector<std::string>& args,
                     ProcessCallbacks callbacks,
                     int timeoutMs = -1,
                     const std::string& workingDir = {});

// --- hashing (QCryptographicHash) -------------------------------------------

// QCryptographicHash::hash(data, Sha256).toHex() as lowercase hex.
std::string sha256Hex(const std::string& data);
std::string sha256Hex(const void* data, std::size_t size);
// MD5 lives in FileFormat/BinaryStream.h as ks::binary::md5Hex (same as
// QCryptographicHash::hash(data, Md5).toHex()) - include that header.

// --- shared library (QLibrary) ----------------------------------------------

class SharedLibrary {
public:
    SharedLibrary() = default;
    explicit SharedLibrary(const std::string& path) { load(path); }
    ~SharedLibrary() { unload(); }

    SharedLibrary(const SharedLibrary&) = delete;
    SharedLibrary& operator=(const SharedLibrary&) = delete;

    // QLibrary::load() / isLoaded() / resolve() / unload()
    bool load(const std::string& path);
    void unload();
    bool isLoaded() const { return m_handle != nullptr; }
    void* resolve(const char* symbol) const;

    template <typename T>
    T resolveAs(const char* symbol) const
    {
        return reinterpret_cast<T>(resolve(symbol));
    }

private:
    void* m_handle = nullptr;
};

// --- endian helpers (QtEndian: qFromLittleEndian / qFromBigEndian) ----------

inline std::uint16_t readLe16(const void* p)
{
    const auto* b = static_cast<const unsigned char*>(p);
    return static_cast<std::uint16_t>(b[0] | (b[1] << 8));
}
inline std::uint32_t readLe32(const void* p)
{
    const auto* b = static_cast<const unsigned char*>(p);
    return static_cast<std::uint32_t>(b[0]) | (static_cast<std::uint32_t>(b[1]) << 8) |
           (static_cast<std::uint32_t>(b[2]) << 16) | (static_cast<std::uint32_t>(b[3]) << 24);
}
inline std::uint16_t readBe16(const void* p)
{
    const auto* b = static_cast<const unsigned char*>(p);
    return static_cast<std::uint16_t>((b[0] << 8) | b[1]);
}
inline std::uint32_t readBe32(const void* p)
{
    const auto* b = static_cast<const unsigned char*>(p);
    return (static_cast<std::uint32_t>(b[0]) << 24) | (static_cast<std::uint32_t>(b[1]) << 16) |
           (static_cast<std::uint32_t>(b[2]) << 8) | static_cast<std::uint32_t>(b[3]);
}

// --- PCM helpers -------------------------------------------------------------

// Qt code converting float PCM to Int16 WAV data by hand (qRound + clamp).
inline void floatToInt16(const float* in, std::int16_t* out, int count)
{
    for (int i = 0; i < count; ++i) {
        float v = in[i] * 32767.0f;
        if (v > 32767.0f) v = 32767.0f;
        if (v < -32768.0f) v = -32768.0f;
        out[i] = static_cast<std::int16_t>(v + (v >= 0.0f ? 0.5f : -0.5f));
    }
}

inline void int16ToFloat(const std::int16_t* in, float* out, int count)
{
    for (int i = 0; i < count; ++i) out[i] = static_cast<float>(in[i]) / 32768.0f;
}

} // namespace ks::audio
