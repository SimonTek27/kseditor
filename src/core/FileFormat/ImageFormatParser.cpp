#include "ImageFormatParser.h"
#include <QFile>
#include <QFileInfo>

namespace ks {

QString ImageFormatParser::s_lastError;

QString ImageFormatParser::detectFormat(const QByteArray& header)
{
    if (header.size() < 4) return "unknown";
    if (header.left(2) == "BM" && header.size() >= 14) return "bmp";
    if (header.left(3) == "\xFF\xD8\xFF") return "jpg";
    if (header.left(8) == "\x89PNG\r\n\x1A\n") return "png";
    if (header.left(3) == "DDS") return "dds";
    if (header.left(2) == "\x01\x00" || header.left(2) == "\x00\x01") return "tga";
    if (header.left(10) == "#?RADIANCE" || header.left(10) == "#?RGBE") return "hdr";
    return "unknown";
}

bool ImageFormatParser::readHeader(const QString& filePath, ImageInfo& outInfo)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        s_lastError = "Cannot open file: " + filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    outInfo.fileSize = data.size();
    QString fmt = detectFormat(data);
    outInfo.format = fmt;

    ImageFormatData imgData;
    bool result = false;

    if (fmt == "bmp") result = readBMP(data, imgData);
    else if (fmt == "jpg") result = readJPG(data, imgData);
    else if (fmt == "png") result = readPNG(data, imgData);
    else if (fmt == "tga") result = readTGA(data, imgData);
    else if (fmt == "dds") result = readDDS(data, imgData);
    else if (fmt == "hdr") result = readHDR(data, imgData);
    else {
        s_lastError = "Unsupported format: " + fmt;
        return false;
    }

    if (result) {
        outInfo = imgData.info;
    }

    return result;
}

bool ImageFormatParser::load(const QString& filePath, ImageFormatData& outData)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        s_lastError = "Cannot open file: " + filePath;
        return false;
    }

    outData.rawData = file.readAll();
    file.close();

    QString fmt = detectFormat(outData.rawData);
    outData.info.format = fmt;
    outData.info.fileSize = outData.rawData.size();

    bool result = false;
    if (fmt == "bmp") result = readBMP(outData.rawData, outData);
    else if (fmt == "jpg") result = readJPG(outData.rawData, outData);
    else if (fmt == "png") result = readPNG(outData.rawData, outData);
    else if (fmt == "tga") result = readTGA(outData.rawData, outData);
    else if (fmt == "dds") result = readDDS(outData.rawData, outData);
    else if (fmt == "hdr") result = readHDR(outData.rawData, outData);
    else s_lastError = "Unsupported format: " + fmt;

    return result;
}

bool ImageFormatParser::readBMP(const QByteArray& data, ImageFormatData& outData)
{
    if (data.size() < 54) { s_lastError = "BMP file too small"; return false; }

    // BMP header
    uint32_t dataOffset = *reinterpret_cast<const uint32_t*>(data.constData() + 10);
    uint32_t headerSize = *reinterpret_cast<const uint32_t*>(data.constData() + 14);

    outData.info.width = *reinterpret_cast<const int32_t*>(data.constData() + 18);
    outData.info.height = *reinterpret_cast<const int32_t*>(data.constData() + 22);
    uint16_t planes = *reinterpret_cast<const uint16_t*>(data.constData() + 26);
    uint16_t bpp = *reinterpret_cast<const uint16_t*>(data.constData() + 28);

    outData.info.bitsPerPixel = bpp;
    outData.info.channels = bpp / 8;
    outData.info.hasAlpha = (bpp == 32);

    // DPI from BMP: meters -> inches
    if (headerSize >= 40) {
        int32_t hRes = *reinterpret_cast<const int32_t*>(data.constData() + 38);
        int32_t vRes = *reinterpret_cast<const int32_t*>(data.constData() + 42);
        if (hRes > 0) outData.info.dpiX = hRes * 0.0254f;
        if (vRes > 0) outData.info.dpiY = vRes * 0.0254f;
    }

    // Compression
    uint32_t compression = *reinterpret_cast<const uint32_t*>(data.constData() + 30);
    outData.info.metadata["compression"] = QString::number(compression);
    outData.info.metadata["planes"] = QString::number(planes);

    outData.isValid = true;
    return true;
}

bool ImageFormatParser::readJPG(const QByteArray& data, ImageFormatData& outData)
{
    int pos = 2;
    while (pos + 4 <= data.size()) {
        if (data[pos] != 0xFF) { s_lastError = "Invalid JPEG marker"; return false; }
        uint8_t marker = (uint8_t)data[pos + 1];

        if (marker == 0xD9) break; // EOI
        if (marker == 0xDA) { // SOS - skip to EOI
            while (pos + 1 < data.size()) {
                if ((uint8_t)data[pos] == 0xFF && (uint8_t)data[pos + 1] == 0xD9) break;
                pos++;
            }
            break;
        }

        if (marker == 0xC0 || marker == 0xC1 || marker == 0xC2) {
            // SOF
            if (pos + 9 <= data.size()) {
                uint16_t length = ((uint8_t)data[pos + 2] << 8) | (uint8_t)data[pos + 3];
                outData.info.bitsPerPixel = (uint8_t)data[pos + 4];
                outData.info.height = ((uint8_t)data[pos + 5] << 8) | (uint8_t)data[pos + 6];
                outData.info.width = ((uint8_t)data[pos + 7] << 8) | (uint8_t)data[pos + 8];
                outData.info.channels = (uint8_t)data[pos + 9];
                outData.info.hasAlpha = false;
                outData.isValid = true;
                return true;
            }
        }

        if (marker == 0xE1 || marker == 0xE0 || marker == 0xED) {
            // APP1/APP0/APP13 for EXIF/JPEG comment
            uint16_t length = ((uint8_t)data[pos + 2] << 8) | (uint8_t)data[pos + 3];
            if (marker == 0xE1 && length >= 14) {
                QByteArray exifData = data.mid(pos + 4, length - 2);
                QString exifStr = QString::fromLatin1(exifData);
                outData.info.metadata["exif"] = exifStr.left(256);
            }
            pos += 2 + length;
        } else {
            uint16_t length = ((uint8_t)data[pos + 2] << 8) | (uint8_t)data[pos + 3];
            pos += 2 + length;
        }
    }

    s_lastError = "JPEG SOF marker not found";
    return false;
}

bool ImageFormatParser::readPNG(const QByteArray& data, ImageFormatData& outData)
{
    if (data.size() < 33) { s_lastError = "PNG file too small"; return false; }

    // IHDR chunk at offset 16
    if (data.mid(12, 4) != "IHDR") { s_lastError = "PNG missing IHDR"; return false; }

    outData.info.width = *reinterpret_cast<const int32_t*>(data.constData() + 16);
    outData.info.height = *reinterpret_cast<const int32_t*>(data.constData() + 20);
    outData.info.bitsPerPixel = (uint8_t)data[24];
    uint8_t colorType = (uint8_t)data[25];

    switch (colorType) {
    case 0: outData.info.channels = 1; outData.info.hasAlpha = false; break; // Grayscale
    case 2: outData.info.channels = 3; outData.info.hasAlpha = false; break; // RGB
    case 3: outData.info.channels = 1; outData.info.hasAlpha = false; break; // Indexed
    case 4: outData.info.channels = 2; outData.info.hasAlpha = true; break;  // Grayscale+Alpha
    case 6: outData.info.channels = 4; outData.info.hasAlpha = true; break;  // RGBA
    default: outData.info.channels = 3; break;
    }

    outData.info.metadata["colorType"] = QString::number(colorType);
    outData.info.metadata["compression"] = QString::number((uint8_t)data[26]);
    outData.info.metadata["filter"] = QString::number((uint8_t)data[27]);
    outData.info.metadata["interlace"] = QString::number((uint8_t)data[28]);

    // Read physical DPI
    int pos = 33;
    while (pos + 8 <= data.size()) {
        uint32_t chunkLen = *reinterpret_cast<const uint32_t*>(data.constData() + pos);
        QString chunkType = data.mid(pos + 4, 4);
        if (chunkType == "pHYs" && chunkLen >= 9) {
            uint32_t ppuX = *reinterpret_cast<const uint32_t*>(data.constData() + pos + 8);
            uint32_t ppuY = *reinterpret_cast<const uint32_t*>(data.constData() + pos + 12);
            uint8_t unit = (uint8_t)data[pos + 16];
            if (unit == 1) { // meters
                outData.info.dpiX = ppuX * 0.0254f;
                outData.info.dpiY = ppuY * 0.0254f;
            }
            break;
        }
        if (chunkType == "IEND") break;
        pos += 12 + chunkLen;
    }

    // Read text chunks
    pos = 33;
    while (pos + 8 <= data.size()) {
        uint32_t chunkLen = *reinterpret_cast<const uint32_t*>(data.constData() + pos);
        QString chunkType = data.mid(pos + 4, 4);
        if (chunkType == "tEXt") {
            QByteArray textData = data.mid(pos + 8, chunkLen);
            int nullPos = textData.indexOf('\0');
            if (nullPos >= 0) {
                QString key = QString::fromLatin1(textData.left(nullPos));
                QString val = QString::fromLatin1(textData.mid(nullPos + 1));
                outData.info.metadata[key] = val;
            }
        }
        if (chunkType == "IEND") break;
        pos += 12 + chunkLen;
    }

    outData.isValid = true;
    return true;
}

bool ImageFormatParser::readTGA(const QByteArray& data, ImageFormatData& outData)
{
    if (data.size() < 18) { s_lastError = "TGA file too small"; return false; }

    uint8_t idLength = (uint8_t)data[0];
    uint8_t colorMapType = (uint8_t)data[1];
    uint8_t imageType = (uint8_t)data[2];

    uint16_t xOrigin = *reinterpret_cast<const uint16_t*>(data.constData() + 8);
    uint16_t yOrigin = *reinterpret_cast<const uint16_t*>(data.constData() + 10);
    outData.info.width = *reinterpret_cast<const uint16_t*>(data.constData() + 12);
    outData.info.height = *reinterpret_cast<const uint16_t*>(data.constData() + 14);
    outData.info.bitsPerPixel = (uint8_t)data[16];
    uint8_t descriptor = (uint8_t)data[17];

    outData.info.channels = outData.info.bitsPerPixel / 8;
    outData.info.hasAlpha = (descriptor & 0x0F) == 8;

    outData.info.metadata["imageType"] = QString::number(imageType);
    outData.info.metadata["colorMapType"] = QString::number(colorMapType);
    outData.info.metadata["descriptor"] = QString::number(descriptor);

    outData.isValid = true;
    return true;
}

bool ImageFormatParser::readDDS(const QByteArray& data, ImageFormatData& outData)
{
    if (data.size() < 128) { s_lastError = "DDS file too small"; return false; }

    if (data.left(4) != "DDS ") { s_lastError = "Invalid DDS magic"; return false; }

    uint32_t height = *reinterpret_cast<const uint32_t*>(data.constData() + 12);
    uint32_t width = *reinterpret_cast<const uint32_t*>(data.constData() + 16);
    uint32_t pitch = *reinterpret_cast<const uint32_t*>(data.constData() + 20);
    uint32_t flags = *reinterpret_cast<const uint32_t*>(data.constData() + 8);
    uint32_t mipMapCount = *reinterpret_cast<const uint32_t*>(data.constData() + 28);

    outData.info.width = width;
    outData.info.height = height;
    outData.info.bitsPerPixel = 0;
    outData.info.channels = 0;

    // FourCC
    QByteArray fourCC = data.mid(84, 4);
    outData.info.metadata["fourCC"] = QString::fromLatin1(fourCC);
    outData.info.metadata["flags"] = QString::number(flags);
    outData.info.metadata["mipMapCount"] = QString::number(mipMapCount);

    // Determine format from fourCC
    if (fourCC == "DXT1") { outData.info.bitsPerPixel = 4; outData.info.channels = 3; outData.info.hasAlpha = false; }
    else if (fourCC == "DXT3") { outData.info.bitsPerPixel = 8; outData.info.channels = 4; outData.info.hasAlpha = true; }
    else if (fourCC == "DXT5") { outData.info.bitsPerPixel = 8; outData.info.channels = 4; outData.info.hasAlpha = true; }
    else if (fourCC == "DX10") { outData.info.bitsPerPixel = 0; outData.info.channels = 0; }
    else { outData.info.bitsPerPixel = 32; outData.info.channels = 4; }

    // Extract mip levels
    int mipWidth = width, mipHeight = height;
    uint32_t dataOffset = 128; // DDS header is 128 bytes
    for (uint32_t i = 0; i < qMax(1u, mipMapCount); ++i) {
        int mipSize = qMax(1, mipWidth / 4) * qMax(1, mipHeight / 4);
        if (fourCC == "DXT1") mipSize = qMax(1, mipWidth / 4) * qMax(1, mipHeight / 4) * 8;
        else if (fourCC == "DXT3" || fourCC == "DXT5") mipSize = qMax(1, mipWidth / 4) * qMax(1, mipHeight / 4) * 16;

        ImageMipLevel mip;
        mip.width = mipWidth;
        mip.height = mipHeight;
        mip.size = mipSize;
        if (dataOffset + mipSize <= data.size()) {
            mip.data = data.mid(dataOffset, mipSize);
        }
        outData.mipMaps.append(mip);

        dataOffset += mipSize;
        mipWidth = qMax(1, mipWidth / 2);
        mipHeight = qMax(1, mipHeight / 2);
    }

    outData.isValid = true;
    return true;
}

bool ImageFormatParser::readHDR(const QByteArray& data, ImageFormatData& outData)
{
    if (data.size() < 20) { s_lastError = "HDR file too small"; return false; }

    QString header = QString::fromLatin1(data.left(256));

    if (!header.startsWith("#?RADIANCE") && !header.startsWith("#?RGBE")) {
        s_lastError = "Invalid HDR header";
        return false;
    }

    // Parse resolution string: "-Y height +X width"
    QRegularExpression resRx(R"(\-Y\s+(\d+)\s+\+X\s+(\d+))");
    auto match = resRx.match(header);
    if (match.hasMatch()) {
        outData.info.height = match.captured(1).toInt();
        outData.info.width = match.captured(2).toInt();
    }

    outData.info.bitsPerPixel = 32; // RGBE: 4 bytes per pixel
    outData.info.channels = 3;
    outData.info.hasAlpha = false;
    outData.info.metadata["format"] = "RGBE";

    outData.isValid = true;
    return true;
}

} // namespace ks