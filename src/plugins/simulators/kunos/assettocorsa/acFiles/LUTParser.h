#pragma once

#include <QString>
#include <QVector>
#include <QImage>
#include <QColor>
#include <QVector3D>

namespace ks { namespace plugins { namespace kunos { namespace assettocorsa {

struct LUTData {
    QString name;
    int size = 0;
    QVector<QVector3D> table;
    bool valid = false;
};

class LUTParser {
public:
    static bool loadCSP(const QString& path, LUTData& out);
    static bool loadAC(const QString& path, LUTData& out);
    static bool saveCSP(const QString& path, const LUTData& data);
    static bool saveAC(const QString& path, const LUTData& data);
    static bool loadImage(const QString& path, LUTData& out);
    static bool saveImage(const QString& path, const LUTData& data);
    static bool validate(const LUTData& lut);
    static bool validate(const LUTData& lut, QString& error);
    static LUTData createIdentity(int size);
    static LUTData applyLUT(const LUTData& lut, const QVector3D& color);
    static QImage lutToImage(const LUTData& lut, int stripHeight = 1);

    static LUTData loadFromImage(const QString& path);
    static LUTData loadFromStrip(const QString& path, int size);
    static bool saveToStrip(const LUTData& lut, const QString& path, int stripHeight = 1);
    static LUTData interpolate(const LUTData& a, const LUTData& b, float t);
};

}}}} // namespace ks::plugins::kunos::assettocorsa
