#pragma once

#include <QImage>
#include <QColor>
#include <QString>
#include <QVector>

namespace ks {

class QRCodeWriter {
public:
    enum EccLevel { L = 0, M = 1, Q = 2, H = 3 };

    static QImage encode(const QString& text, int version = 0,
                         EccLevel eccLevel = M, int moduleSize = 4,
                         const QColor& fg = Qt::black,
                         const QColor& bg = Qt::white);

private:
    static int chooseVersion(const QString& text, EccLevel eccLevel);
    static int alphanumericValue(const QChar& c);
    static QVector<unsigned char> encodeAlphanumeric(const QString& text, int version, EccLevel eccLevel);
    static QVector<unsigned char> encodeByte(const QString& text, int version, EccLevel eccLevel);
    static int dataCodewords(int version, EccLevel eccLevel);
    static int totalCodewords(int version);
    static int eccCodewords(int version, EccLevel eccLevel);
    static int blocks(int version, EccLevel eccLevel);
    static int blockDataWords(int version, EccLevel eccLevel);
    static QVector<unsigned char> rsEncode(const QVector<unsigned char>& data, int eccCount);
    static int gfMul(int a, int b);
    static void initTables();

    static QVector<unsigned char> interleave(const QVector<unsigned char>& data,
                                              const QVector<unsigned char>& ecc,
                                              int version, EccLevel eccLevel);
    static void placeModules(QImage& image, const QVector<unsigned char>& codewords,
                             int version, EccLevel eccLevel);
    static void placeFinder(QImage& image, int row, int col, int moduleSize);
    static void placeAlignment(QImage& image, int row, int col, int moduleSize);
    static void placeTiming(QImage& image, int moduleSize, int version);
    static void placeFormat(QImage& image, int mask, int moduleSize, EccLevel eccLevel);
    static void placeVersion(QImage& image, int version, int moduleSize);
    static void placeData(QImage& image, const QVector<unsigned char>& codewords,
                          int version, int moduleSize, int mask);
    static int applyMask(int row, int col, int mask);
    static int calculatePenalty(const QImage& image, int moduleSize);
    static int bestMask(const QImage& image, int moduleSize, int version);

    static bool s_tablesInit;
    static int s_gfLog[256];
    static int s_gfExp[512];
};

} // namespace ks
