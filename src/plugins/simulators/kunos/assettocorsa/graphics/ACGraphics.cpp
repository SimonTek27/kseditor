// ============================================================================
// ksacGraphics — Implementation
// ============================================================================

#include "ACGraphics.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QImage>
#include <QOpenGLTexture>
#include <QTemporaryFile>
#include <cmath>
#include "../acFiles/KN5Parser.h"

namespace ks {
namespace ac {
namespace graphics {

// ============================================================================
// KN5 ↔ ACScene conversion helpers
// ============================================================================

namespace {

ACTexture convertTexture(const KN5Parser::Texture& src) {
    ACTexture tex;
    tex.name = src.name;
    tex.width = src.width;
    tex.height = src.height;
    tex.format = src.format;
    tex.mipLevels = src.mipmapCount;
    tex.data = src.data;
    return tex;
}

ACMaterialProperties convertMaterial(const KN5Parser::Material& src) {
    ACMaterialProperties mat;
    mat.shaderName = src.shaderName;
    mat.shaderType = acStringToShaderType(src.shaderName);
    mat.alphaBlending = src.alphaBlending;
    mat.alphaTesting = src.alphaTesting;
    mat.alphaRef = src.alphaRef;
    mat.depthTest = src.depthTest;
    mat.depthWrite = src.depthWrite;
    mat.detailUVMult = src.detailUVMultiplier;
    for (auto it = src.textureMapping.begin(); it != src.textureMapping.end(); ++it) {
        ACTextureSlot slot = acStringToTextureSlot(it.key());
        mat.textureSlots[slot] = it.value();
    }
    for (auto it = src.properties.begin(); it != src.properties.end(); ++it) {
        mat.customParams[it.key()] = QVector4D(it.value().toFloat(), 0, 0, 0);
    }
    return mat;
}

ACMesh convertMesh(const KN5Parser::Mesh& src, uint32_t meshId) {
    ACMesh mesh;
    mesh.name = src.name;
    mesh.meshId = meshId;
    mesh.vertexData = src.vertexData;
    mesh.indexData = src.indexData;
    mesh.positions = src.positions;
    mesh.normals = src.normals;
    mesh.tangents = src.tangents;
    mesh.bitangents = src.bitangents;
    mesh.uv0 = src.uv0;
    mesh.uv1 = src.uv1;
    mesh.boneWeights = src.boneWeights;
    mesh.isSkinned = src.isSkinnedMesh;

    if (!src.indexData.isEmpty()) {
        int indexCount = src.indexData.size() / 2;
        mesh.indices16.resize(indexCount);
        memcpy(mesh.indices16.data(), src.indexData.constData(), src.indexData.size());
    }

    for (const auto& sub : src.subMeshes) {
        ACMesh::SubMesh sm;
        sm.startIndex = sub.indexOffset;
        sm.indexCount = sub.indexCount;
        sm.materialId = sub.materialIndex;
        sm.baseVertex = sub.vertexOffset;
        mesh.subMeshes.append(sm);
    }

    mesh.boundsMin = QVector3D(src.boundingMin.x, src.boundingMin.y, src.boundingMin.z);
    mesh.boundsMax = QVector3D(src.boundingMax.x, src.boundingMax.y, src.boundingMax.z);
    mesh.boundsRadius = src.boundingRadius;

    return mesh;
}

ACScene::Bone convertBone(const KN5Parser::Bone& src) {
    ACScene::Bone bone;
    bone.name = src.name;
    bone.parentIndex = src.parentIndex;
    float m[16];
    memcpy(m, src.matrix, sizeof(m));
    bone.bindPose = QMatrix4x4(
        m[0], m[1], m[2], m[3],
        m[4], m[5], m[6], m[7],
        m[8], m[9], m[10], m[11],
        m[12], m[13], m[14], m[15]
    );
    return bone;
}

ACScene::LODGroup convertLOD(const KN5Parser::LODGroup& src) {
    ACScene::LODGroup lod;
    lod.name = src.name;
    lod.distance = src.distance;
    lod.meshIndices = src.meshIndices;
    return lod;
}

bool kn5FileToACScene(const KN5Parser::KN5File& kn5, const QString& name, ACScene& outScene) {
    outScene.modelName = name;
    outScene.textures.reserve(kn5.textures.size());
    for (const auto& t : kn5.textures)
        outScene.textures.append(convertTexture(t));

    outScene.materials.reserve(kn5.materials.size());
    for (const auto& m : kn5.materials)
        outScene.materials.append(convertMaterial(m));

    outScene.meshes.reserve(kn5.meshes.size());
    for (int i = 0; i < kn5.meshes.size(); ++i)
        outScene.meshes.append(convertMesh(kn5.meshes[i], i));

    outScene.bones.reserve(kn5.bones.size());
    for (const auto& b : kn5.bones)
        outScene.bones.append(convertBone(b));

    outScene.lodGroups.reserve(kn5.lodGroups.size());
    for (const auto& l : kn5.lodGroups)
        outScene.lodGroups.append(convertLOD(l));

    QMatrix4x4 wm = kn5.worldMatrix.toQMatrix();
    outScene.rootTransform = wm;

    return outScene.isValid();
}

} // anonymous namespace

// ============================================================================
// ACMaterialProperties
// ============================================================================

bool ACMaterialProperties::isValid() const {
    return validate().isEmpty();
}

QString ACMaterialProperties::validate() const {
    QStringList errors;
    
    if (shaderName.isEmpty()) {
        errors << "Shader name is empty";
    }
    
    if (alphaRef < 0.0f || alphaRef > 1.0f) {
        errors << "AlphaRef must be in range [0, 1]";
    }
    
    if (detailUVMult <= 0.0f) {
        errors << "DetailUVMult must be positive";
    }
    
    if (emissiveMult < 0.0f) {
        errors << "EmissiveMult must be non-negative";
    }
    
    if (reflectivity < 0.0f || reflectivity > 1.0f) {
        errors << "Reflectivity must be in range [0, 1]";
    }
    
    // Check required textures for shader type
    switch (shaderType) {
        case ACShaderType::ksPerPixelMultiMap_NMDetail:
        case ACShaderType::ksPerPixelMultiMap_NMDetail_D:
        case ACShaderType::ksPerPixelMultiMap_NMDetail_N:
        case ACShaderType::ksPerPixelMultiMap_NMDetail_B:
            if (!textureSlots.contains(ACTextureSlot::Normal)) {
                errors << "Normal map required for " + acShaderTypeToString(shaderType);
            }
            if (!textureSlots.contains(ACTextureSlot::DetailNormal)) {
                errors << "Detail normal map required for " + acShaderTypeToString(shaderType);
            }
            break;
        case ACShaderType::ksPerPixelAT:
        case ACShaderType::ksPerPixelMultiMap_AT:
        case ACShaderType::ksPerPixelAT_Skin:
        case ACShaderType::ksEmissiveAT:
            if (!alphaTesting) {
                errors << "Alpha testing must be enabled for alpha-tested shader";
            }
            break;
        default:
            break;
    }
    
    return errors.join("; ");
}

// ============================================================================
// ACTextureLoader implementation
// ============================================================================

class ACTextureLoader::Impl {
public:
    Impl() {}
    ~Impl() {}
};

ACTextureLoader::ACTextureLoader(QObject* parent) : QObject(parent), d(new Impl()) {}
ACTextureLoader::~ACTextureLoader() { delete d; }

bool ACTextureLoader::loadDDS(const QString& path, ACTexture& outTexture) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        emit textureError("Cannot open file: " + path);
        return false;
    }
    QByteArray data = file.readAll();
    file.close();
    return loadDDSFromMemory(data, outTexture);
}

bool ACTextureLoader::loadDDSFromMemory(const QByteArray& data, ACTexture& outTexture) {
    if (data.size() < 128) {
        emit textureError("DDS file too small");
        return false;
    }
    
    // Parse DDS header (128 bytes)
    const char* header = data.constData();
    
    // Check magic "DDS "
    if (memcmp(header, "DDS ", 4) != 0) {
        emit textureError("Invalid DDS magic");
        return false;
    }
    
    // Parse DDS_HEADER (124 bytes after magic)
    const uint32_t* h = reinterpret_cast<const uint32_t*>(header + 4);
    
    uint32_t size = h[0];
    uint32_t flags = h[1];
    uint32_t height = h[2];
    uint32_t width = h[3];
    uint32_t pitchOrLinearSize = h[4];
    uint32_t depth = h[5];
    uint32_t mipMapCount = h[6];
    
    // Skip reserved[11]
    const uint32_t* pf = h + 19;
    uint32_t pfSize = pf[0];
    uint32_t pfFlags = pf[1];
    uint32_t fourCC = pf[2];
    uint32_t rgbBitCount = pf[3];
    uint32_t rBitMask = pf[4];
    uint32_t gBitMask = pf[5];
    uint32_t bBitMask = pf[6];
    uint32_t aBitMask = pf[7];
    
    // Determine format
    uint32_t format = 0;
    if (pfFlags & 0x4) { // DDPF_FOURCC
        format = fourCC;
    } else if (pfFlags & 0x40) { // DDPF_RGB
        if (rgbBitCount == 32 && aBitMask == 0xFF000000) format = DDSFormat::RGBA8;
        else if (rgbBitCount == 24) format = DDSFormat::RGB8;
    }
    
    outTexture.name = "";
    outTexture.width = width;
    outTexture.height = height;
    outTexture.format = format;
    outTexture.mipLevels = mipMapCount ? mipMapCount : 1;
    
    // Copy pixel data (after header)
    outTexture.data = data.mid(128);
    
    if (outTexture.data.isEmpty()) {
        emit textureError("No pixel data in DDS");
        return false;
    }
    
    emit textureLoaded(outTexture.name, outTexture);
    return true;
}

bool ACTextureLoader::loadFromKN5(const QByteArray& ddsData, const QString& name, ACTexture& outTexture) {
    outTexture.name = name;
    bool result = loadDDSFromMemory(ddsData, outTexture);
    if (result) outTexture.name = name;
    return result;
}

void ACTextureLoader::generateMipmaps(ACTexture& texture) {
    if (texture.data.isEmpty() || texture.mipLevels > 1) return;
    
    // For now, just mark as needing mipmap generation on GPU
    texture.mipLevels = 0; // 0 = generate on GPU
}

bool ACTextureLoader::convertFormat(ACTexture& texture, uint32_t targetFormat) {
    if (texture.format == targetFormat) return true;
    
    // Format conversion would require image processing
    // For now, just update format hint
    texture.format = targetFormat;
    return true;
}

bool ACTextureLoader::compress(ACTexture& texture, uint32_t compressionFormat) {
    // Compression would require external library (DirectXTex, etc.)
    // Mark format for GPU compression
    texture.format = compressionFormat;
    return true;
}

void ACTextureLoader::loadAsync(const QString& path, std::function<void(ACTexture)> callback) {
    // Would use QtConcurrent or thread pool
    ACTexture texture;
    if (loadDDS(path, texture)) {
        callback(texture);
    } else {
        callback(ACTexture());
    }
}

// ============================================================================
// ACModelLoader implementation
// ============================================================================

class ACModelLoader::Impl {
public:
    Impl() {}
    ~Impl() {}
};

ACModelLoader::ACModelLoader(QObject* parent) : QObject(parent), d(new Impl()) {}
ACModelLoader::~ACModelLoader() { delete d; }

bool ACModelLoader::loadKN5(const QString& filePath, ACScene& outScene) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit error("Cannot open KN5 file: " + filePath);
        return false;
    }
    QByteArray data = file.readAll();
    file.close();
    
    bool result = loadKN5FromMemory(data, QFileInfo(filePath).fileName(), outScene);
    if (result) {
        outScene.filePath = filePath;
        outScene.modelName = QFileInfo(filePath).baseName();
        emit loaded(outScene);
    }
    return result;
}

bool ACModelLoader::loadKN5FromMemory(const QByteArray& data, const QString& name, ACScene& outScene) {
    QTemporaryFile tmpFile;
    if (!tmpFile.open()) return false;
    tmpFile.write(data);
    tmpFile.flush();
    QString path = tmpFile.fileName();
    tmpFile.close();

    QString parseError;
    KN5Parser::KN5File kn5 = KN5Parser::KN5ParserImpl::parse(path, &parseError);
    if (!kn5.isValid()) {
        emit error("KN5 parse failed: " + parseError);
        return false;
    }

    return kn5FileToACScene(kn5, name, outScene);
}

bool ACModelLoader::saveKN5(const QString& filePath, const ACScene& scene) {
    QByteArray data;
    if (!saveKN5ToMemory(scene, data)) return false;
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit error("Cannot write KN5 file: " + filePath);
        return false;
    }
    file.write(data);
    file.close();
    return true;
}

bool ACModelLoader::saveKN5ToMemory(const ACScene& scene, QByteArray& outData) {
    QTemporaryFile tmpFile;
    if (!tmpFile.open()) return false;
    QString path = tmpFile.fileName();
    tmpFile.close();

    if (!KN5Parser::KN5ParserImpl::write(path, KN5Parser::KN5File())) {
        emit error("KN5 write failed");
        return false;
    }

    QFile readFile(path);
    if (!readFile.open(QIODevice::ReadOnly)) return false;
    outData = readFile.readAll();
    readFile.close();
    return !outData.isEmpty();
}

bool ACModelLoader::loadTextures(const QString& folder, QVector<ACTexture>& outTextures) {
    QDir dir(folder);
    if (!dir.exists()) return false;
    
    QStringList filters = {"*.dds", "*.DDS"};
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    
    ACTextureLoader loader;
    for (const auto& fi : files) {
        ACTexture tex;
        if (loader.loadDDS(fi.absoluteFilePath(), tex)) {
            tex.name = fi.baseName();
            outTextures.append(tex);
            emit progress(0, "Loaded texture: " + fi.fileName());
        }
    }
    return true;
}

bool ACModelLoader::loadMaterials(const QString& iniPath, QVector<ACMaterialProperties>& outMaterials) {
    QFile file(iniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QTextStream stream(&file);
    ACMaterialProperties current;
    bool inSection = false;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(';') || line.startsWith('#')) continue;

        if (line.startsWith('[') && line.endsWith(']')) {
            if (inSection && !current.shaderName.isEmpty()) {
                outMaterials.append(current);
            }
            current = ACMaterialProperties();
            current.shaderName = line.mid(1, line.length() - 2).trimmed();
            inSection = true;
        } else if (inSection) {
            int eq = line.indexOf('=');
            if (eq < 0) continue;
            QString key = line.left(eq).trimmed().toLower();
            QString val = line.mid(eq + 1).trimmed();

            if (key == "shader") {
                current.shaderType = acStringToShaderType(val);
            } else if (key == "alphablending") {
                current.alphaBlending = (val.toLower() == "on" || val == "1");
            } else if (key == "alphatesting") {
                current.alphaTesting = (val.toLower() == "on" || val == "1");
            } else if (key == "alpharef") {
                current.alphaRef = val.toFloat();
            } else if (key == "depthtest") {
                current.depthTest = (val.toLower() != "off" && val != "0");
            } else if (key == "depthwrite") {
                current.depthWrite = (val.toLower() != "off" && val != "0");
            } else if (key == "backfacecull") {
                current.backfaceCull = (val.toLower() != "off" && val != "0");
            } else if (key == "specularexp") {
                current.specularExp = val.toFloat();
            } else if (key == "specularmult") {
                current.specularMult = val.toFloat();
            } else if (key == "detailuvmult") {
                current.detailUVMult = val.toFloat();
            } else if (key == "emissivecolor") {
                QStringList parts = val.split(',');
                if (parts.size() >= 3) {
                    current.emissiveColor = QVector3D(parts[0].toFloat(), parts[1].toFloat(), parts[2].toFloat());
                }
            } else if (key == "emissivemult") {
                current.emissiveMult = val.toFloat();
            } else if (key == "reflectivity") {
                current.reflectivity = val.toFloat();
            } else if (key == "fresnelbias") {
                current.fresnelBias = val.toFloat();
            } else if (key == "fresnelscale") {
                current.fresnelScale = val.toFloat();
            } else if (key == "fresnelpower") {
                current.fresnelPower = val.toFloat();
            } else if (key == "diffuse") {
                current.textureSlots[ACTextureSlot::Diffuse] = val;
            } else if (key == "normal") {
                current.textureSlots[ACTextureSlot::Normal] = val;
            } else if (key == "specular") {
                current.textureSlots[ACTextureSlot::Specular] = val;
            } else if (key == "detail") {
                current.textureSlots[ACTextureSlot::Detail] = val;
            } else if (key == "detailnormal") {
                current.textureSlots[ACTextureSlot::DetailNormal] = val;
            } else if (key == "ambient") {
                current.textureSlots[ACTextureSlot::Ambient] = val;
            } else if (key == "emissive") {
                current.textureSlots[ACTextureSlot::Emissive] = val;
            } else if (key == "reflection") {
                current.textureSlots[ACTextureSlot::Reflection] = val;
            } else if (key == "damage") {
                current.textureSlots[ACTextureSlot::Damage] = val;
            } else {
                QStringList parts = val.split(',');
                QVector4D vecVal;
                for (int i = 0; i < qMin(4, parts.size()); ++i)
                    vecVal[i] = parts[i].toFloat();
                current.customParams[key] = vecVal;
            }
        }
    }

    if (inSection && !current.shaderName.isEmpty()) {
        outMaterials.append(current);
    }

    return true;
}

QVector<QString> ACModelLoader::validateKN5(const ACScene& scene) const {
    QVector<QString> errors;
    
    if (!scene.isValid()) {
        errors << "Invalid scene: no meshes";
    }
    
    // Check for LOD_A
    bool hasLodA = false;
    for (const auto& mesh : scene.meshes) {
        if (mesh.name.startsWith("LOD_A")) hasLodA = true;
    }
    if (!hasLodA && !scene.meshes.isEmpty()) {
        errors << "Missing LOD_A mesh (required by AC)";
    }
    
    // Check for duplicate mesh names
    QStringList names;
    for (const auto& mesh : scene.meshes) {
        if (names.contains(mesh.name)) {
            errors << "Duplicate mesh name: " + mesh.name;
        }
        names << mesh.name;
    }
    
    // Check texture references
    for (const auto& mat : scene.materials) {
        for (auto it = mat.textureSlots.begin(); it != mat.textureSlots.end(); ++it) {
            bool found = false;
            for (const auto& tex : scene.textures) {
                if (tex.name == it.value()) { found = true; break; }
            }
            if (!found) {
                errors << "Missing texture: " + it.value() + " (referenced by material)";
            }
        }
    }
    
    return errors;
}

// ============================================================================
// ACShaderManager implementation
// ============================================================================

class ACShaderManager::Impl {
public:
    QMap<ACShaderType, void*> shaders;
    QMap<ACShaderType, QString> shaderNames;
    QMap<ACShaderType, QMap<QString, int>> uniformLayouts;
};

ACShaderManager::ACShaderManager(QObject* parent) : QObject(parent), d(new Impl()) {
    // Initialize standard shader names
    d->shaderNames[ACShaderType::ksPerPixel] = "ksPerPixel";
    d->shaderNames[ACShaderType::ksPerPixelAT] = "ksPerPixelAT";
    d->shaderNames[ACShaderType::ksPerPixelMultiMap] = "ksPerPixelMultiMap";
    d->shaderNames[ACShaderType::ksPerPixelMultiMap_NMDetail] = "ksPerPixelMultiMap_NMDetail";
    d->shaderNames[ACShaderType::ksPerPixelMultiMap_NMDetail_D] = "ksPerPixelMultiMap_NMDetail_D";
    d->shaderNames[ACShaderType::ksPerPixelMultiMap_NMDetail_N] = "ksPerPixelMultiMap_NMDetail_N";
    d->shaderNames[ACShaderType::ksPerPixelMultiMap_AT] = "ksPerPixelMultiMap_AT";
    d->shaderNames[ACShaderType::ksPerPixel_Skin] = "ksPerPixel_Skin";
    d->shaderNames[ACShaderType::ksPerPixel_Skin_NMDetail] = "ksPerPixel_Skin_NMDetail";
    d->shaderNames[ACShaderType::ksPerPixelAT_Skin] = "ksPerPixelAT_Skin";
    d->shaderNames[ACShaderType::ksWindScreen] = "ksWindScreen";
    d->shaderNames[ACShaderType::ksChrome] = "ksChrome";
    d->shaderNames[ACShaderType::ksGlass] = "ksGlass";
    d->shaderNames[ACShaderType::ksHeadLight] = "ksHeadLight";
    d->shaderNames[ACShaderType::ksRearLight] = "ksRearLight";
    d->shaderNames[ACShaderType::ksEmissive] = "ksEmissive";
    d->shaderNames[ACShaderType::ksEmissiveAT] = "ksEmissiveAT";
    d->shaderNames[ACShaderType::ksDirt] = "ksDirt";
    d->shaderNames[ACShaderType::ksCrowd] = "ksCrowd";
    d->shaderNames[ACShaderType::ksTrees] = "ksTrees";
    d->shaderNames[ACShaderType::ksPerPixelMultiMap_B] = "ksPerPixelMultiMap_B";
    d->shaderNames[ACShaderType::ksPerPixelMultiMap_NMDetail_B] = "ksPerPixelMultiMap_NMDetail_B";
}

ACShaderManager::~ACShaderManager() { delete d; }

bool ACShaderManager::loadShaders(const QString& shaderPath) {
    // Load shader files from disk
    // For now, just validate path exists
    QDir dir(shaderPath);
    if (!dir.exists()) {
        emit shaderError("Shader path does not exist: " + shaderPath);
        return false;
    }
    
    // Would compile shaders here
    return true;
}

void* ACShaderManager::getShader(ACShaderType type) const {
    return d->shaders.value(type, nullptr);
}

QString ACShaderManager::getShaderName(ACShaderType type) const {
    return d->shaderNames.value(type, "unknown");
}

void* ACShaderManager::compileVariant(ACShaderType base, const QStringList& defines) {
    // Would compile shader variant with defines
    return nullptr;
}

ACShaderManager::ShaderLayout ACShaderManager::getLayout(ACShaderType type) const {
    ShaderLayout layout;
    // Return cached layout or default
    return layout;
}

// ============================================================================
// ACMaterial implementation
// ============================================================================

class ACMaterial::Impl {
public:
    QMap<ACTextureSlot, ACTexture*> textures;
    QMap<QString, QVector4D> params;
};

ACMaterial::ACMaterial(QObject* parent) : QObject(parent), d(new Impl()) {}
ACMaterial::~ACMaterial() { delete d; }

void ACMaterial::setTexture(ACTextureSlot slot, ACTexture* texture) {
    d->textures[slot] = texture;
    emit textureChanged(slot);
}

ACTexture* ACMaterial::getTexture(ACTextureSlot slot) const {
    return d->textures.value(slot, nullptr);
}

void ACMaterial::bind(ACRenderer* renderer, const QMatrix4x4& viewProj, const QMatrix4x4& model) {
    // Would bind shader and textures
    // Set uniform parameters
    setParam(ACShaderParams::g_mWorldViewProj, viewProj * model);
    setParam(ACShaderParams::g_mWorld, model);
    setParam(ACShaderParams::g_mWorldInvTranspose, model.inverted().transposed());
}

void ACMaterial::unbind() {
    // Unbind textures and shader
}

void ACMaterial::setParam(const QString& name, const QVector4D& value) {
    d->params[name] = value;
}

void ACMaterial::setParam(const QString& name, float value) {
    d->params[name] = QVector4D(value, 0, 0, 0);
}

void ACMaterial::setParam(const QString& name, const QVector3D& value) {
    d->params[name] = QVector4D(value, 0);
}

void ACMaterial::setParam(const QString& name, const QMatrix4x4& value) {
    // Would set matrix uniform
}

bool ACMaterial::validate(QString* error) const {
    QString err = properties.validate();
    if (!err.isEmpty()) {
        if (error) *error = err;
        return false;
    }
    return true;
}

// ============================================================================
// ACRenderer implementation
// ============================================================================

class ACRenderer::Impl {
public:
    Backend backend = Backend::Null;
    uint32_t width = 0, height = 0;
    Stats stats;
    bool wireframe = false;
    bool showBounds = false;
    bool showNormals = false;
};

ACRenderer::ACRenderer(Backend backend, QObject* parent) : QObject(parent), d(new Impl()) {
    d->backend = backend;
}

ACRenderer::~ACRenderer() {
    shutdown();
    delete d;
}

bool ACRenderer::initialize(void* windowHandle, uint32_t width, uint32_t height) {
    d->width = width;
    d->height = height;
    d->stats = Stats();
    
    // Initialize backend (Vulkan, OpenGL, etc.)
    // For now, just return success for Null backend
    return d->backend == Backend::Null;
}

void ACRenderer::resize(uint32_t width, uint32_t height) {
    d->width = width;
    d->height = height;
}

void ACRenderer::shutdown() {
    d->width = 0;
    d->height = 0;
    d->stats = Stats();
}

    void* ACRenderer::createVertexBuffer(const void* data, uint32_t size) {
        if (d->backend == Backend::Null) {
            QByteArray* buf = new QByteArray(reinterpret_cast<const char*>(data), size);
            return static_cast<void*>(buf);
        }
        return nullptr;
    }

    void* ACRenderer::createIndexBuffer(const void* data, uint32_t size) {
        if (d->backend == Backend::Null) {
            QByteArray* buf = new QByteArray(reinterpret_cast<const char*>(data), size);
            return static_cast<void*>(buf);
        }
        return nullptr;
    }

    void* ACRenderer::createTexture(const ACTexture& texture) {
        if (d->backend == Backend::Null) {
            ACTexture* tex = new ACTexture(texture);
            return static_cast<void*>(tex);
        }
        return nullptr;
    }

    void* ACRenderer::createUniformBuffer(uint32_t size) {
        if (d->backend == Backend::Null) {
            QByteArray* buf = new QByteArray(size, '\0');
            return static_cast<void*>(buf);
        }
        return nullptr;
    }

    void* ACRenderer::createShader(ACShaderType type, const QStringList& defines) {
        if (d->backend == Backend::Null) {
            ACShaderType* shader = new ACShaderType(type);
            return static_cast<void*>(shader);
        }
        return nullptr;
    }

    void ACRenderer::updateBuffer(void* buffer, const void* data, uint32_t offset, uint32_t size) {
        if (!buffer) return;
        auto* buf = static_cast<QByteArray*>(buffer);
        if (offset + size <= static_cast<uint32_t>(buf->size()))
            buf->replace(static_cast<int>(offset), static_cast<int>(size), reinterpret_cast<const char*>(data), static_cast<int>(size));
    }

    void ACRenderer::updateTexture(void* texture, const ACTexture& data) {
        if (!texture) return;
        auto* tex = static_cast<ACTexture*>(texture);
        tex->data = data.data;
        tex->width = data.width;
        tex->height = data.height;
    }

    void ACRenderer::destroyBuffer(void* buffer) {
        delete static_cast<QByteArray*>(buffer);
    }

    void ACRenderer::destroyTexture(void* texture) {
        delete static_cast<ACTexture*>(texture);
    }

    void ACRenderer::destroyShader(void* shader) {
        delete static_cast<ACShaderType*>(shader);
    }

    void ACRenderer::beginFrame() {
        d->stats = Stats();
    }

    void ACRenderer::endFrame() {
        emit frameRendered();
    }

    void ACRenderer::renderScene(const ACScene& scene, const QMatrix4x4& view, const QMatrix4x4& proj,
                                 const QVector3D& cameraPos, const QVector<QVector3D>& lights) {
        if (d->backend == Backend::Null) {
            for (const auto& node : scene.nodes) {
                if (!node.visible || node.meshIndex < 0 || node.meshIndex >= scene.meshes.size()) continue;
                const auto& mesh = scene.meshes[node.meshIndex];
                for (const auto& sub : mesh.subMeshes) {
                    d->stats.drawCalls++;
                    d->stats.triangles += sub.indexCount / 3;
                    d->stats.vertices += mesh.vertexCount();
                }
            }
        }
    }

void ACRenderer::setWireframe(bool enabled) { d->wireframe = enabled; }
void ACRenderer::setShowBounds(bool enabled) { d->showBounds = enabled; }
void ACRenderer::setShowNormals(bool enabled) { d->showNormals = enabled; }

ACRenderer::Stats ACRenderer::getStats() const { return d->stats; }

// ============================================================================
// Utility functions
// ============================================================================

QString acShaderTypeToString(ACShaderType type) {
    switch (type) {
        case ACShaderType::ksPerPixel: return "ksPerPixel";
        case ACShaderType::ksPerPixelAT: return "ksPerPixelAT";
        case ACShaderType::ksPerPixelMultiMap: return "ksPerPixelMultiMap";
        case ACShaderType::ksPerPixelMultiMap_NMDetail: return "ksPerPixelMultiMap_NMDetail";
        case ACShaderType::ksPerPixelMultiMap_NMDetail_D: return "ksPerPixelMultiMap_NMDetail_D";
        case ACShaderType::ksPerPixelMultiMap_NMDetail_N: return "ksPerPixelMultiMap_NMDetail_N";
        case ACShaderType::ksPerPixelMultiMap_AT: return "ksPerPixelMultiMap_AT";
        case ACShaderType::ksPerPixel_Skin: return "ksPerPixel_Skin";
        case ACShaderType::ksPerPixel_Skin_NMDetail: return "ksPerPixel_Skin_NMDetail";
        case ACShaderType::ksPerPixelAT_Skin: return "ksPerPixelAT_Skin";
        case ACShaderType::ksWindScreen: return "ksWindScreen";
        case ACShaderType::ksChrome: return "ksChrome";
        case ACShaderType::ksGlass: return "ksGlass";
        case ACShaderType::ksHeadLight: return "ksHeadLight";
        case ACShaderType::ksRearLight: return "ksRearLight";
        case ACShaderType::ksEmissive: return "ksEmissive";
        case ACShaderType::ksEmissiveAT: return "ksEmissiveAT";
        case ACShaderType::ksDirt: return "ksDirt";
        case ACShaderType::ksCrowd: return "ksCrowd";
        case ACShaderType::ksTrees: return "ksTrees";
        case ACShaderType::ksPerPixelMultiMap_B: return "ksPerPixelMultiMap_B";
        case ACShaderType::ksPerPixelMultiMap_NMDetail_B: return "ksPerPixelMultiMap_NMDetail_B";
        default: return "unknown";
    }
}

ACShaderType acStringToShaderType(const QString& name) {
    if (name.startsWith("ksPerPixelAT_Skin")) return ACShaderType::ksPerPixelAT_Skin;
    if (name.startsWith("ksPerPixel_Skin_NMDetail")) return ACShaderType::ksPerPixel_Skin_NMDetail;
    if (name.startsWith("ksPerPixel_Skin")) return ACShaderType::ksPerPixel_Skin;
    if (name.startsWith("ksPerPixelMultiMap_NMDetail_B")) return ACShaderType::ksPerPixelMultiMap_NMDetail_B;
    if (name.startsWith("ksPerPixelMultiMap_NMDetail_D")) return ACShaderType::ksPerPixelMultiMap_NMDetail_D;
    if (name.startsWith("ksPerPixelMultiMap_NMDetail_N")) return ACShaderType::ksPerPixelMultiMap_NMDetail_N;
    if (name.startsWith("ksPerPixelMultiMap_NMDetail")) return ACShaderType::ksPerPixelMultiMap_NMDetail;
    if (name.startsWith("ksPerPixelMultiMap_AT")) return ACShaderType::ksPerPixelMultiMap_AT;
    if (name.startsWith("ksPerPixelMultiMap_B")) return ACShaderType::ksPerPixelMultiMap_B;
    if (name.startsWith("ksPerPixelMultiMap")) return ACShaderType::ksPerPixelMultiMap;
    if (name.startsWith("ksPerPixelAT")) return ACShaderType::ksPerPixelAT;
    if (name.startsWith("ksPerPixel")) return ACShaderType::ksPerPixel;
    if (name.startsWith("ksWindScreen")) return ACShaderType::ksWindScreen;
    if (name.startsWith("ksChrome")) return ACShaderType::ksChrome;
    if (name.startsWith("ksGlass")) return ACShaderType::ksGlass;
    if (name.startsWith("ksHeadLight")) return ACShaderType::ksHeadLight;
    if (name.startsWith("ksRearLight")) return ACShaderType::ksRearLight;
    if (name.startsWith("ksEmissiveAT")) return ACShaderType::ksEmissiveAT;
    if (name.startsWith("ksEmissive")) return ACShaderType::ksEmissive;
    if (name.startsWith("ksDirt")) return ACShaderType::ksDirt;
    if (name.startsWith("ksCrowd")) return ACShaderType::ksCrowd;
    if (name.startsWith("ksTrees")) return ACShaderType::ksTrees;
    return ACShaderType::ksPerPixel;
}

ACShaderType acGetDefaultShader(const ACMaterialProperties& props) {
    if (props.alphaTesting) return ACShaderType::ksPerPixelAT;
    if (props.textureSlots.contains(ACTextureSlot::Normal) && 
        props.textureSlots.contains(ACTextureSlot::DetailNormal)) {
        return ACShaderType::ksPerPixelMultiMap_NMDetail;
    }
    if (props.textureSlots.contains(ACTextureSlot::Normal)) {
        return ACShaderType::ksPerPixelMultiMap;
    }
    return ACShaderType::ksPerPixel;
}

bool acValidateMaterial(const ACMaterialProperties& props, QString* error) {
    QString err = props.validate();
    if (!err.isEmpty()) {
        if (error) *error = err;
        return false;
    }
    return true;
}

QString acTextureSlotToString(ACTextureSlot slot) {
    switch (slot) {
        case ACTextureSlot::Diffuse: return "Diffuse";
        case ACTextureSlot::Normal: return "Normal";
        case ACTextureSlot::Specular: return "Specular";
        case ACTextureSlot::Detail: return "Detail";
        case ACTextureSlot::DetailNormal: return "DetailNormal";
        case ACTextureSlot::Ambient: return "Ambient";
        case ACTextureSlot::LightMap: return "LightMap";
        case ACTextureSlot::Damage: return "Damage";
        case ACTextureSlot::Reflection: return "Reflection";
        case ACTextureSlot::Emissive: return "Emissive";
        default: return "Unknown";
    }
}

ACTextureSlot acStringToTextureSlot(const QString& str) {
    if (str.toLower() == "diffuse") return ACTextureSlot::Diffuse;
    if (str.toLower() == "normal") return ACTextureSlot::Normal;
    if (str.toLower() == "specular") return ACTextureSlot::Specular;
    if (str.toLower() == "detail") return ACTextureSlot::Detail;
    if (str.toLower() == "detailnormal") return ACTextureSlot::DetailNormal;
    if (str.toLower() == "ambient") return ACTextureSlot::Ambient;
    if (str.toLower() == "lightmap") return ACTextureSlot::LightMap;
    if (str.toLower() == "damage") return ACTextureSlot::Damage;
    if (str.toLower() == "reflection") return ACTextureSlot::Reflection;
    if (str.toLower() == "emissive") return ACTextureSlot::Emissive;
    return ACTextureSlot::Diffuse;
}

} // namespace graphics
} // namespace ac
} // namespace ks