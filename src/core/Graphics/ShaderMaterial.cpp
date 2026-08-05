#include "ShaderMaterial.h"
#include <QDataStream>
#include <QBuffer>
#include <QImage>
#include <QFile>
#include <cmath>

namespace ks {

QByteArray KsMaterial::serialize() const {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);

    stream << name;
    stream << static_cast<int>(shaderType);
    stream << diffuseColor;
    stream << specularColor;
    stream << emissiveColor;
    stream << diffuseW << specularW << emissiveW << alpha;
    stream << alphaTestRef << alphaTestEnabled;
    stream << ambient << diffuse;

    stream << fresnel.c << fresnel.maxLevel << fresnel.minLevel << fresnel.exponent << fresnel.enabled;
    stream << specular.exponent << specular.intensity << specular.color << specular.fresnelFactor;
    stream << multilayer.baseColor << multilayer.flakeColor << multilayer.flakeSize
           << multilayer.flakeDensity << multilayer.flakeStrength << multilayer.clearCoat
           << multilayer.clearCoatStrength << multilayer.noiseScale;

    stream << txDiffuse << txNormal << txSpecular << txEmissive << txDetail << txMask << txAlpha;
    stream << uvScale << uvRotation << uvOffset << detailUVScale;
    stream << doubleSided << castShadows << receiveShadows << renderQueue;

    stream << static_cast<int>(floatParams.size());
    for (auto it = floatParams.constBegin(); it != floatParams.constEnd(); ++it) {
        stream << it.key() << it.value();
    }

    stream << static_cast<int>(colorParams.size());
    for (auto it = colorParams.constBegin(); it != colorParams.constEnd(); ++it) {
        stream << it.key() << it.value();
    }

    return data;
} // KsMaterial::serialize


bool KsMaterial::deserialize(const QByteArray& data) {
    QDataStream stream(data);
    stream.setVersion(QDataStream::Qt_6_0);

    stream >> name;
    int shaderTypeInt;
    stream >> shaderTypeInt;
    shaderType = static_cast<KsShaderType>(shaderTypeInt);
    stream >> diffuseColor >> specularColor >> emissiveColor;
    stream >> diffuseW >> specularW >> emissiveW >> alpha;
    stream >> alphaTestRef >> alphaTestEnabled;
    stream >> ambient >> diffuse;

    stream >> fresnel.c >> fresnel.maxLevel >> fresnel.minLevel >> fresnel.exponent >> fresnel.enabled;
    stream >> specular.exponent >> specular.intensity >> specular.color >> specular.fresnelFactor;
    stream >> multilayer.baseColor >> multilayer.flakeColor >> multilayer.flakeSize
           >> multilayer.flakeDensity >> multilayer.flakeStrength >> multilayer.clearCoat
           >> multilayer.clearCoatStrength >> multilayer.noiseScale;

    stream >> txDiffuse >> txNormal >> txSpecular >> txEmissive >> txDetail >> txMask >> txAlpha;
    stream >> uvScale >> uvRotation >> uvOffset >> detailUVScale;
    stream >> doubleSided >> castShadows >> receiveShadows >> renderQueue;

    int floatCount;
    stream >> floatCount;
    for (int i = 0; i < floatCount; ++i) {
        QString key;
        float value;
        stream >> key >> value;
        floatParams[key] = value;
    }

    int colorCount;
    stream >> colorCount;
    for (int i = 0; i < colorCount; ++i) {
        QString key;
        QColor value;
        stream >> key >> value;
        colorParams[key] = value;
    }

    return true;
}

KsMaterialLibrary& KsMaterialLibrary::instance() {
    static KsMaterialLibrary inst;
    return inst;
}

bool KsMaterialLibrary::loadFromKN5(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QByteArray data = file.readAll();
    file.close();

    QDataStream stream(data);
    quint32 count;
    stream >> count;

    m_materials.clear();
    m_nameToIndex.clear();

    for (quint32 i = 0; i < count; ++i) {
        quint32 len;
        stream >> len;
        QByteArray matData(static_cast<qsizetype>(len), Qt::Uninitialized);
        stream.readRawData(matData.data(), static_cast<qint64>(len));

        KsMaterial mat;
        mat.deserialize(matData);
        addMaterial(mat);
    }

    return true;
}

bool KsMaterialLibrary::saveToKN5(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    stream << static_cast<quint32>(m_materials.size());

    for (const auto& mat : m_materials) {
        QByteArray matData = mat.serialize();
        stream << static_cast<quint32>(matData.size());
        stream.writeRawData(matData.constData(), matData.size());
    }

    file.write(data);
    return true;
}

void KsMaterialLibrary::addMaterial(const KsMaterial& material) {
    m_nameToIndex[material.name] = static_cast<int>(m_materials.size());
    m_materials.push_back(material);
}

void KsMaterialLibrary::removeMaterial(const QString& name) {
    auto it = m_nameToIndex.find(name);
    if (it != m_nameToIndex.end()) {
        int idx = it.value();
        m_materials.erase(m_materials.begin() + idx);
        m_nameToIndex.erase(it);

        for (auto it2 = m_nameToIndex.begin(); it2 != m_nameToIndex.end(); ++it2) {
            if (it2.value() > idx) it2.value()--;
        }
    }
}

KsMaterial* KsMaterialLibrary::getMaterial(const QString& name) {
    auto it = m_nameToIndex.find(name);
    if (it != m_nameToIndex.end() && it.value() >= 0 && it.value() < static_cast<int>(m_materials.size())) {
        return &m_materials[it.value()];
    }
    return nullptr;
}

std::vector<KsMaterial> KsMaterialLibrary::getAllMaterials() const {
    return m_materials;
}

KsMaterial KsMaterialLibrary::createCarPaintMaterial(const QColor& color, float metallic) {
    KsMaterial mat;
    mat.shaderType = KsShaderType::ksCarPaint;
    mat.diffuseColor = color;
    mat.specularColor = QColor(255, 255, 255);
    mat.specular.exponent = 80.0f;
    mat.specular.intensity = metallic;
    mat.fresnel.enabled = true;
    mat.fresnel.c = 0.04f;
    mat.fresnel.exponent = 5.0f;
    mat.multilayer.baseColor = color;
    mat.multilayer.flakeColor = QColor(200, 200, 200);
    mat.multilayer.flakeSize = 0.05f;
    mat.multilayer.flakeDensity = 0.8f;
    mat.multilayer.flakeStrength = 0.5f;
    mat.multilayer.clearCoat = QColor(255, 255, 255);
    mat.multilayer.clearCoatStrength = 1.0f;
    mat.castShadows = true;
    mat.receiveShadows = true;
    return mat;
}

KsMaterial KsMaterialLibrary::createGlassMaterial(float tint) {
    KsMaterial mat;
    mat.shaderType = KsShaderType::ksWindscreen;
    mat.diffuseColor = QColor(
        static_cast<int>(255 * (1.0f - tint)),
        static_cast<int>(255 * (1.0f - tint)),
        static_cast<int>(255 * (1.0f - tint)),
        static_cast<int>(255 * 0.3f)
    );
    mat.alpha = 0.3f;
    mat.specular.exponent = 100.0f;
    mat.specular.intensity = 0.9f;
    mat.fresnel.enabled = true;
    mat.fresnel.c = 0.1f;
    mat.fresnel.exponent = 3.0f;
    mat.doubleSided = true;
    mat.renderQueue = 3000;
    return mat;
}

KsMaterial KsMaterialLibrary::createTyreMaterial() {
    KsMaterial mat;
    mat.shaderType = KsShaderType::ksTyre;
    mat.diffuseColor = QColor(30, 30, 30);
    mat.specularColor = QColor(80, 80, 80);
    mat.specular.exponent = 10.0f;
    mat.specular.intensity = 0.2f;
    mat.ambient = 0.3f;
    mat.fresnel.enabled = false;
    mat.castShadows = true;
    return mat;
}

KsMaterial KsMaterialLibrary::createCarbonMaterial() {
    KsMaterial mat;
    mat.shaderType = KsShaderType::ksCarbon;
    mat.diffuseColor = QColor(20, 20, 20);
    mat.specularColor = QColor(200, 200, 200);
    mat.specular.exponent = 60.0f;
    mat.specular.intensity = 0.7f;
    mat.fresnel.enabled = true;
    mat.fresnel.c = 0.05f;
    mat.fresnel.exponent = 4.0f;
    mat.castShadows = true;
    return mat;
}

KsMaterial KsMaterialLibrary::createChromeMaterial() {
    KsMaterial mat;
    mat.shaderType = KsShaderType::ksChrome;
    mat.diffuseColor = QColor(200, 200, 200);
    mat.specularColor = Qt::white;
    mat.specular.exponent = 200.0f;
    mat.specular.intensity = 1.0f;
    mat.fresnel.enabled = true;
    mat.fresnel.c = 0.9f;
    mat.fresnel.maxLevel = 1.0f;
    mat.fresnel.minLevel = 0.5f;
    mat.fresnel.exponent = 2.0f;
    mat.castShadows = true;
    return mat;
}

KsMaterial KsMaterialLibrary::createLightMaterial(const QColor& color) {
    KsMaterial mat;
    mat.shaderType = KsShaderType::ksLight;
    mat.diffuseColor = color;
    mat.emissiveColor = color;
    mat.emissiveW = 2.0f;
    mat.specular.exponent = 30.0f;
    mat.specular.intensity = 0.5f;
    mat.fresnel.enabled = false;
    mat.castShadows = false;
    return mat;
}

KsMaterialValidator::ValidationResult KsMaterialValidator::validate(const KsMaterial& material) {
    ValidationResult result;

    if (material.name.isEmpty()) {
        result.isValid = false;
        result.errors.push_back("Material name is empty");
    }

    if (!material.txDiffuse.isEmpty() && !QFile::exists(material.txDiffuse)) {
        result.warnings.push_back(QString("Diffuse texture not found: %1").arg(material.txDiffuse));
    }

    if (!material.txNormal.isEmpty()) {
        if (!QFile::exists(material.txNormal)) {
            result.warnings.push_back(QString("Normal map not found: %1").arg(material.txNormal));
        } else {
            QImage img(material.txNormal);
            if (!img.isNull()) {
                bool hasAlpha = img.hasAlphaChannel();
                if (material.shaderType == KsShaderType::ksPerPixelNM && !hasAlpha) {
                    result.warnings.push_back("Normal map should have alpha channel for AC format");
                }
            }
        }
    }

    if (!material.txSpecular.isEmpty()) {
        if (!QFile::exists(material.txSpecular)) {
            result.warnings.push_back(QString("Specular map not found: %1").arg(material.txSpecular));
        }
    }

    if (material.alpha < 1.0f && material.renderQueue == 2000) {
        result.warnings.push_back("Transparent material should use renderQueue 3000");
    }

    if (material.shaderType == KsShaderType::ksCarPaint && material.multilayer.flakeDensity <= 0.0f) {
        result.warnings.push_back("Car paint should have positive flake density");
    }

    if (material.specular.exponent < 0.0f || material.specular.exponent > 200.0f) {
        result.warnings.push_back("Specular exponent out of typical range (0-200)");
    }

    return result;
}

void KsMaterialValidator::fixNormalMapFormat(QImage& normalMap) {
    if (normalMap.isNull()) return;

    QImage fixed = normalMap.convertToFormat(QImage::Format_RGBA8888);

    for (int y = 0; y < fixed.height(); ++y) {
        for (int x = 0; x < fixed.width(); ++x) {
            QRgb pixel = fixed.pixel(x, y);
            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);

            g = 255 - g;

            float bf = std::sqrt(std::max<float>(0.0f, 1.0f - std::pow((r / 255.0f - 0.5f) * 2, 2) - std::pow((g / 255.0f - 0.5f) * 2, 2)));
            b = static_cast<int>(bf * 255.0f);

            fixed.setPixel(x, y, qRgba(r, g, b, qAlpha(pixel)));
        }
    }

    normalMap = fixed;
}

void KsMaterialValidator::fixSpecularMapFormat(QImage& specularMap) {
    if (specularMap.isNull()) return;

    QImage fixed = specularMap.convertToFormat(QImage::Format_RGBA8888);

    for (int y = 0; y < fixed.height(); ++y) {
        for (int x = 0; x < fixed.width(); ++x) {
            QRgb pixel = fixed.pixel(x, y);
            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);

            float luminance = (r + g + b) / (3.0f * 255.0f);
            int gloss = static_cast<int>(luminance * 255.0f);

            fixed.setPixel(x, y, qRgba(r, g, b, gloss));
        }
    }

    specularMap = fixed;
}

void KsMaterialValidator::ensurePowerOfTwoTextures(KsMaterial& material) {
    auto ensurePOT = [](QString& path) {
        if (path.isEmpty()) return;
        QImage img(path);
        if (img.isNull()) return;

        int w = img.width();
        int h = img.height();
        int potW = 1;
        int potH = 1;

        while (potW < w) potW *= 2;
        while (potH < h) potH *= 2;

        if (potW != w || potH != h) {
            QImage resized = img.scaled(potW, potH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            resized.save(path);
        }
    };

    ensurePOT(material.txDiffuse);
    ensurePOT(material.txNormal);
    ensurePOT(material.txSpecular);
    ensurePOT(material.txEmissive);
    ensurePOT(material.txDetail);
    ensurePOT(material.txMask);
    ensurePOT(material.txAlpha);
}

}
