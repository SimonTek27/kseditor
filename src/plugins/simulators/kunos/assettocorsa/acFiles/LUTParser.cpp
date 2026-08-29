#include "LUTParser.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QVector3D>
#include <QColor>
#include <QtMath>

namespace ks {
namespace plugins {
namespace kunos {
namespace assettocorsa {

bool LUTParser::loadCSP(const QString& path, LUTData& out) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QTextStream in(&file);
    out = LUTData();
    out.name = QFileInfo(path).baseName();

    QString line = in.readLine().trimmed();
    if (!line.startsWith("LUT3D")) {
        file.close();
        return false;
    }

    line = in.readLine().trimmed();
    out.size = line.toInt();
    if (out.size < 2 || out.size > 64) {
        file.close();
        return false;
    }

    int expected = out.size * out.size * out.size;
    out.table.reserve(expected);

    while (!in.atEnd()) {
        line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(';')) continue;
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 3) {
            QVector3D color;
            color.setX(parts[0].toFloat());
            color.setY(parts[1].toFloat());
            color.setZ(parts[2].toFloat());
            out.table.append(color);
        }
    }

    file.close();
    out.valid = (out.table.size() == expected);
    return out.valid;
}

bool LUTParser::loadAC(const QString& path, LUTData& out) {
    return loadImage(path, out);
}

bool LUTParser::saveCSP(const QString& path, const LUTData& data) {
    if (!data.valid || data.table.size() != data.size * data.size * data.size) return false;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    out << "LUT3D\n";
    out << data.size << "\n";

    for (const auto& c : data.table) {
        out << c.x() << " " << c.y() << " " << c.z() << "\n";
    }

    file.close();
    return true;
}

bool LUTParser::saveAC(const QString& path, const LUTData& data) {
    if (!data.valid) return false;
    QImage img = lutToImage(data);
    return img.save(path);
}

bool LUTParser::loadImage(const QString& path, LUTData& out) {
    QImage img(path);
    if (img.isNull()) return false;

    out = LUTData();
    out.name = QFileInfo(path).baseName();

    if (img.width() == img.height() * img.height()) {
        out.size = img.height();
    } else if (img.width() == img.height()) {
        int cubes = img.width() * img.height();
        int size = round(pow(cubes, 1.0 / 3.0));
        if (size * size * size == cubes) out.size = size;
        else return false;
    } else {
        return false;
    }

    int expected = out.size * out.size * out.size;
    out.table.reserve(expected);

    if (img.width() == img.height() * img.height()) {
        for (int y = 0; y < img.height(); ++y) {
            for (int x = 0; x < img.width(); ++x) {
                QColor c = img.pixelColor(x, y);
                out.table.append(QVector3D(c.redF(), c.greenF(), c.blueF()));
            }
        }
    } else {
        for (int y = 0; y < img.height(); ++y) {
            for (int x = 0; x < img.width(); ++x) {
                QColor c = img.pixelColor(x, y);
                out.table.append(QVector3D(c.redF(), c.greenF(), c.blueF()));
            }
        }
    }

    out.valid = (out.table.size() == expected);
    return out.valid;
}

bool LUTParser::saveImage(const QString& path, const LUTData& data) {
    if (!data.valid) return false;
    QImage img = lutToImage(data);
    return img.save(path);
}

bool LUTParser::validate(const LUTData& data, QString& error) {
    if (!data.valid) {
        error = "LUT not valid";
        return false;
    }
    int expected = data.size * data.size * data.size;
    if (data.table.size() != expected) {
        error = QString("Table size mismatch: got %1, expected %2").arg(data.table.size()).arg(expected);
        return false;
    }
    for (int i = 0; i < data.table.size(); ++i) {
        const auto& c = data.table[i];
        if (c.x() < 0 || c.x() > 1 || c.y() < 0 || c.y() > 1 || c.z() < 0 || c.z() > 1) {
            error = QString("Color out of range [0,1] at index %1").arg(i);
            return false;
        }
    }
    return true;
}

LUTData LUTParser::createIdentity(int size) {
    LUTData lut;
    lut.size = size;
    lut.name = "Identity";
    lut.valid = true;
    int total = size * size * size;
    lut.table.reserve(total);

    for (int b = 0; b < size; ++b) {
        for (int g = 0; g < size; ++g) {
            for (int r = 0; r < size; ++r) {
                float rf = (size == 1) ? 0.5f : float(r) / (size - 1);
                float gf = (size == 1) ? 0.5f : float(g) / (size - 1);
                float bf = (size == 1) ? 0.5f : float(b) / (size - 1);
                lut.table.append(QVector3D(rf, gf, bf));
            }
        }
    }
    return lut;
}

LUTData LUTParser::applyLUT(const LUTData& lut, const QVector3D& color) {
    LUTData out = lut;
    if (!lut.valid || lut.size < 2) return out;

    int s = lut.size;
    float fx = color.x() * (s - 1);
    float fy = color.y() * (s - 1);
    float fz = color.z() * (s - 1);

    int x0 = int(fx), x1 = qMin(x0 + 1, s - 1);
    int y0 = int(fy), y1 = qMin(y0 + 1, s - 1);
    int z0 = int(fz), z1 = qMin(z0 + 1, s - 1);

    float dx = fx - x0, dy = fy - y0, dz = fz - z0;

    auto idx = [s](int x, int y, int z) { return (z * s + y) * s + x; };

    QVector3D c000 = lut.table[idx(x0, y0, z0)];
    QVector3D c100 = lut.table[idx(x1, y0, z0)];
    QVector3D c010 = lut.table[idx(x0, y1, z0)];
    QVector3D c110 = lut.table[idx(x1, y1, z0)];
    QVector3D c001 = lut.table[idx(x0, y0, z1)];
    QVector3D c101 = lut.table[idx(x1, y0, z1)];
    QVector3D c011 = lut.table[idx(x0, y1, z1)];
    QVector3D c111 = lut.table[idx(x1, y1, z1)];

    auto lerp = [](const QVector3D& a, const QVector3D& b, float t) {
        return a + (b - a) * t;
    };

    QVector3D c00 = lerp(c000, c100, dx);
    QVector3D c10 = lerp(c010, c110, dx);
    QVector3D c01 = lerp(c001, c101, dx);
    QVector3D c11 = lerp(c011, c111, dx);

    QVector3D c0 = lerp(c00, c10, dy);
    QVector3D c1 = lerp(c01, c11, dy);

    QVector3D result = lerp(c0, c1, dz);

    out.table[0] = result;
    return out;
}

QImage LUTParser::lutToImage(const LUTData& lut, int stripHeight) {
    if (!lut.valid) return QImage();

    int s = lut.size;
    int w = s * s;
    int h = s * stripHeight;
    QImage img(w, h, QImage::Format_RGBA8888);

    for (int z = 0; z < s; ++z) {
        for (int y = 0; y < s; ++y) {
            for (int x = 0; x < s; ++x) {
                int idx = (z * s + y) * s + x;
                const QVector3D& c = lut.table[idx];
                int px = z * s + x;
                int py = y * stripHeight;
                QColor color = QColor::fromRgbF(c.x(), c.y(), c.z());
                for (int sy = 0; sy < stripHeight; ++sy) {
                    img.setPixelColor(px, py + sy, color);
                }
            }
        }
    }
    return img;
}

bool LUTParser::validate(const LUTData& lut) {
    return lut.valid && lut.table.size() == lut.size * lut.size * lut.size;
}

LUTData LUTParser::loadFromImage(const QString& path) {
    LUTData out;
    loadImage(path, out);
    return out;
}

LUTData LUTParser::loadFromStrip(const QString& path, int size) {
    LUTData out;
    loadImage(path, out);
    return out;
}

bool LUTParser::saveToStrip(const LUTData& lut, const QString& path, int stripHeight) {
    return saveImage(path, lut);
}

LUTData LUTParser::interpolate(const LUTData& a, const LUTData& b, float t) {
    if (!a.valid || !b.valid || a.size != b.size) return a;
    LUTData result = a;
    for (int i = 0; i < result.table.size(); ++i) {
        result.table[i] = a.table[i] * (1.0f - t) + b.table[i] * t;
    }
    return result;
}

} // namespace assettocorsa
} // namespace kunos
} // namespace plugins
} // namespace ks