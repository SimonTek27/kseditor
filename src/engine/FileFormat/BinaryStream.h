#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <istream>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

// Qt-free replacement for QDataStream (byte-compatible with the Qt default:
// big-endian integers, IEEE-754 floats/doubles, SinglePrecision by default)
// plus the MD5 helper that replaces QCryptographicHash::hash(..., Md5).

namespace ks {

enum class BinaryByteOrder { BigEndian, LittleEndian };
enum class BinaryFloatPrecision { SinglePrecision, DoublePrecision };
enum class BinaryStatus { Ok, ReadPastEnd, WriteFailed, ReadCorruptData };

namespace binary {

namespace detail {

inline std::uint32_t rotl(std::uint32_t x, int c) {
    return (x << c) | (x >> (32 - c));
}

inline void md5Transform(std::uint32_t state[4], const unsigned char* block) {
    static const std::uint32_t K[64] = {
        0xd76aa478u, 0xe8c7b756u, 0x242070dbu, 0xc1bdceeeu,
        0xf57c0fafu, 0x4787c62au, 0xa8304613u, 0xfd469501u,
        0x698098d8u, 0x8b44f7afu, 0xffff5bb1u, 0x895cd7beu,
        0x6b901122u, 0xfd987193u, 0xa679438eu, 0x49b40821u,
        0xf61e2562u, 0xc040b340u, 0x265e5a51u, 0xe9b6c7aau,
        0xd62f105du, 0x02441453u, 0xd8a1e681u, 0xe7d3fbc8u,
        0x21e1cde6u, 0xc33707d6u, 0xf4d50d87u, 0x455a14edu,
        0xa9e3e905u, 0xfcefa3f8u, 0x676f02d9u, 0x8d2a4c8au,
        0xfffa3942u, 0x8771f681u, 0x6d9d6122u, 0xfde5380cu,
        0xa4beea44u, 0x4bdecfa9u, 0xf6bb4b60u, 0xbebfbc70u,
        0x289b7ec6u, 0xeaa127fau, 0xd4ef3085u, 0x04881d05u,
        0xd9d4d039u, 0xe6db99e5u, 0x1fa27cf8u, 0xc4ac5665u,
        0xf4292244u, 0x432aff97u, 0xab9423a7u, 0xfc93a039u,
        0x655b59c3u, 0x8f0ccc92u, 0xffeff47du, 0x85845dd1u,
        0x6fa87e4fu, 0xfe2ce6e0u, 0xa3014314u, 0x4e0811a1u,
        0xf7537e82u, 0xbd3af235u, 0x2ad7d2bbu, 0xeb86d391u
    };
    static const unsigned S[64] = {
        7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
        5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
        4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
        6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
    };

    std::uint32_t M[16];
    for (int i = 0; i < 16; ++i) {
        M[i] = static_cast<std::uint32_t>(block[i * 4]) |
               (static_cast<std::uint32_t>(block[i * 4 + 1]) << 8) |
               (static_cast<std::uint32_t>(block[i * 4 + 2]) << 16) |
               (static_cast<std::uint32_t>(block[i * 4 + 3]) << 24);
    }

    std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    for (int i = 0; i < 64; ++i) {
        std::uint32_t f;
        int g;
        if (i < 16) {
            f = (b & c) | (~b & d);
            g = i;
        } else if (i < 32) {
            f = (d & b) | (~d & c);
            g = (5 * i + 1) % 16;
        } else if (i < 48) {
            f = b ^ c ^ d;
            g = (3 * i + 5) % 16;
        } else {
            f = c ^ (b | ~d);
            g = (7 * i) % 16;
        }
        const std::uint32_t tmp = d;
        d = c;
        c = b;
        b = b + rotl(a + f + K[i] + M[g], S[i]);
        a = tmp;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

} // namespace detail

// Lowercase hex digest of the RFC 1321 MD5 of data (QCryptographicHash::Md5).
inline std::string md5Hex(const std::string& input) {
    const std::size_t len = input.size();
    const std::size_t padded = ((len + 8) / 64 + 1) * 64;
    std::vector<unsigned char> msg(padded, 0);
    if (len) std::memcpy(msg.data(), input.data(), len);
    msg[len] = 0x80;
    const std::uint64_t bits = static_cast<std::uint64_t>(len) * 8u;
    for (int i = 0; i < 8; ++i) {
        msg[padded - 8 + i] = static_cast<unsigned char>((bits >> (8 * i)) & 0xFFu);
    }

    std::uint32_t state[4] = {0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u};
    for (std::size_t off = 0; off < padded; off += 64) {
        detail::md5Transform(state, msg.data() + off);
    }

    static const char* hexDigits = "0123456789abcdef";
    std::string out;
    out.reserve(32);
    for (int i = 0; i < 4; ++i) {
        for (int b = 0; b < 4; ++b) {
            const unsigned char byte = static_cast<unsigned char>((state[i] >> (8 * b)) & 0xFFu);
            out.push_back(hexDigits[byte >> 4]);
            out.push_back(hexDigits[byte & 0x0F]);
        }
    }
    return out;
}

} // namespace binary

// Writes to std::ostream (files opened in binary mode, or an ostringstream
// when the Qt code used a QBuffer/QByteArray as the device).
class BinaryWriter {
public:
    explicit BinaryWriter(std::ostream& os,
                          BinaryByteOrder byteOrder = BinaryByteOrder::BigEndian)
        : m_os(&os), m_byteOrder(byteOrder) {}

    void setByteOrder(BinaryByteOrder order) { m_byteOrder = order; }
    BinaryByteOrder byteOrder() const { return m_byteOrder; }
    void setFloatingPointPrecision(BinaryFloatPrecision precision) {
        m_floatPrecision = precision;
    }
    BinaryStatus status() const { return m_status; }
    void resetStatus() { m_status = BinaryStatus::Ok; }

    template <typename T,
              typename = std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>>>
    BinaryWriter& operator<<(T value) {
        if (m_status != BinaryStatus::Ok) return *this;
        if constexpr (std::is_floating_point_v<T>) {
            if (sizeof(T) == 4 || m_floatPrecision == BinaryFloatPrecision::SinglePrecision) {
                const float f = static_cast<float>(value);
                writeRaw(&f, sizeof(float));
            } else {
                const double d = static_cast<double>(value);
                writeRaw(&d, sizeof(double));
            }
        } else {
            writeRaw(&value, sizeof(T));
        }
        return *this;
    }

    // QDataStream::writeBytes(): quint32 length (0xFFFFFFFF when s is null) + raw data.
    void writeBytes(const char* s, std::uint32_t len) {
        if (!s) {
            *this << static_cast<std::uint32_t>(0xFFFFFFFFu);
            return;
        }
        *this << len;
        writeRaw(s, len);
    }

    // QByteArray/QString payload: quint32 byte length + UTF-8 bytes (Qt 6).
    void writeByteArray(const std::string& bytes) {
        *this << static_cast<std::uint32_t>(bytes.size());
        if (!bytes.empty()) writeRaw(bytes.data(), bytes.size());
    }

    void writeRaw(const void* data, std::size_t size) {
        if (m_status != BinaryStatus::Ok || !m_os) return;
        if (!size) return;
        unsigned char buf[256];
        const unsigned char* src = static_cast<const unsigned char*>(data);
        std::size_t offset = 0;
        while (offset < size) {
            const std::size_t chunk = std::min<std::size_t>(size - offset, sizeof(buf));
            std::memcpy(buf, src + offset, chunk);
            if (m_byteOrder == BinaryByteOrder::BigEndian && chunk > 1) {
                std::reverse(buf, buf + chunk);
            }
            m_os->write(reinterpret_cast<const char*>(buf),
                        static_cast<std::streamsize>(chunk));
            if (!*m_os) {
                m_status = BinaryStatus::WriteFailed;
                return;
            }
            offset += chunk;
        }
    }

private:
    std::ostream* m_os;
    BinaryByteOrder m_byteOrder;
    BinaryFloatPrecision m_floatPrecision = BinaryFloatPrecision::SinglePrecision;
    BinaryStatus m_status = BinaryStatus::Ok;
};

// Reads from std::istream (files opened in binary mode, or an istringstream
// when the Qt code read from a QByteArray/QBuffer).
class BinaryReader {
public:
    explicit BinaryReader(std::istream& is,
                          BinaryByteOrder byteOrder = BinaryByteOrder::BigEndian)
        : m_is(&is), m_byteOrder(byteOrder) {}

    void setByteOrder(BinaryByteOrder order) { m_byteOrder = order; }
    BinaryByteOrder byteOrder() const { return m_byteOrder; }
    void setFloatingPointPrecision(BinaryFloatPrecision precision) {
        m_floatPrecision = precision;
    }
    BinaryStatus status() const { return m_status; }
    void resetStatus() { m_status = BinaryStatus::Ok; }

    template <typename T,
              typename = std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>>>
    BinaryReader& operator>>(T& value) {
        if (m_status != BinaryStatus::Ok) return *this;
        if constexpr (std::is_floating_point_v<T>) {
            if (sizeof(T) == 4 || m_floatPrecision == BinaryFloatPrecision::SinglePrecision) {
                float f = 0.0f;
                if (readRawInto(&f, sizeof(float)) != sizeof(float)) return *this;
                value = static_cast<T>(f);
            } else {
                double d = 0.0;
                if (readRawInto(&d, sizeof(double)) != sizeof(double)) return *this;
                value = static_cast<T>(d);
            }
        } else {
            if (readRawInto(&value, sizeof(T)) != sizeof(T)) return *this;
        }
        return *this;
    }

    // QDataStream::readBytes(): quint32 length + raw data.
    std::string readBytes() {
        std::uint32_t len = 0;
        *this >> len;
        if (m_status != BinaryStatus::Ok) return std::string();
        if (len == 0xFFFFFFFFu) return std::string();
        return readRawString(len);
    }

    std::string readByteArray() { return readBytes(); }

    std::size_t readRaw(void* data, std::size_t size) { return readRawInto(data, size); }

    std::string readRawString(std::size_t size) {
        std::string out(size, '\0');
        if (!size) return out;
        const std::size_t got = readRawInto(&out[0], size);
        if (got < size) out.resize(got);
        return out;
    }

private:
    std::size_t readRawInto(void* data, std::size_t size) {
        if (m_status != BinaryStatus::Ok || !m_is || !size) return 0;
        unsigned char buf[256];
        unsigned char* dst = static_cast<unsigned char*>(data);
        std::size_t offset = 0;
        while (offset < size) {
            const std::size_t chunk = std::min<std::size_t>(size - offset, sizeof(buf));
            m_is->read(reinterpret_cast<char*>(buf), static_cast<std::streamsize>(chunk));
            const std::size_t got = static_cast<std::size_t>(m_is->gcount());
            if (m_byteOrder == BinaryByteOrder::BigEndian && got > 1) {
                std::reverse(buf, buf + got);
            }
            std::memcpy(dst + offset, buf, got);
            offset += got;
            if (got < chunk) {
                m_status = BinaryStatus::ReadPastEnd;
                return offset;
            }
        }
        return offset;
    }

    std::istream* m_is;
    BinaryByteOrder m_byteOrder;
    BinaryFloatPrecision m_floatPrecision = BinaryFloatPrecision::SinglePrecision;
    BinaryStatus m_status = BinaryStatus::Ok;
};

} // namespace ks
