#pragma once

#include <QIODevice>
#include <QByteArray>
#include <QString>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace ks::fileformat {

// Arrays >= this size use LZSS compression in BIS binary formats.
constexpr size_t BIS_COMPRESSION_THRESHOLD = 1024;

// ============================================================================
// BinaryReader - Reads BIS (Bohemia Interactive Studios) binary formats
// Rewritten from CWR/Poseidon BISBinaryStream.hpp for Qt
// ============================================================================
class BinaryReader {
public:
    explicit BinaryReader(QIODevice& device) : m_device(device) {}

    qint64 tell() const { return m_device.pos(); }
    void seek(qint64 pos) { m_device.seek(pos); }
    void seekRelative(qint64 offset) { m_device.seek(m_device.pos() + offset); }
    bool fail() const { return m_device.status() != QIODevice::NoError; }
    qint64 remaining() const { return m_device.bytesAvailable(); }

    template<typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, T>::type read() {
        if constexpr (std::is_same_v<T, bool>) {
            char raw = 0;
            if (m_device.read(&raw, sizeof(raw)) != sizeof(raw))
                throw std::runtime_error("Failed to read bool");
            return raw != 0;
        } else {
            T value{};
            if (m_device.read(reinterpret_cast<char*>(&value), sizeof(T)) != sizeof(T))
                throw std::runtime_error("Failed to read primitive");
            return value;
        }
    }

    template<typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, void>::type read(T& value) {
        value = read<T>();
    }

    QString readAsciiz() {
        QString result;
        char c;
        while (m_device.getChar(&c)) {
            if (c == '\0') break;
            result.append(c);
            if (result.size() > 1024 * 1024)
                throw std::runtime_error("String too long (no null terminator)");
        }
        return result;
    }

    QString readString() {
        int32_t length = read<int32_t>();
        if (length < 0 || length > 1024 * 1024)
            throw std::runtime_error("Invalid string length");
        if (length == 0) return {};
        QByteArray data = m_device.read(length);
        if (data.size() != length)
            throw std::runtime_error("Failed to read string data");
        return QString::fromLatin1(data);
    }

    template<typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value, std::vector<T>>::type
    readArray() {
        uint32_t count = read<uint32_t>();
        if (count == 0) return {};
        if (static_cast<uint64_t>(count) * sizeof(T) > static_cast<uint64_t>(remaining()))
            throw std::runtime_error("Array count exceeds remaining input");
        std::vector<T> result(count);
        if (m_device.read(reinterpret_cast<char*>(result.data()), count * sizeof(T))
            != static_cast<qint64>(count * sizeof(T)))
            throw std::runtime_error("Failed to read array data");
        return result;
    }

    template<typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value, void>::type
    readArray(std::vector<T>& result) {
        result = readArray<T>();
    }

    template<typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value, std::vector<T>>::type
    readCompressedArray() {
        uint32_t count = read<uint32_t>();
        if (count == 0) return {};
        if (static_cast<uint64_t>(count) * sizeof(T) > (256ull * 1024 * 1024))
            throw std::runtime_error("Compressed array count too large");
        std::vector<T> result(count);
        size_t sizeInBytes = count * sizeof(T);
        if (sizeInBytes >= BIS_COMPRESSION_THRESHOLD) {
            if (!decompressLZSS(reinterpret_cast<uint8_t*>(result.data()), sizeInBytes))
                throw std::runtime_error("LZSS decompression failed");
        } else {
            if (m_device.read(reinterpret_cast<char*>(result.data()), sizeInBytes)
                != static_cast<qint64>(sizeInBytes))
                throw std::runtime_error("Failed to read array data");
        }
        return result;
    }

    template<typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value, void>::type
    readCompressedArray(std::vector<T>& result) {
        result = readCompressedArray<T>();
    }

    void readBytes(void* buffer, size_t count) {
        if (m_device.read(static_cast<char*>(buffer), count) != static_cast<qint64>(count))
            throw std::runtime_error("Failed to read raw bytes");
    }

private:
    QIODevice& m_device;

    // BIS LZSS decompression
    // Ring buffer of 4096 bytes, initialized to 0x20 (space)
    bool decompressLZSS(uint8_t* dst, size_t dstSize) {
        constexpr int N = 4096;
        constexpr int F = 18;
        constexpr int THRESHOLD = 2;
        uint8_t ringBuf[N];
        memset(ringBuf, 0x20, N);

        int r = N - F;
        size_t dstIdx = 0;
        int flags = 0;

        while (dstIdx < dstSize) {
            flags >>= 1;
            if ((flags & 0x100) == 0) {
                char c;
                if (m_device.getChar(&c) != 1) return false;
                flags = static_cast<uint8_t>(c) | 0xFF00;
            }

            if (flags & 1) {
                char c;
                if (m_device.getChar(&c) != 1) return false;
                dst[dstIdx++] = static_cast<uint8_t>(c);
                ringBuf[r] = static_cast<uint8_t>(c);
                r = (r + 1) & (N - 1);
            } else {
                uint8_t lo, hi;
                if (m_device.read(reinterpret_cast<char*>(&lo), 1) != 1) return false;
                if (m_device.read(reinterpret_cast<char*>(&hi), 1) != 1) return false;
                int offset = lo | ((hi & 0xF0) << 4);
                int len = (hi & 0x0F) + THRESHOLD + 1;
                for (int i = 0; i < len && dstIdx < dstSize; ++i) {
                    uint8_t c = ringBuf[(offset + i) & (N - 1)];
                    dst[dstIdx++] = c;
                    ringBuf[r] = c;
                    r = (r + 1) & (N - 1);
                }
            }
        }
        return true;
    }
};

// ============================================================================
// BIS POD Structures - Rewritten from CWR BISStructures.hpp
// ============================================================================

struct BisVector2 { float u = 0, v = 0; };
struct BisVector3 { float x = 0, y = 0, z = 0; };
struct BisVector4 { float x = 0, y = 0, z = 0, w = 0; };
struct BisMatrix3x3 { float data[9] = {}; };
struct BisMatrix4x3 { BisVector3 rows[4]; };
struct BisBoundingBox { BisVector3 min, max; };
struct BisBoundingSphere { BisVector3 center; float radius = 0; };

inline void read(BinaryReader& r, BisVector2& v) { r.read(v.u); r.read(v.v); }
inline void read(BinaryReader& r, BisVector3& v) { r.read(v.x); r.read(v.y); r.read(v.z); }
inline void read(BinaryReader& r, BisVector4& v) { r.read(v.x); r.read(v.y); r.read(v.z); r.read(v.w); }
inline void read(BinaryReader& r, BisMatrix3x3& m) { for (int i = 0; i < 9; ++i) r.read(m.data[i]); }
inline void read(BinaryReader& r, BisMatrix4x3& m) { for (int i = 0; i < 4; ++i) read(r, m.rows[i]); }
inline void read(BinaryReader& r, BisBoundingBox& b) { read(r, b.min); read(r, b.max); }
inline void read(BinaryReader& r, BisBoundingSphere& s) { read(r, s.center); r.read(s.radius); }

// ============================================================================
// P3D Format Detector
// ============================================================================

enum class P3DFormat { Unknown, ODOL, MLOD };

struct P3DFormatInfo {
    P3DFormat format = P3DFormat::Unknown;
    uint32_t version = 0;
    bool isSupported = false;
    QString errorMessage;

    QString versionString() const {
        if (format == P3DFormat::MLOD)
            return QString("%1.%2").arg((version >> 8) & 0xFF).arg(version & 0xFF);
        return QString::number(version);
    }
};

inline P3DFormatInfo detectP3DFormat(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return {P3DFormat::Unknown, 0, false, "Cannot open file"};

    char sig[5] = {};
    if (file.read(sig, 4) != 4)
        return {P3DFormat::Unknown, 0, false, "Cannot read signature"};

    P3DFormatInfo info;
    if (memcmp(sig, "ODOL", 4) == 0) {
        info.format = P3DFormat::ODOL;
        file.read(reinterpret_cast<char*>(&info.version), 4);
        info.isSupported = (info.version == 7);
        if (!info.isSupported)
            info.errorMessage = QString("Unsupported ODOL version %1").arg(info.version);
    } else if (memcmp(sig, "MLOD", 4) == 0) {
        info.format = P3DFormat::MLOD;
        file.read(reinterpret_cast<char*>(&info.version), 4);
        info.isSupported = true;
    } else {
        info.errorMessage = "Not a P3D file (unknown signature)";
    }
    return info;
}

} // namespace ks::fileformat
