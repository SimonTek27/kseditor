#pragma once

#include <QString>
#include <QByteArray>
#include <QVector>
#include <QMap>

namespace ks {

struct ImageInfo {
    QString format;
    int width = 0;
    int height = 0;
    int bitsPerPixel = 0;
    int channels = 0;
    bool hasAlpha = false;
    float dpiX = 72.0f;
    float dpiY = 72.0f;
    qint64 fileSize = 0;
    QMap<QString, QString> metadata;
};

struct ImageMipLevel {
    int width;
    int height;
    int size;
    QByteArray data;
};

struct ImageFormatData {
    ImageInfo info;
    QByteArray rawData;
    QVector<ImageMipLevel> mipMaps;
    bool isValid = false;
};

class ImageFormatParser {
public:
    static bool readHeader(const QString& filePath, ImageInfo& outInfo);
    static bool load(const QString& filePath, ImageFormatData& outData);
    static QString detectFormat(const QByteArray& header);

    static bool readBMP(const QByteArray& data, ImageFormatData& outData);
    static bool readJPG(const QByteArray& data, ImageFormatData& outData);
    static bool readPNG(const QByteArray& data, ImageFormatData& outData);
    static bool readTGA(const QByteArray& data, ImageFormatData& outData);
    static bool readDDS(const QByteArray& data, ImageFormatData& outData);
    static bool readHDR(const QByteArray& data, ImageFormatData& outData);

    static QString lastError() { return s_lastError; }

private:
    static QString s_lastError;
};

} // namespace ks