#pragma once

#include <QIODevice>
#include <QFile>
#include <QByteArray>
#include <QString>
#include <QImage>
#include <QVector>
#include <cstdint>

namespace ks::fileformat {

// ============================================================================
// PAA/PAC Texture Format Converter
// Rewritten from CWR PAADecoder.cpp for Qt - handles PAA and PAC files
// Supports: ARGB8888, ARGB4444, ARGB1555, AI88, DXT1, DXT3, DXT5
// ============================================================================

enum class PAAPixelFormat : uint16_t {
    ARGB8888 = 0x8888,
    ARGB4444 = 0x4444,
    ARGB1555 = 0x1555,
    AI88     = 0x8080,
    DXT1     = 0xFF01,
    DXT2     = 0xFF02,
    DXT3     = 0xFF03,
    DXT4     = 0xFF04,
    DXT5     = 0xFF05,
    Unknown  = 0x0000,
};

struct PAATexture {
    int width = 0;
    int height = 0;
    PAAPixelFormat format = PAAPixelFormat::Unknown;
    QString formatName;
    bool isPAA = true; // true = PAA (palette), false = PAC (no palette)
    QByteArray pixelData; // raw pixel data
    QImage decodedImage;  // decoded RGBA image

    bool isValid() const { return width > 0 && height > 0; }

    static const char* formatToString(PAAPixelFormat fmt) {
        switch (fmt) {
            case PAAPixelFormat::ARGB8888: return "ARGB8888";
            case PAAPixelFormat::ARGB4444: return "ARGB4444";
            case PAAPixelFormat::ARGB1555: return "ARGB1555";
            case PAAPixelFormat::AI88:     return "AI88";
            case PAAPixelFormat::DXT1:     return "DXT1";
            case PAAPixelFormat::DXT2:     return "DXT2";
            case PAAPixelFormat::DXT3:     return "DXT3";
            case PAAPixelFormat::DXT4:     return "DXT4";
            case PAAPixelFormat::DXT5:     return "DXT5";
            default: return "Unknown";
        }
    }
};

class PAADecoder {
public:
    PAATexture load(const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
            return {};

        PAATexture tex;
        tex.isPaa = filePath.toLower().endsWith(".paa");
        return decodeFromDevice(file, tex);
    }

    PAATexture decodeFromDevice(QIODevice& device, PAATexture& tex) {
        if (device.bytesAvailable() < 4)
            return {};

        // Read first 2 bytes - format magic or palette marker
        uint16_t magic = 0;
        device.read(reinterpret_cast<char*>(&magic), 2);

        // Check if it's a known format
        tex.format = detectFormat(magic);
        tex.formatName = PAATexture::formatToString(tex.format);

        if (tex.format == PAAPixelFormat::Unknown) {
            // Could be palette-based - rewind and try palette path
            device.seek(0);
        }

        // Skip TAGG sections (metadata)
        skipTaggSections(device);

        // Read palette (PAA only)
        QVector<QColor> palette;
        if (tex.isPaa && tex.format == PAAPixelFormat::Unknown) {
            palette = readPalette(device);
            tex.format = PAAPixelFormat::ARGB4444; // default for palette-based
            tex.formatName = PAATexture::formatToString(tex.format);
        }

        // Read mipmap levels and decode the first (largest) level
        tex = decodeMipLevel(device, tex, palette, 0);
        return tex;
    }

private:
    PAAPixelFormat detectFormat(uint16_t magic) {
        switch (magic) {
            case 0x8888: return PAAPixelFormat::ARGB8888;
            case 0x4444: return PAAPixelFormat::ARGB4444;
            case 0x1555: return PAAPixelFormat::ARGB1555;
            case 0x8080: return PAAPixelFormat::AI88;
            case 0xFF01: return PAAPixelFormat::DXT1;
            case 0xFF02: return PAAPixelFormat::DXT2;
            case 0xFF03: return PAAPixelFormat::DXT3;
            case 0xFF04: return PAAPixelFormat::DXT4;
            case 0xFF05: return PAAPixelFormat::DXT5;
            default: return PAAPixelFormat::Unknown;
        }
    }

    void skipTaggSections(QIODevice& device) {
        while (device.bytesAvailable() >= 12) {
            char tag[4];
            device.read(tag, 4);
            if (memcmp(tag, "TAGG", 4) != 0) {
                device.seek(device.pos() - 4);
                return;
            }
            device.read(reinterpret_cast<char*>(&m_padding), 4); // version
            uint32_t size = 0;
            device.read(reinterpret_cast<char*>(&size), 4);
            if (size > 0 && size < 1024 * 1024) {
                device.seek(device.pos() + size);
            }
        }
    }

    QVector<QColor> readPalette(QIODevice& device) {
        uint16_t count = 0;
        device.read(reinterpret_cast<char*>(&count), 2);
        QVector<QColor> palette(count);
        for (int i = 0; i < count; ++i) {
            uint8_t rgb[3];
            device.read(reinterpret_cast<char*>(rgb), 3);
            palette[i] = QColor(rgb[0], rgb[1], rgb[2]);
        }
        return palette;
    }

    PAATexture decodeMipLevel(QIODevice& device, PAATexture& tex,
                               const QVector<QColor>& palette, int mipLevel) {
        // Read mipmap header: width (uint16) + height (uint16) + data size (uint24)
        while (device.bytesAvailable() >= 8) {
            uint16_t w = 0, h = 0;
            device.read(reinterpret_cast<char*>(&w), 2);
            device.read(reinterpret_cast<char*>(&h), 2);

            if (w == 0 && h == 0)
                break;

            // Handle large textures (dimensions > 65535 encoded specially)
            if (w == 1234 && h == 8765) {
                device.read(reinterpret_cast<char*>(&w), 2);
                device.read(reinterpret_cast<char*>(&h), 2);
            }

            uint32_t dataSize = 0;
            device.read(reinterpret_cast<char*>(&dataSize), 3);
            dataSize &= 0xFFFFFF;

            if (mipLevel == 0) {
                tex.width = w;
                tex.height = h;

                if (w > 8192 || h > 8192 || dataSize > 64 * 1024 * 1024) {
                    device.seek(device.pos() + dataSize);
                    break;
                }

                QByteArray data = device.read(dataSize);
                if (data.size() != dataSize)
                    break;

                tex.pixelData = data;
                tex.decodedImage = decodePixels(data, w, h, tex.format, palette);
                return tex;
            }

            // Skip non-zero mipmap data
            device.seek(device.pos() + dataSize);
        }
        return tex;
    }

    QImage decodePixels(const QByteArray& data, int w, int h,
                         PAAPixelFormat format, const QVector<QColor>& palette) {
        QImage img(w, h, QImage::Format_RGBA8888);
        img.fill(Qt::transparent);

        switch (format) {
            case PAAPixelFormat::ARGB8888:
                decodeARGB8888(data, img, w, h);
                break;
            case PAAPixelFormat::ARGB4444:
            case PAAPixelFormat::AI88:
                decodeARGB4444(data, img, w, h);
                break;
            case PAAPixelFormat::ARGB1555:
                decodeARGB1555(data, img, w, h);
                break;
            case PAAPixelFormat::DXT1:
                decodeDXT1(data, img, w, h);
                break;
            case PAAPixelFormat::DXT2:
            case PAAPixelFormat::DXT3:
                decodeDXT3(data, img, w, h);
                break;
            case PAAPixelFormat::DXT4:
            case PAAPixelFormat::DXT5:
                decodeDXT5(data, img, w, h);
                break;
            default:
                break;
        }
        return img;
    }

    // --- Pixel format decoders ---

    void decodeARGB8888(const QByteArray& data, QImage& img, int w, int h) {
        const uint8_t* src = reinterpret_cast<const uint8_t*>(data.constData());
        int srcSize = data.size();
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int idx = (y * w + x) * 4;
                if (idx + 3 >= srcSize) return;
                // PAA stores as BGRA in memory
                uint8_t b = src[idx], g = src[idx + 1], r = src[idx + 2], a = src[idx + 3];
                img.setPixelColor(x, y, QColor(r, g, b, a));
            }
        }
    }

    void decodeARGB4444(const QByteArray& data, QImage& img, int w, int h) {
        const uint8_t* src = reinterpret_cast<const uint8_t*>(data.constData());
        int srcSize = data.size();
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int idx = (y * w + x) * 2;
                if (idx + 1 >= srcSize) return;
                uint16_t p = src[idx] | (src[idx + 1] << 8);
                uint8_t a = (p >> 12) & 0xF;
                uint8_t r = (p >> 8) & 0xF;
                uint8_t g = (p >> 4) & 0xF;
                uint8_t b = p & 0xF;
                img.setPixelColor(x, y, QColor((r << 4) | r, (g << 4) | g,
                                               (b << 4) | b, (a << 4) | a));
            }
        }
    }

    void decodeARGB1555(const QByteArray& data, QImage& img, int w, int h) {
        const uint8_t* src = reinterpret_cast<const uint8_t*>(data.constData());
        int srcSize = data.size();
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int idx = (y * w + x) * 2;
                if (idx + 1 >= srcSize) return;
                uint16_t p = src[idx] | (src[idx + 1] << 8);
                uint8_t r = (p >> 10) & 0x1F;
                uint8_t g = (p >> 5) & 0x1F;
                uint8_t b = p & 0x1F;
                img.setPixelColor(x, y, QColor((r << 3) | (r >> 2),
                                               (g << 3) | (g >> 2),
                                               (b << 3) | (b >> 2),
                                               (p & 0x8000) ? 255 : 0));
            }
        }
    }

    // --- DXT decompression ---

    static inline void expand565(uint16_t c, uint8_t out[4]) {
        int r = (c >> 11) & 0x1F;
        int g = (c >> 5) & 0x3F;
        int b = c & 0x1F;
        out[0] = (r << 3) | (r >> 2);
        out[1] = (g << 2) | (g >> 4);
        out[2] = (b << 3) | (b >> 2);
        out[3] = 255;
    }

    void decodeDXT1Block(const uint8_t* block, uint8_t pixels[4][4][4]) {
        uint16_t c0 = block[0] | (block[1] << 8);
        uint16_t c1 = block[2] | (block[3] << 8);
        uint8_t colors[4][4];
        expand565(c0, colors[0]);
        expand565(c1, colors[1]);

        if (c0 > c1) {
            for (int i = 0; i < 3; i++) {
                colors[2][i] = (2 * colors[0][i] + colors[1][i] + 1) / 3;
                colors[3][i] = (colors[0][i] + 2 * colors[1][i] + 1) / 3;
            }
            colors[2][3] = colors[3][3] = 255;
        } else {
            for (int i = 0; i < 3; i++)
                colors[2][i] = (colors[0][i] + colors[1][i]) / 2;
            colors[2][3] = 255;
            colors[3][0] = colors[3][1] = colors[3][2] = 0;
            colors[3][3] = 0; // transparent black
        }

        uint32_t idx = block[4] | (block[5] << 8) | (block[6] << 16) | (static_cast<uint32_t>(block[7]) << 24);
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 4; x++)
                memcpy(pixels[y][x], colors[(idx >> ((y * 4 + x) * 2)) & 3], 4);
    }

    void writeDXTBlock(QImage& img, int bx, int by, const uint8_t pixels[4][4][4]) {
        int imgW = img.width(), imgH = img.height();
        for (int py = 0; py < 4 && by * 4 + py < imgH; py++)
            for (int px = 0; px < 4 && bx * 4 + px < imgW; px++)
                img.setPixelColor(bx * 4 + px, by * 4 + py,
                    QColor(pixels[py][px][0], pixels[py][px][1],
                           pixels[py][px][2], pixels[py][px][3]));
    }

    void decodeDXT1(const QByteArray& data, QImage& img, int w, int h) {
        const uint8_t* src = reinterpret_cast<const uint8_t*>(data.constData());
        int srcSize = data.size();
        int bw = (w + 3) / 4, bh = (h + 3) / 4;
        int srcOffset = 0;
        for (int by = 0; by < bh; ++by) {
            for (int bx = 0; bx < bw; ++bx) {
                if (srcOffset + 8 > srcSize) return;
                uint8_t pixels[4][4][4];
                decodeDXT1Block(src + srcOffset, pixels);
                writeDXTBlock(img, bx, by, pixels);
                srcOffset += 8;
            }
        }
    }

    void decodeDXT3Block(const uint8_t* block, uint8_t pixels[4][4][4]) {
        decodeDXT1Block(block + 8, pixels);
        // Apply explicit alpha from first 8 bytes (4-bit per pixel)
        for (int i = 0; i < 16; i++) {
            int nibble = (i & 1) ? (block[i / 2] >> 4) : (block[i / 2] & 0xF);
            pixels[i / 4][i % 4][3] = (nibble << 4) | nibble;
        }
    }

    void decodeDXT3(const QByteArray& data, QImage& img, int w, int h) {
        const uint8_t* src = reinterpret_cast<const uint8_t*>(data.constData());
        int srcSize = data.size();
        int bw = (w + 3) / 4, bh = (h + 3) / 4;
        int srcOffset = 0;
        for (int by = 0; by < bh; ++by) {
            for (int bx = 0; bx < bw; ++bx) {
                if (srcOffset + 16 > srcSize) return;
                uint8_t pixels[4][4][4];
                decodeDXT3Block(src + srcOffset, pixels);
                writeDXTBlock(img, bx, by, pixels);
                srcOffset += 16;
            }
        }
    }

    void decodeDXT5Block(const uint8_t* block, uint8_t pixels[4][4][4]) {
        // Interpolate alpha from 2 bytes
        uint8_t a0 = block[0], a1 = block[1];
        uint8_t alphas[8];
        alphas[0] = a0;
        alphas[1] = a1;
        if (a0 > a1) {
            for (int i = 1; i <= 6; i++)
                alphas[i + 1] = static_cast<uint8_t>(((7 - i) * a0 + i * a1 + 3) / 7);
        } else {
            for (int i = 1; i <= 4; i++)
                alphas[i + 1] = static_cast<uint8_t>(((5 - i) * a0 + i * a1 + 2) / 5);
            alphas[6] = 0;
            alphas[7] = 255;
        }

        // Decode color block
        decodeDXT1Block(block + 8, pixels);

        // Apply interpolated alpha from 6 bytes (48 bits, 3 bits per pixel)
        uint64_t aBits = 0;
        for (int i = 0; i < 6; i++)
            aBits |= static_cast<uint64_t>(block[2 + i]) << (i * 8);

        for (int i = 0; i < 16; i++)
            pixels[i / 4][i % 4][3] = alphas[(aBits >> (i * 3)) & 7];
    }

    void decodeDXT5(const QByteArray& data, QImage& img, int w, int h) {
        const uint8_t* src = reinterpret_cast<const uint8_t*>(data.constData());
        int srcSize = data.size();
        int bw = (w + 3) / 4, bh = (h + 3) / 4;
        int srcOffset = 0;
        for (int by = 0; by < bh; ++by) {
            for (int bx = 0; bx < bw; ++bx) {
                if (srcOffset + 16 > srcSize) return;
                uint8_t pixels[4][4][4];
                decodeDXT5Block(src + srcOffset, pixels);
                writeDXTBlock(img, bx, by, pixels);
                srcOffset += 16;
            }
        }
    }

    uint32_t m_padding = 0;
};

// ============================================================================
// PAA Info reader - reads metadata without full decode
// ============================================================================

struct PAAMetadata {
    PAAPixelFormat format = PAAPixelFormat::Unknown;
    QString formatName;
    int width = 0;
    int height = 0;
    int mipmapCount = 0;
    bool hasTransparentBlocks = false; // DXT1 1-bit alpha blocks
};

inline PAAMetadata readPAAMetadata(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    PAAMetadata meta;
    meta.format = PAAPixelFormat::Unknown;

    uint16_t magic = 0;
    file.read(reinterpret_cast<char*>(&magic), 2);

    auto fmt = magic;
    if (fmt == 0x8888) meta.format = PAAPixelFormat::ARGB8888;
    else if (fmt == 0x4444) meta.format = PAAPixelFormat::ARGB4444;
    else if (fmt == 0x1555) meta.format = PAAPixelFormat::ARGB1555;
    else if (fmt == 0x8080) meta.format = PAAPixelFormat::AI88;
    else if (fmt == 0xFF01) meta.format = PAAPixelFormat::DXT1;
    else if (fmt == 0xFF02) meta.format = PAAPixelFormat::DXT2;
    else if (fmt == 0xFF03) meta.format = PAAPixelFormat::DXT3;
    else if (fmt == 0xFF04) meta.format = PAAPixelFormat::DXT4;
    else if (fmt == 0xFF05) meta.format = PAAPixelFormat::DXT5;

    meta.formatName = PAATexture::formatToString(meta.format);

    if (meta.format == PAAPixelFormat::Unknown) {
        file.seek(0);
    }

    // Skip TAGG
    char tag[4];
    while (file.bytesAvailable() >= 12) {
        file.read(tag, 4);
        if (memcmp(tag, "TAGG", 4) != 0) {
            file.seek(file.pos() - 4);
            break;
        }
        file.read(reinterpret_cast<char*>(&magic), 4); // version
        uint32_t sz = 0;
        file.read(reinterpret_cast<char*>(&sz), 4);
        file.seek(file.pos() + sz);
    }

    // Palette
    if (meta.format == PAAPixelFormat::Unknown) {
        uint16_t palCount = 0;
        file.read(reinterpret_cast<char*>(&palCount), 2);
        file.seek(file.pos() + palCount * 3);
        meta.format = PAAPixelFormat::ARGB4444;
        meta.formatName = PAATexture::formatToString(meta.format);
    }

    // Mipmaps
    bool first = true;
    while (file.bytesAvailable() >= 8) {
        uint16_t w = 0, h = 0;
        file.read(reinterpret_cast<char*>(&w), 2);
        file.read(reinterpret_cast<char*>(&h), 2);
        if (w == 0 && h == 0) break;
        if (w == 1234 && h == 8765) {
            file.read(reinterpret_cast<char*>(&w), 2);
            file.read(reinterpret_cast<char*>(&h), 2);
        }
        if (first) { meta.width = w; meta.height = h; first = false; }
        meta.mipmapCount++;
        uint32_t ds = 0;
        file.read(reinterpret_cast<char*>(&ds), 3);
        ds &= 0xFFFFFF;

        // DXT1: scan for transparent blocks
        if (meta.mipmapCount == 1 && meta.format == PAAPixelFormat::DXT1 && ds >= 8) {
            auto pos = file.pos();
            QByteArray blockData = file.read(ds);
            for (int off = 0; off + 8 <= blockData.size(); off += 8) {
                uint16_t c0 = static_cast<uint8_t>(blockData[off]) | (static_cast<uint8_t>(blockData[off + 1]) << 8);
                uint16_t c1 = static_cast<uint8_t>(blockData[off + 2]) | (static_cast<uint8_t>(blockData[off + 3]) << 8);
                if (c0 <= c1) {
                    meta.hasTransparentBlocks = true;
                    break;
                }
            }
        } else {
            file.seek(file.pos() + ds);
        }
    }
    return meta;
}

} // namespace ks::fileformat
