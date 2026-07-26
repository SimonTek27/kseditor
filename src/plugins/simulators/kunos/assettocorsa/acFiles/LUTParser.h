#pragma once

#include <QString>
#include <QVector>
#include <QImage>
#include <QColor>

namespace ks { namespace plugins { namespace kunos { namespace assettocorsa {

struct LUTData {
    int size = 0;
    QVector<QVector3D> table;
    bool valid = false;
};

class LUTParser {
public:
    static LUTData loadFromImage(const QString& path);
    static LUTData loadFromStrip(const QString& path, int size);
    static bool saveToStrip(const LUTData& lut, const QString& path, int stripHeight = 1);
    static QImage lutToImage(const LUTData& lut, int stripHeight = 1);

    static bool validate(const LUTData& lut);
    static LUTData createIdentity(int size);
    static LUTData interpolate(const LUTData& a, const LUTData& b, float t);
};

}}}} // namespace ks::plugins::kunos::assettocorsa