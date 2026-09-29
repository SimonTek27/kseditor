#pragma once

// Small Qt-free helpers used by the material module: timestamps (QDateTime),
// UUIDs (QUuid), random numbers (QRandomGenerator), directories (QDir) and
// plain file IO (QFile/QStandardPaths). Header only, so every material file
// can reach them without adding a new translation unit to the Qt-free target.

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

namespace ks::mat {

namespace fs = std::filesystem;

inline std::int64_t nowMs()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

inline std::int64_t nowSecs() { return nowMs() / 1000; }

inline std::string formatIsoDate(std::int64_t ms)
{
    const std::time_t t = static_cast<std::time_t>(ms / 1000);
    std::tm tmv{};
#if defined(_WIN32)
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    std::ostringstream os;
    os << std::put_time(&tmv, "%Y-%m-%dT%H:%M:%S");
    return os.str();
}

inline std::string isoDateNow() { return formatIsoDate(nowMs()); }

// Parses the Qt::ISODate shape produced by formatIsoDate(). Accepts an
// optional trailing "Z" or "+hh:mm"/"+hhmm" suffix (they are ignored, the
// value is read as local time). Returns 0 ms when the text is not a date.
inline std::int64_t parseIsoDate(const std::string& text)
{
    if (text.size() < 19) return 0;
    std::tm tmv{};
    std::istringstream is(text);
    is >> std::get_time(&tmv, "%Y-%m-%dT%H:%M:%S");
    if (is.fail()) return 0;
    const std::time_t t = std::mktime(&tmv);
    if (t == static_cast<std::time_t>(-1)) return 0;
    return static_cast<std::int64_t>(t) * 1000;
}

// Random helpers (std::mt19937 instead of QRandomGenerator).
inline std::mt19937& rng()
{
    static std::mt19937 gen([] {
        std::random_device rd;
        std::seed_seq seq{rd(), rd(), rd(), rd(),
                          static_cast<unsigned>(nowMs() & 0xFFFFFFFF)};
        return std::mt19937(seq);
    }());
    return gen;
}

inline int randomInt(int low, int high)
{
    if (high <= low) return low;
    std::uniform_int_distribution<int> dist(low, high);
    return dist(rng());
}

// 32 hex characters, the QUuid::WithoutBraces shape.
inline std::string randomUuid()
{
    static const char* hex = "0123456789abcdef";
    std::string out(32, '0');
    for (int i = 0; i < 32; ++i) {
        if (i == 12) out[static_cast<std::size_t>(i)] = '4';
        else out[static_cast<std::size_t>(i)] = hex[randomInt(0, 15)];
    }
    return out;
}

// Same text QUuid::createUuid().toString() produced: braces plus dashes.
inline std::string uuidWithBraces()
{
    const std::string u = randomUuid();
    return "{" + u.substr(0, 8) + "-" + u.substr(8, 4) + "-" + u.substr(12, 4) + "-" +
           u.substr(16, 4) + "-" + u.substr(20, 12) + "}";
}

// %APPDATA%\ksEditor\ksEditor on Windows, the XDG data home elsewhere -
// the same location QStandardPaths::AppDataLocation resolved to for the
// editor, so existing shader presets and material libraries keep loading.
inline std::string envValue(const char* name)
{
#if defined(_MSC_VER)
    char* value = nullptr;
    std::size_t len = 0;
    if (_dupenv_s(&value, &len, name) == 0 && value != nullptr) {
        std::string out(value);
        free(value);
        return out;
    }
    return std::string();
#else
    const char* value = std::getenv(name);
    return value ? std::string(value) : std::string();
#endif
}

inline std::string appDataLocation()
{
#if defined(_WIN32)
    const std::string appdata = envValue("APPDATA");
    fs::path base = !appdata.empty() ? fs::path(appdata)
                                     : fs::path(".") / "AppData" / "Roaming";
    return (base / "ksEditor" / "ksEditor").string();
#else
    const std::string xdg = envValue("XDG_DATA_HOME");
    fs::path base = !xdg.empty() ? fs::path(xdg) : fs::path(".") / ".local" / "share";
    return (base / "ksEditor" / "ksEditor").string();
#endif
}

inline bool makeDirectories(const std::string& path)
{
    if (path.empty()) return false;
    std::error_code ec;
    fs::create_directories(fs::path(path), ec);
    return !ec || fs::exists(fs::path(path));
}

inline bool fileExists(const std::string& path)
{
    std::error_code ec;
    return fs::exists(fs::path(path), ec);
}

inline std::string parentPath(const std::string& path)
{
    return fs::path(path).parent_path().string();
}

inline std::string fileName(const std::string& path)
{
    return fs::path(path).filename().string();
}

inline bool readTextFile(const std::string& path, std::string* out, std::string* err = nullptr)
{
    std::ifstream file(fs::path(path), std::ios::binary);
    if (!file.is_open()) {
        if (err) *err = "cannot open file: " + path;
        return false;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    if (out) *out = ss.str();
    return true;
}

inline bool writeTextFile(const std::string& path, const std::string& text,
                          std::string* err = nullptr)
{
    const fs::path p(path);
    if (p.has_parent_path()) makeDirectories(p.parent_path().string());
    std::ofstream file(p, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        if (err) *err = "cannot open file for writing: " + path;
        return false;
    }
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!file.good()) {
        if (err) *err = "cannot write file: " + path;
        return false;
    }
    return true;
}

inline bool writeBinaryFile(const std::string& path, const void* data, std::size_t size,
                            std::string* err = nullptr)
{
    const fs::path p(path);
    if (p.has_parent_path()) makeDirectories(p.parent_path().string());
    std::ofstream file(p, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        if (err) *err = "cannot open file for writing: " + path;
        return false;
    }
    file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    if (!file.good()) {
        if (err) *err = "cannot write file: " + path;
        return false;
    }
    return true;
}

} // namespace ks::mat
