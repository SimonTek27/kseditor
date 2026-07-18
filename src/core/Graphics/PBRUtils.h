#pragma once

#include <QObject>
#include <QImage>
#include <QVector3D>
#include <QVector2D>

namespace ks {
namespace graphics {

class PBRUtils : public QObject {
    Q_OBJECT
public:
    static QImage generateBRDFLUT(int size = 512);
    static QImage generateIrradianceMap(const QImage& environmentMap, int size = 256);
    static QImage generatePrefilterMap(const QImage& environmentMap, int size = 256, int mipLevels = 5);
    static QVector<QVector3D> hammersleySequence(int n, int N);
    static QVector3D importanceSampleGGX(const QVector2D& xi, float roughness, const QVector3D& N);
    static QVector3D sampleEnvironment(const QImage& envMap, const QVector3D& dir);
    static float GGX_D(float NdotH, float roughness);
    static float GGX_V(float NdotV, float roughness);
    static QVector3D F_Schlick(float VdotH, const QVector3D& F0);
    
    static void saveLUTAsKtx(const QImage& image, const QString& path);
    
private:
    static float radicalInverse_VdC(uint32_t bits);
};

} // namespace graphics
} // namespace ks