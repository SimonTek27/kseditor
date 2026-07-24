#pragma once

#include <QString>
#include <QByteArray>
#include <QVector>

namespace ks {

struct FontTableRecord {
    QString tag;
    uint32_t checksum = 0;
    uint32_t offset = 0;
    uint32_t length = 0;
};

struct FontNameRecord {
    uint16_t platformID = 0;
    uint16_t encodingID = 0;
    uint16_t languageID = 0;
    uint16_t nameID = 0;
    QString name;
};

struct FontGlyph {
    uint32_t index = 0;
    uint32_t codePoint = 0;
    int16_t advanceWidth = 0;
    int16_t lsb = 0;
    int16_t xMin = 0, yMin = 0, xMax = 0, yMax = 0;
};

struct FontInfo {
    QString familyName;
    QString styleName;
    QString format; // "TTF", "OTF"
    QString postScriptName;
    QString version;
    QString copyright;
    QString license;

    uint16_t unitsPerEm = 1000;
    int16_t ascent = 0;
    int16_t descent = 0;
    int16_t lineGap = 0;
    uint16_t numGlyphs = 0;
    uint16_t upem = 1000;

    QVector<FontTableRecord> tables;
    QVector<FontNameRecord> nameRecords;
    QVector<FontGlyph> glyphs;

    bool isValid = false;
    float pointSize = 0;
    int glyphCount = 0;
};

class FontFormatParser {
public:
    static bool readHeader(const QString& filePath, FontInfo& outInfo);
    static bool loadTable(const QString& filePath, const QString& tableTag, QByteArray& outData);
    static QString lastError() { return s_lastError; }

    static QString detectFormat(const QByteArray& data);

private:
    static QString s_lastError;

    static bool parseNameTable(const QByteArray& data, FontInfo& info);
    static bool parseHeadTable(const QByteArray& data, FontInfo& info);
    static bool parseHheaTable(const QByteArray& data, FontInfo& info);
    static bool parseMaxpTable(const QByteArray& data, FontInfo& info);
};

} // namespace ks