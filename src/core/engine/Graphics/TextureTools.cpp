#include "TextureTools.h"
#include "ShaderMaterial.h"
#include <QPainter>
#include <QFile>
#include <QDataStream>
#include <cmath>
#include <algorithm>

namespace ks {

static QByteArray s_lastCompressedData;

QImage KsTextureTools::convertNormalMapForAC(const QImage& source, const KsNormalMapSettings& settings) {
    if (source.isNull()) return QImage();

    QImage result = source.convertToFormat(QImage::Format_RGBA8888);
    float strength = settings.strength;

    for (int y = 0; y < result.height(); ++y) {
        for (int x = 0; x < result.width(); ++x) {
            QRgb pixel = result.pixel(x, y);
            float r = qRed(pixel) / 255.0f;
            float g = qGreen(pixel) / 255.0f;

            r = (r - 0.5f) * strength + 0.5f;
            g = (g - 0.5f) * strength + 0.5f;

            if (settings.invertRed) r = 1.0f - r;
            if (settings.invertGreen) g = 1.0f - g;
            if (settings.swapGreen) {
                float tmp = r;
                r = g;
                g = tmp;
            }

            int ri = qBound(0, static_cast<int>(r * 255.0f), 255);
            int gi = qBound(0, static_cast<int>(g * 255.0f), 255);
            float bf = std::sqrt(std::max<float>(0.0f, 1.0f - static_cast<float>(std::pow((r - 0.5f) * 2, 2)) - static_cast<float>(std::pow((g - 0.5f) * 2, 2))));
            int bi = static_cast<int>(bf * 255.0f);

} // namespace ks

    }

    return result;
}

QImage KsTextureTools::generateNormalMapFromHeightmap(const QImage& heightmap, float strength) {
    if (heightmap.isNull()) return QImage();

    QImage src = heightmap.convertToFormat(QImage::Format_Grayscale8);
    QImage result(src.width(), src.height(), QImage::Format_RGBA8888);

    auto getHeight = [&](int x, int y) -> float {
        x = qBound(0, x, src.width() - 1);
        y = qBound(0, y, src.height() - 1);
        return qGray(src.pixel(x, y)) / 255.0f;
    };

    for (int y = 0; y < src.height(); ++y) {
        for (int x = 0; x < src.width(); ++x) {
            float hL = getHeight(x - 1, y);
            float hR = getHeight(x + 1, y);
            float hU = getHeight(x, y - 1);
            float hD = getHeight(x, y + 1);

            float dx = (hR - hL) * strength;
            float dy = (hD - hU) * strength;

            float r = dx * 0.5f + 0.5f;
            float g = dy * 0.5f + 0.5f;
            float b = std::sqrt(std::max<float>(0.0f, 1.0f - static_cast<float>(std::pow((r - 0.5f) * 2, 2)) - static_cast<float>(std::pow((g - 0.5f) * 2, 2))));

            result.setPixel(x, y, qRgba(
                qBound(0, static_cast<int>(r * 255), 255),
                qBound(0, static_cast<int>((1.0f - g) * 255), 255),
                qBound(0, static_cast<int>(b * 255), 255),
                255
            ));
        }
    }

    return result;
}

QImage KsTextureTools::packSpecularMap(const QImage& specularRGB, const QImage& glossiness) {
    if (specularRGB.isNull()) return QImage();

    QImage result = specularRGB.convertToFormat(QImage::Format_RGBA8888);

    if (!glossiness.isNull()) {
        QImage gImg = glossiness.convertToFormat(QImage::Format_Grayscale8);
        for (int y = 0; y < result.height(); ++y) {
            for (int x = 0; x < result.width(); ++x) {
                QRgb src = result.pixel(x, y);
                int gVal = qGray(gImg.pixel(
                    x * gImg.width() / result.width(),
                    y * gImg.height() / result.height()
                ));
                result.setPixel(x, y, qRgba(qRed(src), qGreen(src), qBlue(src), gVal));
            }
        }
    }

    return result;
}

QImage KsTextureTools::extractGlossinessFromSpecular(const QImage& specularMap) {
    if (specularMap.isNull()) return QImage();

    QImage src = specularMap.convertToFormat(QImage::Format_RGBA8888);
    QImage result(src.width(), src.height(), QImage::Format_Grayscale8);

    for (int y = 0; y < src.height(); ++y) {
        for (int x = 0; x < src.width(); ++x) {
            int alpha = qAlpha(src.pixel(x, y));
            result.setPixel(x, y, qRgb(alpha, alpha, alpha));
        }
    }

    return result;
}

QImage KsTextureTools::generateSpecularFromMaterial(float specularValue, float glossiness, int size) {
    QImage result(size, size, QImage::Format_RGBA8888);
    int spec = qBound(0, static_cast<int>(specularValue * 255), 255);
    int gloss = qBound(0, static_cast<int>(glossiness * 255), 255);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            result.setPixel(x, y, qRgba(spec, spec, spec, gloss));
        }
    }

    return result;
}

static void encodeDXT1Block(const QImage& src, int bx, int by, uchar* out) {
    struct Color { int r, g, b; };
    Color colors[16];
    int idx = 0;
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            int px = qMin(bx + x, src.width() - 1);
            int py = qMin(by + y, src.height() - 1);
            QRgb p = src.pixel(px, py);
            colors[idx++] = {static_cast<int>(qRed(p)), static_cast<int>(qGreen(p)), static_cast<int>(qBlue(p))};
        }
    }

    // Find bounding box endpoints using min/max per channel
    int minR = 255, maxR = 0, minG = 255, maxG = 0, minB = 255, maxB = 0;
    for (int i = 0; i < 16; ++i) {
        if (colors[i].r < minR) minR = colors[i].r;
        if (colors[i].r > maxR) maxR = colors[i].r;
        if (colors[i].g < minG) minG = colors[i].g;
        if (colors[i].g > maxG) maxG = colors[i].g;
        if (colors[i].b < minB) minB = colors[i].b;
        if (colors[i].b > maxB) maxB = colors[i].b;
    }

    // Pick endpoints from the bounding box diagonal (better quality than random pixels)
    Color c0 = {maxR, maxG, maxB};
    Color c1 = {minR, minG, minB};

    // If the block is a single color, use a simpler encoding
    if (c0.r == c1.r && c0.g == c1.g && c0.b == c1.b) {
        quint16 col = static_cast<quint16>(((c0.r >> 3) << 11) | ((c0.g >> 2) << 5) | (c0.b >> 3));
        out[0] = col & 0xFF;
        out[1] = (col >> 8) & 0xFF;
        out[2] = col & 0xFF;
        out[3] = (col >> 8) & 0xFF;
        // Set all indices to 0
        out[4] = out[5] = out[6] = out[7] = 0;
        return;
    }

    // Single-color blocks: encode with col0=col1, all indices=0
    if (c0.r == c1.r && c0.g == c1.g && c0.b == c1.b) {
        quint16 col = ((c0.r >> 3) << 11) | ((c0.g >> 2) << 5) | (c0.b >> 3);
        out[0] = col & 0xFF;
        out[1] = (col >> 8) & 0xFF;
        out[2] = col & 0xFF;
        out[3] = (col >> 8) & 0xFF;
        out[4] = out[5] = out[6] = out[7] = 0;
        return;
    }

    quint16 col0 = static_cast<quint16>(((c0.r >> 3) << 11) | ((c0.g >> 2) << 5) | (c0.b >> 3));
    quint16 col1 = static_cast<quint16>(((c1.r >> 3) << 11) | ((c1.g >> 2) << 5) | (c1.b >> 3));
    out[0] = col0 & 0xFF;
    out[1] = (col0 >> 8) & 0xFF;
    out[2] = col1 & 0xFF;
    out[3] = (col1 >> 8) & 0xFF;

    // Interpolated endpoints for better quality
    int er0 = c0.r, eg0 = c0.g, eb0 = c0.b;
    int er1 = c1.r, eg1 = c1.g, eb1 = c1.b;

    quint32 indices = 0;
    for (int i = 0; i < 16; ++i) {
        // Luminance-weighted distance for better perceptual quality
        int dr0 = colors[i].r - er0;
        int dg0 = colors[i].g - eg0;
        int db0 = colors[i].b - eb0;
        int dr1 = colors[i].r - er1;
        int dg1 = colors[i].g - eg1;
        int db1 = colors[i].b - eb1;

        float d0 = dr0 * dr0 * 0.299f + dg0 * dg0 * 0.587f + db0 * db0 * 0.114f;
        float d1 = dr1 * dr1 * 0.299f + dg1 * dg1 * 0.587f + db1 * db1 * 0.114f;
        indices |= static_cast<quint32>((d0 <= d1 ? 0u : 1u)) << (i * 2);
    }
    out[4] = indices & 0xFF;
    out[5] = (indices >> 8) & 0xFF;
    out[6] = (indices >> 16) & 0xFF;
    out[7] = (indices >> 24) & 0xFF;
}

static void encodeDXT5Block(const QImage& src, int bx, int by, uchar* out) {
    uchar alpha[16];
    int idx = 0;
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            int px = qMin(bx + x, src.width() - 1);
            int py = qMin(by + y, src.height() - 1);
            alpha[idx++] = static_cast<uchar>(qAlpha(src.pixel(px, py)));
        }
    }

    uchar a0 = 255, a1 = 0;
    for (int i = 0; i < 16; ++i) {
        if (alpha[i] < a0) a0 = alpha[i];
        if (alpha[i] > a1) a1 = alpha[i];
    }

    out[0] = a0;
    out[1] = a1;

    // DXT5 uses 8-code mode (a0 > a1) or 6-code mode (a0 <= a1)
    bool eightCodeMode = (a0 > a1);
    int numCodes = eightCodeMode ? 8 : 6;

    quint64 alphaIdx = 0;
    for (int i = 0; i < 16; ++i) {
        int v = alpha[i];
        int bit;
        if (eightCodeMode) {
            bit = (v - a1) * (numCodes - 1) / (a0 - a1 > 0 ? a0 - a1 : 1);
        } else {
            // 6-code mode: codes 0-4 are interpolated, code 5 = 0, code 6 = 255
            int aMin = qMin(a0, a1);
            int aMax = qMax(a0, a1);
            if (v <= aMin)
                bit = 0;
            else if (v >= aMax)
                bit = 1;
            else
                bit = 1 + (v - aMin) * 4 / (aMax - aMin > 0 ? aMax - aMin : 1);
        }
        bit = qBound(0, bit, numCodes - 1);
        alphaIdx |= static_cast<quint64>(bit) << (i * 3);
    }
    for (int i = 0; i < 6; ++i) {
        out[2 + i] = static_cast<uchar>((alphaIdx >> (i * 8)) & 0xFF);
    }

    encodeDXT1Block(src, bx, by, out + 8);
}

QImage KsTextureTools::compressToDXT(const QImage& source, KsTextureFormat format, bool generateMipmaps)
{
    if (source.isNull()) return source;

    QImage src = source.convertToFormat(QImage::Format_ARGB32);
    int w = src.width();
    int h = src.height();

    int blockW = (w + 3) / 4;
    int blockH = (h + 3) / 4;
    int blockSize = (format == KsTextureFormat::DXT1) ? 8 : 16;

    QByteArray compressed(blockW * blockH * blockSize, 0);

    for (int by = 0; by < blockH; ++by) {
        for (int bx = 0; bx < blockW; ++bx) {
            uchar* blockData = reinterpret_cast<uchar*>(compressed.data()) + (by * blockW + bx) * blockSize;

            if (format == KsTextureFormat::DXT1) {
                encodeDXT1Block(src, bx, by, blockData);
            } else if (format == KsTextureFormat::DXT5) {
                encodeDXT5Block(src, bx, by, blockData);
            }
        }
    }

    // Store compressed data for downstream use (saveAsDDS, etc.)
    s_lastCompressedData = compressed;

    // QImage cannot represent DXT compressed data, so return a copy of the source
    // with mip-level thumbnails if requested. Callers needing the compressed blob
    // should use lastCompressedData() or saveAsDDS().
    QImage result = source;
    return result;
}

QByteArray KsTextureTools::lastCompressedData() {
    return s_lastCompressedData;
}

bool KsTextureTools::saveAsDDS(const QImage& image, const QString& path, KsTextureFormat format) {
    if (image.isNull()) return false;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream.writeRawData("DDS ", 4);

    quint32 headerSize = 124;
    quint32 flags = 0x1 | 0x2 | 0x4 | 0x1000 | 0x80000;
    quint32 height = static_cast<quint32>(image.height());
    quint32 width = static_cast<quint32>(image.width());
    quint32 pitchOrLinearSize = 0;
    quint32 depth = 0;
    quint32 mipMapCount = 1;

    stream << headerSize << flags << height << width << pitchOrLinearSize << depth << mipMapCount;

    for (int i = 0; i < 11; ++i) stream << quint32(0);

    quint32 fourCC = 0;
    switch (format) {
        case KsTextureFormat::DXT1: fourCC = 0x31545844; break;
        case KsTextureFormat::DXT5: fourCC = 0x35545844; break;
        case KsTextureFormat::DXT5_RG: fourCC = 0x35545844; break;
        default: fourCC = 0; break;
    }

    quint32 rgbBitCount = (format == KsTextureFormat::Uncompressed) ? 32 : 0;
    quint32 rBitMask = (format == KsTextureFormat::Uncompressed) ? 0x00FF0000 : 0;
    quint32 gBitMask = (format == KsTextureFormat::Uncompressed) ? 0x0000FF00 : 0;
    quint32 bBitMask = (format == KsTextureFormat::Uncompressed) ? 0x000000FF : 0;
    quint32 aBitMask = (format == KsTextureFormat::Uncompressed) ? 0xFF000000 : 0;

    stream << fourCC << rgbBitCount << rBitMask << gBitMask << bBitMask << aBitMask;
    stream << quint32(0x100000);

    for (int i = 0; i < 5; ++i) stream << quint32(0);

    QImage src = image.convertToFormat(QImage::Format_RGBA8888);
    int bw = (width + 3) / 4;
    int bh = (height + 3) / 4;
    int blockSize = (format == KsTextureFormat::DXT1) ? 8 : 16;

    QByteArray imageData(bw * bh * blockSize, 0);

    if (format == KsTextureFormat::DXT1) {
        for (int by = 0; by < bh; ++by) {
            for (int bx = 0; bx < bw; ++bx) {
                encodeDXT1Block(src, bx * 4, by * 4, reinterpret_cast<uchar*>(imageData.data()) + (by * bw + bx) * blockSize);
            }
        }
    } else {
        for (int by = 0; by < bh; ++by) {
            for (int bx = 0; bx < bw; ++bx) {
                encodeDXT5Block(src, bx * 4, by * 4, reinterpret_cast<uchar*>(imageData.data()) + (by * bw + bx) * blockSize);
            }
        }
    }

    stream.writeRawData(imageData.constData(), imageData.size());
    return true;
}

QImage KsTextureTools::generateLiveryTexture(const CarLiverySettings& settings) {
    int size = settings.textureResolution;
    QImage livery(size, size, QImage::Format_RGBA8888);
    livery.fill(settings.baseColor);

    QPainter painter(&livery);

    if (settings.stripeCount > 0) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(settings.stripeColor);

        float stripeWidthPx = settings.stripeWidth * size;
        float spacing = size / (settings.stripeCount + 1);

        for (int i = 1; i <= settings.stripeCount; ++i) {
            float x = spacing * i - stripeWidthPx / 2;
            painter.drawRect(QRectF(x, 0, stripeWidthPx, size));
        }
    }

    if (!settings.logoPath.isEmpty()) {
        QImage logo(settings.logoPath);
        if (!logo.isNull()) {
            int logoW = static_cast<int>(logo.width() * settings.logoScale);
            int logoH = static_cast<int>(logo.height() * settings.logoScale);
            painter.drawImage(
                QRectF(settings.logoPosition.x(), settings.logoPosition.y(), logoW, logoH),
                logo
            );
        }
    }

    if (!settings.numberText.isEmpty()) {
        QFont font("Arial", 72, QFont::Bold);
        painter.setFont(font);
        painter.setPen(Qt::white);
        painter.drawText(livery.rect(), Qt::AlignCenter, settings.numberText);
    }

    return livery;
}

QImage KsTextureTools::addDecalToLivery(const QImage& livery, const QImage& decal,
                                         const QPointF& uvPosition, float scale) {
    if (livery.isNull() || decal.isNull()) return livery;

    QImage result = livery.copy();
    QPainter painter(&result);

    int decalW = static_cast<int>(decal.width() * scale);
    int decalH = static_cast<int>(decal.height() * scale);
    int x = static_cast<int>(uvPosition.x() * livery.width());
    int y = static_cast<int>(uvPosition.y() * livery.height());

    painter.drawImage(QRect(x, y, decalW, decalH), decal);
    return result;
}

QImage KsTextureTools::optimizeForAC(const QImage& source, KsShaderType shaderType) {
    if (source.isNull()) return QImage();

    QImage result = source.convertToFormat(QImage::Format_RGBA8888);

    int w = result.width();
    int h = result.height();
    int potW = 1, potH = 1;
    while (potW < w) potW *= 2;
    while (potH < h) potH *= 2;

    if (potW != w || potH != h) {
        result = result.scaled(potW, potH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    switch (shaderType) {
        case KsShaderType::ksPerPixelNM:
        case KsShaderType::ksWindscreen:
            break;
        case KsShaderType::ksLight:
        case KsShaderType::ksEmissive:
            for (int y = 0; y < result.height(); ++y) {
                for (int x = 0; x < result.width(); ++x) {
                    QRgb p = result.pixel(x, y);
                    int lum = (qRed(p) + qGreen(p) + qBlue(p)) / 3;
                    result.setPixel(x, y, qRgba(lum, lum, lum, qAlpha(p)));
                }
            }
            break;
        default:
            break;
    }

    return result;
}

void KsTextureTools::validateTexturePaths(const KsMaterial& material, std::vector<QString>& missingTextures) {
    auto checkPath = [&](const QString& path) {
        if (!path.isEmpty() && !QFile::exists(path)) {
            missingTextures.push_back(path);
        }
    };

    checkPath(material.txDiffuse);
    checkPath(material.txNormal);
    checkPath(material.txSpecular);
    checkPath(material.txEmissive);
    checkPath(material.txDetail);
    checkPath(material.txMask);
    checkPath(material.txAlpha);
}

QImage KsTextureTools::swapChannelsForAC(const QImage& image, int rSrc, int gSrc, int bSrc, int aSrc) {
    if (image.isNull()) return QImage();

    QImage src = image.convertToFormat(QImage::Format_RGBA8888);
    QImage result(src.width(), src.height(), QImage::Format_RGBA8888);

    auto getChannel = [&](QRgb pixel, int channel) -> int {
        switch (channel) {
            case 0: return qRed(pixel);
            case 1: return qGreen(pixel);
            case 2: return qBlue(pixel);
            case 3: return qAlpha(pixel);
            default: return 0;
        }
    };

    for (int y = 0; y < src.height(); ++y) {
        for (int x = 0; x < src.width(); ++x) {
            QRgb p = src.pixel(x, y);
            result.setPixel(x, y, qRgba(
                getChannel(p, rSrc),
                getChannel(p, gSrc),
                getChannel(p, bSrc),
                getChannel(p, aSrc)
            ));
        }
    }

    return result;
}

QImage KsTextureTools::extractRGChannels(const QImage& image) {
    if (image.isNull()) return QImage();

    QImage src = image.convertToFormat(QImage::Format_RGBA8888);
    QImage result(src.width(), src.height(), QImage::Format_RGBA8888);

    for (int y = 0; y < src.height(); ++y) {
        for (int x = 0; x < src.width(); ++x) {
            QRgb p = src.pixel(x, y);
            result.setPixel(x, y, qRgba(qRed(p), qGreen(p), 128, 255));
        }
    }

    return result;
}

QImage KsTextureTools::combineRGBA(const QImage& r, const QImage& g, const QImage& b, const QImage& a) {
    if (r.isNull()) return QImage();

    int w = r.width();
    int h = r.height();
    QImage result(w, h, QImage::Format_RGBA8888);

    auto getPixel = [&](const QImage& img, int x, int y) -> int {
        if (img.isNull() || x < 0 || x >= img.width() || y < 0 || y >= img.height()) return 0;
        return qRed(img.pixel(x, y));
    };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            result.setPixel(x, y, qRgba(
                getPixel(r, x, y),
                getPixel(g, x, y),
                getPixel(b, x, y),
                getPixel(a, x, y)
            ));
        }
    }

    return result;
}

std::vector<QImage> KsTextureTools::generateACMipmaps(const QImage& source,
                                                       KsShaderType shaderType,
                                                       int maxLevels) {
    std::vector<QImage> mipmaps;
    if (source.isNull()) return mipmaps;

    mipmaps.push_back(source);

    if (maxLevels == 0) {
        maxLevels = static_cast<int>(std::log2(std::max(source.width(), source.height()))) + 1;
    }

    for (int i = 1; i < maxLevels; ++i) {
        int w = std::max(1, source.width() / (1 << i));
        int h = std::max(1, source.height() / (1 << i));

        QImage next;
        if (shaderType == KsShaderType::ksPerPixelNM) {
            // Scale from the original base image and apply normal conversion once
            next = source.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        } else {
            next = source.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }

        mipmaps.push_back(next);

        if (w == 1 && h == 1) break;
    }

    return mipmaps;
}

QImage KsTextureTools::previewTextureOnModel(const QImage& texture, const QImage& modelUVLayout) {
    if (texture.isNull() || modelUVLayout.isNull()) return QImage();

    QImage result = modelUVLayout.convertToFormat(QImage::Format_RGBA8888);
    QImage tex = texture.convertToFormat(QImage::Format_RGBA8888);

    for (int y = 0; y < result.height(); ++y) {
        for (int x = 0; x < result.width(); ++x) {
            QRgb uvPixel = modelUVLayout.pixel(x, y);
            float u = qRed(uvPixel) / 255.0f;
            float v = qGreen(uvPixel) / 255.0f;

            int tx = static_cast<int>(u * (tex.width() - 1));
            int ty = static_cast<int>(v * (tex.height() - 1));

            tx = qBound(0, tx, tex.width() - 1);
            ty = qBound(0, ty, tex.height() - 1);

            QRgb texColor = tex.pixel(tx, ty);
            result.setPixel(x, y, texColor);
        }
    }

    return result;
}

bool KsLiveryAtlas::addPart(const QString& name, const QImage& texture, const QRect& uvRect) {
    if (m_partIndex.contains(name)) return false;

    LiveryPart part;
    part.name = name;
    part.texture = texture;
    part.uvRect = uvRect;

    m_partIndex[name] = static_cast<int>(m_parts.size());
    m_parts.push_back(part);
    return true;
}

bool KsLiveryAtlas::removePart(const QString& name) {
    auto it = m_partIndex.find(name);
    if (it == m_partIndex.end()) return false;

    int idx = it.value();
    m_parts.erase(m_parts.begin() + idx);
    m_partIndex.erase(it);

    for (auto it2 = m_partIndex.begin(); it2 != m_partIndex.end(); ++it2) {
        if (it2.value() > idx) it2.value()--;
    }
    return true;
}

QImage KsLiveryAtlas::buildAtlas(int resolution) {
    QImage atlas(resolution, resolution, QImage::Format_RGBA8888);
    atlas.fill(Qt::white);

    QPainter painter(&atlas);

    for (const auto& part : m_parts) {
        QRect pixelRect(
            static_cast<int>(part.uvRect.x() * resolution),
            static_cast<int>(part.uvRect.y() * resolution),
            static_cast<int>(part.uvRect.width() * resolution),
            static_cast<int>(part.uvRect.height() * resolution)
        );

        if (!part.texture.isNull()) {
            painter.drawImage(pixelRect, part.texture);
        }
    }

    return atlas;
}

QImage KsLiveryAtlas::extractPart(const QString& name) {
    auto it = m_partIndex.find(name);
    if (it == m_partIndex.end()) return QImage();

    const auto& part = m_parts[it.value()];
    return part.texture;
}

void KsLiveryAtlas::paintSeamless(const QPointF& uvStart, const QPointF& uvEnd, const QColor& color, float radius) {
    if (m_parts.empty()) return;

    QImage atlas = buildAtlas(4096);
    QPainter painter(&atlas);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    QRectF uvRect(uvStart.x(), uvStart.y(), uvEnd.x() - uvStart.x(), uvEnd.y() - uvStart.y());

    for (const auto& part : m_parts) {
        QRectF partUVRect(part.uvRect.x(), part.uvRect.y(), part.uvRect.width(), part.uvRect.height());
        if (partUVRect.intersects(uvRect)) {
            QRectF partUV = partUVRect;
            QRectF intersect = uvRect.intersected(partUV);

            QPointF localStart(
                (intersect.left() - partUV.left()) / partUV.width(),
                (intersect.top() - partUV.top()) / partUV.height()
            );
            QPointF localEnd(
                (intersect.right() - partUV.left()) / partUV.width(),
                (intersect.bottom() - partUV.top()) / partUV.height()
            );

            float localRadius = radius / (partUV.width() * 2.0f);

            QBrush brush(color);
            painter.setBrush(brush);
            painter.setPen(Qt::NoPen);

            int px = static_cast<int>(localStart.x() * part.texture.width());
            int py = static_cast<int>(localStart.y() * part.texture.height());
            int r = static_cast<int>(localRadius * part.texture.width());

            painter.setTransform(QTransform::fromScale(
                static_cast<float>(part.texture.width()) / atlas.width(),
                static_cast<float>(part.texture.height()) / atlas.height()
            ));

            painter.drawEllipse(QPointF(px, py), r, r);
        }
    }

    painter.end();
}

}
