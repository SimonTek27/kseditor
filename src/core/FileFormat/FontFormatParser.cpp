#include "FontFormatParser.h"
#include <QFile>
#include <QDataStream>

namespace ks {

QString FontFormatParser::s_lastError;

QString FontFormatParser::detectFormat(const QByteArray& data)
{
    if (data.size() < 4) return "unknown";
    // TrueType: 0x00010000 or 0x74727565 ("true")
    uint32_t val = *reinterpret_cast<const uint32_t*>(data.constData());
    if (val == 0x00010000 || val == 0x74727565) return "ttf";
    // OpenType: 0x4F54544F ("OTTO")
    if (val == 0x4F54544F) return "otf";
    // TrueType collection: 0x74746366 ("ttcf")
    if (val == 0x74746366) return "ttc";
    // WOFF: 0x774F4646 ("wOFF")
    if (val == 0x774F4646) return "woff";
    // WOFF2: 0x774F4632 ("wOF2")
    if (val == 0x774F4632) return "woff2";
    return "unknown";
}

bool FontFormatParser::readHeader(const QString& filePath, FontInfo& outInfo)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        s_lastError = "Cannot open file: " + filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.size() < 12) {
        s_lastError = "Font file too small";
        return false;
    }

    outInfo.format = detectFormat(data);
    if (outInfo.format == "unknown") {
        s_lastError = "Unknown font format";
        return false;
    }

    uint32_t sfVersion = *reinterpret_cast<const uint32_t*>(data.constData());

    if (outInfo.format == "woff") {
        // WOFF header
        uint16_t numTables = *reinterpret_cast<const uint16_t*>(data.constData() + 12);
        outInfo.glyphCount = numTables;
        outInfo.upem = 1000;
        outInfo.isValid = true;
        return true;
    }

    if (outInfo.format == "ttc") {
        // TTC header - first offset to first font
        uint32_t numFonts = *reinterpret_cast<const uint32_t*>(data.constData() + 8);
        if (numFonts > 0) {
            uint32_t fontOffset = *reinterpret_cast<const uint32_t*>(data.constData() + 12);
            // Treat as TTF from this offset
            QByteArray subData = data.mid(fontOffset);
            return readHeader(filePath, outInfo); // simplified
        }
    }

    // Read table directory for TTF/OTF
    uint16_t numTables = *reinterpret_cast<const uint16_t*>(data.constData() + 4);
    uint16_t searchRange = *reinterpret_cast<const uint16_t*>(data.constData() + 6);
    uint16_t entrySelector = *reinterpret_cast<const uint16_t*>(data.constData() + 8);
    uint16_t rangeShift = *reinterpret_cast<const uint16_t*>(data.constData() + 10);

    // Read table records
    outInfo.tables.clear();
    for (int i = 0; i < numTables && 12 + (i + 1) * 16 <= data.size(); ++i) {
        int pos = 12 + i * 16;
        FontTableRecord rec;
        rec.tag = QString::fromLatin1(data.mid(pos, 4));
        rec.checksum = *reinterpret_cast<const uint32_t*>(data.constData() + pos + 4);
        rec.offset = *reinterpret_cast<const uint32_t*>(data.constData() + pos + 8);
        rec.length = *reinterpret_cast<const uint32_t*>(data.constData() + pos + 12);
        outInfo.tables.append(rec);
    }

    // Parse key tables
    for (const auto& table : outInfo.tables) {
        if (table.tag == "name") {
            QByteArray nameData = data.mid(table.offset, table.length);
            parseNameTable(nameData, outInfo);
        } else if (table.tag == "head") {
            QByteArray headData = data.mid(table.offset, table.length);
            parseHeadTable(headData, outInfo);
        } else if (table.tag == "hhea") {
            QByteArray hheaData = data.mid(table.offset, table.length);
            parseHheaTable(hheaData, outInfo);
        } else if (table.tag == "maxp") {
            QByteArray maxpData = data.mid(table.offset, table.length);
            parseMaxpTable(maxpData, outInfo);
        }
    }

    outInfo.isValid = true;
    return true;
}

bool FontFormatParser::loadTable(const QString& filePath, const QString& tableTag, QByteArray& outData)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        s_lastError = "Cannot open file: " + filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QString fmt = detectFormat(data);
    if (fmt == "unknown" || (fmt != "ttf" && fmt != "otf")) {
        s_lastError = "Font format does not support table extraction";
        return false;
    }

    uint16_t numTables = *reinterpret_cast<const uint16_t*>(data.constData() + 4);
    for (int i = 0; i < numTables; ++i) {
        int pos = 12 + i * 16;
        QString tag = QString::fromLatin1(data.mid(pos, 4));
        if (tag == tableTag) {
            uint32_t offset = *reinterpret_cast<const uint32_t*>(data.constData() + pos + 8);
            uint32_t length = *reinterpret_cast<const uint32_t*>(data.constData() + pos + 12);
            outData = data.mid(offset, length);
            return true;
        }
    }

    s_lastError = "Table " + tableTag + " not found";
    return false;
}

bool FontFormatParser::parseNameTable(const QByteArray& data, FontInfo& info)
{
    if (data.size() < 6) return false;

    uint16_t format = *reinterpret_cast<const uint16_t*>(data.constData());
    uint16_t count = *reinterpret_cast<const uint16_t*>(data.constData() + 2);
    uint16_t stringOffset = *reinterpret_cast<const uint16_t*>(data.constData() + 4);

    for (int i = 0; i < count && 6 + (i + 1) * 12 <= data.size(); ++i) {
        int pos = 6 + i * 12;
        FontNameRecord rec;
        rec.platformID = *reinterpret_cast<const uint16_t*>(data.constData() + pos);
        rec.encodingID = *reinterpret_cast<const uint16_t*>(data.constData() + pos + 2);
        rec.languageID = *reinterpret_cast<const uint16_t*>(data.constData() + pos + 4);
        rec.nameID = *reinterpret_cast<const uint16_t*>(data.constData() + pos + 6);
        uint16_t length = *reinterpret_cast<const uint16_t*>(data.constData() + pos + 8);
        uint16_t offset = *reinterpret_cast<const uint16_t*>(data.constData() + pos + 10);

        if (stringOffset + offset + length <= data.size()) {
            QByteArray nameData = data.mid(stringOffset + offset, length);
            if (rec.platformID == 3 || rec.platformID == 0) {
                // Windows/Mac - UTF-16BE
                rec.name = QString::fromUtf16(reinterpret_cast<const char16_t*>(nameData.constData()), length / 2);
            } else if (rec.platformID == 1) {
                // Macintosh - ASCII/Mac Roman
                rec.name = QString::fromLatin1(nameData);
            } else {
                rec.name = QString::fromLatin1(nameData);
            }

            if (rec.nameID == 1 && info.familyName.isEmpty())
                info.familyName = rec.name;
            else if (rec.nameID == 2 && info.styleName.isEmpty())
                info.styleName = rec.name;
            else if (rec.nameID == 6 && info.postScriptName.isEmpty())
                info.postScriptName = rec.name;
            else if (rec.nameID == 5 && info.version.isEmpty())
                info.version = rec.name;
            else if (rec.nameID == 0 && info.copyright.isEmpty())
                info.copyright = rec.name;
            else if (rec.nameID == 13 && info.license.isEmpty())
                info.license = rec.name;

            info.nameRecords.append(rec);
        }
    }

    return !info.familyName.isEmpty();
}

bool FontFormatParser::parseHeadTable(const QByteArray& data, FontInfo& info)
{
    if (data.size() < 54) return false;

    info.upem = *reinterpret_cast<const uint16_t*>(data.constData() + 18);
    info.unitsPerEm = info.upem;

    int16_t xMin = *reinterpret_cast<const int16_t*>(data.constData() + 36);
    int16_t yMin = *reinterpret_cast<const int16_t*>(data.constData() + 38);
    int16_t xMax = *reinterpret_cast<const int16_t*>(data.constData() + 40);
    int16_t yMax = *reinterpret_cast<const int16_t*>(data.constData() + 42);

    return true;
}

bool FontFormatParser::parseHheaTable(const QByteArray& data, FontInfo& info)
{
    if (data.size() < 36) return false;

    info.ascent = *reinterpret_cast<const int16_t*>(data.constData() + 4);
    info.descent = *reinterpret_cast<const int16_t*>(data.constData() + 6);
    info.lineGap = *reinterpret_cast<const int16_t*>(data.constData() + 8);
    info.glyphCount = *reinterpret_cast<const uint16_t*>(data.constData() + 34);

    return true;
}

bool FontFormatParser::parseMaxpTable(const QByteArray& data, FontInfo& info)
{
    if (data.size() < 6) return false;

    info.numGlyphs = *reinterpret_cast<const uint16_t*>(data.constData() + 4);
    info.glyphCount = info.numGlyphs;

    return true;
}

} // namespace ks