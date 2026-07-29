#include "PBRUtils.h"
#include <QVector3D>
#include <QVector2D>
#include <QImage>
#include <cmath>
#include <algorithm>
#include <QtMath>

namespace ks {

float PBRUtils::radicalInverse_VdC(uint32_t bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f;
}

QVector<QVector3D> PBRUtils::hammersleySequence(int n, int N) {
    QVector<QVector3D> samples;
    samples.reserve(N);
    for (int i = 0; i < N; ++i) {
        float u = float(i) / float(N);
        float v = radicalInverse_VdC(i);
        samples.append(QVector3D(u, v, 0.0f));
    }
    return samples;
}

float PBRUtils::GGX_D(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
    return a2 / (3.14159265359f * denom * denom);
}

float PBRUtils::GGX_V(float NdotV, float roughness) {
    float k = (roughness + 1.0f) * (roughness + 1.0f) / 8.0f;
    return 1.0f / (NdotV * (1.0f - k) + k);
}

QVector3D PBRUtils::F_Schlick(float VdotH, const QVector3D& F0) {
    return F0 + (QVector3D(1.0f, 1.0f, 1.0f) - F0) * pow(1.0f - VdotH, 5.0f);
}

QVector3D PBRUtils::importanceSampleGGX(const QVector2D& xi, float roughness, const QVector3D& N) {
    float a = roughness * roughness;
    float phi = 2.0f * 3.14159265359f * xi.x();
    float cosTheta = sqrt((1.0f - xi.y()) / (1.0f + (a * a - 1.0f) * xi.y()));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    
    QVector3D H;
    H.setX(sinTheta * cos(phi));
    H.setY(sinTheta * sin(phi));
    H.setZ(cosTheta);
    
    QVector3D up = qAbs(N.z()) < 0.999f ? QVector3D(0, 0, 1) : QVector3D(1, 0, 0);
    QVector3D tangent = QVector3D::crossProduct(up, N).normalized();
    QVector3D bitangent = QVector3D::crossProduct(N, tangent);
    
    return tangent * H.x() + bitangent * H.y() + N * H.z();
}

QVector3D PBRUtils::sampleEnvironment(const QImage& envMap, const QVector3D& dir) {
    if (envMap.isNull()) return QVector3D(0, 0, 0);
    
    float u = 0.5f + atan2(dir.z(), dir.x()) / (2.0f * 3.14159265359f);
    float v = 0.5f - asin(qBound(-1.0f, dir.y(), 1.0f)) / 3.14159265359f;
    
    int x = int(u * (envMap.width() - 1));
    int y = int(v * (envMap.height() - 1));
    x = qBound(0, x, envMap.width() - 1);
    y = qBound(0, y, envMap.height() - 1);
    
    QRgb pixel = envMap.pixel(x, y);
    return QVector3D(qRed(pixel) / 255.0f, qGreen(pixel) / 255.0f, qBlue(pixel) / 255.0f);
}

QImage PBRUtils::generateBRDFLUT(int size) {
    QImage lut(size, size, QImage::Format_RGBX8888);
    lut.fill(Qt::transparent);
    
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float NdotV = float(x) / float(size - 1);
            float roughness = float(y) / float(size - 1);
            
            QVector3D V(0.0f, 0.0f, 1.0f);
            QVector3D N(0.0f, 0.0f, 1.0f);
            
            float a = 0.0f;
            float b = 0.0f;
            
            const int SAMPLES = 1024;
            for (int i = 0; i < SAMPLES; ++i) {
                float u = float(i + 0.5f) / SAMPLES;
                float v = radicalInverse_VdC(i);
                
                float a2 = roughness * roughness;
                float phi = 2.0f * 3.14159265359f * u;
                float cosTheta = sqrt((1.0f - v) / (1.0f + (a2 * a2 - 1.0f) * v));
                float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
                
                QVector3D H(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
                QVector3D L = 2.0f * QVector3D::dotProduct(V, H) * H - V;
                
                float NdotL = qMax(L.z(), 0.0f);
                float NdotV = qMax(V.z(), 0.0f);
                float NdotH = qMax(H.z(), 0.0f);
                float VdotH = qMax(QVector3D::dotProduct(V, H), 0.0f);
                
                if (NdotL > 0.0f) {
                    float D = GGX_D(NdotH, roughness);
                    float V_smith = GGX_V(NdotV, roughness) * GGX_V(NdotL, roughness);
                    float pdf = D * NdotH / (4.0f * VdotH);
                    
                    float Fc = pow(1.0f - VdotH, 5.0f);
                    a += (1.0f - Fc) * V_smith;
                    b += Fc * V_smith;
                }
            }
            a /= float(SAMPLES);
            b /= float(SAMPLES);
            
            QRgb* ptr = reinterpret_cast<QRgb*>(lut.scanLine(y)) + x;
            auto fa = static_cast<quint8>(qBound(0.0f, a, 1.0f) * 255.0f);
            auto fb = static_cast<quint8>(qBound(0.0f, b, 1.0f) * 255.0f);
            *ptr = qRgba(fa, fb, 0, 255);
        }
    }
    return lut;
}

QImage PBRUtils::generateIrradianceMap(const QImage& environmentMap, int size) {
    if (environmentMap.isNull()) return QImage();
    
    QImage irradiance(size, size, QImage::Format_RGBA8888);
    irradiance.fill(Qt::transparent);
    
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float u = float(x) / float(size - 1);
            float v = float(y) / float(size - 1);
            
            float phi = 2.0f * 3.14159265359f * u;
            float theta = 3.14159265359f * v;
            
            QVector3D N(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
            
            QVector3D color(0, 0, 0);
            float totalWeight = 0.0f;
            const int SAMPLES = 2048;
            
            for (int i = 0; i < SAMPLES; ++i) {
                float xi1 = float(i + 0.5f) / SAMPLES;
                float xi2 = radicalInverse_VdC(i);
                
                float phi_s = 2.0f * 3.14159265359f * xi1;
                float cosTheta = sqrt(1.0f - xi2);
                float sinTheta = sqrt(xi2);
                
                QVector3D L(sinTheta * cos(phi_s), cosTheta, sinTheta * sin(phi_s));
                float NdotL = qMax(QVector3D::dotProduct(N, L), 0.0f);
                
                if (NdotL > 0.0f) {
                    color += sampleEnvironment(environmentMap, L) * NdotL;
                    totalWeight += NdotL;
                }
            }
            
            if (totalWeight > 0.0f) {
                color *= (3.14159265359f / totalWeight);
            }
            
            QRgb* ptr = reinterpret_cast<QRgb*>(irradiance.scanLine(y)) + x;
            auto r = static_cast<quint8>(qBound(0.0f, color.x(), 1.0f) * 255.0f);
            auto g = static_cast<quint8>(qBound(0.0f, color.y(), 1.0f) * 255.0f);
            auto b = static_cast<quint8>(qBound(0.0f, color.z(), 1.0f) * 255.0f);
            *ptr = qRgba(r, g, b, 255);
        }
    }
    return irradiance;
}

QImage PBRUtils::generatePrefilterMap(const QImage& environmentMap, int size, int mipLevels) {
    if (environmentMap.isNull()) return QImage();
    
    QImage prefilter(size, size, QImage::Format_RGBA8888);
    prefilter.fill(Qt::transparent);
    
    for (int mip = 0; mip < mipLevels; ++mip) {
        int mipSize = qMax(1, size >> mip);
        float roughness = mipSize > 1 ? float(mip) / float(mipLevels - 1) : 0.0f;
        
        for (int y = 0; y < mipSize; ++y) {
            for (int x = 0; x < mipSize; ++x) {
                float u = float(x) / float(mipSize - 1);
                float v = float(y) / float(mipSize - 1);
                
                float phi = 2.0f * 3.14159265359f * u;
                float theta = 3.14159265359f * v;
                
                QVector3D R(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
                
                QVector3D color(0, 0, 0);
                float totalWeight = 0.0f;
                const int SAMPLES = 1024;
                
                for (int i = 0; i < SAMPLES; ++i) {
                    float xi1 = float(i + 0.5f) / SAMPLES;
                    float xi2 = radicalInverse_VdC(i);
                    
                    QVector3D H = importanceSampleGGX(QVector2D(xi1, xi2), roughness, R);
                    QVector3D L = 2.0f * QVector3D::dotProduct(R, H) * H - R;
                    
                    float NdotL = qMax(L.y(), 0.0f);
                    if (NdotL > 0.0f) {
                        color += sampleEnvironment(environmentMap, L);
                        totalWeight += 1.0f;
                    }
                }
                
                if (totalWeight > 0.0f) {
                    color /= totalWeight;
                }
                
                QRgb* ptr = reinterpret_cast<QRgb*>(prefilter.scanLine(y)) + x;
                auto r = static_cast<quint8>(qBound(0.0f, color.x(), 1.0f) * 255.0f);
                auto g = static_cast<quint8>(qBound(0.0f, color.y(), 1.0f) * 255.0f);
                auto b = static_cast<quint8>(qBound(0.0f, color.z(), 1.0f) * 255.0f);
                *ptr = qRgba(r, g, b, 255);
            }
        }
    }
    return prefilter;
}

void PBRUtils::saveLUTAsKtx(const QImage& image, const QString& path) {
    image.save(path);
}

} // namespace ks