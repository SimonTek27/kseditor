#pragma once

// Qt-free helpers shared by the chat / collaboration code:
//   * wall-clock timestamps (replaces QDateTime::current*SinceEpoch)
//   * id generation  (replaces QUuid::createUuid)
//   * random numbers (replaces QRandomGenerator)
//   * "hh:mm:ss" formatting used by the collaboration UI

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <random>
#include <string>

namespace ks::net {

inline std::int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

inline std::int64_t nowSecs() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// Format an epoch-second timestamp as "hh:mm:ss" (local time),
// matching QDateTime::fromSecsSinceEpoch(t).toString("hh:mm:ss").
inline std::string formatHms(std::int64_t epochSeconds);

// Format an epoch-millisecond timestamp as "yyyy-MM-ddTHH:mm:ss" (local time),
// matching QDateTime::fromMSecsSinceEpoch(t).toString(Qt::ISODate).
inline std::string formatIsoDate(std::int64_t epochMs);

// 32 lowercase hex chars, i.e. a UUID v4 without braces and dashes
// (matches QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-')).
inline std::string randomId();

// Canonical UUID text form "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
// (QUuid::createUuid().toString(QUuid::WithoutBraces)).
inline std::string randomUuid();

// uniform integer in [low, high] (QRandomGenerator::bounded(low, high + 1))
inline int randomInt(int low, int high);

namespace detail {

inline std::mt19937_64& rng() {
    static std::mt19937_64 engine{[] {
        std::random_device rd;
        std::seed_seq seq{rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};
        return std::mt19937_64(seq);
    }()};
    return engine;
}

} // namespace detail

inline std::string formatHms(std::int64_t epochSeconds) {
    std::time_t t = static_cast<std::time_t>(epochSeconds);
    std::tm tmv{};
#if defined(_WIN32)
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
    return std::string(buf);
}

inline std::string formatIsoDate(std::int64_t epochMs) {
    std::time_t t = static_cast<std::time_t>(epochMs / 1000);
    std::tm tmv{};
#if defined(_WIN32)
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
                  tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                  tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
    return std::string(buf);
}

inline std::string randomId() {
    static const char* kHex = "0123456789abcdef";
    std::string out;
    out.reserve(32);
    std::uniform_int_distribution<int> nibble(0, 15);
    auto& e = detail::rng();
    for (int i = 0; i < 32; ++i) out.push_back(kHex[nibble(e)]);
    return out;
}

inline std::string randomUuid() {
    const std::string hex = randomId();
    std::string out;
    out.reserve(36);
    out.append(hex, 0, 8);   out.push_back('-');
    out.append(hex, 8, 4);   out.push_back('-');
    out.append(hex, 12, 4);  out.push_back('-');
    out.append(hex, 16, 4);  out.push_back('-');
    out.append(hex, 20, 12);
    return out;
}

inline int randomInt(int low, int high) {
    if (high <= low) return low;
    std::uniform_int_distribution<int> dist(low, high);
    return dist(detail::rng());
}

} // namespace ks::net
