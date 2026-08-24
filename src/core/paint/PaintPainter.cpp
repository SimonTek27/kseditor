#include "PaintPainter.h"
#include <QPainter>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QVariantMap>
#include <QVector>
#include <algorithm>
#include <cmath>
#include <random>

namespace ks {
namespace paint {

namespace {

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

inline QRgb blendPixel(QRgb base, QRgb top, float alpha)
{
    float a = clampf(alpha, 0.0f, 1.0f);
    int br = qRed(base), bg = qGreen(base), bb = qBlue(base);
    int tr = qRed(top), tg = qGreen(top), tb = qBlue(top);
    return qRgba(int(br + (tr - br) * a),
                 int(bg + (tg - bg) * a),
                 int(bb + (tb - bb) * a),
                 qAlpha(base));
}

inline QRgb dodgePixel(QRgb base, float strength)
{
    int r = qRed(base), g = qGreen(base), b = qBlue(base);
    auto dodge = [](int c, float s) {
        return int(255.0f * std::pow(c / 255.0f, 1.0f - clampf(s, 0.0f, 1.0f)));
    };
    return qRgba(dodge(r, strength), dodge(g, strength), dodge(b, strength), qAlpha(base));
}

inline QRgb burnPixel(QRgb base, float strength)
{
    int r = qRed(base), g = qGreen(base), b = qBlue(base);
    auto burn = [](int c, float s) {
        return int(255.0f * (1.0f - std::pow(1.0f - c / 255.0f, 1.0f + clampf(s, 0.0f, 1.0f))));
    };
    return qRgba(burn(r, strength), burn(g, strength), burn(b, strength), qAlpha(base));
}

inline QVector<QPoint> bresenham(const QPoint& from, const QPoint& to)
{
    QVector<QPoint> pts;
    int x0 = from.x(), y0 = from.y();
    int x1 = to.x(), y1 = to.y();
    int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    while (true) {
        pts.append(QPoint(x0, y0));
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
    return pts;
}

inline float maskAt(const QImage& mask, int x, int y)
{
    if (mask.isNull()) return 1.0f;
    if (x < 0 || y < 0 || x >= mask.width() || y >= mask.height()) return 0.0f;
    return qAlpha(mask.pixel(x, y)) / 255.0f;
}

} // namespace

QPoint PaintPainter::clampTo(const QPoint& p, const QSize& size)
{
    return QPoint(qBound(0, p.x(), size.width() - 1),
                  qBound(0, p.y(), size.height() - 1));
}

bool PaintPainter::inside(const QPoint& p, const QSize& size)
{
    return p.x() >= 0 && p.y() >= 0 && p.x() < size.width() && p.y() < size.height();
}

void PaintPainter::paintAt(QImage& image, const QImage& mask, const QPoint& pos, const PaintBrush& brush)
{
    if (image.isNull()) return;
    float p = clampf(brush.pressure, 0.05f, 1.0f);
    float effRadius = brush.radius * (0.3f + 0.7f * p);
    float effOpacity = brush.opacity * p;
    const int r = int(effRadius);
    if (r <= 0) return;
    const int innerR = int(effRadius * brush.hardness);
    const bool eraser = (brush.tool == PaintTool::Eraser);
    const bool airbrush = (brush.tool == PaintTool::Airbrush);

    // Spray mode: scatter multiple dabs randomly within the radius
    if (brush.strokeType == StrokeType::Spray) {
        static thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        const int count = qMax(1, brush.sprayDensity);
        const float scatter = effRadius * brush.sprayScatter;
        for (int i = 0; i < count; ++i) {
            float ox = dist(rng) * scatter;
            float oy = dist(rng) * scatter;
            QPoint samplePos(pos.x() + int(ox), pos.y() + int(oy));
            paintSingleDab(image, mask, samplePos, brush, effRadius, effOpacity, innerR, eraser, airbrush, p);
        }
        return;
    }

    paintSingleDab(image, mask, pos, brush, effRadius, effOpacity, innerR, eraser, airbrush, p);
}

void PaintPainter::paintSingleDab(QImage& image, const QImage& mask, const QPoint& pos, const PaintBrush& brush,
                                   float effRadius, float effOpacity, int innerR, bool eraser, bool airbrush, float pressure)
{
    const int r = int(effRadius);
    const int x0 = pos.x() - r, x1 = pos.x() + r;
    const int y0 = pos.y() - r, y1 = pos.y() + r;

    QImage img = image.convertToFormat(QImage::Format_ARGB32);

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            if (x < 0 || y < 0 || x >= img.width() || y >= img.height()) continue;
            float dist = std::sqrt(float((x - pos.x()) * (x - pos.x()) + (y - pos.y()) * (y - pos.y())));
            if (dist > r) continue;

            float falloff = 1.0f;
            if (innerR > 0 && dist > innerR) {
                falloff = 1.0f - (dist - innerR) / float(r - innerR);
            }
            if (!brush.stampTexture.isNull()) {
                float u = float(x - pos.x() + r) / float(2*r);
                float v = float(y - pos.y() + r) / float(2*r);
                int sx = qBound(0, int(u * brush.stampTexture.width()), brush.stampTexture.width()-1);
                int sy = qBound(0, int(v * brush.stampTexture.height()), brush.stampTexture.height()-1);
                float stampA = qAlpha(brush.stampTexture.pixel(sx, sy)) / 255.0f;
                if (brush.stampTexture.format() == QImage::Format_Grayscale8) stampA = qRed(brush.stampTexture.pixel(sx, sy))/255.0f;
                falloff *= stampA;
            }
            float alpha = effOpacity * falloff * maskAt(mask, x, y);
            if (airbrush) alpha *= brush.flow * pressure;
            if (alpha <= 0.0f) continue;

            QRgb base = img.pixel(x, y);
            if (eraser) {
                int a = int(qAlpha(base) * (1.0f - alpha));
                img.setPixel(x, y, qRgba(qRed(base), qGreen(base), qBlue(base), a));
            } else {
                img.setPixel(x, y, blendPixel(base, brush.color.rgb(), alpha));
            }
        }
    }
    image = img;
}

void PaintPainter::paintRect(QImage& image, const QImage& mask, const QPoint& from, const QPoint& to, const PaintBrush& brush, bool filled)
{
    if (image.isNull()) return;
    float p = clampf(brush.pressure, 0.05f, 1.0f);
    float effOpacity = brush.opacity * p;
    const bool eraser = (brush.tool == PaintTool::Eraser);

    QImage img = image.convertToFormat(QImage::Format_ARGB32);
    int x0 = qMin(from.x(), to.x()), x1 = qMax(from.x(), to.x());
    int y0 = qMin(from.y(), to.y()), y1 = qMax(from.y(), to.y());
    int thickness = qMax(1, int(brush.radius * 0.2f));

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            if (x < 0 || y < 0 || x >= img.width() || y >= img.height()) continue;

            bool onBorder = filled ? (x <= x0 + thickness || x >= x1 - thickness || y <= y0 + thickness || y >= y1 - thickness) : true;
            if (!onBorder) continue;

            float alpha = effOpacity * maskAt(mask, x, y);
            if (alpha <= 0.0f) continue;

            QRgb base = img.pixel(x, y);
            if (eraser) {
                int a = int(qAlpha(base) * (1.0f - alpha));
                img.setPixel(x, y, qRgba(qRed(base), qGreen(base), qBlue(base), a));
            } else {
                img.setPixel(x, y, blendPixel(base, brush.color.rgb(), alpha));
            }
        }
    }
    image = img;
}

void PaintPainter::paintLine(QImage& image, const QImage& mask, const QPoint& from, const QPoint& to, const PaintBrush& brush)
{
    const QVector<QPoint> pts = bresenham(from, to);
    for (const QPoint& p : pts) {
        paintAt(image, mask, p, brush);
    }
}

void PaintPainter::blurAt(QImage& image, const QImage& mask, const QPoint& pos, float radius, float strength)
{
    if (image.isNull()) return;
    const int r = qMax(1, int(radius));
    QImage img = image.convertToFormat(QImage::Format_ARGB32);
    QImage src = img.copy();

    for (int y = pos.y() - r; y <= pos.y() + r; ++y) {
        for (int x = pos.x() - r; x <= pos.x() + r; ++x) {
            if (x < 0 || y < 0 || x >= img.width() || y >= img.height()) continue;
            float dist = std::sqrt(float((x - pos.x()) * (x - pos.x()) + (y - pos.y()) * (y - pos.y())));
            if (dist > r) continue;
            float falloff = 1.0f - dist / float(r);
            float alpha = strength * falloff * maskAt(mask, x, y);
            if (alpha <= 0.0f) continue;

            long sr = 0, sg = 0, sb = 0;
            int cnt = 0;
            for (int yy = y - r; yy <= y + r; ++yy) {
                for (int xx = x - r; xx <= x + r; ++xx) {
                    if (xx < 0 || yy < 0 || xx >= src.width() || yy >= src.height()) continue;
                    QRgb p = src.pixel(xx, yy);
                    sr += qRed(p); sg += qGreen(p); sb += qBlue(p); ++cnt;
                }
            }
            if (cnt == 0) continue;
            QRgb base = img.pixel(x, y);
            QRgb avg = qRgba(int(sr / cnt), int(sg / cnt), int(sb / cnt), qAlpha(base));
            img.setPixel(x, y, blendPixel(base, avg, alpha));
        }
    }
    image = img;
}

void PaintPainter::sharpenAt(QImage& image, const QImage& mask, const QPoint& pos, float radius, float strength)
{
    if (image.isNull()) return;
    const int r = qMax(1, int(radius));
    QImage img = image.convertToFormat(QImage::Format_ARGB32);
    QImage src = img.copy();

    const float kernel[3][3] = {
        { 0.0f, -1.0f,  0.0f },
        { -1.0f,  5.0f, -1.0f },
        { 0.0f, -1.0f,  0.0f }
    };

    for (int y = pos.y() - r; y <= pos.y() + r; ++y) {
        for (int x = pos.x() - r; x <= pos.x() + r; ++x) {
            if (x < 1 || y < 1 || x >= img.width() - 1 || y >= img.height() - 1) continue;
            float dist = std::sqrt(float((x - pos.x()) * (x - pos.x()) + (y - pos.y()) * (y - pos.y())));
            if (dist > r) continue;
            float alpha = strength * (1.0f - dist / float(r)) * maskAt(mask, x, y);
            if (alpha <= 0.0f) continue;

            float sr = 0, sg = 0, sb = 0;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    QRgb p = src.pixel(x + kx, y + ky);
                    float w = kernel[ky + 1][kx + 1];
                    sr += qRed(p) * w; sg += qGreen(p) * w; sb += qBlue(p) * w;
                }
            }
            QRgb base = img.pixel(x, y);
            QRgb sharp = qRgba(qBound(0, int(sr), 255), qBound(0, int(sg), 255), qBound(0, int(sb), 255), qAlpha(base));
            img.setPixel(x, y, blendPixel(base, sharp, alpha));
        }
    }
    image = img;
}

void PaintPainter::dodgeAt(QImage& image, const QImage& mask, const QPoint& pos, float radius, float strength)
{
    if (image.isNull()) return;
    const int r = qMax(1, int(radius));
    QImage img = image.convertToFormat(QImage::Format_ARGB32);
    for (int y = pos.y() - r; y <= pos.y() + r; ++y) {
        for (int x = pos.x() - r; x <= pos.x() + r; ++x) {
            if (x < 0 || y < 0 || x >= img.width() || y >= img.height()) continue;
            float dist = std::sqrt(float((x - pos.x()) * (x - pos.x()) + (y - pos.y()) * (y - pos.y())));
            if (dist > r) continue;
            float falloff = 1.0f - dist / float(r);
            float alpha = strength * falloff * maskAt(mask, x, y);
            if (alpha <= 0.0f) continue;
            img.setPixel(x, y, dodgePixel(img.pixel(x, y), alpha));
        }
    }
    image = img;
}

void PaintPainter::burnAt(QImage& image, const QImage& mask, const QPoint& pos, float radius, float strength)
{
    if (image.isNull()) return;
    const int r = qMax(1, int(radius));
    QImage img = image.convertToFormat(QImage::Format_ARGB32);
    for (int y = pos.y() - r; y <= pos.y() + r; ++y) {
        for (int x = pos.x() - r; x <= pos.x() + r; ++x) {
            if (x < 0 || y < 0 || x >= img.width() || y >= img.height()) continue;
            float dist = std::sqrt(float((x - pos.x()) * (x - pos.x()) + (y - pos.y()) * (y - pos.y())));
            if (dist > r) continue;
            float falloff = 1.0f - dist / float(r);
            float alpha = strength * falloff * maskAt(mask, x, y);
            if (alpha <= 0.0f) continue;
            img.setPixel(x, y, burnPixel(img.pixel(x, y), alpha));
        }
    }
    image = img;
}

void PaintPainter::smudgeAt(QImage& image, const QImage& mask, const QPoint& from, const QPoint& to, float radius, float strength)
{
    if (image.isNull()) return;
    const int r = qMax(1, int(radius * 0.5f));
    QImage img = image.convertToFormat(QImage::Format_ARGB32);

    for (int y = to.y() - r; y <= to.y() + r; ++y) {
        for (int x = to.x() - r; x <= to.x() + r; ++x) {
            if (x < 0 || y < 0 || x >= img.width() || y >= img.height()) continue;
            float dist = std::sqrt(float((x - to.x()) * (x - to.x()) + (y - to.y()) * (y - to.y())));
            if (dist > r) continue;
            float falloff = 1.0f - dist / float(r);
            float alpha = strength * falloff * maskAt(mask, x, y);

            int sx = from.x() + (x - to.x());
            int sy = from.y() + (y - to.y());
            if (!inside(QPoint(sx, sy), img.size())) continue;
            QRgb srcColor = img.pixel(sx, sy);
            img.setPixel(x, y, blendPixel(img.pixel(x, y), srcColor, alpha));
        }
    }
    image = img;
}

void PaintPainter::cloneAt(QImage& image, const QImage& mask, const QPoint& pos, const QPoint& source, float radius, float opacity)
{
    if (image.isNull()) return;
    const int r = qMax(1, int(radius));
    QImage img = image.convertToFormat(QImage::Format_ARGB32);

    for (int y = pos.y() - r; y <= pos.y() + r; ++y) {
        for (int x = pos.x() - r; x <= pos.x() + r; ++x) {
            if (x < 0 || y < 0 || x >= img.width() || y >= img.height()) continue;
            float dist = std::sqrt(float((x - pos.x()) * (x - pos.x()) + (y - pos.y()) * (y - pos.y())));
            if (dist > r) continue;
            float falloff = 1.0f - dist / float(r);
            float a = opacity * falloff * maskAt(mask, x, y);
            if (a <= 0.0f) continue;

            int sx = source.x() + (x - pos.x());
            int sy = source.y() + (y - pos.y());
            if (!inside(QPoint(sx, sy), img.size())) continue;
            img.setPixel(x, y, blendPixel(img.pixel(x, y), img.pixel(sx, sy), a));
        }
    }
    image = img;
}

void PaintPainter::healAt(QImage& image, const QImage& mask, const QPoint& pos, const QPoint& source, float radius, float opacity)
{
    if (image.isNull()) return;
    const int r = qMax(1, int(radius));
    QImage img = image.convertToFormat(QImage::Format_ARGB32);
    QImage srcImg = img.copy();

    for (int y = pos.y() - r; y <= pos.y() + r; ++y) {
        for (int x = pos.x() - r; x <= pos.x() + r; ++x) {
            if (x < 0 || y < 0 || x >= img.width() || y >= img.height()) continue;
            float dist = std::sqrt(float((x - pos.x()) * (x - pos.x()) + (y - pos.y()) * (y - pos.y())));
            if (dist > r) continue;
            float falloff = 1.0f - dist / float(r);
            float alpha = opacity * falloff * maskAt(mask, x, y);
            if (alpha <= 0.0f) continue;

            int sx = source.x() + (x - pos.x());
            int sy = source.y() + (y - pos.y());
            if (!inside(QPoint(sx, sy), srcImg.size())) continue;
            img.setPixel(x, y, blendPixel(img.pixel(x, y), srcImg.pixel(sx, sy), alpha));
        }
    }
    image = img;
}

void PaintPainter::fill(QImage& image, const QImage& mask, const QPoint& start, const QColor& color, float tolerance)
{
    if (image.isNull() || !inside(start, image.size())) return;
    QImage img = image.convertToFormat(QImage::Format_ARGB32);
    const QRgb target = img.pixel(start);
    const QRgb fillRgb = color.rgb();
    if (target == fillRgb) return;

    const int tol = int(tolerance * 255.0f);
    QVector<QPoint> stack;
    stack.append(start);
    QImage visited(img.size(), QImage::Format_Indexed8);
    visited.fill(0);

    while (!stack.isEmpty()) {
        QPoint p = stack.takeLast();
        if (!inside(p, img.size())) continue;
        if (visited.pixel(p.x(), p.y())) continue;
        visited.setPixel(p.x(), p.y(), 1);

        QRgb cur = img.pixel(p.x(), p.y());
        if (qAbs(qRed(cur) - qRed(target)) > tol ||
            qAbs(qGreen(cur) - qGreen(target)) > tol ||
            qAbs(qBlue(cur) - qBlue(target)) > tol) continue;

        float a = maskAt(mask, p.x(), p.y());
        if (a <= 0.0f) continue;
        img.setPixel(p.x(), p.y(), blendPixel(cur, fillRgb, a));

        stack.append(QPoint(p.x() - 1, p.y()));
        stack.append(QPoint(p.x() + 1, p.y()));
        stack.append(QPoint(p.x(), p.y() - 1));
        stack.append(QPoint(p.x(), p.y() + 1));
    }
    image = img;
}

void PaintPainter::gradient(QImage& image, const QImage& mask,
                            const QPoint& from, const QPoint& to,
                            const QColor& startColor, const QColor& endColor, bool radial)
{
    if (image.isNull()) return;
    QImage img = image.convertToFormat(QImage::Format_ARGB32);

    QImage gradImg = img.copy();
    gradImg.fill(Qt::transparent);
    {
        QPainter p(&gradImg);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        if (radial) {
            float radius = QLineF(from, to).length();
            QRadialGradient rg(from, qMax(radius, 1.0f));
            rg.setColorAt(0.0, startColor);
            rg.setColorAt(1.0, endColor);
            p.fillRect(img.rect(), rg);
        } else {
            QLinearGradient lg(from, to);
            lg.setColorAt(0.0, startColor);
            lg.setColorAt(1.0, endColor);
            p.fillRect(img.rect(), lg);
        }
        p.end();
    }

    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            float a = maskAt(mask, x, y);
            if (a <= 0.0f) continue;
            img.setPixel(x, y, blendPixel(img.pixel(x, y), gradImg.pixel(x, y), a));
        }
    }
    image = img;
}

QImage PaintPainter::applyFilter(const QImage& src, const QString& filter, const QVariantMap& params)
{
    if (src.isNull()) return QImage();
    QImage img = src.convertToFormat(QImage::Format_ARGB32);

    if (filter == QStringLiteral("invert")) {
        img.invertPixels();
        return img;
    }
    if (filter == QStringLiteral("grayscale")) {
        return img.convertToFormat(QImage::Format_Grayscale8).convertToFormat(QImage::Format_ARGB32);
    }
    if (filter == QStringLiteral("blur")) {
        int radius = params.value(QStringLiteral("radius"), 2).toInt();
        QImage out = img.copy();
        blurAt(out, QImage(), QPoint(out.width() / 2, out.height() / 2), radius, 1.0f);
        return out;
    }
    if (filter == QStringLiteral("sharpen")) {
        int amount = params.value(QStringLiteral("amount"), 50).toInt();
        QImage out = img.copy();
        sharpenAt(out, QImage(), QPoint(out.width() / 2, out.height() / 2), 1, amount / 100.0f);
        return out;
    }
    if (filter == QStringLiteral("sepia")) {
        QImage out = img.copy();
        for (int y = 0; y < out.height(); ++y) {
            QRgb* line = reinterpret_cast<QRgb*>(out.scanLine(y));
            for (int x = 0; x < out.width(); ++x) {
                QRgb p = line[x];
                int r = qRed(p), g = qGreen(p), b = qBlue(p);
                int nr = qBound(0, int(r * 0.393f + g * 0.769f + b * 0.189f), 255);
                int ng = qBound(0, int(r * 0.349f + g * 0.686f + b * 0.168f), 255);
                int nb = qBound(0, int(r * 0.272f + g * 0.534f + b * 0.131f), 255);
                line[x] = qRgba(nr, ng, nb, qAlpha(p));
            }
        }
        return out;
    }
    if (filter == QStringLiteral("brightness")) {
        int delta = params.value(QStringLiteral("delta"), 0).toInt();
        QImage out = img.copy();
        for (int y = 0; y < out.height(); ++y) {
            QRgb* line = reinterpret_cast<QRgb*>(out.scanLine(y));
            for (int x = 0; x < out.width(); ++x) {
                QRgb p = line[x];
                line[x] = qRgba(qBound(0, qRed(p) + delta, 255),
                                qBound(0, qGreen(p) + delta, 255),
                                qBound(0, qBlue(p) + delta, 255),
                                qAlpha(p));
            }
        }
        return out;
    }
    if (filter == QStringLiteral("contrast")) {
        int delta = params.value(QStringLiteral("delta"), 0).toInt();
        float f = (259.0f * (delta + 255.0f)) / (255.0f * (259.0f - delta));
        QImage out = img.copy();
        for (int y = 0; y < out.height(); ++y) {
            QRgb* line = reinterpret_cast<QRgb*>(out.scanLine(y));
            for (int x = 0; x < out.width(); ++x) {
                QRgb p = line[x];
                auto adjust = [f](int c) { return qBound(0, int(f * (c - 128) + 128), 255); };
                line[x] = qRgba(adjust(qRed(p)), adjust(qGreen(p)), adjust(qBlue(p)), qAlpha(p));
            }
        }
        return out;
    }
    if (filter == QStringLiteral("levels")) {
        int inLow = params.value("inLow", 0).toInt(), inHigh = params.value("inHigh", 255).toInt();
        int outLow = params.value("outLow", 0).toInt(), outHigh = params.value("outHigh", 255).toInt();
        float gamma = params.value("gamma", 1.0).toFloat();
        QImage out = img.copy();
        for (int y=0;y<out.height();++y){ QRgb* l=reinterpret_cast<QRgb*>(out.scanLine(y)); for(int x=0;x<out.width();++x){ auto lvl=[&](int c){ float n=(c-inLow)/float(qMax(1,inHigh-inLow)); n=qBound(0.0f,n,1.0f); n=pow(n,1.0f/qMax(0.1f,gamma)); return qBound(0,int(outLow+n*(outHigh-outLow)),255); }; QRgb p=l[x]; l[x]=qRgba(lvl(qRed(p)),lvl(qGreen(p)),lvl(qBlue(p)),qAlpha(p)); } } return out;
    }
    if (filter == QStringLiteral("curves")) {
        QVariantList curve = params.value("curve").toList();
        QVector<int> lut(256); for(int i=0;i<256;++i) lut[i]=i;
        if(curve.size()>=4){ for(int i=0;i<256;++i){ float t=i/255.0f; float best=0; for(int k=0;k+3<curve.size();k+=2){ float x0=curve[k].toFloat(), y0=curve[k+1].toFloat(), x1=curve[k+2].toFloat(), y1=curve[k+3].toFloat(); if(t>=x0 && t<=x1){ float nt=(t-x0)/qMax(0.001f,x1-x0); best=y0+nt*(y1-y0); break; } } lut[i]=qBound(0,int(best*255),255); } }
        QImage out = img.copy(); for(int y=0;y<out.height();++y){ QRgb* l=reinterpret_cast<QRgb*>(out.scanLine(y)); for(int x=0;x<out.width();++x){ QRgb p=l[x]; l[x]=qRgba(lut[qRed(p)],lut[qGreen(p)],lut[qBlue(p)],qAlpha(p)); } } return out;
    }
    if (filter == QStringLiteral("pathTraceAOV")) {
        return img;
    }
    return img;
}

} // namespace paint
} // namespace ks