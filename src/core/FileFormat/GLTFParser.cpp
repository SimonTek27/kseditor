#include "GLTFParser.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QBuffer>
#include <QDebug>
#include <QByteArray>
#include <QDir>
#include <QStandardPaths>
#include <cstring>
#include <cmath>

namespace ks {
namespace fileformat {

// ─── GLTF Parser Implementation ──────────────────────────────────────────────

GLTFParser::GLTFParser() = default;

bool GLTFParser::loadFromFile(const QString& path) {
    m_filePath = path;
    m_baseDir = QFileInfo(path).absolutePath();
    m_isBinary = path.toLower().endsWith(".glb");
    
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open file: " + path;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    if (m_isBinary) {
        return parseBinary(data);
    } else {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            m_lastError = "JSON parse error: " + error.errorString();
            return false;
        }
        return parseJson(doc.object());
    }
}

bool GLTFParser::parseBinary(const QByteArray& data) {
    if (data.size() < 12) {
        m_lastError = "GLB file too small";
        return false;
    }
    
    // Check magic
    const char* ptr = data.constData();
    uint32_t magic = *reinterpret_cast<const uint32_t*>(ptr);
    if (magic != 0x46546C67) { // "glTF"
        m_lastError = "Invalid GLB magic";
        return false;
    }
    
    ptr += 4;
    uint32_t version = *reinterpret_cast<const uint32_t*>(ptr);
    ptr += 4;
    uint32_t length = *reinterpret_cast<const uint32_t*>(ptr);
    ptr += 4;
    
    if (version != 2) {
        m_lastError = "Unsupported GLB version: " + QString::number(version);
        return false;
    }
    
    // Parse chunks
    const char* end = data.constData() + data.size();
    ptr += 0; // After header
    
    QByteArray jsonChunk;
    QByteArray binChunk;
    
    while (ptr + 8 <= end) {
        uint32_t chunkLength = *reinterpret_cast<const uint32_t*>(ptr);
        ptr += 4;
        uint32_t chunkType = *reinterpret_cast<const uint32_t*>(ptr);
        ptr += 4;
        
        if (ptr + chunkLength > end) {
            m_lastError = "Chunk exceeds file bounds";
            return false;
        }
        
        if (chunkType == 0x4E4F534A) { // "JSON"
            jsonChunk = QByteArray(ptr, chunkLength);
        } else if (chunkType == 0x004E4942) { // "BIN\0"
            binChunk = QByteArray(ptr, chunkLength);
        }
        
        ptr += chunkLength;
    }
    
    if (jsonChunk.isEmpty()) {
        m_lastError = "No JSON chunk in GLB";
        return false;
    }
    
    m_binaryBuffer = binChunk;
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonChunk, &error);
    if (error.error != QJsonParseError::NoError) {
        m_lastError = "JSON parse error: " + error.errorString();
        return false;
    }
    
    return parseJson(doc.object());
}

bool GLTFParser::parseJson(const QJsonObject& json) {
    m_gltf.clear();
    m_gltf.asset.copyright = json["asset"].toObject()["copyright"].toString();
    m_gltf.asset.generator = json["asset"].toObject()["generator"].toString();
    m_gltf.asset.version = json["asset"].toObject()["version"].toString("2.0");
    m_gltf.asset.minVersion = json["asset"].toObject()["minVersion"].toString("2.0");
    
    // Scene
    m_gltf.scene = json["scene"].toInt(-1);
    QJsonArray scenesArr = json["scenes"].toArray();
    for (const auto& v : scenesArr) {
        GLTFData::Scene scene;
        QJsonObject s = v.toObject();
        scene.name = s["name"].toString();
        QJsonArray nodesArr = s["nodes"].toArray();
        for (const auto& n : nodesArr) {
            scene.nodes.append(n.toInt());
        }
        m_gltf.scenes.append(scene);
    }
    
    // Nodes
    QJsonArray nodesArr = json["nodes"].toArray();
    for (const auto& v : nodesArr) {
        GLTFData::Node node;
        QJsonObject n = v.toObject();
        node.name = n["name"].toString();
        node.mesh = n["mesh"].toInt(-1);
        node.skin = n["skin"].toInt(-1);
        node.camera = n["camera"].toInt(-1);
        
        QJsonArray childrenArr = n["children"].toArray();
        for (const auto& c : childrenArr) {
            node.children.append(c.toInt());
        }
        
        // Transform
        if (n.contains("matrix")) {
            QJsonArray m = n["matrix"].toArray();
            node.matrix = QMatrix4x4();
            for (int i = 0; i < 16; ++i) {
                node.matrix(i / 4, i % 4) = static_cast<float>(m[i].toDouble());
            }
            node.hasMatrix = true;
        } else {
            if (n.contains("translation")) {
                QJsonArray t = n["translation"].toArray();
                node.translation = QVector3D(t[0].toDouble(), t[1].toDouble(), t[2].toDouble());
            }
            if (n.contains("rotation")) {
                QJsonArray r = n["rotation"].toArray();
                node.rotation = QQuaternion(r[3].toDouble(), r[0].toDouble(), r[1].toDouble(), r[2].toDouble());
            }
            if (n.contains("scale")) {
                QJsonArray s = n["scale"].toArray();
                node.scale = QVector3D(s[0].toDouble(), s[1].toDouble(), s[2].toDouble());
            }
        }
        
        m_gltf.nodes.append(node);
    }
    
    // Meshes
    QJsonArray meshesArr = json["meshes"].toArray();
    for (const auto& v : meshesArr) {
        GLTFData::Mesh mesh;
        QJsonObject m = v.toObject();
        mesh.name = m["name"].toString();
        
        QJsonArray primsArr = m["primitives"].toArray();
        for (const auto& p : primsArr) {
            GLTFData::Primitive prim;
            QJsonObject primObj = p.toObject();
            prim.mode = primObj["mode"].toInt(4); // TRIANGLES
            prim.indices = primObj["indices"].toInt(-1);
            prim.material = primObj["material"].toInt(-1);
            
            QJsonObject attrs = primObj["attributes"].toObject();
            prim.attributes["POSITION"] = attrs["POSITION"].toInt(-1);
            prim.attributes["NORMAL"] = attrs["NORMAL"].toInt(-1);
            prim.attributes["TANGENT"] = attrs["TANGENT"].toInt(-1);
            prim.attributes["TEXCOORD_0"] = attrs["TEXCOORD_0"].toInt(-1);
            prim.attributes["TEXCOORD_1"] = attrs["TEXCOORD_1"].toInt(-1);
            prim.attributes["COLOR_0"] = attrs["COLOR_0"].toInt(-1);
            prim.attributes["JOINTS_0"] = attrs["JOINTS_0"].toInt(-1);
            prim.attributes["WEIGHTS_0"] = attrs["WEIGHTS_0"].toInt(-1);
            
            // Morph targets
            QJsonArray targetsArr = primObj["targets"].toArray();
            for (const auto& t : targetsArr) {
                GLTFData::Primitive::Target target;
                QJsonObject tObj = t.toObject();
                target.POSITION = tObj["POSITION"].toInt(-1);
                target.NORMAL = tObj["NORMAL"].toInt(-1);
                target.TANGENT = tObj["TANGENT"].toInt(-1);
                prim.targets.append(target);
            }
            
            mesh.primitives.append(prim);
        }
        
        QJsonArray weightsArr = m["weights"].toArray();
        for (const auto& w : weightsArr) {
            mesh.weights.append(w.toDouble());
        }
        
        m_gltf.meshes.append(mesh);
    }
    
    // Materials
    QJsonArray matsArr = json["materials"].toArray();
    for (const auto& v : matsArr) {
        GLTFData::Material mat;
        QJsonObject m = v.toObject();
        mat.name = m["name"].toString();
        mat.alphaMode = m["alphaMode"].toString("OPAQUE");
        mat.alphaCutoff = m["alphaCutoff"].toDouble(0.5);
        mat.doubleSided = m["doubleSided"].toBool(false);
        
        // PBR Metallic Roughness
        QJsonObject pbr = m["pbrMetallicRoughness"].toObject();
        QJsonArray baseColor = pbr["baseColorFactor"].toArray();
        if (baseColor.size() == 4) {
            mat.baseColorFactor = QVector4D(baseColor[0].toDouble(), baseColor[1].toDouble(), 
                                            baseColor[2].toDouble(), baseColor[3].toDouble());
        }
        mat.metallicFactor = pbr["metallicFactor"].toDouble(1.0);
        mat.roughnessFactor = pbr["roughnessFactor"].toDouble(1.0);
        
        // Textures
        if (pbr.contains("baseColorTexture")) {
            QJsonObject tex = pbr["baseColorTexture"].toObject();
            mat.baseColorTexture = tex["index"].toInt(-1);
            mat.baseColorTexCoord = tex["texCoord"].toInt(0);
        }
        if (pbr.contains("metallicRoughnessTexture")) {
            QJsonObject tex = pbr["metallicRoughnessTexture"].toObject();
            mat.metallicRoughnessTexture = tex["index"].toInt(-1);
            mat.metallicRoughnessTexCoord = tex["texCoord"].toInt(0);
        }
        
        // Normal texture
        if (m.contains("normalTexture")) {
            QJsonObject tex = m["normalTexture"].toObject();
            mat.normalTexture = tex["index"].toInt(-1);
            mat.normalTexCoord = tex["texCoord"].toInt(0);
            mat.normalScale = tex["scale"].toDouble(1.0);
        }
        
        // Occlusion texture
        if (m.contains("occlusionTexture")) {
            QJsonObject tex = m["occlusionTexture"].toObject();
            mat.occlusionTexture = tex["index"].toInt(-1);
            mat.occlusionTexCoord = tex["texCoord"].toInt(0);
            mat.occlusionStrength = tex["strength"].toDouble(1.0);
        }
        
        // Emissive texture
        if (m.contains("emissiveTexture")) {
            QJsonObject tex = m["emissiveTexture"].toObject();
            mat.emissiveTexture = tex["index"].toInt(-1);
            mat.emissiveTexCoord = tex["texCoord"].toInt(0);
        }
        
        QJsonArray emissive = m["emissiveFactor"].toArray();
        if (emissive.size() == 3) {
            mat.emissiveFactor = QVector3D(emissive[0].toDouble(), emissive[1].toDouble(), emissive[2].toDouble());
        }
        
        // Extensions
        QJsonObject ext = m["extensions"].toObject();
        // KHR_materials_clearcoat
        if (ext.contains("KHR_materials_clearcoat")) {
            QJsonObject cc = ext["KHR_materials_clearcoat"].toObject();
            mat.clearcoatFactor = cc["clearcoatFactor"].toDouble(0.0);
            mat.clearcoatRoughnessFactor = cc["clearcoatRoughnessFactor"].toDouble(0.0);
            if (cc.contains("clearcoatTexture")) {
                mat.clearcoatTexture = cc["clearcoatTexture"].toObject()["index"].toInt(-1);
            }
            if (cc.contains("clearcoatRoughnessTexture")) {
                mat.clearcoatRoughnessTexture = cc["clearcoatRoughnessTexture"].toObject()["index"].toInt(-1);
            }
            if (cc.contains("clearcoatNormalTexture")) {
                mat.clearcoatNormalTexture = cc["clearcoatNormalTexture"].toObject()["index"].toInt(-1);
            }
        }
        // KHR_materials_transmission
        if (ext.contains("KHR_materials_transmission")) {
            QJsonObject tr = ext["KHR_materials_transmission"].toObject();
            mat.transmissionFactor = tr["transmissionFactor"].toDouble(0.0);
            if (tr.contains("transmissionTexture")) {
                mat.transmissionTexture = tr["transmissionTexture"].toObject()["index"].toInt(-1);
            }
        }
        // KHR_materials_ior
        if (ext.contains("KHR_materials_ior")) {
            QJsonObject ior = ext["KHR_materials_ior"].toObject();
            mat.ior = ior["ior"].toDouble(1.5);
        }
        // KHR_materials_specular
        if (ext.contains("KHR_materials_specular")) {
            QJsonObject sp = ext["KHR_materials_specular"].toObject();
            mat.specularFactor = sp["specularFactor"].toDouble(1.0);
            QJsonArray sc = sp["specularColorFactor"].toArray();
            if (sc.size() == 3) {
                mat.specularColorFactor = QVector3D(sc[0].toDouble(), sc[1].toDouble(), sc[2].toDouble());
            }
            if (sp.contains("specularTexture")) {
                mat.specularTexture = sp["specularTexture"].toObject()["index"].toInt(-1);
            }
            if (sp.contains("specularColorTexture")) {
                mat.specularColorTexture = sp["specularColorTexture"].toObject()["index"].toInt(-1);
            }
        }
        // KHR_materials_sheen
        if (ext.contains("KHR_materials_sheen")) {
            QJsonObject sh = ext["KHR_materials_sheen"].toObject();
            QJsonArray sc = sh["sheenColorFactor"].toArray();
            if (sc.size() == 3) {
                mat.sheenColorFactor = QVector3D(sc[0].toDouble(), sc[1].toDouble(), sc[2].toDouble());
            }
            mat.sheenRoughnessFactor = sh["sheenRoughnessFactor"].toDouble(0.0);
            if (sh.contains("sheenColorTexture")) {
                mat.sheenColorTexture = sh["sheenColorTexture"].toObject()["index"].toInt(-1);
            }
            if (sh.contains("sheenRoughnessTexture")) {
                mat.sheenRoughnessTexture = sh["sheenRoughnessTexture"].toObject()["index"].toInt(-1);
            }
        }
        // KHR_materials_anisotropy
        if (ext.contains("KHR_materials_anisotropy")) {
            QJsonObject an = ext["KHR_materials_anisotropy"].toObject();
            mat.anisotropyStrength = an["anisotropyStrength"].toDouble(0.0);
            mat.anisotropyRotation = an["anisotropyRotation"].toDouble(0.0);
            if (an.contains("anisotropyTexture")) {
                mat.anisotropyTexture = an["anisotropyTexture"].toObject()["index"].toInt(-1);
            }
        }
        // KHR_materials_volume
        if (ext.contains("KHR_materials_volume")) {
            QJsonObject vol = ext["KHR_materials_volume"].toObject();
            mat.thicknessFactor = vol["thicknessFactor"].toDouble(0.0);
            mat.attenuationDistance = vol["attenuationDistance"].toDouble(0.0);
            QJsonArray ac = vol["attenuationColor"].toArray();
            if (ac.size() == 3) {
                mat.attenuationColor = QVector3D(ac[0].toDouble(), ac[1].toDouble(), ac[2].toDouble());
            }
            if (vol.contains("thicknessTexture")) {
                mat.thicknessTexture = vol["thicknessTexture"].toObject()["index"].toInt(-1);
            }
        }
        // KHR_materials_emissive_strength
        if (ext.contains("KHR_materials_emissive_strength")) {
            QJsonObject es = ext["KHR_materials_emissive_strength"].toObject();
            mat.emissiveStrength = es["emissiveStrength"].toDouble(1.0);
        }
        
        m_gltf.materials.append(mat);
    }
    
    // Accessors
    QJsonArray accessorsArr = json["accessors"].toArray();
    for (const auto& v : accessorsArr) {
        GLTFData::Accessor acc;
        QJsonObject a = v.toObject();
        acc.bufferView = a["bufferView"].toInt(-1);
        acc.byteOffset = a["byteOffset"].toInt(0);
        acc.componentType = a["componentType"].toInt(5126); // FLOAT
        acc.count = a["count"].toInt(0);
        acc.type = a["type"].toString("VEC3");
        acc.normalized = a["normalized"].toBool(false);
        
        QJsonArray minArr = a["min"].toArray();
        for (const auto& m : minArr) acc.min.append(m.toDouble());
        QJsonArray maxArr = a["max"].toArray();
        for (const auto& m : maxArr) acc.max.append(m.toDouble());
        
        m_gltf.accessors.append(acc);
    }
    
    // Buffer Views
    QJsonArray viewsArr = json["bufferViews"].toArray();
    for (const auto& v : viewsArr) {
        GLTFData::BufferView view;
        QJsonObject bv = v.toObject();
        view.buffer = bv["buffer"].toInt(0);
        view.byteOffset = bv["byteOffset"].toInt(0);
        view.byteLength = bv["byteLength"].toInt(0);
        view.byteStride = bv["byteStride"].toInt(0);
        view.target = bv["target"].toInt(0);
        m_gltf.bufferViews.append(view);
    }
    
    // Buffers
    QJsonArray buffersArr = json["buffers"].toArray();
    for (const auto& v : buffersArr) {
        GLTFData::Buffer buf;
        QJsonObject b = v.toObject();
        buf.uri = b["uri"].toString();
        buf.byteLength = b["byteLength"].toInt(0);
        m_gltf.buffers.append(buf);
    }
    
    // Load external buffers if needed
    if (!m_isBinary) {
        for (size_t i = 0; i < m_gltf.buffers.size(); ++i) {
            if (!m_gltf.buffers[i].uri.isEmpty() && !m_gltf.buffers[i].uri.startsWith("data:")) {
                loadExternalBuffer(i);
            }
        }
    }
    
    // Textures
    QJsonArray texArr = json["textures"].toArray();
    for (const auto& v : texArr) {
        GLTFData::Texture tex;
        QJsonObject t = v.toObject();
        tex.sampler = t["sampler"].toInt(-1);
        tex.source = t["source"].toInt(-1);
        m_gltf.textures.append(tex);
    }
    
    // Images
    QJsonArray imgArr = json["images"].toArray();
    for (const auto& v : imgArr) {
        GLTFData::Image img;
        QJsonObject i = v.toObject();
        img.uri = i["uri"].toString();
        img.mimeType = i["mimeType"].toString();
        img.bufferView = i["bufferView"].toInt(-1);
        img.name = i["name"].toString();
        m_gltf.images.append(img);
    }
    
    // Samplers
    QJsonArray sampArr = json["samplers"].toArray();
    for (const auto& v : sampArr) {
        GLTFData::Sampler samp;
        QJsonObject s = v.toObject();
        samp.magFilter = s["magFilter"].toInt(9729);
        samp.minFilter = s["minFilter"].toInt(9987);
        samp.wrapS = s["wrapS"].toInt(10497);
        samp.wrapT = s["wrapT"].toInt(10497);
        m_gltf.samplers.append(samp);
    }
    
    // Skins
    QJsonArray skinsArr = json["skins"].toArray();
    for (const auto& v : skinsArr) {
        GLTFData::Skin skin;
        QJsonObject sk = v.toObject();
        skin.name = sk["name"].toString();
        skin.inverseBindMatrices = sk["inverseBindMatrices"].toInt(-1);
        skin.skeleton = sk["skeleton"].toInt(-1);
        QJsonArray jointsArr = sk["joints"].toArray();
        for (const auto& j : jointsArr) {
            skin.joints.append(j.toInt());
        }
        m_gltf.skins.append(skin);
    }
    
    // Animations
    QJsonArray animsArr = json["animations"].toArray();
    for (const auto& v : animsArr) {
        GLTFData::Animation anim;
        QJsonObject a = v.toObject();
        anim.name = a["name"].toString();
        
        QJsonArray channelsArr = a["channels"].toArray();
        for (const auto& c : channelsArr) {
            GLTFData::Animation::Channel ch;
            QJsonObject chObj = c.toObject();
            ch.targetNode = chObj["target"].toObject()["node"].toInt(-1);
            ch.targetPath = chObj["target"].toObject()["path"].toString();
            ch.sampler = chObj["sampler"].toInt(-1);
            anim.channels.append(ch);
        }
        
        QJsonArray samplersArr = a["samplers"].toArray();
        for (const auto& s : samplersArr) {
            GLTFData::Animation::AnimSampler samp;
            QJsonObject sObj = s.toObject();
            samp.input = sObj["input"].toInt(-1);
            samp.interpolation = sObj["interpolation"].toString("LINEAR");
            samp.output = sObj["output"].toInt(-1);
            anim.samplers.append(samp);
        }
        
        m_gltf.animations.append(anim);
    }
    
    // Cameras
    QJsonArray camsArr = json["cameras"].toArray();
    for (const auto& v : camsArr) {
        GLTFData::Camera cam;
        QJsonObject c = v.toObject();
        cam.name = c["name"].toString();
        cam.type = c["type"].toString("perspective");
        
        if (cam.type == "perspective") {
            QJsonObject persp = c["perspective"].toObject();
            cam.perspective.aspectRatio = persp["aspectRatio"].toDouble(1.0);
            cam.perspective.yfov = persp["yfov"].toDouble(0.785);
            cam.perspective.znear = persp["znear"].toDouble(0.1);
            cam.perspective.zfar = persp["zfar"].toDouble(1000.0);
        } else if (cam.type == "orthographic") {
            QJsonObject ortho = c["orthographic"].toObject();
            cam.orthographic.xmag = ortho["xmag"].toDouble(1.0);
            cam.orthographic.ymag = ortho["ymag"].toDouble(1.0);
            cam.orthographic.znear = ortho["znear"].toDouble(0.1);
            cam.orthographic.zfar = ortho["zfar"].toDouble(1000.0);
        }
        
        m_gltf.cameras.append(cam);
    }
    
    // Extensions used/required
    QJsonArray extUsed = json["extensionsUsed"].toArray();
    for (const auto& e : extUsed) m_gltf.extensionsUsed.append(e.toString());
    
    QJsonArray extReq = json["extensionsRequired"].toArray();
    for (const auto& e : extReq) m_gltf.extensionsRequired.append(e.toString());
    
    return true;
}

bool GLTFParser::loadExternalBuffer(size_t bufferIndex) {
    if (bufferIndex >= m_gltf.buffers.size()) return false;
    
    const QString& uri = m_gltf.buffers[bufferIndex].uri;
    if (uri.isEmpty() || uri.startsWith("data:")) return true;
    
    QString fullPath = m_baseDir + "/" + uri;
    QFile file(fullPath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot load buffer: " + fullPath;
        return false;
    }
    
    m_gltf.buffers[bufferIndex].data = file.readAll();
    file.close();
    return true;
}

const GLTFData& GLTFParser::gltf() const {
    return m_gltf;
}

bool GLTFParser::getMeshData(int meshIndex, MeshData& outData) const {
    if (meshIndex < 0 || meshIndex >= m_gltf.meshes.size()) return false;
    
    const GLTFData::Mesh& mesh = m_gltf.meshes[meshIndex];
    outData.name = mesh.name;
    
    // For simplicity, merge all primitives
    for (const auto& prim : mesh.primitives) {
        if (prim.attributes["POSITION"] < 0) continue;
        
        const GLTFData::Accessor& posAcc = m_gltf.accessors[prim.attributes["POSITION"]];
        const GLTFData::BufferView& posView = m_gltf.bufferViews[posAcc.bufferView];
        const GLTFData::Buffer& posBuf = m_gltf.buffers[posView.buffer];
        
        int posOffset = posView.byteOffset + posAcc.byteOffset;
        int posStride = posView.byteStride > 0 ? posView.byteStride : 12; // VEC3 = 12 bytes
        
        // Read positions
        for (int i = 0; i < posAcc.count; ++i) {
            const float* posPtr = reinterpret_cast<const float*>(posBuf.data.constData() + posOffset + i * posStride);
            MeshVertex v{};
            v.px = posPtr[0];
            v.py = posPtr[1];
            v.pz = posPtr[2];
            outData.vertices.append(v);
        }
        
        // Read normals
        if (prim.attributes["NORMAL"] >= 0) {
            const GLTFData::Accessor& normAcc = m_gltf.accessors[prim.attributes["NORMAL"]];
            const GLTFData::BufferView& normView = m_gltf.bufferViews[normAcc.bufferView];
            const GLTFData::Buffer& normBuf = m_gltf.buffers[normView.buffer];
            
            int normOffset = normView.byteOffset + normAcc.byteOffset;
            int normStride = normView.byteStride > 0 ? normView.byteStride : 12;
            
            for (int i = 0; i < normAcc.count && i < outData.vertices.size(); ++i) {
                const float* normPtr = reinterpret_cast<const float*>(normBuf.data.constData() + normOffset + i * normStride);
                outData.vertices[i].nx = normPtr[0];
                outData.vertices[i].ny = normPtr[1];
                outData.vertices[i].nz = normPtr[2];
            }
        }
        
        // Read UVs
        if (prim.attributes["TEXCOORD_0"] >= 0) {
            const GLTFData::Accessor& uvAcc = m_gltf.accessors[prim.attributes["TEXCOORD_0"]];
            const GLTFData::BufferView& uvView = m_gltf.bufferViews[uvAcc.bufferView];
            const GLTFData::Buffer& uvBuf = m_gltf.buffers[uvView.buffer];
            
            int uvOffset = uvView.byteOffset + uvAcc.byteOffset;
            int uvStride = uvView.byteStride > 0 ? uvView.byteStride : 8; // VEC2 = 8 bytes
            
            for (int i = 0; i < uvAcc.count && i < outData.vertices.size(); ++i) {
                const float* uvPtr = reinterpret_cast<const float*>(uvBuf.data.constData() + uvOffset + i * uvStride);
                outData.vertices[i].u0 = uvPtr[0];
                outData.vertices[i].v0 = uvPtr[1];
            }
        }
        
        // Read indices
        if (prim.indices >= 0) {
            const GLTFData::Accessor& idxAcc = m_gltf.accessors[prim.indices];
            const GLTFData::BufferView& idxView = m_gltf.bufferViews[idxAcc.bufferView];
            const GLTFData::Buffer& idxBuf = m_gltf.buffers[idxView.buffer];
            
            int idxOffset = idxView.byteOffset + idxAcc.byteOffset;
            int componentSize = (idxAcc.componentType == 5123) ? 2 : 4; // UNSIGNED_SHORT or UNSIGNED_INT
            
            for (int i = 0; i < idxAcc.count; ++i) {
                if (idxAcc.componentType == 5123) { // UNSIGNED_SHORT
                    const uint16_t* idxPtr = reinterpret_cast<const uint16_t*>(idxBuf.data.constData() + idxOffset + i * componentSize);
                    outData.indices.append(*idxPtr);
                } else { // UNSIGNED_INT
                    const uint32_t* idxPtr = reinterpret_cast<const uint32_t*>(idxBuf.data.constData() + idxOffset + i * componentSize);
                    outData.indices.append(*idxPtr);
                }
            }
        }
        
        // Material
        if (prim.material >= 0) {
            MeshSubmesh sub;
            sub.indexOffset = 0;
            sub.indexCount = static_cast<uint32_t>(outData.indices.size());
            sub.materialName = m_gltf.materials[prim.material].name;
            outData.submeshes.append(sub);
        }
    }
    
    return true;
}

bool GLTFParser::getMaterialData(int materialIndex, MaterialData& outMat) const {
    if (materialIndex < 0 || materialIndex >= m_gltf.materials.size()) return false;
    
    const GLTFData::Material& mat = m_gltf.materials[materialIndex];
    outMat.name = mat.name;
    outMat.diffuseColor = mat.baseColorFactor;
    outMat.metallic = mat.metallicFactor;
    outMat.roughness = mat.roughnessFactor;
    outMat.emissiveColor = QVector4D(mat.emissiveFactor.x(), mat.emissiveFactor.y(), mat.emissiveFactor.z(), 1.0f);
    outMat.opacity = mat.baseColorFactor.w();
    outMat.alphaMode = mat.alphaMode.toStdString();
    outMat.alphaCutoff = mat.alphaCutoff;
    outMat.doubleSided = mat.doubleSided;
    
    // Textures
    if (mat.baseColorTexture >= 0) {
        outMat.diffuseTexture = getTexturePath(mat.baseColorTexture);
    }
    if (mat.normalTexture >= 0) {
        outMat.normalTexture = getTexturePath(mat.normalTexture);
    }
    if (mat.metallicRoughnessTexture >= 0) {
        outMat.metallicRoughnessTexture = getTexturePath(mat.metallicRoughnessTexture);
    }
    if (mat.occlusionTexture >= 0) {
        outMat.occlusionTexture = getTexturePath(mat.occlusionTexture);
    }
    if (mat.emissiveTexture >= 0) {
        outMat.emissiveTexture = getTexturePath(mat.emissiveTexture);
    }
    
    // Extensions
    outMat.clearcoat = mat.clearcoatFactor;
    outMat.clearcoatRoughness = mat.clearcoatRoughnessFactor;
    outMat.transmission = mat.transmissionFactor;
    outMat.ior = mat.ior;
    outMat.specular = mat.specularFactor;
    outMat.specularColor = QVector3D(mat.specularColorFactor.x(), mat.specularColorFactor.y(), mat.specularColorFactor.z());
    outMat.sheenColor = QVector3D(mat.sheenColorFactor.x(), mat.sheenColorFactor.y(), mat.sheenColorFactor.z());
    outMat.sheenRoughness = mat.sheenRoughnessFactor;
    outMat.anisotropy = mat.anisotropyStrength;
    outMat.anisotropyRotation = mat.anisotropyRotation;
    outMat.thickness = mat.thicknessFactor;
    outMat.attenuationDistance = mat.attenuationDistance;
    outMat.attenuationColor = QVector3D(mat.attenuationColor.x(), mat.attenuationColor.y(), mat.attenuationColor.z());
    outMat.emissiveStrength = mat.emissiveStrength;
    
    return true;
}

QString GLTFParser::getTexturePath(int textureIndex) const {
    if (textureIndex < 0 || textureIndex >= m_gltf.textures.size()) return QString();
    
    const GLTFData::Texture& tex = m_gltf.textures[textureIndex];
    if (tex.source < 0 || tex.source >= m_gltf.images.size()) return QString();
    
    const GLTFData::Image& img = m_gltf.images[tex.source];
    if (!img.uri.isEmpty()) {
        if (img.uri.startsWith("data:")) {
            // Embedded data URI - would need to extract and save
            return QString("embedded:%1").arg(textureIndex);
        }
        return m_baseDir + "/" + img.uri;
    }
    
    return QString("bufferView:%1").arg(img.bufferView);
}

bool GLTFParser::getAnimationData(int animIndex, AnimationData& outAnim) const {
    if (animIndex < 0 || animIndex >= m_gltf.animations.size()) return false;
    
    const GLTFData::Animation& anim = m_gltf.animations[animIndex];
    outAnim.name = anim.name;
    
    // Build animation curves from samplers
    for (const auto& channel : anim.channels) {
        if (channel.sampler < 0 || channel.sampler >= anim.samplers.size()) continue;
        
        const GLTFData::Animation::AnimSampler& samp = anim.samplers[channel.sampler];
        
        // Get input (time) accessor
        if (samp.input < 0 || samp.input >= m_gltf.accessors.size()) continue;
        const GLTFData::Accessor& inputAcc = m_gltf.accessors[samp.input];
        const GLTFData::BufferView& inputView = m_gltf.bufferViews[inputAcc.bufferView];
        const GLTFData::Buffer& inputBuf = m_gltf.buffers[inputView.buffer];
        
        // Get output accessor
        if (samp.output < 0 || samp.output >= m_gltf.accessors.size()) continue;
        const GLTFData::Accessor& outputAcc = m_gltf.accessors[samp.output];
        const GLTFData::BufferView& outputView = m_gltf.bufferViews[outputAcc.bufferView];
        const GLTFData::Buffer& outputBuf = m_gltf.buffers[outputView.buffer];
        
        AnimationData::Curve curve;
        curve.targetNode = channel.targetNode;
        curve.targetPath = channel.targetPath;
        curve.interpolation = samp.interpolation.toStdString();
        
        int inputOffset = inputView.byteOffset + inputAcc.byteOffset;
        int outputOffset = outputView.byteOffset + outputAcc.byteOffset;
        int inputStride = inputView.byteStride > 0 ? inputView.byteStride : 4;
        int outputStride = outputView.byteStride > 0 ? outputView.byteStride : 16; // VEC4 = 16 bytes
        
        for (int i = 0; i < inputAcc.count; ++i) {
            const float* timePtr = reinterpret_cast<const float*>(inputBuf.data.constData() + inputOffset + i * inputStride);
            const float* valuePtr = reinterpret_cast<const float*>(outputBuf.data.constData() + outputOffset + i * outputStride);
            
            AnimationData::Keyframe kf;
            kf.time = *timePtr;
            
            if (channel.targetPath == "translation") {
                kf.value = QVector3D(valuePtr[0], valuePtr[1], valuePtr[2]);
            } else if (channel.targetPath == "rotation") {
                kf.value = QQuaternion(valuePtr[3], valuePtr[0], valuePtr[1], valuePtr[2]);
            } else if (channel.targetPath == "scale") {
                kf.value = QVector3D(valuePtr[0], valuePtr[1], valuePtr[2]);
            } else if (channel.targetPath == "weights") {
                // Morph target weights
                QVector<float> weights;
                int weightCount = outputAcc.count / inputAcc.count;
                for (int w = 0; w < weightCount; ++w) {
                    weights.append(valuePtr[w]);
                }
                kf.value = QVariant::fromValue(weights);
            }
            
            curve.keyframes.append(kf);
        }
        
        if (!curve.keyframes.isEmpty()) {
            outAnim.curves.append(curve);
        }
    }
    
    return !outAnim.curves.isEmpty();
}

bool GLTFParser::getSkinData(int skinIndex, SkinData& outSkin) const {
    if (skinIndex < 0 || skinIndex >= m_gltf.skins.size()) return false;
    
    const GLTFData::Skin& skin = m_gltf.skins[skinIndex];
    outSkin.name = skin.name;
    outSkin.joints = skin.joints;
    outSkin.skeletonRoot = skin.skeleton;
    
    if (skin.inverseBindMatrices >= 0) {
        const GLTFData::Accessor& ibmAcc = m_gltf.accessors[skin.inverseBindMatrices];
        const GLTFData::BufferView& ibmView = m_gltf.bufferViews[ibmAcc.bufferView];
        const GLTFData::Buffer& ibmBuf = m_gltf.buffers[ibmView.buffer];
        
        int offset = ibmView.byteOffset + ibmAcc.byteOffset;
        for (int i = 0; i < ibmAcc.count; ++i) {
            const float* matPtr = reinterpret_cast<const float*>(ibmBuf.data.constData() + offset + i * 64); // 4x4 matrix = 64 bytes
            QMatrix4x4 mat;
            for (int r = 0; r < 4; ++r) {
                for (int c = 0; c < 4; ++c) {
                    mat(r, c) = matPtr[r * 4 + c];
                }
            }
            outSkin.inverseBindMatrices.append(mat);
        }
    }
    
    return true;
}

QString GLTFParser::lastError() const {
    return m_lastError;
}

// ─── Writer ──────────────────────────────────────────────────────────────────

bool GLTFWriter::writeToFile(const QString& path, const MeshData& mesh, const MaterialData* material) {
    GLTFData gltf;
    
    // Asset
    gltf.asset.generator = "ksEditor GLTF Writer";
    gltf.asset.version = "2.0";
    
    // Scene
    GLTFData::Scene scene;
    scene.name = "Scene";
    scene.nodes.append(0);
    gltf.scenes.append(scene);
    gltf.scene = 0;
    
    // Node
    GLTFData::Node node;
    node.name = mesh.name.isEmpty() ? "Mesh" : mesh.name;
    node.mesh = 0;
    gltf.nodes.append(node);
    
    // Mesh
    GLTFData::Mesh gltfMesh;
    gltfMesh.name = node.name;
    
    GLTFData::Primitive prim;
    prim.mode = 4; // TRIANGLES
    prim.attributes["POSITION"] = 0;
    
    // Will set indices, normals, uvs after creating accessors
    
    if (material) {
        // Add material
        GLTFData::Material mat;
        mat.name = material->name.isEmpty() ? "Material" : material->name;
        mat.baseColorFactor = material->diffuseColor;
        mat.metallicFactor = material->metallic;
        mat.roughnessFactor = material->roughness;
        mat.emissiveFactor = QVector3D(material->emissiveColor.x(), material->emissiveColor.y(), material->emissiveColor.z());
        mat.alphaMode = QString::fromStdString(material->alphaMode);
        mat.alphaCutoff = material->alphaCutoff;
        mat.doubleSided = material->doubleSided;
        
        gltf.materials.append(mat);
        prim.material = 0;
    }
    
    gltfMesh.primitives.append(prim);
    gltf.meshes.append(gltfMesh);
    
    // Create accessors and buffer views for vertex data
    // This is a simplified version - real implementation would be more complete
    
    return writeGLTF(path, gltf);
}

bool GLTFWriter::writeToFile(const QString& path, const SceneGraph& scene) {
    // Full scene export - would traverse scene graph
    return false; // Not implemented yet
}

bool GLTFWriter::writeGLTF(const QString& path, const GLTFData& gltf) {
    QJsonObject root;
    
    // Asset
    QJsonObject asset;
    asset["copyright"] = gltf.asset.copyright;
    asset["generator"] = gltf.asset.generator;
    asset["version"] = gltf.asset.version;
    asset["minVersion"] = gltf.asset.minVersion;
    root["asset"] = asset;
    
    root["scene"] = gltf.scene;
    
    // Scenes
    QJsonArray scenesArr;
    for (const auto& s : gltf.scenes) {
        QJsonObject scene;
        scene["name"] = s.name;
        QJsonArray nodesArr;
        for (int n : s.nodes) nodesArr.append(n);
        scene["nodes"] = nodesArr;
        scenesArr.append(scene);
    }
    root["scenes"] = scenesArr;
    
    // Nodes
    QJsonArray nodesArr;
    for (const auto& n : gltf.nodes) {
        QJsonObject node;
        node["name"] = n.name;
        if (n.mesh >= 0) node["mesh"] = n.mesh;
        if (n.skin >= 0) node["skin"] = n.skin;
        if (n.camera >= 0) node["camera"] = n.camera;
        
        if (n.hasMatrix) {
            QJsonArray matArr;
            for (int i = 0; i < 16; ++i) matArr.append(n.matrix(i / 4, i % 4));
            node["matrix"] = matArr;
        } else {
            if (!n.translation.isNull()) node["translation"] = QJsonArray{n.translation.x(), n.translation.y(), n.translation.z()};
            if (!n.rotation.isIdentity()) node["rotation"] = QJsonArray{n.rotation.x(), n.rotation.y(), n.rotation.z(), n.rotation.scalar()};
            if (n.scale != QVector3D(1,1,1)) node["scale"] = QJsonArray{n.scale.x(), n.scale.y(), n.scale.z()};
        }
        
        if (!n.children.isEmpty()) {
            QJsonArray childrenArr;
            for (int c : n.children) childrenArr.append(c);
            node["children"] = childrenArr;
        }
        
        nodesArr.append(node);
    }
    root["nodes"] = nodesArr;
    
    // Meshes, materials, accessors, bufferViews, buffers - simplified
    // Full implementation would serialize all data with proper binary buffers
    
    QJsonDocument doc(root);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool GLTFWriter::writeGLB(const QString& path, const GLTFData& gltf) {
    // Binary GLTF - would need to serialize JSON + binary buffer
    return writeGLTF(path, gltf); // Simplified
}

} // namespace fileformat
} // namespace ks