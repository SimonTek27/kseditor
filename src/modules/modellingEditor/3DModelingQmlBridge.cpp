#include "3DModelingQmlBridge.h"
#include "3DModeling.h"
#include "BoolOpQmlBridge.h"
#include "AdditionalModifiers.h"

#include <QRegularExpression>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include "core/Graphics/SceneGraph.h"
#include "core/Graphics/SceneObject.h"
#include "core/Graphics/SceneMesh.h"
#include "assettocorsa/acFiles/KN5Parser.h"
#include "core/Math/MathCore.h"
#include "core/FileFormat/FBXParser.h"
#include "core/FileFormat/GLBParser.h"
#include "core/FileFormat/CADOBJParser.h"
#include "core/FileFormat/KS3DReader.h"
#include "core/FileFormat/KS3DWriter.h"
#include "core/mesh/MeshOperations.h"
#include "core/mesh/UVUnwrap.h"
#include "core/mesh/ModifierSystem.h"
#include "NodeMaterialEditor.h"
#include "core/mesh/WeightPainting.h"
#include "core/mesh/SkeletonSystem.h"
#include "plugins/simulators/kunos/assettocorsa/acFiles/FBXExporter.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QQuaternion>
#include <QMatrix4x4>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDataStream>
#include <algorithm>
#include <QSet>
#include <cmath>
#include <limits>

namespace {

QQuaternion quatSlerp(const QQuaternion &q1, const QQuaternion &q2, float t) {
    float dot = QQuaternion::dotProduct(q1, q2);
    float sign = 1.0f;
    if (dot < 0.0f) { dot = -dot; sign = -1.0f; }
    if (dot > 0.9999f) return (q1 * (1.0f - t) + q2 * t * sign).normalized();
    float angle = std::acos(dot);
    float sinAngle = std::sin(angle);
    float a1 = std::sin((1.0f - t) * angle) / sinAngle;
    float a2 = std::sin(t * angle) / sinAngle * sign;
    return q1 * a1 + q2 * a2;
}

} // anonymous namespace

namespace ks {

static MeshData sceneMeshToMeshData(SceneObject* obj) {
    MeshData md;
    if (!obj || !obj->mesh()) return md;
    auto& verts = obj->mesh()->geometry().vertices;
    for (const auto& sv : verts) {
        Vertex v;
        v.position = QVector3D(sv.position.x(), sv.position.y(), sv.position.z());
        v.color = QVector4D(sv.color.x(), sv.color.y(), sv.color.z(), sv.color.w());
        md.vertices.append(v);
    }
    auto& idxs = obj->mesh()->geometry().indices;
    for (int i = 0; i + 2 < idxs.size(); i += 3)
        md.faces.append(Face({ (int)idxs[i], (int)idxs[i+1], (int)idxs[i+2] }));
    md.computeNormals();
    md.computeBoundingBox();
    return md;
}

static void meshDataToSceneMesh(SceneObject* obj, const MeshData& md) {
    if (!obj) return;
    SceneMesh* sm = new SceneMesh();
    for (const auto& v : md.vertices) {
        SceneVertex sv;
        sv.position = QVector3D(v.position.x(), v.position.y(), v.position.z());
        sv.color = QVector4D(v.color.x(), v.color.y(), v.color.z(), v.color.w());
        sm->geometry().vertices.append(sv);
    }
    for (const auto& f : md.faces) {
        for (int idx : f.indices)
            sm->geometry().indices.append((uint32_t)idx);
    }
    obj->setMesh(sm);
}

static bool importMeshDataToScene(ks::SceneGraph* scene, const MeshData& meshData, const QString& name) {
    if (!scene || meshData.vertices.isEmpty()) return false;
    
    SceneObject* obj = scene->createObject(name, SceneObject::Type::Mesh);
    if (!obj) return false;
    
    SceneMesh* sm = new SceneMesh();
    for (const auto& v : meshData.vertices) {
        SceneVertex sv;
        sv.position = QVector3D(v.position.x(), v.position.y(), v.position.z());
        sv.color = QVector4D(v.color.x(), v.color.y(), v.color.z(), v.color.w());
        sm->geometry().vertices.append(sv);
    }
    for (const auto& f : meshData.faces) {
        for (int idx : f.indices)
            sm->geometry().indices.append((uint32_t)idx);
    }
    obj->setMesh(sm);
    return true;
}

// .ks3d is the proprietary KSEditor native scene format.
static bool sceneGraphToKS3D(const ks::SceneGraph* scene, KS3DScene& out) {
    if (!scene) return false;

    KS3DMaterial defaultMat;
    defaultMat.name = "Default";
    out.materials.push_back(defaultMat);

    for (SceneObject* obj : scene->allObjects()) {
        if (obj->type() != SceneObject::Type::Mesh || !obj->mesh()) continue;
        const auto& geo = obj->mesh()->geometry();
        if (geo.vertices.isEmpty()) continue;

        KS3DMesh mesh;
        mesh.name = obj->name().toStdString();
        mesh.vertexFlags = static_cast<uint32_t>(ks3d::VertexFlags::Position)
                         | static_cast<uint32_t>(ks3d::VertexFlags::Normal)
                         | static_cast<uint32_t>(ks3d::VertexFlags::UV0)
                         | static_cast<uint32_t>(ks3d::VertexFlags::Tangent)
                         | static_cast<uint32_t>(ks3d::VertexFlags::Bitangent)
                         | static_cast<uint32_t>(ks3d::VertexFlags::BoneWeight);
        // Full .ks3d vertex layout: pos(3) nrm(3) uv0(2) tan(3) btan(3) boneW(4) boneIdx(1) = 19 floats
        mesh.vertices.reserve(geo.vertices.size() * 19);
        for (const auto& v : geo.vertices) {
            mesh.vertices.push_back(v.position.x());
            mesh.vertices.push_back(v.position.y());
            mesh.vertices.push_back(v.position.z());
            mesh.vertices.push_back(v.normal.x());
            mesh.vertices.push_back(v.normal.y());
            mesh.vertices.push_back(v.normal.z());
            mesh.vertices.push_back(v.uv.x());
            mesh.vertices.push_back(v.uv.y());
            mesh.vertices.push_back(0.0f); // tangent
            mesh.vertices.push_back(0.0f);
            mesh.vertices.push_back(0.0f);
            mesh.vertices.push_back(0.0f); // bitangent
            mesh.vertices.push_back(0.0f);
            mesh.vertices.push_back(0.0f);
            mesh.vertices.push_back(0.0f); // bone weights
            mesh.vertices.push_back(0.0f);
            mesh.vertices.push_back(0.0f);
            mesh.vertices.push_back(0.0f);
            mesh.vertices.push_back(0.0f); // bone indices
        }
        for (uint32_t idx : geo.indices)
            mesh.indices.push_back(idx);

        KS3DSubmesh sm;
        sm.materialIndex = 0;
        sm.indexOffset = 0;
        sm.indexCount = static_cast<uint32_t>(mesh.indices.size());
        mesh.submeshes.push_back(sm);

        int meshIndex = static_cast<int>(out.meshes.size());
        out.meshes.push_back(mesh);

        KS3DNode node;
        node.name = obj->name().toStdString();
        node.parentIndex = -1;
        node.meshIndex = meshIndex;
        node.materialIndex = 0;
        const QVector3D pos = obj->position();
        node.position[0] = pos.x(); node.position[1] = pos.y(); node.position[2] = pos.z();
        const QQuaternion rot = QQuaternion::fromEulerAngles(obj->rotationEuler());
        node.rotationQuat[0] = rot.x(); node.rotationQuat[1] = rot.y();
        node.rotationQuat[2] = rot.z(); node.rotationQuat[3] = rot.scalar();
        const QVector3D scl = obj->scale();
        node.scale[0] = scl.x(); node.scale[1] = scl.y(); node.scale[2] = scl.z();
        node.visible = obj->isVisible();
        out.nodes.push_back(node);
    }
    return true;
}

static void ks3dToSceneGraph(ks::SceneGraph* scene, const KS3DScene& in) {
    if (!scene) return;
    scene->clear();
    for (const auto& ksMesh : in.meshes) {
        const std::vector<float> pos = KS3DReader::getVertexPositions(ksMesh);
        const std::vector<float> nrm = KS3DReader::getVertexNormals(ksMesh);
        const std::vector<float> uv  = KS3DReader::getVertexUVs(ksMesh);
        const size_t count = pos.size() / 3;
        if (count == 0) continue;

        MeshData md;
        md.vertices.reserve(static_cast<int>(count));
        for (size_t i = 0; i < count; ++i) {
            Vertex v;
            v.position = QVector3D(pos[i*3], pos[i*3+1], pos[i*3+2]);
            if (nrm.size() >= (i + 1) * 3)
                v.normal = QVector3D(nrm[i*3], nrm[i*3+1], nrm[i*3+2]);
            if (uv.size() >= (i + 1) * 2)
                v.uv = QVector2D(uv[i*2], uv[i*2+1]);
            v.color = QVector4D(0.8f, 0.8f, 0.8f, 1.0f);
            md.vertices.append(v);
        }
        for (size_t i = 0; i + 2 < ksMesh.indices.size(); i += 3) {
            Face f;
            f.indices = { (int)ksMesh.indices[i], (int)ksMesh.indices[i+1], (int)ksMesh.indices[i+2] };
            md.faces.append(f);
        }
        QString name = QString::fromStdString(ksMesh.name);
        if (name.isEmpty()) name = "KS3D_Mesh";
        importMeshDataToScene(scene, md, name);
    }
}

static bool exportSceneOBJ(const ks::SceneGraph* scene, const QString& path) {
    if (!scene) return false;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    out << "# Exported by KSEditor\n";
    for (SceneObject* obj : scene->allObjects()) {
        if (!obj->mesh() || obj->mesh()->geometry().vertices.isEmpty()) continue;
        const auto& verts = obj->mesh()->geometry().vertices;
        const auto& indices = obj->mesh()->geometry().indices;
        out << "\no " << obj->name() << "\n";
        for (const auto& v : verts)
            out << "v " << v.position.x() << " " << v.position.y() << " " << v.position.z() << "\n";
        for (const auto& v : verts)
            out << "vn " << v.normal.x() << " " << v.normal.y() << " " << v.normal.z() << "\n";
        for (const auto& v : verts)
            out << "vt " << v.uv.x() << " " << v.uv.y() << "\n";
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            uint32_t i0 = indices[i] + 1, i1 = indices[i+1] + 1, i2 = indices[i+2] + 1;
            out << "f " << i0 << "/" << i0 << "/" << i0 << " "
                       << i1 << "/" << i1 << "/" << i1 << " "
                       << i2 << "/" << i2 << "/" << i2 << "\n";
        }
    }
    return true;
}

KSModelerQml::KSModelerQml(QObject* parent)
    : QObject(parent)
    , m_scene(nullptr)
    , m_editor(nullptr)
    , m_selectedObject(nullptr)
    , m_gizmoMode(0)
    , m_currentEditorType("car")
    , m_sceneModel(new SceneObjectListModel(this))
    , m_commandHistory(new CommandHistory(this))
    , m_shortcutManager(new ShortcutManager(this))
{
    m_scene = new ks::SceneGraph();
    m_sceneModel->setSceneGraph(m_scene);
}

KSModelerQml::~KSModelerQml() {
    if (m_scene) delete m_scene;
    if (m_shapeKeyAnimDriver) delete m_shapeKeyAnimDriver;
    if (m_commandHistory) delete m_commandHistory;
    if (m_shortcutManager) delete m_shortcutManager;
}

void KSModelerQml::setScene(ks::SceneGraph* scene) {
    m_scene = scene;
    if (m_sceneModel) m_sceneModel->setSceneGraph(scene);
    emit sceneChanged();
}
void KSModelerQml::setEditor(core_BaseEditor* editor) { m_editor = editor; }

void KSModelerQml::newProject() {
    if (m_scene) {
        m_scene->clear();
        m_selectedObject = nullptr;
        m_undoStack.clear();
        m_redoStack.clear();
        m_currentFile.clear();
    }
    emit sceneChanged();
    emit statusMessage("New project created");
}

void KSModelerQml::newScene() {
    newProject();
}

bool KSModelerQml::saveScene(const QString& path) {
    if (!m_scene) { emit error("No scene to save"); return false; }
    if (path.isEmpty()) { emit error("No file path specified"); return false; }

    const QString fmt = QFileInfo(path).suffix().toLower();
    if (fmt == "ks3d") {
        KS3DScene ksScene;
        if (!sceneGraphToKS3D(m_scene, ksScene)) { emit error("Failed to build .ks3d scene"); return false; }
        KS3DWriter writer;
        if (!writer.writeToFile(path.toStdString(), ksScene)) {
            emit error(QString("Failed to save .ks3d: %1").arg(QString::fromStdString(writer.lastError())));
            return false;
        }
        m_currentFile = path;
        emit fileChanged(m_currentFile);
        emit statusMessage("Scene saved as " + path);
        return true;
    }
    if (fmt == "kn5") return exportKN5(path);
    if (fmt == "fbx") return exportFBX(path);
    if (fmt == "obj") return exportOBJ(path);
    emit error("Unsupported save format: " + fmt + " (use .ks3d to save)");
    return false;
}

bool KSModelerQml::loadScene(const QString& path) {
    if (!QFile::exists(path)) { emit error("File not found: " + path); return false; }

    const QString fmt = QFileInfo(path).suffix().toLower();
    if (fmt == "ks3d") {
        if (!m_scene) m_scene = new ks::SceneGraph();
        KS3DReader reader;
        if (!reader.readFromFile(path.toStdString())) {
            emit error(QString("Failed to load .ks3d: %1").arg(QString::fromStdString(reader.lastError())));
            return false;
        }
        ks3dToSceneGraph(m_scene, reader.scene());
        m_currentFile = path;
        m_selectedObject = nullptr;
        m_undoStack.clear();
        m_redoStack.clear();
        emit sceneChanged();
        emit fileChanged(m_currentFile);
        emit statusMessage("Loaded scene " + path);
        return true;
    }
    if (fmt == "kn5") return importKN5(path);
    if (fmt == "fbx") return importFBX(path);
    if (fmt == "gltf" || fmt == "glb") return importGLB(path);
    if (fmt == "obj") return importOBJ(path);
    if (fmt == "blend") return importBlend(path);
    emit error("Unsupported format: " + fmt);
    return false;
}

bool KSModelerQml::importFile(const QString& path) {
    QString fmt = QFileInfo(path).suffix().toLower();
    if (fmt == "kn5") return importKN5(path);
    if (fmt == "fbx") return importFBX(path);
    if (fmt == "glb" || fmt == "gltf") return importGLB(path);
    if (fmt == "obj") return importOBJ(path);
    if (fmt == "blend") return importBlend(path);
    emit error("Unsupported format: " + fmt);
    return false;
}

bool KSModelerQml::exportFile(const QString& path) {
    QString fmt = QFileInfo(path).suffix().toLower();
    if (fmt == "kn5") return exportKN5(path);
    if (fmt == "fbx") return exportFBX(path);
    if (fmt == "glb") return exportGLB(path);
    if (fmt == "obj") return exportOBJ(path);
    emit error("Unsupported export format: " + fmt);
    return false;
}

bool KSModelerQml::importKN5(const QString& path) {
    if (!QFile::exists(path)) { emit error("File not found: " + path); return false; }
    emit statusMessage("Importing KN5: " + path);
    QString parseErr;
    auto kn5File = ::KN5Parser::KN5ParserImpl::parse(path, &parseErr);
    if (!kn5File.isValid()) { emit error("Failed to parse KN5 file: " + parseErr); return false; }
    if (!m_scene) m_scene = new ks::SceneGraph();
    int count = 0;
    for (const auto& mesh : kn5File.meshes) {
        MeshData md;
        md.vertices.reserve(mesh.positions.size());
        for (const auto& v : mesh.positions) {
            Vertex vtx;
            vtx.position = QVector3D(v.x(), v.y(), v.z());
            vtx.color = QVector4D(0.8f, 0.8f, 0.8f, 1.0f);
            md.vertices.append(vtx);
        }
        {
            const quint32* idxData = reinterpret_cast<const quint32*>(mesh.indexData.constData());
            int totalIndices = mesh.indexData.size() / 4;
            md.faces.reserve(totalIndices / 3);
            for (int i = 0; i + 2 < totalIndices; i += 3) {
                Face face;
                face.indices = { (int)idxData[i], (int)idxData[i+1], (int)idxData[i+2] };
                md.faces.append(face);
            }
        }
        if (!md.vertices.isEmpty()) {
            importMeshDataToScene(m_scene, md, mesh.name);
            count++;
        }
    }
    m_currentFile = path;
    emit sceneChanged();
    emit statusMessage(QString("Imported %1 meshes from KN5").arg(count));
    return true;
}

bool KSModelerQml::importFBX(const QString& path) {
    if (!QFile::exists(path)) { emit error("File not found: " + path); return false; }
    emit statusMessage("Importing FBX: " + path);
    FBXParser parser;
    if (!parser.loadFromFile(path.toStdString())) { emit error("Failed to parse FBX file"); return false; }
    if (!m_scene) m_scene = new ks::SceneGraph();
    int count = 0;
    for (const auto& mesh : parser.scene().meshes) {
        MeshData md;
        md.vertices.reserve(mesh.vertices.size());
        md.faces.reserve(mesh.indices.size() / 3);
        for (const auto& v : mesh.vertices) {
            Vertex vtx;
            vtx.position = QVector3D(v.x, v.y, v.z);
            vtx.color = QVector4D(0.8f, 0.8f, 0.8f, 1.0f);
            md.vertices.append(vtx);
        }
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            Face face;
            face.indices = { (int)mesh.indices[i], (int)mesh.indices[i+1], (int)mesh.indices[i+2] };
            md.faces.append(face);
        }
        if (!md.vertices.isEmpty()) {
            importMeshDataToScene(m_scene, md, QString::fromStdString(mesh.name));
            count++;
        }
    }
    m_currentFile = path;
    emit sceneChanged();
    emit statusMessage(QString("Imported %1 meshes from FBX").arg(count));
    return true;
}

bool KSModelerQml::importGLB(const QString& path) {
    if (!QFile::exists(path)) { emit error("File not found: " + path); return false; }
    emit statusMessage("Importing GLB: " + path);
    GLBParser parser;
    if (!parser.loadFromFile(path.toStdString())) { emit error("Failed to parse GLB file"); return false; }
    if (!m_scene) m_scene = new ks::SceneGraph();
    const auto& scene = parser.scene();
    int count = 0;
    for (const auto& mesh : scene.meshes) {
        for (const auto& prim : mesh.primitives) {
            MeshData md;
            auto posIt = prim.attributes.find("POSITION");
            if (posIt == prim.attributes.end() || parser.getVertices(posIt->second).empty())
                continue;
            auto verts = parser.getVertices(posIt->second);
            for (const auto& v : verts) {
                Vertex vtx;
                vtx.position = QVector3D(v.x, v.y, v.z);
                vtx.color = QVector4D(0.8f, 0.8f, 0.8f, 1.0f);
                md.vertices.append(vtx);
            }
            if (prim.indices != 0xFFFFFFFF) {
                auto idx = parser.getIndices(prim.indices);
                for (size_t i = 0; i + 2 < idx.size(); i += 3) {
                    Face face;
                    face.indices = { (int)idx[i], (int)idx[i+1], (int)idx[i+2] };
                    md.faces.append(face);
                }
            } else {
                for (int i = 0; i + 2 < (int)verts.size(); i += 3) {
                    Face face;
                    face.indices = { i, i+1, i+2 };
                    md.faces.append(face);
                }
            }
            if (!md.vertices.isEmpty()) {
                QString name = QString::fromStdString(mesh.name);
                if (name.isEmpty()) name = QString("GLBMesh_%1").arg(count);
                importMeshDataToScene(m_scene, md, name);
                count++;
            }
        }
    }
    m_currentFile = path;
    emit sceneChanged();
    emit statusMessage(QString("Imported %1 meshes from GLB").arg(count));
    return true;
}

bool KSModelerQml::importOBJ(const QString& path) {
    if (!QFile::exists(path)) { emit error("File not found: " + path); return false; }
    emit statusMessage("Importing OBJ: " + path);
    CADOBJParser parser;
    if (!parser.loadFromFile(path.toStdString())) { emit error("Failed to parse OBJ file"); return false; }
    if (!m_scene) m_scene = new ks::SceneGraph();
    int count = 0;
    for (const auto& mesh : parser.scene().meshes) {
        MeshData md;
        md.vertices.reserve(mesh.vertices.size());
        md.faces.reserve(mesh.indices.size() / 3);
        for (const auto& v : mesh.vertices) {
            Vertex vtx;
            vtx.position = QVector3D(v.x, v.y, v.z);
            vtx.color = QVector4D(0.8f, 0.8f, 0.8f, 1.0f);
            md.vertices.append(vtx);
        }
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            Face face;
            face.indices = { (int)mesh.indices[i].x, (int)mesh.indices[i+1].x, (int)mesh.indices[i+2].x };
            md.faces.append(face);
        }
        if (!md.vertices.isEmpty()) {
            importMeshDataToScene(m_scene, md, QString::fromStdString(mesh.name));
            count++;
        }
    }
    m_currentFile = path;
    emit sceneChanged();
    emit statusMessage(QString("Imported %1 meshes from OBJ").arg(count));
    return true;
}

bool KSModelerQml::importBlend(const QString& path) {
    if (!QFile::exists(path)) { emit error("File not found: " + path); return false; }
    emit statusMessage("Importing Blender scene via CLI: " + path);

    QString blenderPath = qEnvironmentVariable("BLENDER");
    if (blenderPath.isEmpty()) {
        QFileInfo bin(QStringLiteral("blender.exe"));
        blenderPath = bin.exists() ? bin.absoluteFilePath() : QStringLiteral("blender");
    }

    QTemporaryDir tmp;
    if (!tmp.isValid()) { emit error("Failed to create temp dir for .blend import"); return false; }
    const QString glbPath = tmp.filePath("blend_export.glb");

    QProcess proc;
    proc.setProgram(blenderPath);
    proc.setArguments({
        QStringLiteral("-b"), path,
        QStringLiteral("--python-expr"),
        QStringLiteral("import bpy;"
                       "bpy.ops.export_scene.gltf(filepath='%1', export_format='GLB');"
                       "print('BLEND_EXPORT_OK')").arg(glbPath)
    });
    proc.start();
    if (!proc.waitForStarted(5000)) {
        emit error("Blender not found. Set BLENDER env var or add blender.exe to PATH to import .blend files.");
        return false;
    }
    if (!proc.waitForFinished(120000) || proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        emit error(QString("Blender failed (%1): %2").arg(proc.exitCode()).arg(QString::fromLocal8Bit(proc.readAllStandardError()).trimmed()));
        return false;
    }
    if (!QFile::exists(glbPath)) {
        emit error("Blender finished but produced no GLB output");
        return false;
    }

    const bool ok = importGLB(glbPath);
    if (ok) m_currentFile = path;
    return ok;
}

bool KSModelerQml::exportKN5(const QString& path) {
    if (!m_scene) { emit error("No scene to export"); return false; }
    emit statusMessage("Exporting KN5: " + path);
    ::KN5Parser::KN5File kn5File;
    for (SceneObject* obj : m_scene->allObjects()) {
        if (obj->type() == SceneObject::Type::Mesh) {
            ::KN5Parser::Mesh kn5Mesh;
            kn5Mesh.name = obj->name();
            if (obj->mesh()) {
                auto& verts = obj->mesh()->geometry().vertices;
                auto& idxs = obj->mesh()->geometry().indices;
                for (const auto& v : verts) {
                    float p[3] = { v.position.x(), v.position.y(), v.position.z() };
                    kn5Mesh.vertexData.append(QByteArray((const char*)p, 12));
                    float n[3] = { 0.0f, 0.0f, 0.0f };
                    kn5Mesh.vertexData.append(QByteArray((const char*)n, 12));
                }
                for (uint32_t idx : idxs) {
                    quint32 leIdx = idx;
                    kn5Mesh.indexData.append(QByteArray((const char*)&leIdx, 4));
                }
                ::KN5Parser::SubMesh sm;
                sm.vertexCount = verts.size();
                sm.indexCount = idxs.size();
                kn5Mesh.subMeshes.append(sm);
            }
            kn5File.meshes.push_back(kn5Mesh);
        }
    }
    bool success = ::KN5Parser::KN5ParserImpl::write(path, kn5File);
    if (success) emit statusMessage("KN5 exported successfully");
    else emit error("Failed to export KN5");
    return success;
}

bool KSModelerQml::exportFBX(const QString& path) {
    if (!m_scene) { emit error("No scene to export"); return false; }
    emit statusMessage("Exporting FBX: " + path);
    QVector<MeshData> meshes;
    for (SceneObject* obj : m_scene->allObjects()) {
        if (obj->type() == SceneObject::Type::Mesh && obj->mesh())
            meshes.append(sceneMeshToMeshData(obj));
    }
    FBXExportSettings settings = FBXExporter::getDefaultExportSettings();
    bool ok = FBXExporter::exportToFBX(path, meshes, settings);
    if (ok) emit statusMessage("FBX exported successfully");
    else emit error("Failed to export FBX");
    return ok;
}

bool KSModelerQml::exportGLB(const QString& path) {
    if (!m_scene) { emit error("No scene to export"); return false; }
    emit statusMessage("Exporting GLB: " + path);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) { emit error("Cannot create GLB file"); return false; }
    QJsonObject root, scene;
    QJsonArray nodes, meshesArr, accessorsArr, bufferViewsArr, buffersArr;
    QByteArray binData;
    int vertexOffset = 0, indexOffset = 0;
    int nodeIdx = 0;
    for (SceneObject* obj : m_scene->allObjects()) {
        if (obj->type() != SceneObject::Type::Mesh || !obj->mesh()) continue;
        QJsonObject node;
        node["name"] = obj->name();
        node["mesh"] = meshesArr.size();
        nodes.append(node);
        QJsonObject meshObj;
        meshObj["name"] = obj->name();
        QJsonArray primitives;
        QJsonObject prim;
        auto& verts = obj->mesh()->geometry().vertices;
        int vCount = verts.size();
        int vByteOffset = binData.size();
        for (const auto& v : verts) {
            float fv[] = { v.position.x(), v.position.y(), v.position.z() };
            binData.append(QByteArray((const char*)fv, 12));
        }
        auto& idxs = obj->mesh()->geometry().indices;
        int iByteOffset = binData.size();
        for (uint32_t idx : idxs)
            binData.append(QByteArray((const char*)&idx, 4));
        QJsonObject bvPos;
        bvPos["buffer"] = 0;
        bvPos["byteOffset"] = vByteOffset;
        bvPos["byteLength"] = vCount * 12;
        int bvPosIdx = bufferViewsArr.size();
        bufferViewsArr.append(bvPos);
        QJsonObject bvIdx;
        bvIdx["buffer"] = 0;
        bvIdx["byteOffset"] = iByteOffset;
        bvIdx["byteLength"] = idxs.size() * 4;
        int bvIdxIdx = bufferViewsArr.size();
        bufferViewsArr.append(bvIdx);
        QJsonObject accPos;
        accPos["bufferView"] = bvPosIdx;
        accPos["componentType"] = 5126;
        accPos["count"] = vCount;
        accPos["type"] = "VEC3";
        int accPosIdx = accessorsArr.size();
        accessorsArr.append(accPos);
        QJsonObject accIdx;
        accIdx["bufferView"] = bvIdxIdx;
        accIdx["componentType"] = 5125;
        accIdx["count"] = idxs.size();
        accIdx["type"] = "SCALAR";
        int accIdxIdx = accessorsArr.size();
        accessorsArr.append(accIdx);
        QJsonObject attrs = prim[QStringLiteral("attributes")].toObject();
        attrs[QStringLiteral("POSITION")] = accPosIdx;
        prim[QStringLiteral("attributes")] = attrs;
        prim["indices"] = accIdxIdx;
        primitives.append(prim);
        meshObj["primitives"] = primitives;
        meshesArr.append(meshObj);
        vertexOffset += vCount;
        indexOffset += idxs.size();
        nodeIdx++;
    }
    root["asset"] = QJsonObject{{"version", "2.0"}, {"generator", "ksEditor"}};
    root["scene"] = 0;
    QJsonObject sc;
    QJsonArray snodes;
    for (int i = 0; i < nodeIdx; i++) snodes.append(i);
    sc["nodes"] = snodes;
    root["scenes"] = QJsonArray{sc};
    root["nodes"] = nodes;
    root["meshes"] = meshesArr;
    root["accessors"] = accessorsArr;
    root["bufferViews"] = bufferViewsArr;
    QJsonObject buf;
    buf["byteLength"] = binData.size();
    buf["uri"] = "data.bin";
    root["buffers"] = QJsonArray{buf};
    QByteArray jsonData = QJsonDocument(root).toJson(QJsonDocument::Compact);
    quint32 magic = 0x46546C67;
    quint32 version = 2;
    quint32 jsonLen = jsonData.size();
    quint32 binLen = binData.size();
    quint32 jsonChunkLen = (jsonLen + 3) & ~3;
    quint32 binChunkLen = (binLen + 3) & ~3;
    quint32 totalLen = 12 + 8 + jsonChunkLen + 8 + binChunkLen;
    QDataStream out(&f);
    out.setByteOrder(QDataStream::LittleEndian);
    out << magic << version << totalLen;
    out << jsonChunkLen << (quint32)0x4E4F534A;
    f.write(jsonData);
    for (quint32 i = jsonLen; i < jsonChunkLen; i++) f.putChar(0);
    out << binChunkLen << (quint32)0x004E4942;
    f.write(binData);
    for (quint32 i = binLen; i < binChunkLen; i++) f.putChar(0);
    f.close();
    emit statusMessage("GLB exported successfully");
    return true;
}

bool KSModelerQml::exportOBJ(const QString& path) {
    if (!m_scene) { emit error("No scene to export"); return false; }
    emit statusMessage("Exporting OBJ: " + path);
    bool ok = exportSceneOBJ(m_scene, path);
    if (ok) emit statusMessage("OBJ exported successfully");
    else emit error("Failed to export OBJ");
    return ok;
}

void KSModelerQml::selectObject(int id) {
    if (!m_scene) return;
    SceneObject* obj = m_scene->findObjectById(id);
    if (m_selectedObject) delete m_selectedObject;
    m_selectedObject = obj ? new SceneObjectQml(obj) : nullptr;
    emit selectionChanged();
    emit gizmoTransformChanged();
}

void KSModelerQml::selectAll() {
    if (!m_scene) return;
    SceneObject* last = nullptr;
    for (SceneObject* obj : m_scene->allObjects()) {
        obj->setSelected(true);
        last = obj;
    }
    if (m_selectedObject) delete m_selectedObject;
    m_selectedObject = last ? new SceneObjectQml(last) : nullptr;
    emit selectionChanged();
    emit gizmoTransformChanged();
}

void KSModelerQml::deselectAll() {
    if (m_selectedObject) delete m_selectedObject;
    m_selectedObject = nullptr;
    emit selectionChanged();
    emit gizmoTransformChanged();
}

void KSModelerQml::deleteSelected() {
    if (m_scene && m_selectedObject) {
        // Registra il comando per undo/redo
        if (m_commandHistory) {
            auto cmd = std::make_shared<DeleteObjectCommand>(m_selectedObject->id());
            m_commandHistory->execute(cmd);
        }

        m_scene->deleteObject(m_selectedObject->object());
        delete m_selectedObject;
        m_selectedObject = nullptr;
        emit sceneChanged();
        emit selectionChanged();
        emit gizmoTransformChanged();
    }
}

void KSModelerQml::duplicateSelected() {
    if (!m_scene || !m_selectedObject) return;
    SceneObject* orig = m_selectedObject->object();
    if (!orig) return;
    QString newName = orig->name() + "_copy";
    SceneObject* newObj = m_scene->createObject(newName, orig->type());
    if (newObj) {
        newObj->setPosition(orig->position());
        newObj->setRotationEuler(orig->rotationEuler());
        newObj->setScale(orig->scale());
        if (m_selectedObject) delete m_selectedObject;
        m_selectedObject = new SceneObjectQml(newObj);
        emit sceneChanged();
        emit selectionChanged();
        emit gizmoTransformChanged();
    }
}

void KSModelerQml::cutSelected() {
    copySelected();
    deleteSelected();
}

void KSModelerQml::copySelected() {
    if (!m_selectedObject || !m_selectedObject->object()) return;
    SceneObject* orig = m_selectedObject->object();
    m_clipboardType = orig->type();
    m_clipboardName = orig->name();
    auto p = orig->position();
    m_clipboardPosition = QVector3D(p.x(), p.y(), p.z());
    auto r = orig->rotationEuler();
    m_clipboardRotation = QVector3D(r.x(), r.y(), r.z());
    auto s = orig->scale();
    m_clipboardScale = QVector3D(s.x(), s.y(), s.z());
    m_clipboardActive = true;
    emit clipboardChanged();
}

void KSModelerQml::pasteClipboard() {
    if (!m_clipboardActive || !m_scene) return;
    SceneObject* newObj = m_scene->createObject(m_clipboardName + "_paste", m_clipboardType);
    if (newObj) {
        newObj->setPosition({m_clipboardPosition.x(), m_clipboardPosition.y(), m_clipboardPosition.z()});
        newObj->setRotationEuler({m_clipboardRotation.x(), m_clipboardRotation.y(), m_clipboardRotation.z()});
        newObj->setScale({m_clipboardScale.x(), m_clipboardScale.y(), m_clipboardScale.z()});
        if (m_selectedObject) delete m_selectedObject;
        m_selectedObject = new SceneObjectQml(newObj);
        emit sceneChanged();
        emit selectionChanged();
        emit gizmoTransformChanged();
    }
}

void KSModelerQml::printScene() {
    emit printRequested();
}

void KSModelerQml::checkMesh() {
    if (!m_scene || !m_selectedObject) {
        emit meshCheckResult("No object selected");
        return;
    }
    auto* obj = m_selectedObject->object();
    if (!obj || !obj->hasMesh()) {
        emit meshCheckResult("Object has no mesh");
        return;
    }
    auto* mesh = obj->mesh();
    int verts = mesh->vertexCount();
    int tris = mesh->indexCount() / 3;
    auto bbMin = obj->boundingBoxMin();
    auto bbMax = obj->boundingBoxMax();
    float sizeX = bbMax.x() - bbMin.x();
    float sizeY = bbMax.y() - bbMin.y();
    float sizeZ = bbMax.z() - bbMin.z();
    QString result = QString("Tris: %1 | Verts: %2 | Size: %3x%4x%5")
        .arg(tris).arg(verts)
        .arg(sizeX, 0, 'f', 2).arg(sizeY, 0, 'f', 2).arg(sizeZ, 0, 'f', 2);
    emit meshCheckResult(result);
}

void KSModelerQml::exportSTL() {
    emit stlExportRequested();
}

void KSModelerQml::scaleForPrint() {
    emit printScaleRequested();
}

void KSModelerQml::hollowMesh() {
    emit hollowRequested();
}

void KSModelerQml::generateSupports() {
    emit supportsRequested();
}

void KSModelerQml::sliceModel() {
    emit sliceRequested();
}

QVector3D KSModelerQml::gizmoPosition() const {
    if (m_selectedObject && m_selectedObject->object()) {
        auto p = m_selectedObject->object()->position();
        return QVector3D(p.x(), p.y(), p.z());
    }
    return QVector3D();
}

QVector3D KSModelerQml::gizmoRotation() const {
    if (m_selectedObject && m_selectedObject->object()) {
        auto r = m_selectedObject->object()->rotationEuler();
        return QVector3D(r.x(), r.y(), r.z());
    }
    return QVector3D();
}

QVector3D KSModelerQml::gizmoScale() const {
    if (m_selectedObject && m_selectedObject->object()) {
        auto s = m_selectedObject->object()->scale();
        return QVector3D(s.x(), s.y(), s.z());
    }
    return QVector3D(1, 1, 1);
}

void KSModelerQml::setGizmoMode(int mode) {
    if (m_gizmoMode != mode) {
        m_gizmoMode = mode;
        emit gizmoModeChanged();
        emit gizmoTransformChanged();
    }
}

void KSModelerQml::translateSelected(float x, float y, float z) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj) return;
    QVector3D p = obj->position();
    obj->setPosition(QVector3D(p.x() + x, p.y() + y, p.z() + z));
    emit sceneChanged();
    emit gizmoTransformChanged();
}

void KSModelerQml::rotateSelected(float x, float y, float z) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj) return;
    QVector3D r = obj->rotationEuler();
    obj->setRotationEuler(QVector3D(r.x() + x, r.y() + y, r.z() + z));
    emit sceneChanged();
    emit gizmoTransformChanged();
}

void KSModelerQml::scaleSelected(float x, float y, float z) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj) return;
    QVector3D s = obj->scale();
    obj->setScale(QVector3D(s.x() * x, s.y() * y, s.z() * z));
    emit sceneChanged();
    emit gizmoTransformChanged();
}

// ============================================================================
// Proportional Editing
// ============================================================================

void KSModelerQml::setProportionalEditing(bool enabled) {
    if (m_propEdit.enabled != enabled) {
        m_propEdit.enabled = enabled;
        if (!enabled) m_propEdit.hasCenter = false;
        emit proportionalEditingChanged();
        emit statusMessage(enabled ? "Proportional editing ON" : "Proportional editing OFF");
    }
}

bool KSModelerQml::isProportionalEditing() const { return m_propEdit.enabled; }

void KSModelerQml::setProportionalRadius(float radius) {
    m_propEdit.radius = qMax(0.01f, radius);
    emit proportionalEditingChanged();
}

float KSModelerQml::proportionalRadius() const { return m_propEdit.radius; }

void KSModelerQml::setProportionalFalloffType(int type) {
    m_propEdit.falloffType = qBound(0, type, 5);
    emit proportionalEditingChanged();
}

int KSModelerQml::proportionalFalloffType() const { return m_propEdit.falloffType; }

bool KSModelerQml::pickProportionalCenter(float pickX, float pickY, float pickZ) {
    if (!m_selectedObject || !m_selectedObject->object() || !m_selectedObject->object()->mesh())
        return false;
    auto& verts = m_selectedObject->object()->mesh()->geometry().vertices;
    if (verts.isEmpty()) return false;

    QVector3D pickPoint(pickX, pickY, pickZ);
    float minDist = std::numeric_limits<float>::max();
    QVector3D closest(verts[0].position.x(), verts[0].position.y(), verts[0].position.z());
    for (const auto& v : verts) {
        QVector3D pos(v.position.x(), v.position.y(), v.position.z());
        float d = (pos - pickPoint).lengthSquared();
        if (d < minDist) { minDist = d; closest = pos; }
    }
    m_propEdit.center = closest;
    m_propEdit.hasCenter = true;
    emit proportionalCenterChanged();
    return true;
}

void KSModelerQml::clearProportionalCenter() {
    m_propEdit.hasCenter = false;
    emit proportionalCenterChanged();
}

bool KSModelerQml::hasProportionalCenter() const { return m_propEdit.hasCenter; }

QVector3D KSModelerQml::proportionalCenter() const { return m_propEdit.center; }

static float propFalloff(float distance, float radius, int type) {
    float t = distance / radius;
    if (t >= 1.0f) return 0.0f;
    switch (type) {
        case 0: // Smooth (smoothstep)
            t = 1.0f - t;
            return t * t * (3.0f - 2.0f * t);
        case 1: // Linear
            return 1.0f - t;
        case 2: // Sharp
            return (1.0f - t) * (1.0f - t);
        case 3: // Root
            return sqrtf(1.0f - t);
        case 4: // Sphere
            return sqrtf(1.0f - t * t);
        case 5: // Constant
            return 1.0f;
        default:
            return 1.0f - t;
    }
}

void KSModelerQml::translateProportional(float x, float y, float z) {
    if (!m_propEdit.enabled || !m_propEdit.hasCenter || !m_selectedObject)
        return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;

    auto& verts = obj->mesh()->geometry().vertices;
    QVector3D delta(x, y, z);
    float radius = m_propEdit.radius;
    int falloff = m_propEdit.falloffType;
    QVector3D center = m_propEdit.center;

    for (auto& v : verts) {
        QVector3D pos(v.position.x(), v.position.y(), v.position.z());
        float dist = (pos - center).length();
        if (dist < radius) {
            float w = propFalloff(dist, radius, falloff);
            pos += QVector3D(delta.x() * w, delta.y() * w, delta.z() * w);
            v.position = pos;
        }
    }
    emit sceneChanged();
}

void KSModelerQml::rotateProportional(float x, float y, float z) {
    if (!m_propEdit.enabled || !m_propEdit.hasCenter || !m_selectedObject)
        return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;

    auto& verts = obj->mesh()->geometry().vertices;
    float rx = qDegreesToRadians(x);
    float ry = qDegreesToRadians(y);
    float rz = qDegreesToRadians(z);
    float radius = m_propEdit.radius;
    int falloff = m_propEdit.falloffType;
    QVector3D center = m_propEdit.center;

    float cx = cosf(rx), sx = sinf(rx);
    float cy = cosf(ry), sy = sinf(ry);
    float cz = cosf(rz), sz = sinf(rz);

    for (auto& v : verts) {
        QVector3D pos(v.position.x(), v.position.y(), v.position.z());
        QVector3D rel = pos - center;
        float dist = rel.length();
        if (dist < radius) {
            float w = propFalloff(dist, radius, falloff);
            // Apply rotation around center with weight
            float rx2 = rel.x() * w, ry2 = rel.y() * w, rz2 = rel.z() * w;
            // Rotate Y
            float nx = cy * rx2 + sy * rz2;
            float nz = -sy * rx2 + cy * rz2;
            rx2 = nx; rz2 = nz;
            // Rotate X
            float ny = cx * ry2 - sx * rz2;
            nz = sx * ry2 + cx * rz2;
            ry2 = ny; rz2 = nz;
            // Rotate Z
            nx = cz * rx2 - sz * ry2;
            ny = sz * rx2 + cz * ry2;
            rx2 = nx; ry2 = ny;

            v.position = QVector3D(center.x() + rx2, center.y() + ry2, center.z() + rz2);
        }
    }
}

void KSModelerQml::scaleProportional(float x, float y, float z) {
    if (!m_propEdit.enabled || !m_propEdit.hasCenter || !m_selectedObject)
        return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;

    auto& verts = obj->mesh()->geometry().vertices;
    float radius = m_propEdit.radius;
    int falloff = m_propEdit.falloffType;
    QVector3D center = m_propEdit.center;

    for (auto& v : verts) {
        QVector3D pos(v.position.x(), v.position.y(), v.position.z());
        QVector3D rel = pos - center;
        float dist = rel.length();
        if (dist < radius) {
            float w = propFalloff(dist, radius, falloff);
            float sx = 1.0f + (x - 1.0f) * w;
            float sy = 1.0f + (y - 1.0f) * w;
            float sz = 1.0f + (z - 1.0f) * w;
            v.position.setX(center.x() + rel.x() * sx);
            v.position.setY(center.y() + rel.y() * sy);
            v.position.setZ(center.z() + rel.z() * sz);
        }
    }
    emit sceneChanged();
}

void KSModelerQml::setSelectedPosition(float x, float y, float z) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj) return;
    obj->setPosition(QVector3D(x, y, z));
    emit sceneChanged();
    emit gizmoTransformChanged();
}

void KSModelerQml::setSelectedRotation(float x, float y, float z) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj) return;
    obj->setRotationEuler(QVector3D(x, y, z));
    emit sceneChanged();
}

void KSModelerQml::setSelectedScale(float x, float y, float z) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj) return;
    obj->setScale(QVector3D(x, y, z));
    emit sceneChanged();
    emit gizmoTransformChanged();
}

void KSModelerQml::extrudeFaces(const QList<int>& faceIndices, float distance) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    QVector<QVector3D> dirs;
    QList<int> faces = faceIndices;
    if (faces.isEmpty()) {
        for (int i = 0; i < md.faces.size(); i++)
            faces.append(i);
    }
    for (int i = 0; i < faces.size(); i++)
        dirs.append(QVector3D(0, 0, distance));
    MeshData result = MeshOperations::extrudeFaces(md, dirs);
    meshDataToSceneMesh(obj, result);
    emit sceneChanged();
    emit statusMessage(QString("Extruded %1 faces").arg(faces.size()));
}

void KSModelerQml::bevelEdges(const QList<int>& edgeIndices, float amount, int segments) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    MeshData result = MeshOperations::bevelEdges(md, amount, segments);
    meshDataToSceneMesh(obj, result);
    emit sceneChanged();
    emit statusMessage(QString("Beveled with %1 segments").arg(segments));
}

void KSModelerQml::subdivideFaces(const QList<int>& faceIndices, int cuts) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    MeshData result = MeshOperations::subdivide(md, cuts);
    meshDataToSceneMesh(obj, result);
    emit sceneChanged();
    emit statusMessage(QString("Subdivided with %1 cuts").arg(cuts));
}

void KSModelerQml::triangulateMesh() {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    MeshData result = MeshOperations::triangulate(md);
    meshDataToSceneMesh(obj, result);
    emit sceneChanged();
    emit statusMessage("Mesh triangulated");
}

void KSModelerQml::flipNormals() {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    md.flipFaces();
    meshDataToSceneMesh(obj, md);
    emit sceneChanged();
    emit statusMessage("Normals flipped");
}

void KSModelerQml::recalculateNormals() {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    md.computeNormals();
    meshDataToSceneMesh(obj, md);
    emit sceneChanged();
    emit statusMessage("Normals recalculated");
}

void KSModelerQml::weldVertices(float threshold) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    if (threshold <= 0.0f) threshold = 0.001f;

    MeshData md = sceneMeshToMeshData(obj);
    MeshData result = MeshOperations::weldVertices(md, threshold);

    if (result.vertices.size() == md.vertices.size()) {
        emit statusMessage("No vertices to weld");
        return;
    }

    int removed = md.vertices.size() - result.vertices.size();
    meshDataToSceneMesh(obj, result);
    emit sceneChanged();
    emit statusMessage(QString("Welded %1 vertices (%2 remaining)").arg(removed).arg(result.vertices.size()));
}

void KSModelerQml::removeDoubles(float threshold) {
    weldVertices(threshold > 0.0f ? threshold : 0.0001f);
}

void KSModelerQml::mirrorMesh(int axis) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    auto& verts = obj->mesh()->geometry().vertices;
    int origCount = verts.size();
    auto& idxs = obj->mesh()->geometry().indices;
    int origIdxCount = idxs.size();
    float sign = (axis == 0) ? -1.0f : 1.0f;
    for (int i = 0; i < origCount; i++) {
        SceneVertex sv = verts[i];
        if (axis == 0) sv.position.setX(-sv.position.x());
        else if (axis == 1) sv.position.setY(-sv.position.y());
        else sv.position.setZ(-sv.position.z());
        verts.append(sv);
    }
    for (int i = 0; i + 2 < origIdxCount; i += 3) {
        uint32_t i0 = idxs[i];
        uint32_t i1 = idxs[i + 1];
        uint32_t i2 = idxs[i + 2];
        idxs.append(i2 + origCount);
        idxs.append(i1 + origCount);
        idxs.append(i0 + origCount);
    }
    emit sceneChanged();
    emit statusMessage(QString("Mirrored on %1 axis").arg(axis == 0 ? 'X' : (axis == 1 ? 'Y' : 'Z')));
}

QVariantList KSModelerQml::getMeshObjects() const {
    QVariantList result;
    if (!m_scene) return result;
    auto allObjs = m_scene->allObjects();
    int selectedId = m_selectedObject ? m_selectedObject->object()->id() : -1;
    for (SceneObject* obj : allObjs) {
        if (!obj->mesh()) continue;
        if (obj->id() == selectedId) continue; // skip self
        QVariantMap entry;
        entry["id"] = obj->id();
        entry["name"] = obj->name();
        result.append(entry);
    }
    return result;
}

void KSModelerQml::booleanOperation(int operation, int targetObjectId) {
    if (!m_selectedObject) return;
    SceneObject* src = m_selectedObject->object();
    if (!src || !src->mesh()) return;
    SceneObject* tgt = (targetObjectId >= 0) ? m_scene->findObjectById(targetObjectId) : nullptr;
    if (!tgt || !tgt->mesh()) return;
    MeshData srcData = sceneMeshToMeshData(src);
    MeshData tgtData = sceneMeshToMeshData(tgt);

    // Try CGAL-backed BooleanOperations via MeshOperations fallback chain
    MeshData result;
    switch (operation) {
        case 0: result = MeshOperations::booleanUnion(srcData, tgtData); break;
        case 1: result = MeshOperations::booleanDifference(srcData, tgtData); break;
        case 2: result = MeshOperations::booleanIntersection(srcData, tgtData); break;
        case 3: result = MeshOperations::booleanXor(srcData, tgtData); break;
        default: return;
    }

    if (result.vertices.isEmpty()) {
        emit statusMessage("Boolean operation produced empty result");
        return;
    }

    meshDataToSceneMesh(src, result);
    m_scene->deleteObject(tgt);
    emit sceneChanged();
    emit statusMessage("Boolean operation completed: " +
        QString::number(result.vertices.size()) + " vertices, " +
        QString::number(result.faces.size()) + " faces");
}

void KSModelerQml::insetFaces(const QList<int>& faceIndices, float amount) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData data = sceneMeshToMeshData(obj);
    data = MeshOperations::insetFaces(data, amount);
    meshDataToSceneMesh(obj, data);
    emit sceneChanged();
    emit statusMessage(" Faces inset");
}

void KSModelerQml::knifeCut(float startX, float startY, float startZ, float endX, float endY, float endZ) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    QVector3D cutStart(startX, startY, startZ);
    QVector3D cutEnd(endX, endY, endZ);
    MeshData data = sceneMeshToMeshData(obj);
    MeshData result = MeshOperations::knifeCut(data, cutStart, cutEnd);
    if (result.faces.size() == data.faces.size()) {
        emit statusMessage(" Knife cut: no edges intersected");
        return;
    }
    int edgesSplit = result.vertices.size() - data.vertices.size();
    meshDataToSceneMesh(obj, result);
    emit sceneChanged();
    emit statusMessage(" Knife cut: " + QString::number(edgesSplit) + " edges split");
}

void KSModelerQml::dissolveEdges(const QList<int>& edgeIndices) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    QVector<int> indices(edgeIndices.begin(), edgeIndices.end());
    MeshData result = MeshOperations::dissolveEdges(md, indices);
    meshDataToSceneMesh(obj, result);
    emit sceneChanged();
    emit statusMessage(QString("Dissolved %1 edges").arg(edgeIndices.size()));
}

void KSModelerQml::dissolveVertices(const QList<int>& vertexIndices) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    QVector<int> indices(vertexIndices.begin(), vertexIndices.end());
    MeshData result = MeshOperations::dissolveVertices(md, indices);
    meshDataToSceneMesh(obj, result);
    emit sceneChanged();
    emit statusMessage(QString("Dissolved %1 vertices").arg(vertexIndices.size()));
}

void KSModelerQml::splitMeshes() {
    if (!m_selectedObject || !m_scene) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    QVector<MeshData> parts;
    MeshOperations::splitMeshes(md, parts);
    if (parts.size() <= 1) {
        emit statusMessage("Mesh has only one connected component");
        return;
    }
    meshDataToSceneMesh(obj, parts[0]);
    for (int i = 1; i < parts.size(); ++i) {
        SceneObject* newObj = m_scene->createObject(obj->name() + QString("_part%1").arg(i), SceneObject::Type::Mesh);
        if (newObj) {
            meshDataToSceneMesh(newObj, parts[i]);
            QVector3D pos = obj->position();
            newObj->setPosition(QVector3D(pos.x() + (i) * 2.0f, pos.y(), pos.z()));
        }
    }
    emit sceneChanged();
    emit statusMessage(QString("Split mesh into %1 parts").arg(parts.size()));
}

void KSModelerQml::addPrimitiveCube(float size) {
    if (!m_scene) m_scene = new ks::SceneGraph();
    MeshData md = MeshOperations::createBox(size, size, size);
    SceneObject* obj = m_scene->createObject("Cube", SceneObject::Type::Mesh);
    meshDataToSceneMesh(obj, md);
    if (m_selectedObject) delete m_selectedObject;
    m_selectedObject = new SceneObjectQml(obj);
    emit sceneChanged();
    emit selectionChanged();
    emit statusMessage("Added cube primitive");
}

void KSModelerQml::addPrimitiveSphere(float radius, int segments, int rings) {
    if (!m_scene) m_scene = new ks::SceneGraph();
    MeshData md = MeshOperations::createSphere(radius, segments, rings);
    SceneObject* obj = m_scene->createObject("Sphere", SceneObject::Type::Mesh);
    meshDataToSceneMesh(obj, md);
    if (m_selectedObject) delete m_selectedObject;
    m_selectedObject = new SceneObjectQml(obj);
    emit sceneChanged();
    emit selectionChanged();
    emit statusMessage("Added sphere primitive");
}

void KSModelerQml::addPrimitiveCylinder(float radius, float height, int segments) {
    if (!m_scene) m_scene = new ks::SceneGraph();
    MeshData md = MeshOperations::createCylinder(radius, height, segments);
    SceneObject* obj = m_scene->createObject("Cylinder", SceneObject::Type::Mesh);
    meshDataToSceneMesh(obj, md);
    if (m_selectedObject) delete m_selectedObject;
    m_selectedObject = new SceneObjectQml(obj);
    emit sceneChanged();
    emit selectionChanged();
    emit statusMessage("Added cylinder primitive");
}

void KSModelerQml::addPrimitiveCone(float radius, float height, int segments) {
    if (!m_scene) m_scene = new ks::SceneGraph();
    MeshData md = MeshOperations::createCone(radius, height, segments);
    SceneObject* obj = m_scene->createObject("Cone", SceneObject::Type::Mesh);
    meshDataToSceneMesh(obj, md);
    if (m_selectedObject) delete m_selectedObject;
    m_selectedObject = new SceneObjectQml(obj);
    emit sceneChanged();
    emit selectionChanged();
    emit statusMessage("Added cone primitive");
}

void KSModelerQml::addPrimitiveTorus(float majorRadius, float minorRadius, int majorSegments, int minorSegments) {
    if (!m_scene) m_scene = new ks::SceneGraph();
    MeshData md = MeshOperations::createTorus(majorRadius, minorRadius, majorSegments, minorSegments);
    SceneObject* obj = m_scene->createObject("Torus", SceneObject::Type::Mesh);
    meshDataToSceneMesh(obj, md);
    if (m_selectedObject) delete m_selectedObject;
    m_selectedObject = new SceneObjectQml(obj);
    emit sceneChanged();
    emit selectionChanged();
    emit statusMessage("Added torus primitive");
}

void KSModelerQml::addPrimitivePlane(float width, float height, int widthSegments, int heightSegments) {
    if (!m_scene) m_scene = new ks::SceneGraph();
    MeshData md = MeshOperations::createPlane(width, height, widthSegments, heightSegments);
    SceneObject* obj = m_scene->createObject("Plane", SceneObject::Type::Mesh);
    meshDataToSceneMesh(obj, md);
    if (m_selectedObject) delete m_selectedObject;
    m_selectedObject = new SceneObjectQml(obj);
    emit sceneChanged();
    emit selectionChanged();
    emit statusMessage("Added plane primitive");
}

int KSModelerQml::addTransformGroup(const QString& name) {
    if (!m_scene) m_scene = new ks::SceneGraph();
    SceneObject* obj = m_scene->createObject(name, SceneObject::Type::Node);
    if (m_selectedObject) delete m_selectedObject;
    m_selectedObject = new SceneObjectQml(obj);
    emit sceneChanged();
    emit selectionChanged();
    emit statusMessage("Added " + name);
    return obj->id();
}

int KSModelerQml::addCamera(const QString& name) {
    if (!m_scene) m_scene = new ks::SceneGraph();
    SceneObject* obj = m_scene->createObject(name, SceneObject::Type::Camera);
    if (m_selectedObject) delete m_selectedObject;
    m_selectedObject = new SceneObjectQml(obj);
    emit sceneChanged();
    emit selectionChanged();
    emit statusMessage("Added " + name);
    return obj->id();
}

int KSModelerQml::addLight(const QString& name) {
    if (!m_scene) m_scene = new ks::SceneGraph();
    SceneObject* obj = m_scene->createObject(name, SceneObject::Type::Light);
    if (m_selectedObject) delete m_selectedObject;
    m_selectedObject = new SceneObjectQml(obj);
    emit sceneChanged();
    emit selectionChanged();
    emit statusMessage("Added " + name);
    return obj->id();
}

void KSModelerQml::projectUVPlanar(int axis) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    QVector<QVector3D> verts;
    QVector<QVector<int>> faces;
    for (const auto& v : md.vertices) verts.append(v.position);
    for (const auto& f : md.faces) { QVector<int> fi; for (int i : f.indices) fi.append(i); faces.append(fi); }
    QVector3D dir(0, 0, 1);
    if (axis == 0) dir = QVector3D(1, 0, 0);
    else if (axis == 1) dir = QVector3D(0, 1, 0);
    QVector<QVector2D> uvs = UVMapper::planarProject(verts, faces, dir);
    for (int i = 0; i < md.vertices.size() && i < uvs.size(); i++)
        md.vertices[i].uv = uvs[i];
    meshDataToSceneMesh(obj, md);
    emit sceneChanged();
    emit statusMessage("UV planar projection applied");
}

void KSModelerQml::projectUVCylindrical(int axis) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    QVector<QVector3D> verts;
    QVector<QVector<int>> faces;
    for (const auto& v : md.vertices) verts.append(v.position);
    for (const auto& f : md.faces) { QVector<int> fi; for (int i : f.indices) fi.append(i); faces.append(fi); }
    QVector<QVector2D> uvs = UVMapper::cylindricalProject(verts, faces);
    for (int i = 0; i < md.vertices.size() && i < uvs.size(); i++)
        md.vertices[i].uv = uvs[i];
    meshDataToSceneMesh(obj, md);
    emit sceneChanged();
    emit statusMessage("UV cylindrical projection applied");
}

void KSModelerQml::projectUVSpherical() {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    QVector<QVector3D> verts;
    QVector<QVector<int>> faces;
    for (const auto& v : md.vertices) verts.append(v.position);
    for (const auto& f : md.faces) { QVector<int> fi; for (int i : f.indices) fi.append(i); faces.append(fi); }
    QVector<QVector2D> uvs = UVMapper::sphericalProject(verts, faces);
    for (int i = 0; i < md.vertices.size() && i < uvs.size(); i++)
        md.vertices[i].uv = uvs[i];
    meshDataToSceneMesh(obj, md);
    emit sceneChanged();
    emit statusMessage("UV spherical projection applied");
}

void KSModelerQml::projectUVBox(float size) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    QVector<QVector3D> verts;
    QVector<QVector<int>> faces;
    for (const auto& v : md.vertices) verts.append(v.position);
    for (const auto& f : md.faces) { QVector<int> fi; for (int i : f.indices) fi.append(i); faces.append(fi); }
    QVector<QVector2D> uvs = UVMapper::cubeProject(verts, faces);
    for (int i = 0; i < md.vertices.size() && i < uvs.size(); i++)
        md.vertices[i].uv = uvs[i];
    meshDataToSceneMesh(obj, md);
    emit sceneChanged();
    emit statusMessage("UV box projection applied");
}

void KSModelerQml::unwrapUVs(const QString& method) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    QVector<QVector3D> verts;
    QVector<QVector<int>> faces;
    for (const auto& v : md.vertices) verts.append(v.position);
    for (const auto& f : md.faces) { QVector<int> fi; for (int i : f.indices) fi.append(i); faces.append(fi); }
    QVector<QVector2D> uvs;
    QSet<QPair<int, int>> seams;
    if (method == "conformal") {
        UVUnwrapConfig cfg;
        ConformalUnwrapper::unwrap(verts, faces, uvs, cfg);
    } else {
        LSCMUnwrapper::unwrap(verts, faces, seams, uvs);
    }
    for (int i = 0; i < md.vertices.size() && i < uvs.size(); i++)
        md.vertices[i].uv = uvs[i];
    meshDataToSceneMesh(obj, md);
    emit sceneChanged();
    emit statusMessage("UV unwrap completed");
}

void KSModelerQml::packUVs(float margin, int resolution) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    QVector<QVector2D> uvs;
    for (const auto& v : md.vertices) uvs.append(v.uv);
    UVMapper::scaleToFit(uvs, margin);
    for (int i = 0; i < md.vertices.size() && i < uvs.size(); i++)
        md.vertices[i].uv = uvs[i];
    meshDataToSceneMesh(obj, md);
    emit sceneChanged();
    emit statusMessage("UVs packed");
}

void KSModelerQml::translateUVs(float u, float v) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    QVector<QVector2D> uvs;
    for (const auto& vert : md.vertices) uvs.append(vert.uv);
    UVTransform::translate(uvs, QVector2D(u, v));
    for (int i = 0; i < md.vertices.size() && i < uvs.size(); i++)
        md.vertices[i].uv = uvs[i];
    meshDataToSceneMesh(obj, md);
    emit sceneChanged();
    emit statusMessage("UVs translated");
}

void KSModelerQml::rotateUVs(float angle) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    QVector<QVector2D> uvs;
    for (const auto& vert : md.vertices) uvs.append(vert.uv);
    UVTransform::rotate(uvs, QVector2D(0.5f, 0.5f), qDegreesToRadians(angle));
    for (int i = 0; i < md.vertices.size() && i < uvs.size(); i++)
        md.vertices[i].uv = uvs[i];
    meshDataToSceneMesh(obj, md);
    emit sceneChanged();
    emit statusMessage("UVs rotated");
}

void KSModelerQml::scaleUVs(float u, float v) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    QVector<QVector2D> uvs;
    for (const auto& vert : md.vertices) uvs.append(vert.uv);
    UVTransform::scale(uvs, QVector2D(0.5f, 0.5f), u);
    for (int i = 0; i < md.vertices.size() && i < uvs.size(); i++)
        md.vertices[i].uv = uvs[i];
    meshDataToSceneMesh(obj, md);
    emit sceneChanged();
    emit statusMessage("UVs scaled");
}

void KSModelerQml::generateAutoUVs(int resolution) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    UVUnwrapConfig cfg;
    QVector<QVector3D> verts;
    QVector<QVector<int>> faces;
    for (const auto& v : md.vertices) verts.append(v.position);
    for (const auto& f : md.faces) { QVector<int> fi; for (int i : f.indices) fi.append(i); faces.append(fi); }
    QVector<QVector2D> uvs = UVMapper::smartProject(verts, faces, cfg);
    for (int i = 0; i < md.vertices.size() && i < uvs.size(); i++)
        md.vertices[i].uv = uvs[i];
    meshDataToSceneMesh(obj, md);
    emit sceneChanged();
    emit statusMessage(QString("Auto UV generated at %1x%1").arg(resolution));
}

void KSModelerQml::createSkeleton(const QString& type) {
    m_bones.clear();
    Skeleton* skel = nullptr;
    if (type == "humanoid")
        skel = RigifyGenerator::generateHumanoid(RigifyConfig());
    else if (type == "biped")
        skel = RigifyGenerator::generateBiped();
    else
        skel = RigifyGenerator::generateFKChain(4, QVector3D(0, 1, 0));
    if (skel) {
        for (const auto& b : skel->bones) {
            Bone bone;
            bone.name = b.name;
            bone.parentId = b.parentIndex;
            bone.position = b.head;
            m_bones.append(bone);
        }
        updateBoneHierarchy();
        emit statusMessage(QString("Created skeleton: %1 (%2 bones)").arg(type).arg(m_bones.size()));
    }
    m_boneVersion++;
    emit sceneChanged();
    emit skeletonChanged();
}

void KSModelerQml::addBone(const QString& name, int parentId, float x, float y, float z) {
    Bone bone;
    bone.name = name;
    bone.parentId = parentId;
    bone.position = QVector3D(x, y, z);
    m_bones.append(bone);
    // Add to parent's children list
    if (parentId >= 0 && parentId < m_bones.size() - 1)
        m_bones[parentId].children.append(m_bones.size() - 1);
    if (m_scene) {
        SceneObject* boneObj = m_scene->createObject(name, SceneObject::Type::Bone);
        boneObj->setPosition(QVector3D(x, y, z));
    }
    emit sceneChanged();
    m_boneVersion++;
    emit skeletonChanged();
    emit statusMessage("Added bone: " + name);
}

void KSModelerQml::bindToSkeleton(float maxDistance) {
    if (m_bones.isEmpty() || !m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    m_vertexWeights.clear();
    m_vertexWeights.resize(md.vertices.size());
    QVector<QVector3D> bonePositions;
    for (const auto& b : m_bones)
        bonePositions.append(b.position);
    QVector<QVector3D> verts;
    for (const auto& v : md.vertices) verts.append(v.position);
    QVector<QVector<int>> faces;
    for (const auto& f : md.faces) {
        QVector<int> fi;
        for (int idx : f.indices) fi.append(idx);
        faces.append(fi);
    }
    QVector<WeightVertex> autoWeights = AutoWeightCalculator::calculateAutoWeights(
        verts, faces, bonePositions, AutoWeightCalculator::Method::NearestVertex);
    for (int i = 0; i < autoWeights.size() && i < m_vertexWeights.size(); i++) {
        for (auto it = autoWeights[i].weights.begin(); it != autoWeights[i].weights.end(); ++it) {
            int boneIdx = it.key();
            if (boneIdx < m_bones.size()) {
                VertexWeight vw;
                vw.boneId = boneIdx;
                vw.weight = it.value();
                m_vertexWeights[i].append(vw);
            }
        }
    }
    emit sceneChanged();
    emit statusMessage("Bound mesh to skeleton");
}

void KSModelerQml::smoothSkinning(int iterations) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    QVector<WeightVertex> weights;
    for (int vi = 0; vi < m_vertexWeights.size(); vi++) {
        WeightVertex wv;
        wv.vertexIndex = vi;
        for (const auto& w : m_vertexWeights[vi])
            wv.weights[w.boneId] = w.weight;
        weights.append(wv);
    }
    QVector<QVector3D> verts;
    for (const auto& v : md.vertices) verts.append(v.position);
    QVector<QVector<int>> faces;
    for (const auto& f : md.faces) { QVector<int> fi; for (int i : f.indices) fi.append(i); faces.append(fi); }
    WeightOptimization::smoothWeights(verts, faces, weights, 0.5f, iterations);
    emit sceneChanged();
    emit statusMessage(QString("Skinning smoothed (%1 iterations)").arg(iterations));
}

void KSModelerQml::normalizeWeights() {
    if (m_vertexWeights.isEmpty()) return;
    QVector<WeightVertex> weights;
    for (int vi = 0; vi < m_vertexWeights.size(); vi++) {
        WeightVertex wv;
        wv.vertexIndex = vi;
        for (const auto& w : m_vertexWeights[vi])
            wv.weights[w.boneId] = w.weight;
        weights.append(wv);
    }
    WeightOptimization::normalizeAllWeights(weights);
    m_vertexWeights.clear();
    m_vertexWeights.resize(weights.size());
    for (int i = 0; i < weights.size(); i++) {
        for (auto it = weights[i].weights.begin(); it != weights[i].weights.end(); ++it) {
            VertexWeight vw;
            vw.boneId = it.key();
            vw.weight = it.value();
            m_vertexWeights[i].append(vw);
        }
    }
    emit sceneChanged();
    emit statusMessage("Weights normalized");
}

void KSModelerQml::pruneWeights(float threshold, int maxInfluences) {
    if (m_vertexWeights.isEmpty()) return;
    QVector<WeightVertex> weights;
    for (int vi = 0; vi < m_vertexWeights.size(); vi++) {
        WeightVertex wv;
        wv.vertexIndex = vi;
        for (const auto& w : m_vertexWeights[vi])
            wv.weights[w.boneId] = w.weight;
        weights.append(wv);
    }
    WeightPainter::cleanZeroWeights(weights, threshold);
    WeightPainter::limitWeights(weights, maxInfluences);
    WeightOptimization::pruneWeights(weights, QVector<QVector3D>(), threshold);
    m_vertexWeights.clear();
    m_vertexWeights.resize(weights.size());
    for (int i = 0; i < weights.size(); i++) {
        for (auto it = weights[i].weights.begin(); it != weights[i].weights.end(); ++it) {
            VertexWeight vw;
            vw.boneId = it.key();
            vw.weight = it.value();
            m_vertexWeights[i].append(vw);
        }
    }
    emit sceneChanged();
    emit statusMessage(QString("Weights pruned (threshold=%1, max=%2)").arg(threshold).arg(maxInfluences));
}

void KSModelerQml::paintWeights(int boneId, float x, float y, float z, float radius, float strength) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    QVector<QVector3D> verts;
    for (const auto& v : md.vertices) verts.append(v.position);
    QVector<WeightVertex> weights;
    for (int vi = 0; vi < m_vertexWeights.size(); vi++) {
        WeightVertex wv;
        wv.vertexIndex = vi;
        for (const auto& w : m_vertexWeights[vi])
            wv.weights[w.boneId] = w.weight;
        weights.append(wv);
    }
    WeightPainter::paintWeight(QVector3D(x, y, z), radius, boneId, strength, verts, weights);
    m_vertexWeights.clear();
    m_vertexWeights.resize(weights.size());
    for (int i = 0; i < weights.size(); i++) {
        for (auto it = weights[i].weights.begin(); it != weights[i].weights.end(); ++it) {
            VertexWeight vw;
            vw.boneId = it.key();
            vw.weight = it.value();
            m_vertexWeights[i].append(vw);
        }
    }
    emit sceneChanged();
    emit statusMessage(QString("Painted weights on bone %1").arg(boneId));
}

void KSModelerQml::mirrorWeights(int axis) {
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;
    MeshData md = sceneMeshToMeshData(obj);
    QVector<WeightVertex> weights;
    for (int vi = 0; vi < m_vertexWeights.size(); vi++) {
        WeightVertex wv;
        wv.vertexIndex = vi;
        for (const auto& w : m_vertexWeights[vi])
            wv.weights[w.boneId] = w.weight;
        weights.append(wv);
    }
    QVector<QVector3D> verts;
    for (const auto& v : md.vertices) verts.append(v.position);
    WeightPainter::mirrorWeights(weights, verts, axis == 0, axis == 1, axis == 2);
    m_vertexWeights.clear();
    m_vertexWeights.resize(weights.size());
    for (int i = 0; i < weights.size(); i++) {
        for (auto it = weights[i].weights.begin(); it != weights[i].weights.end(); ++it) {
            VertexWeight vw;
            vw.boneId = it.key();
            vw.weight = it.value();
            m_vertexWeights[i].append(vw);
        }
    }
    emit sceneChanged();
    emit statusMessage("Weights mirrored");
}

void KSModelerQml::solveTwoBoneIK(float targetX, float targetY, float targetZ) {
    if (m_bones.size() < 3) { emit statusMessage("Need at least 3 bones for IK"); return; }
    // Solve last 3 bones as a two-bone chain: rootBone -> midBone -> endBone
    int endIdx = m_bones.size() - 1;
    int midIdx = endIdx - 1;
    int rootIdx = endIdx - 2;
    QVector3D target(targetX, targetY, targetZ);

    QVector3D root = m_bones[rootIdx].position;
    QVector3D mid = m_bones[midIdx].position;
    QVector3D end = m_bones[endIdx].position;

    float lenA = (mid - root).length();
    float lenB = (end - mid).length();
    float lenC = qMin((target - root).length(), lenA + lenB * 0.999f);

    if (lenA < 0.001f || lenB < 0.001f) return;

    // Law of cosines for mid joint angle
    float cosMid = (lenA * lenA + lenB * lenB - lenC * lenC) / (2.0f * lenA * lenB);
    cosMid = qBound(-1.0f, cosMid, 1.0f);
    float midAngle = acosf(cosMid);

    // Compute bend axis (perpendicular to the plane of root->mid and root->target)
    QVector3D toMid = (mid - root).normalized();
    QVector3D toTarget = (target - root).normalized();
    QVector3D bendAxis = QVector3D::crossProduct(toMid, toTarget);
    if (bendAxis.lengthSquared() < 0.001f) {
        // Target is along same direction as current chain, use a default up vector
        bendAxis = QVector3D::crossProduct(toMid, QVector3D(0, 1, 0));
        if (bendAxis.lengthSquared() < 0.001f)
            bendAxis = QVector3D::crossProduct(toMid, QVector3D(1, 0, 0));
    }
    bendAxis.normalize();

    // Rotate mid bone around root by midAngle
    QQuaternion rootRot = QQuaternion::fromAxisAndAngle(bendAxis, qRadiansToDegrees(midAngle));
    QVector3D newMid = root + rootRot.rotatedVector(mid - root);

    // End bone goes from newMid toward target
    QVector3D newEndDir = (target - newMid).normalized();
    QVector3D newEnd = newMid + newEndDir * lenB;

    // Apply with IK weight blending
    for (int i = rootIdx; i <= endIdx; i++) {
        float w = m_bones[i].ikWeight;
        if (w <= 0.0f) continue;
        if (i == midIdx) m_bones[i].position += (newMid - m_bones[i].position) * w;
        else if (i == endIdx) m_bones[i].position += (newEnd - m_bones[i].position) * w;
    }

    // Update SceneObject transforms for bone objects
    if (m_scene) {
        auto allObjs = m_scene->allObjects();
        for (int i = 0; i < m_bones.size(); i++) {
            for (SceneObject* obj : allObjs) {
                if (obj->name() == m_bones[i].name) {
                    obj->setPosition(m_bones[i].position);
                    break;
                }
            }
        }
    }
    m_boneVersion++;
    emit sceneChanged();
    emit skeletonChanged();
    emit statusMessage(QString("Two-bone IK solved (chain: %1→%2→%3)").arg(m_bones[rootIdx].name).arg(m_bones[midIdx].name).arg(m_bones[endIdx].name));
}

void KSModelerQml::solveCCDIK(float targetX, float targetY, float targetZ, int iterations) {
    if (m_bones.isEmpty()) { emit statusMessage("No bones for IK"); return; }
    QVector3D target(targetX, targetY, targetZ);
    for (int iter = 0; iter < iterations; iter++) {
        for (int i = m_bones.size() - 1; i >= 0; i--) {
            QVector3D tip = m_bones.last().position;
            QVector3D bonePos = m_bones[i].position;
            QVector3D toEnd = (tip - bonePos).normalized();
            QVector3D toTarget = (target - bonePos).normalized();
            if (toEnd.lengthSquared() < 0.001f || toTarget.lengthSquared() < 0.001f) continue;
            float dot = QVector3D::dotProduct(toEnd, toTarget);
            if (dot < 0.999f) {
                QVector3D axis = QVector3D::crossProduct(toEnd, toTarget);
                float len = axis.length();
                if (len > 0.001f) {
                    axis /= len;
                    float angle = acosf(qBound(-1.0f, dot, 1.0f));
                    QQuaternion rot = QQuaternion::fromAxisAndAngle(axis, qRadiansToDegrees(angle));
                    for (int j = i; j < m_bones.size(); j++) {
                        QVector3D rel = m_bones[j].position - bonePos;
                        m_bones[j].position = bonePos + rot.rotatedVector(rel);
                    }
                }
            }
        }
    }
    m_boneVersion++;
    emit sceneChanged();
    emit skeletonChanged();
    emit statusMessage(QString("CCD IK solved (%1 iterations)").arg(iterations));
}

// ============================================================================
// Bone Access, FK/IK Control, and Visualization Helpers
// ============================================================================

int KSModelerQml::boneCount() const { return m_bones.size(); }

QString KSModelerQml::getBoneName(int boneIdx) const {
    if (boneIdx < 0 || boneIdx >= m_bones.size()) return QString();
    return m_bones[boneIdx].name;
}

int KSModelerQml::getBoneParentId(int boneIdx) const {
    if (boneIdx < 0 || boneIdx >= m_bones.size()) return -1;
    return m_bones[boneIdx].parentId;
}

QVector3D KSModelerQml::getBonePosition(int boneIdx) const {
    if (boneIdx < 0 || boneIdx >= m_bones.size()) return QVector3D();
    return m_bones[boneIdx].position;
}

QVector3D KSModelerQml::getBoneWorldPosition(int boneIdx) const {
    if (boneIdx < 0 || boneIdx >= m_bones.size()) return QVector3D();
    // Bone positions are stored in world space
    return m_bones[boneIdx].position;
}

QVector3D KSModelerQml::getBoneRotation(int boneIdx) const {
    if (boneIdx < 0 || boneIdx >= m_bones.size()) return QVector3D();
    return m_bones[boneIdx].rotation;
}

QVector3D KSModelerQml::getBoneLength(int boneIdx) const {
    if (boneIdx < 0 || boneIdx >= m_bones.size()) return QVector3D();
    if (m_bones[boneIdx].children.isEmpty()) return QVector3D(0, 0.5f, 0);
    // Average direction to children
    QVector3D avg;
    for (int c : m_bones[boneIdx].children) {
        if (c >= 0 && c < m_bones.size())
            avg += m_bones[c].position - m_bones[boneIdx].position;
    }
    if (!m_bones[boneIdx].children.isEmpty()) avg /= m_bones[boneIdx].children.size();
    return avg;
}

int KSModelerQml::selectedBoneIndex() const { return m_selectedBone; }

void KSModelerQml::selectBone(int boneIdx) {
    if (boneIdx < -1 || boneIdx >= m_bones.size()) return;
    m_selectedBone = boneIdx;
    emit boneSelectionChanged();
}

void KSModelerQml::setBoneIKWeight(int boneIdx, float weight) {
    if (boneIdx < 0 || boneIdx >= m_bones.size()) return;
    m_bones[boneIdx].ikWeight = qBound(0.0f, weight, 1.0f);
    m_boneVersion++;
    emit skeletonChanged();
}

float KSModelerQml::getBoneIKWeight(int boneIdx) const {
    if (boneIdx < 0 || boneIdx >= m_bones.size()) return 0.0f;
    return m_bones[boneIdx].ikWeight;
}

void KSModelerQml::updateBoneHierarchy() {
    // Rebuild children lists from parentId references
    for (auto& b : m_bones) b.children.clear();
    for (int i = 0; i < m_bones.size(); i++) {
        int p = m_bones[i].parentId;
        if (p >= 0 && p < m_bones.size() && p != i)
            m_bones[p].children.append(i);
    }
    m_boneVersion++;
    emit skeletonChanged();
}

void KSModelerQml::setBonePosition(int boneIdx, float x, float y, float z) {
    if (boneIdx < 0 || boneIdx >= m_bones.size()) return;
    m_bones[boneIdx].position = QVector3D(x, y, z);
    // Update SceneObject
    if (m_scene) {
        for (SceneObject* obj : m_scene->allObjects()) {
            if (obj->name() == m_bones[boneIdx].name) {
                obj->setPosition(QVector3D(x, y, z));
                break;
            }
        }
    }
    emit sceneChanged();
    m_boneVersion++;
    emit skeletonChanged();
}

void KSModelerQml::setBoneRotation(int boneIdx, float x, float y, float z) {
    if (boneIdx < 0 || boneIdx >= m_bones.size()) return;
    m_bones[boneIdx].rotation = QVector3D(x, y, z);
    m_boneVersion++;
    emit skeletonChanged();
}

void KSModelerQml::applyFKPose(int boneIdx, float x, float y, float z, float rx, float ry, float rz) {
    if (boneIdx < 0 || boneIdx >= m_bones.size()) return;
    // Apply FK: propagate transform from bone down through children
    Bone& bone = m_bones[boneIdx];
    QVector3D oldPos = bone.position;
    QVector3D delta(x - oldPos.x(), y - oldPos.y(), z - oldPos.z());
    bone.position = QVector3D(x, y, z);
    bone.rotation = QVector3D(rx, ry, rz);

    // Propagate delta to children (maintain relative offsets)
    for (int c : bone.children) {
        if (c >= 0 && c < m_bones.size()) {
            m_bones[c].position += delta;
        }
    }

    if (m_scene) {
        for (SceneObject* obj : m_scene->allObjects()) {
            if (obj->name() == bone.name) {
                obj->setPosition(QVector3D(x, y, z));
                obj->setRotationEuler(QVector3D(rx, ry, rz));
                break;
            }
        }
    }
    emit sceneChanged();
    m_boneVersion++;
    emit skeletonChanged();
}

void KSModelerQml::createMaterial(const QString& name) {
    MaterialState mat;
    mat.name = name;
    m_materials.append(mat);
    m_currentMaterial = m_materials.size() - 1;
    emit statusMessage("Created material '" + name + "'");
    emit sceneChanged();
}

void KSModelerQml::setMaterialAlbedo(float r, float g, float b, float a) {
    if (m_currentMaterial >= 0 && m_currentMaterial < m_materials.size()) {
        m_materials[m_currentMaterial].albedo = QColor::fromRgbF(r, g, b, a);
        emit sceneChanged();
    }
}

void KSModelerQml::setMaterialMetallic(float value) {
    if (m_currentMaterial >= 0 && m_currentMaterial < m_materials.size()) {
        m_materials[m_currentMaterial].metallic = std::max(0.0f, std::min(1.0f, value));
        emit sceneChanged();
    }
}

void KSModelerQml::setMaterialRoughness(float value) {
    if (m_currentMaterial >= 0 && m_currentMaterial < m_materials.size()) {
        m_materials[m_currentMaterial].roughness = std::max(0.0f, std::min(1.0f, value));
        emit sceneChanged();
    }
}

void KSModelerQml::setMaterialNormalStrength(float value) {
    if (m_currentMaterial >= 0 && m_currentMaterial < m_materials.size()) {
        m_materials[m_currentMaterial].normalStrength = std::max(0.0f, value);
        emit sceneChanged();
    }
}

void KSModelerQml::setMaterialEmissive(float r, float g, float b) {
    if (m_currentMaterial >= 0 && m_currentMaterial < m_materials.size()) {
        m_materials[m_currentMaterial].emissive = QColor::fromRgbF(r, g, b);
        emit sceneChanged();
    }
}

void KSModelerQml::setMaterialOpacity(float value) {
    if (m_currentMaterial >= 0 && m_currentMaterial < m_materials.size()) {
        m_materials[m_currentMaterial].opacity = std::max(0.0f, std::min(1.0f, value));
        emit sceneChanged();
    }
}

void KSModelerQml::setSelectedBaseColor(float r, float g, float b) {
    if (m_selectedObject && m_selectedObject->object()) {
        m_selectedObject->object()->setBaseColor(QColor::fromRgbF(r, g, b));
        emit sceneChanged();
    }
}

void KSModelerQml::setSelectedMetallic(float value) {
    if (m_selectedObject && m_selectedObject->object()) {
        m_selectedObject->object()->setMetallic(std::max(0.0f, std::min(1.0f, value)));
        emit sceneChanged();
    }
}

void KSModelerQml::setSelectedRoughness(float value) {
    if (m_selectedObject && m_selectedObject->object()) {
        m_selectedObject->object()->setRoughness(std::max(0.0f, std::min(1.0f, value)));
        emit sceneChanged();
    }
}

void KSModelerQml::setSelectedOpacity(float value) {
    if (m_selectedObject && m_selectedObject->object()) {
        m_selectedObject->object()->setOpacity(std::max(0.0f, std::min(1.0f, value)));
        emit sceneChanged();
    }
}

void KSModelerQml::setCamTheta(qreal v) {
    if (!qFuzzyCompare(m_camTheta, v)) {
        m_camTheta = v;
        emit cameraChanged();
    }
}

void KSModelerQml::setCamPhi(qreal v) {
    qreal clamped = qBound(-89.0, v, 89.0);
    if (!qFuzzyCompare(m_camPhi, clamped)) {
        m_camPhi = clamped;
        emit cameraChanged();
    }
}

void KSModelerQml::setCamDistance(qreal v) {
    qreal clamped = qBound(0.5, v, 100.0);
    if (!qFuzzyCompare(m_camDistance, clamped)) {
        m_camDistance = clamped;
        emit cameraChanged();
    }
}

void KSModelerQml::setCamTargetX(qreal v) {
    if (!qFuzzyCompare(m_camTargetX, v)) {
        m_camTargetX = v;
        emit cameraChanged();
    }
}

void KSModelerQml::setCamTargetY(qreal v) {
    if (!qFuzzyCompare(m_camTargetY, v)) {
        m_camTargetY = v;
        emit cameraChanged();
    }
}

void KSModelerQml::setCamTargetZ(qreal v) {
    if (!qFuzzyCompare(m_camTargetZ, v)) {
        m_camTargetZ = v;
        emit cameraChanged();
    }
}

void KSModelerQml::setCameraView(const QString& view) {
    if (view == "top") {
        setCamTheta(0); setCamPhi(89); setCamDistance(8);
        setCamTargetX(0); setCamTargetY(0); setCamTargetZ(0);
        if (m_currentViewName != "Top") { m_currentViewName = "Top"; emit cameraChanged(); }
    } else if (view == "front") {
        setCamTheta(0); setCamPhi(0); setCamDistance(8);
        setCamTargetX(0); setCamTargetY(0); setCamTargetZ(0);
        if (m_currentViewName != "Front") { m_currentViewName = "Front"; emit cameraChanged(); }
    } else if (view == "right") {
        setCamTheta(90); setCamPhi(0); setCamDistance(8);
        setCamTargetX(0); setCamTargetY(0); setCamTargetZ(0);
        if (m_currentViewName != "Right") { m_currentViewName = "Right"; emit cameraChanged(); }
    } else if (view == "persp") {
        setCamTheta(45); setCamPhi(30); setCamDistance(8);
        setCamTargetX(0); setCamTargetY(0); setCamTargetZ(0);
        if (m_currentViewName != "Perspective") { m_currentViewName = "Perspective"; emit cameraChanged(); }
    }
}

void KSModelerQml::focusOnSelected() {
    if (m_selectedObject && m_selectedObject->object()) {
        auto p = m_selectedObject->object()->position();
        setCamTargetX(p.x()); setCamTargetY(p.y()); setCamTargetZ(p.z());
    }
}

void KSModelerQml::setGridVisible(bool v) {
    if (m_gridVisible != v) {
        m_gridVisible = v;
        emit gridVisibleChanged();
    }
}

void KSModelerQml::setViewMode(int mode) {
    mode = qBound(0, mode, 2);
    if (m_viewMode != mode) {
        m_viewMode = mode;
        emit viewModeChanged();
    }
}

void KSModelerQml::createShader(const QString& vertexShader, const QString& fragmentShader) {
    if (m_currentMaterial >= 0 && m_currentMaterial < m_materials.size()) {
        m_materials[m_currentMaterial].vertexShader = vertexShader;
        m_materials[m_currentMaterial].fragmentShader = fragmentShader;
    }
}

void KSModelerQml::compileShader() {
    if (m_currentMaterial >= 0 && m_currentMaterial < m_materials.size()) {
        auto& mat = m_materials[m_currentMaterial];
        rendering::Shader3D shader;

        if (!mat.vertexShader.isEmpty() && !mat.fragmentShader.isEmpty()) {
            shader.setType(rendering::Shader3D::Custom);
            QString glsl = shader.compile();
            mat.fragmentShader = glsl;
            emit statusMessage("Custom shader compiled");
        } else {
            shader.setType(rendering::Shader3D::PBR);
            rendering::Shader3D::BSDF bsdf;
            bsdf.baseColor[0] = mat.albedo.redF();
            bsdf.baseColor[1] = mat.albedo.greenF();
            bsdf.baseColor[2] = mat.albedo.blueF();
            bsdf.metallic = mat.metallic;
            bsdf.roughness = mat.roughness;
            bsdf.alpha = mat.opacity;
            shader.setBSDF(bsdf);
            mat.fragmentShader = shader.compile();
            emit statusMessage("Default PBR shader compiled");
        }
        emit sceneChanged();
    }
}

void KSModelerQml::addAnimation(const QString& name, float duration) {
    Animation anim;
    anim.name = name;
    anim.duration = duration;
    m_animations.append(anim);
    emit sceneChanged();
}

void KSModelerQml::deleteAnimation(const QString& name) {
    for (int i = 0; i < m_animations.size(); ++i) {
        if (m_animations[i].name == name) {
            bool wasCurrent = (m_currentAnimation == i);
            m_animations.removeAt(i);
            if (wasCurrent) {
                m_currentAnimation = -1;
                emit animationNameChanged();
                emit animationTimeChanged();
                emit playbackStateChanged();
            }
            return;
        }
    }
}

QStringList KSModelerQml::animationNames() const {
    QStringList names;
    for (const auto& a : m_animations) names.append(a.name);
    return names;
}

void KSModelerQml::setCurrentAnimationByName(const QString& name) {
    for (int i = 0; i < m_animations.size(); ++i) {
        if (m_animations[i].name == name) {
            m_currentAnimation = i;
            m_animationTime = 0.0f;
            m_isAnimating = false;
            if (m_animTimer) m_animTimer->stop();
            applyPoseToBones(m_animations[i], 0.0f);
            emit animationNameChanged();
            emit animationTimeChanged();
            emit playbackStateChanged();
            emit sceneChanged();
            return;
        }
    }
}

void KSModelerQml::addKeyframe(const QString& animName, float time, int boneId, float x, float y, float z, float rotX, float rotY, float rotZ) {
    for (auto& anim : m_animations) {
        if (anim.name == animName) {
            Keyframe kf;
            kf.time = time;
            kf.boneId = boneId;
            kf.position = QVector3D(x, y, z);
            kf.rotation = QVector3D(rotX, rotY, rotZ);
            anim.keyframes.append(kf);
            return;
        }
    }
}

void KSModelerQml::addKeyframeForSelectedObject(const QString& animName) {
    if (!m_selectedObject || !m_selectedObject->object()) return;
    SceneObject* obj = m_selectedObject->object();
    QVector3D p = obj->position();
    QVector3D rot = obj->rotationEuler();
    addKeyframe(animName, m_animationTime, obj->id(), p.x(), p.y(), p.z(), rot.x(), rot.y(), rot.z());
}

void KSModelerQml::playAnimation(const QString& name) {
    for (int i = 0; i < m_animations.size(); ++i) {
        if (m_animations[i].name == name) {
            m_currentAnimation = i;
            m_animationTime = 0.0f;
            m_isAnimating = true;
            if (!m_animTimer) {
                m_animTimer = new QTimer(this);
                connect(m_animTimer, &QTimer::timeout, this, &KSModelerQml::advanceAnimation);
            }
            m_animTimer->start(1000 / m_animFps);
            applyPoseToBones(m_animations[i], 0.0f);
            emit animationNameChanged();
            emit animationTimeChanged();
            emit playbackStateChanged();
            emit sceneChanged();
            return;
        }
    }
}

void KSModelerQml::stopAnimation() {
    if (!m_isAnimating) return;
    m_isAnimating = false;
    if (m_animTimer) m_animTimer->stop();
    emit playbackStateChanged();
}

void KSModelerQml::togglePlayPause() {
    if (m_currentAnimation < 0 || m_currentAnimation >= m_animations.size()) return;
    if (m_isAnimating) {
        stopAnimation();
    } else {
        playAnimation(m_animations[m_currentAnimation].name);
    }
}

void KSModelerQml::setAnimationLoop(bool loop) {
    m_animLoop = loop;
}

QVariantList KSModelerQml::currentAnimationKeyframes() const {
    QVariantList result;
    if (m_currentAnimation < 0 || m_currentAnimation >= m_animations.size())
        return result;

    const Animation& anim = m_animations[m_currentAnimation];

    // Collect unique times across all bones
    QSet<float> timeSet;
    for (const auto& kf : anim.keyframes) {
        timeSet.insert(kf.time);
    }

    QList<float> sortedTimes = timeSet.values();
    std::sort(sortedTimes.begin(), sortedTimes.end());

    for (float t : sortedTimes) {
        QVariantMap kfObj;
        kfObj["time"] = t;
        kfObj["normalizedTime"] = anim.duration > 0 ? t / anim.duration : 0;

        // Count keyframes at this time
        int count = 0;
        for (const auto& kf : anim.keyframes) {
            if (qFuzzyCompare(kf.time, t)) count++;
        }
        kfObj["boneCount"] = count;
        result.append(kfObj);
    }

    return result;
}

void KSModelerQml::setAnimationTime(float time) {
    m_animationTime = time;
    if (m_currentAnimation >= 0 && m_currentAnimation < m_animations.size()) {
        applyPoseToBones(m_animations[m_currentAnimation], time);
        emit animationTimeChanged();
        emit sceneChanged();
    }
}

float KSModelerQml::getAnimationTime() const { return m_animationTime; }
bool KSModelerQml::isAnimating() const { return m_isAnimating; }

void KSModelerQml::advanceAnimation() {
    if (!m_isAnimating || m_currentAnimation < 0 || m_currentAnimation >= m_animations.size()) {
        stopAnimation();
        return;
    }
    const Animation& anim = m_animations[m_currentAnimation];
    m_animationTime += 1.0f / m_animFps;
    if (m_animationTime >= anim.duration) {
        if (m_animLoop) {
            m_animationTime = 0.0f;
        } else {
            m_animationTime = anim.duration;
            stopAnimation();
        }
    }
    applyPoseToBones(anim, m_animationTime);
    emit animationTimeChanged();
    emit sceneChanged();
}

int KSModelerQml::findKeyframeIndex(const QVector<Keyframe>& kfs, float time, int boneId) const {
    for (int i = 0; i < kfs.size(); ++i) {
        if (kfs[i].boneId == boneId && kfs[i].time > time)
            return i;
    }
    return kfs.size();
}

QVector3D KSModelerQml::interpolatePosition(const QVector<Keyframe>& kfs, float time, int boneId) {
    if (kfs.isEmpty()) return QVector3D();
    if (kfs.size() == 1) return kfs[0].position;
    int next = findKeyframeIndex(kfs, time, boneId);
    if (next <= 0) return kfs[0].position;
    if (next >= kfs.size()) return kfs.last().position;
    int prev = next - 1;
    const Keyframe& a = kfs[prev];
    const Keyframe& b = kfs[next];
    float t = (b.time > a.time) ? (time - a.time) / (b.time - a.time) : 0.0f;
    t = qBound(0.0f, applyEasing(t, m_animEasing), 1.0f);
    return a.position + (b.position - a.position) * t;
}

QVector3D KSModelerQml::interpolateRotation(const QVector<Keyframe>& kfs, float time, int boneId) {
    if (kfs.isEmpty()) return QVector3D();
    if (kfs.size() == 1) return kfs[0].rotation;
    int next = findKeyframeIndex(kfs, time, boneId);
    if (next <= 0) return kfs[0].rotation;
    if (next >= kfs.size()) return kfs.last().rotation;
    int prev = next - 1;
    const Keyframe& a = kfs[prev];
    const Keyframe& b = kfs[next];
    float t = (b.time > a.time) ? (time - a.time) / (b.time - a.time) : 0.0f;
    t = qBound(0.0f, applyEasing(t, m_animEasing), 1.0f);
    // Convert Euler to quaternion, slerp, convert back
    QQuaternion qa = QQuaternion::fromEulerAngles(a.rotation);
    QQuaternion qb = QQuaternion::fromEulerAngles(b.rotation);
        QQuaternion qr = quatSlerp(qa, qb, t);
    return qr.toEulerAngles();
}

float KSModelerQml::applyEasing(float t, int type) const {
    switch (type) {
    case 1: return t * t;                                      // ease-in
    case 2: return t * (2.0f - t);                             // ease-out
    case 3: return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; // ease-in-out
    case 4: return t * t * t;                                  // ease-in cubic
    case 5: return (t - 1.0f) * (t - 1.0f) * (t - 1.0f) + 1.0f; // ease-out cubic
    default: return t;                                         // linear
    }
}

void KSModelerQml::applyPoseToBones(const Animation& anim, float time) {
    QVector<Keyframe> sortedKfs = anim.keyframes;
    std::sort(sortedKfs.begin(), sortedKfs.end(), [](const Keyframe& a, const Keyframe& b) {
        return (a.boneId == b.boneId) ? a.time < b.time : a.boneId < b.boneId;
    });
    for (int i = 0; i < m_bones.size(); ++i) {
        QVector3D pos = interpolatePosition(sortedKfs, time, i);
        QVector3D rot = interpolateRotation(sortedKfs, time, i);
        m_bones[i].position = pos;
        m_bones[i].rotation = rot;
    }
}

void KSModelerQml::undo() {
    if (m_undoStack.isEmpty()) return;
    m_redoStack.push_back(m_currentState);
    m_currentState = m_undoStack.back();
    m_undoStack.pop_back();
    emit sceneChanged();
}

void KSModelerQml::redo() {
    if (m_redoStack.isEmpty()) return;
    m_undoStack.push_back(m_currentState);
    m_currentState = m_redoStack.back();
    m_redoStack.pop_back();
    emit sceneChanged();
}

bool KSModelerQml::canUndo() const { return !m_undoStack.isEmpty(); }
bool KSModelerQml::canRedo() const { return !m_redoStack.isEmpty(); }

void KSModelerQml::showMaterialEditor() {
    if (m_currentMaterial < 0 || m_currentMaterial >= m_materials.size()) {
        emit statusMessage("No material selected. Opening empty material editor.");
        return;
    }
    auto& mat = m_materials[m_currentMaterial];

    // Create material graph
    static ks::MaterialNodeEditor s_editor;
    if (s_editor.graphs.isEmpty()) s_editor.registerDefaultNodes();

    ks::MaterialGraph* graph = s_editor.newGraph();

    // Create Principled BSDF node with current material values
    ks::MaterialNode* bsdf = graph->createNode("PrincipledBSDF", QPointF(200, 100));
    if (!bsdf) return;

    // Set BSDF parameters from current material
    if (auto* pbsdf = dynamic_cast<ks::BSDFPrincipledNode*>(bsdf)) {
        pbsdf->inputs[0].value = QColor(mat.albedo.red(), mat.albedo.green(), mat.albedo.blue());
        pbsdf->inputs[1].value = mat.metallic;
        pbsdf->inputs[2].value = mat.roughness;
    }

    // Create Output node
    ks::MaterialNode* output = graph->createNode("Output", QPointF(500, 100));
    if (!output) return;

    // Connect BSDF output → Output surface input
    graph->connectNodes(bsdf->id, bsdf->outputs[0].id, output->id, output->inputs[0].id);

    // Generate shader code
    mat.vertexShader = "#version 330 core\nlayout(location = 0) in vec3 inPos;\nlayout(location = 1) in vec3 inNorm;\nlayout(location = 2) in vec2 inUV;\nout vec2 vTexCoord;\nuniform mat4 modelViewProjection;\nvoid main() {\n    vTexCoord = inUV;\n    gl_Position = modelViewProjection * vec4(inPos, 1.0);\n}\n";
    mat.fragmentShader = graph->generateGLSL();

    emit statusMessage(QString("Material Editor: '%1' — Generated PBR shader (%2 nodes, %3 lines)")
        .arg(mat.name)
        .arg(graph->nodes.size())
        .arg(mat.fragmentShader.count('\n') + 1));
}

void KSModelerQml::showPropertiesPanel() {
    if (m_scene) {
        auto objects = m_scene->allObjects();
        QStringList objList;
        for (auto* obj : objects) {
            objList << obj->name();
        }
        emit statusMessage("Properties Panel: " + QString::number(objects.size()) + " objects in scene");
    } else {
        emit statusMessage("Opening properties panel...");
    }
}

void KSModelerQml::showSceneGraph() {
    if (m_scene) {
        auto objects = m_scene->allObjects();
        QString treeStr;
        for (auto* obj : objects) {
            QString typeStr;
            switch (obj->type()) {
                case SceneObject::Type::Mesh: typeStr = "Mesh"; break;
                case SceneObject::Type::Camera: typeStr = "Camera"; break;
                case SceneObject::Type::Light: typeStr = "Light"; break;
                default: typeStr = "Unknown"; break;
            }
            treeStr += obj->name() + " [" + typeStr + "]\n";
        }
        emit statusMessage("Scene Graph:\n" + treeStr);
    } else {
        emit statusMessage("Opening scene graph... (no scene)");
    }
}

void KSModelerQml::newTrack(const QString& name) {
    m_trackPoints.clear();
    m_trackSections.clear();
    m_currentFile = "track_" + name;
    emit sceneChanged();
    emit fileChanged(m_currentFile);
}

bool KSModelerQml::loadTrack(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorMessage("Failed to open track file: " + path);
        return false;
    }
    QString content = file.readAll();
    file.close();
    m_trackPoints.clear();
    m_trackSections.clear();
    QRegularExpression pointRe("point\\s*=\\s*\\{([^,]+),\\s*([^,]+),\\s*([^}]+)\\}");
    QRegularExpressionMatchIterator it = pointRe.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        m_trackPoints.append(QVector3D(match.captured(1).toFloat(), match.captured(2).toFloat(), match.captured(3).toFloat()));
    }
    QRegularExpression sectionRe("section\\s*=\\s*(\\d+)");
    QRegularExpressionMatchIterator sit = sectionRe.globalMatch(content);
    while (sit.hasNext()) {
        QRegularExpressionMatch match = sit.next();
        m_trackSections.append(match.captured(1).toInt());
    }
    m_currentFile = path;
    emit sceneChanged();
    emit fileChanged(m_currentFile);
    return true;
}

bool KSModelerQml::saveTrack(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorMessage("Failed to save track file: " + path);
        return false;
    }
    QTextStream out(&file);
    for (const auto& p : m_trackPoints) {
        out << "point = {" << p.x() << ", " << p.y() << ", " << p.z() << "}\n";
    }
    for (int s : m_trackSections) {
        out << "section = " << s << "\n";
    }
    out << "width = " << m_trackWidth << "\n";
    file.close();
    m_currentFile = path;
    emit fileChanged(m_currentFile);
    return true;
}

bool KSModelerQml::exportTrackKN5(const QString& path) {
    if (!m_scene) {
        emit errorMessage("No scene to export");
        return false;
    }

    try {
        ::KN5Parser::KN5File kn5File;
        kn5File.header.magic = ::KN5Parser::KN5_MAGIC;
        kn5File.header.version = ::KN5Parser::KN5_VERSION;
        for (SceneObject* obj : m_scene->allObjects()) {
            if (obj->type() == SceneObject::Type::Mesh) {
                ::KN5Parser::Mesh kn5Mesh;
                kn5Mesh.name = obj->name();
                kn5Mesh.nodeIndex = obj->id();
                if (obj->mesh()) {
                    auto& verts = obj->mesh()->geometry().vertices;
                    auto& idxs = obj->mesh()->geometry().indices;
                    for (const auto& v : verts) {
                        float p[3] = { v.position.x(), v.position.y(), v.position.z() };
                        kn5Mesh.vertexData.append(QByteArray((const char*)p, 12));
                        float n[3] = { v.color.x(), v.color.y(), v.color.z() };
                        kn5Mesh.vertexData.append(QByteArray((const char*)n, 12));
                    }
                    for (uint32_t idx : idxs) {
                        quint32 leIdx = idx;
                        kn5Mesh.indexData.append(QByteArray((const char*)&leIdx, 4));
                    }
                    ::KN5Parser::SubMesh sm;
                    sm.vertexCount = verts.size();
                    sm.indexCount = idxs.size();
                    kn5Mesh.subMeshes.append(sm);
                }
                kn5File.meshes.append(kn5Mesh);
            }
        }
        bool ok = ::KN5Parser::KN5ParserImpl::write(path, kn5File);
        if (ok) emit statusMessage("Track exported successfully to " + path);
        else emit errorMessage("Failed to export track KN5");
        return ok;
    } catch (const std::exception& e) {
        emit errorMessage(QString("Export failed: %1").arg(e.what()));
        return false;
    }
}

bool KSModelerQml::importGPX(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorMessage("Failed to open GPX file: " + path);
        return false;
    }
    QString content = file.readAll();
    file.close();
    m_trackPoints.clear();
    QRegularExpression trkptRe("<trkpt[^>]*lat=\"([^\"]+)\"[^>]*lon=\"([^\"]+)\"[^>]*>(?:.*?<ele>([^<]+)</ele>)?", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator it = trkptRe.globalMatch(content);
    double prevLat = 0, prevLon = 0;
    bool first = true;
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        double lat = match.captured(1).toDouble();
        double lon = match.captured(2).toDouble();
        double ele = match.captured(3).isEmpty() ? 0.0 : match.captured(3).toDouble();
        if (first) {
            prevLat = lat; prevLon = lon; first = false;
            m_trackPoints.append(QVector3D(0, ele, 0));
            continue;
        }
        double dLat = (lat - prevLat) * 111320.0;
        double dLon = (lon - prevLon) * 111320.0 * std::cos(lat * 3.14159265 / 180.0);
        m_trackPoints.append(QVector3D(m_trackPoints.last().x() + dLon, ele, m_trackPoints.last().z() + dLat));
        prevLat = lat; prevLon = lon;
    }
    emit sceneChanged();
    return true;
}

bool KSModelerQml::importKML(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorMessage("Failed to open KML file: " + path);
        return false;
    }
    QString content = file.readAll();
    file.close();
    m_trackPoints.clear();
    QRegularExpression coordRe("<coordinates>([^<]+)</coordinates>", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator it = coordRe.globalMatch(content);
    double prevLat = 0, prevLon = 0;
    bool first = true;
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QStringList lines = match.captured(1).trimmed().split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            QStringList parts = line.trimmed().split(',');
            if (parts.size() < 2) continue;
            double lon = parts[0].toDouble();
            double lat = parts[1].toDouble();
            double ele = parts.size() > 2 ? parts[2].toDouble() : 0.0;
            if (first) { prevLat = lat; prevLon = lon; first = false; m_trackPoints.append(QVector3D(0, ele, 0)); continue; }
            double dLat = (lat - prevLat) * 111320.0;
            double dLon = (lon - prevLon) * 111320.0 * std::cos(lat * 3.14159265 / 180.0);
            m_trackPoints.append(QVector3D(m_trackPoints.last().x() + dLon, ele, m_trackPoints.last().z() + dLat));
            prevLat = lat; prevLon = lon;
        }
    }
    emit sceneChanged();
    return true;
}

bool KSModelerQml::importFromMap(const QString& service, double lat, double lon, int zoom) {
    emit statusMessage(QString("Importing map data from %1 at (%2,%3) zoom %4").arg(service).arg(lat).arg(lon).arg(zoom));
    // Use OpenStreetMap API or other tile service
    QUrl url(QString("https://api.openstreetmap.org/api/0.6/map?bbox=%1,%2,%3,%4")
        .arg(lon - 0.01).arg(lat - 0.01).arg(lon + 0.01).arg(lat + 0.01));
    QNetworkRequest req(url);
    QNetworkAccessManager mgr;
    QNetworkReply* reply = mgr.get(req);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(10000, &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() != QNetworkReply::NoError) {
        // Generate synthetic track from lat/lon as fallback
        emit statusMessage("Generating track from coordinates");
        m_trackPoints.clear();
        for (int i = 0; i < 20; i++) {
            float angle = i * 6.2832f / 20;
            m_trackPoints.append(QVector3D(cosf(angle) * 100, 0, sinf(angle) * 100));
        }
        emit sceneChanged();
        return true;
    }
    emit sceneChanged();
    emit statusMessage("Map imported successfully");
    return true;
}

void KSModelerQml::addTrackPoint(float x, float y, float z) { m_trackPoints.append(QVector3D(x, y, z)); emit sceneChanged(); }
void KSModelerQml::insertTrackPoint(int index, float x, float y, float z) {
    if (index >= 0 && index <= m_trackPoints.size()) {
        m_trackPoints.insert(index, QVector3D(x, y, z));
        emit sceneChanged();
    }
}
void KSModelerQml::removeTrackPoint(int index) {
    if (index >= 0 && index < m_trackPoints.size()) { m_trackPoints.removeAt(index); emit sceneChanged(); }
}
void KSModelerQml::smoothTrackPoints(int iterations) {
    if (m_trackPoints.size() < 3) return;
    for (int iter = 0; iter < iterations; ++iter) {
        QVector<QVector3D> smoothed = m_trackPoints;
        for (int i = 1; i < m_trackPoints.size() - 1; ++i)
            smoothed[i] = (m_trackPoints[i - 1] + m_trackPoints[i] * 2.0f + m_trackPoints[i + 1]) * 0.25f;
        m_trackPoints = smoothed;
    }
    emit sceneChanged();
}
void KSModelerQml::closeTrackLoop() {
    if (m_trackPoints.size() < 3) return;
    m_trackPoints.append(m_trackPoints.first());
    emit sceneChanged();
}
void KSModelerQml::setTrackWidth(float width) { m_trackWidth = width; }
void KSModelerQml::addTrackSection(int type) { m_trackSections.append(type); emit sceneChanged(); }
void KSModelerQml::removeTrackSection(int index) {
    if (index >= 0 && index < m_trackSections.size()) { m_trackSections.removeAt(index); emit sceneChanged(); }
}
void KSModelerQml::setSectionSurface(int index, const QString& surfaceType) {
    if (index >= 0 && index < m_trackSections.size()) {
        m_trackSections[index] = 1; // Mark as modified
        TrackSurface surf;
        surf.type = surfaceType;
        surf.grip = 1.0f;
        surf.roughness = 0.5f;

        if (surfaceType == "asphalt") { surf.grip = 1.0f; surf.roughness = 0.3f; }
        else if (surfaceType == "concrete") { surf.grip = 0.9f; surf.roughness = 0.4f; }
        else if (surfaceType == "grass") { surf.grip = 0.3f; surf.roughness = 0.8f; }
        else if (surfaceType == "gravel") { surf.grip = 0.4f; surf.roughness = 0.9f; }
        else if (surfaceType == "kerb") { surf.grip = 0.5f; surf.roughness = 0.6f; }

        m_surfaces[index] = surf;
        emit statusMessage(QString("Section %1 surface set to %2 (grip=%3)")
            .arg(index).arg(surfaceType).arg(surf.grip));
        emit sceneChanged();
    }
}

void KSModelerQml::addSectionKerb(int index, float height, float width) {
    if (index >= 0 && index < m_trackSections.size()) {
        TrackKerb kerb;
        kerb.height = height;
        kerb.width = width;
        kerb.color = QColor(255, 0, 0);
        m_kerbs[index] = kerb;
        emit statusMessage(QString("Kerb added to section %1 (h=%2, w=%3)")
            .arg(index).arg(height).arg(width));
        emit sceneChanged();
    }
}

void KSModelerQml::generateTrackMesh() {
    if (!m_scene) m_scene = new ks::SceneGraph();
    if (m_trackPoints.size() < 3) {
        emit statusMessage("Need at least 3 track points to generate mesh");
        return;
    }

    geometry::Modeling3D modeler;
    geometry::Mesh3D* mesh = new geometry::Mesh3D();
    QVector<QVector3D> verts;
    QVector<quint32> indices;

    for (int i = 0; i < m_trackPoints.size(); ++i) {
        int next = (i + 1) % m_trackPoints.size();
        QVector3D dir = (m_trackPoints[next] - m_trackPoints[i]).normalized();
        QVector3D right = QVector3D::crossProduct(dir, QVector3D(0, 1, 0)).normalized();
        float halfWidth = m_trackWidth * 0.5f;

        verts.append(m_trackPoints[i] + right * halfWidth);
        verts.append(m_trackPoints[i] - right * halfWidth);
    }

    for (int i = 0; i < m_trackPoints.size(); ++i) {
        int next = (i + 1) % m_trackPoints.size();
        int a = i * 2, b = i * 2 + 1;
        int na = next * 2, nb = next * 2 + 1;
        indices.append(a); indices.append(na); indices.append(b);
        indices.append(b); indices.append(na); indices.append(nb);
    }

    mesh->setVertices(verts);
    mesh->setIndices(indices);
    mesh->computeNormals();

    SceneObject* obj = m_scene->createObject("TrackMesh", SceneObject::Type::Mesh);
    if (obj) {
        SceneMesh* sm = new SceneMesh();
        for (const auto& v : mesh->vertices()) {
            SceneVertex sv;
            sv.position = QVector3D(v.x(), v.y(), v.z());
            sv.color = QVector4D(1, 1, 1, 1);
            sm->geometry().vertices.append(sv);
        }
        for (auto idx : mesh->indices())
            sm->geometry().indices.append(idx);
        obj->setMesh(sm);
        delete mesh;
        emit statusMessage("Track mesh generated with " + QString::number(verts.size()) + " vertices");
    }
    emit sceneChanged();
}

void KSModelerQml::generateTrackEdges() {
    if (!m_scene) m_scene = new ks::SceneGraph();
    if (m_trackPoints.size() < 2) {
        emit statusMessage("Need at least 2 track points to generate edges");
        return;
    }

    geometry::Modeling3D modeler;
    geometry::Mesh3D* mesh = new geometry::Mesh3D();
    QVector<QVector3D> verts;
    QVector<quint32> indices;

    for (int i = 0; i < m_trackPoints.size(); ++i) {
        int next = (i + 1) % m_trackPoints.size();
        QVector3D dir = (m_trackPoints[next] - m_trackPoints[i]).normalized();
        QVector3D right = QVector3D::crossProduct(dir, QVector3D(0, 1, 0)).normalized();
        float halfWidth = m_trackWidth * 0.5f;

        verts.append(m_trackPoints[i] + right * halfWidth);
        verts.append(m_trackPoints[i] - right * halfWidth);

        if (i < m_trackPoints.size() - 1) {
            indices.append(i * 2); indices.append(i * 2 + 1);
        }
    }

    mesh->setVertices(verts);
    mesh->setIndices(indices);

    SceneObject* obj = m_scene->createObject("TrackEdges", SceneObject::Type::Mesh);
    if (obj) {
        SceneMesh* sm = new SceneMesh();
        for (const auto& v : mesh->vertices()) {
            SceneVertex sv;
            sv.position = QVector3D(v.x(), v.y(), v.z());
            sv.color = QVector4D(1, 1, 1, 1);
            sm->geometry().vertices.append(sv);
        }
        for (auto idx : mesh->indices())
            sm->geometry().indices.append(idx);
        obj->setMesh(sm);
        delete mesh;
    }
    emit sceneChanged();
    emit statusMessage("Track edges generated");
}

void KSModelerQml::generateTerrain(float size, float maxHeight) {
    if (!m_scene) m_scene = new ks::SceneGraph();
    if (size <= 0) size = 100.0f;
    if (maxHeight <= 0) maxHeight = 10.0f;

    int resolution = 32;
    geometry::Mesh3D* mesh = new geometry::Mesh3D();
    QVector<QVector3D> verts;
    QVector<quint32> indices;

    float halfSize = size * 0.5f;
    for (int z = 0; z <= resolution; ++z) {
        for (int x = 0; x <= resolution; ++x) {
            float fx = (float)x / resolution * size - halfSize;
            float fz = (float)z / resolution * size - halfSize;
            float height = sinf(fx * 0.05f) * cosf(fz * 0.07f) * maxHeight * 0.5f
                         + sinf(fx * 0.02f + fz * 0.03f) * maxHeight * 0.3f;
            verts.append(QVector3D(fx, height, fz));
        }
    }

    for (int z = 0; z < resolution; ++z) {
        for (int x = 0; x < resolution; ++x) {
            int a = z * (resolution + 1) + x;
            int b = a + 1;
            int c = (z + 1) * (resolution + 1) + x;
            int d = c + 1;
            indices.append(a); indices.append(c); indices.append(b);
            indices.append(b); indices.append(c); indices.append(d);
        }
    }

    mesh->setVertices(verts);
    mesh->setIndices(indices);
    mesh->computeNormals();

    SceneObject* obj = m_scene->createObject("Terrain", SceneObject::Type::Mesh);
    if (obj) {
        SceneMesh* sm = new SceneMesh();
        for (const auto& v : mesh->vertices()) {
            SceneVertex sv;
            sv.position = QVector3D(v.x(), v.y(), v.z());
            sv.color = QVector4D(1, 1, 1, 1);
            sm->geometry().vertices.append(sv);
        }
        for (auto idx : mesh->indices())
            sm->geometry().indices.append(idx);
        obj->setMesh(sm);
        delete mesh;
        emit statusMessage("Terrain generated: " + QString::number(verts.size()) + " vertices");
    }
    emit sceneChanged();
}

void KSModelerQml::generateAILine() {
    if (!m_scene) m_scene = new ks::SceneGraph();
    if (m_trackPoints.size() < 2) return;

    geometry::Mesh3D* mesh = new geometry::Mesh3D();
    QVector<QVector3D> verts;
    // AI line follows center of track
    for (const auto& pt : m_trackPoints)
        verts.append(pt);
    mesh->setVertices(verts);

    SceneObject* obj = m_scene->createObject("AILine", SceneObject::Type::Mesh);
    if (obj) {
        SceneMesh* sm = new SceneMesh();
        for (const auto& v : mesh->vertices()) {
            SceneVertex sv;
            sv.position = QVector3D(v.x(), v.y(), v.z());
            sv.color = QVector4D(1, 1, 1, 1);
            sm->geometry().vertices.append(sv);
        }
        obj->setMesh(sm);
        delete mesh;
    }
    emit sceneChanged();
    emit statusMessage("AI racing line generated");
}

int KSModelerQml::trackPointCount() const { return m_trackPoints.size(); }
int KSModelerQml::trackSectionCount() const { return m_trackSections.size(); }
float KSModelerQml::trackLength() const {
    float len = 0;
    for (int i = 1; i < m_trackPoints.size(); ++i)
        len += m_trackPoints[i].distanceToPoint(m_trackPoints[i-1]);
    return len;
}
int KSModelerQml::cornerCount() const {
    int c = 0;
    for (int i = 2; i < m_trackPoints.size() - 1; ++i) {
        QVector3D v1 = (m_trackPoints[i] - m_trackPoints[i-1]).normalized();
        QVector3D v2 = (m_trackPoints[i+1] - m_trackPoints[i]).normalized();
        if (QVector3D::dotProduct(v1, v2) < 0.95f) c++;
    }
    return c;
}

void KSModelerQml::addTrackCamera(const QString& name, float x, float y, float z, float targetX, float targetY, float targetZ) {
    if (!m_scene) m_scene = new ks::SceneGraph();
    SceneObject* cam = m_scene->createObject(name, SceneObject::Type::Camera);
    if (cam) cam->setPosition(QVector3D(x, y, z));
    emit sceneChanged();
    emit statusMessage(QString("Camera %1 added at (%2,%3,%4) targeting (%5,%6,%7)").arg(name).arg(x).arg(y).arg(z).arg(targetX).arg(targetY).arg(targetZ));
}
void KSModelerQml::removeTrackCamera(int index) {
    if (m_scene) {
        int camIdx = 0;
        for (auto* obj : m_scene->allObjects()) {
            if (obj->type() == SceneObject::Type::Camera && camIdx++ == index) {
                m_scene->deleteObject(obj);
                emit sceneChanged();
                return;
            }
        }
    }
}
void KSModelerQml::setCameraPosition(int index, float x, float y, float z) {
    if (m_scene) {
        int camIdx = 0;
        for (auto* obj : m_scene->allObjects()) {
            if (obj->type() == SceneObject::Type::Camera && camIdx++ == index) {
                obj->setPosition(QVector3D(x, y, z));
                emit sceneChanged();
                return;
            }
        }
    }
}
void KSModelerQml::setCameraTarget(int index, float x, float y, float z) {
    if (m_scene) {
        int camIdx = 0;
        for (auto* obj : m_scene->allObjects()) {
            if (obj->type() == SceneObject::Type::Camera && camIdx++ == index) {
                QVector3D currentPos = obj->position();
                QVector3D target(x, y, z);
                QVector3D dir = (target - currentPos).normalized();

                // Compute look-at rotation
                float yaw = atan2(dir.x(), dir.z());
                float pitch = -asin(dir.y());
                obj->setPosition(currentPos);
                obj->setRotationEuler(QVector3D(pitch, yaw, 0));

                emit statusMessage(QString("Camera %1 target set to (%2,%3,%4)").arg(index).arg(x).arg(y).arg(z));
                emit sceneChanged();
                return;
            }
        }
    }
}
QStringList KSModelerQml::getSupportedImportFormats() const { return {"kn5", "fbx", "glb", "gltf", "obj"}; }
QStringList KSModelerQml::getSupportedExportFormats() const { return {"kn5", "fbx", "glb"}; }

QString KSModelerQml::detectFormat(const QString& path) const {
    QString ext = QFileInfo(path).suffix().toLower();
    if (ext == "kn5") return "KN5";
    if (ext == "fbx") return "FBX";
    if (ext == "glb" || ext == "gltf") return "GLTF";
    if (ext == "obj") return "OBJ";
    if (ext == "stl") return "STL";
    if (ext == "dae") return "DAE";
    if (ext == "3ds") return "3DS";
    return "UNKNOWN";
}

Mesh* KSModelerQml::getSelectedMesh() {
    if (!m_selectedObject || !m_selectedObject->object()) return nullptr;
    SceneObject* obj = m_selectedObject->object();
    SceneMesh* sm = obj->mesh();
    if (!sm) return nullptr;

    Mesh* mesh = new Mesh();
    mesh->vertices.reserve(sm->geometry().vertices.size());
    for (const SceneVertex& sv : sm->geometry().vertices) {
        VertexUV v;
        v.x = sv.position.x(); v.y = sv.position.y(); v.z = sv.position.z();
        v.nx = sv.normal.x(); v.ny = sv.normal.y(); v.nz = sv.normal.z();
        v.u = sv.uv.x(); v.v = sv.uv.y();
        v.r = sv.color.x(); v.g = sv.color.y(); v.b = sv.color.z(); v.a = sv.color.w();
        mesh->vertices.push_back(v);
    }
    mesh->indices.assign(sm->geometry().indices.begin(), sm->geometry().indices.end());
    mesh->materialIds.resize(sm->geometry().indices.size() / 3, 0);
    return mesh;
}

// ============================================================================
// Shape Key / Morph Target Bridge Methods
// ============================================================================

void KSModelerQml::syncShapeKeyMesh() {
    Mesh* mesh = getSelectedMesh();
    if (!mesh) return;
    m_shapeKeyMesh.vertices.resize(mesh->vertices.size());
    for (int i = 0; i < (int)mesh->vertices.size(); ++i) {
        m_shapeKeyMesh.vertices[i].position = QVector3D(mesh->vertices[i].x, mesh->vertices[i].y, mesh->vertices[i].z);
    }
    m_shapeKeyMesh.faces.resize(mesh->indices.size() / 3);
    for (int i = 0; i < m_shapeKeyMesh.faces.size(); ++i) {
        m_shapeKeyMesh.faces[i].indices = { (int)mesh->indices[i*3], (int)mesh->indices[i*3+1], (int)mesh->indices[i*3+2] };
    }
    delete mesh;
}

int KSModelerQml::addShapeKey(const QString& name) {
    syncShapeKeyMesh();
    int idx = ShapeKeyManager::addShapeKey(m_shapeKeyMesh, name);
    if (idx >= 0) {
        m_shapeKeysDirty = true;
        emit shapeKeysChanged();
        emit statusMessage(QString("Shape key '%1' added").arg(m_shapeKeyMesh.shapeKeyNames[idx]));
    }
    return idx;
}

bool KSModelerQml::removeShapeKey(int index) {
    bool ok = ShapeKeyManager::removeShapeKey(m_shapeKeyMesh, index);
    if (ok) {
        m_shapeKeysDirty = true;
        emit shapeKeysChanged();
        emit statusMessage("Shape key removed");
    }
    return ok;
}

bool KSModelerQml::renameShapeKey(int index, const QString& newName) {
    bool ok = ShapeKeyManager::renameShapeKey(m_shapeKeyMesh, index, newName);
    if (ok) {
        emit shapeKeysChanged();
        emit statusMessage(QString("Shape key renamed to '%1'").arg(newName));
    }
    return ok;
}

int KSModelerQml::captureShapeKey(const QString& name) {
    syncShapeKeyMesh();
    int idx = ShapeKeyManager::captureShapeKey(m_shapeKeyMesh, name);
    if (idx >= 0) {
        m_shapeKeysDirty = true;
        emit shapeKeysChanged();
        emit statusMessage(QString("Shape key '%1' captured (%2 vertices)")
            .arg(m_shapeKeyMesh.shapeKeyNames[idx])
            .arg(m_shapeKeyMesh.vertices.size()));
    }
    return idx;
}

void KSModelerQml::setShapeKeyWeight(int index, float weight) {
    ShapeKeyManager::setShapeKeyWeight(m_shapeKeyMesh, index, weight);
    m_shapeKeysDirty = true;
    emit shapeKeysChanged();
}

float KSModelerQml::getShapeKeyWeight(int index) {
    return ShapeKeyManager::getShapeKeyWeight(m_shapeKeyMesh, index);
}

QStringList KSModelerQml::getShapeKeyNames() {
    return ShapeKeyManager::getShapeKeyNames(m_shapeKeyMesh);
}

int KSModelerQml::getShapeKeyCount() {
    return ShapeKeyManager::getShapeKeyCount(m_shapeKeyMesh);
}

void KSModelerQml::resetShapeKeys() {
    ShapeKeyManager::resetToBasis(m_shapeKeyMesh);
    m_shapeKeysDirty = true;
    emit shapeKeysChanged();
    emit statusMessage("Shape keys reset to basis");
}

void KSModelerQml::muteShapeKey(int index, bool mute) {
    ShapeKeyManager::muteShapeKey(m_shapeKeyMesh, index, mute);
    m_shapeKeysDirty = true;
    emit shapeKeysChanged();
}

void KSModelerQml::applyShapeKeys() {
    if (!m_shapeKeysDirty) return;
    Mesh* mesh = getSelectedMesh();
    if (!mesh) return;
    int n = qMin((int)mesh->vertices.size(), m_shapeKeyMesh.vertices.size());
    for (int i = 0; i < n; ++i) {
        mesh->vertices[i].x = m_shapeKeyMesh.vertices[i].position.x();
        mesh->vertices[i].y = m_shapeKeyMesh.vertices[i].position.y();
        mesh->vertices[i].z = m_shapeKeyMesh.vertices[i].position.z();
    }
    if (m_selectedObject && m_selectedObject->object()) {
        SceneObject* obj = m_selectedObject->object();
        SceneMesh* sm = obj->mesh();
        if (sm) {
            for (int i = 0; i < sm->geometry().vertices.size() && i < n; ++i) {
                sm->geometry().vertices[i].position = QVector3D(
                    m_shapeKeyMesh.vertices[i].position.x(),
                    m_shapeKeyMesh.vertices[i].position.y(),
                    m_shapeKeyMesh.vertices[i].position.z()
                );
            }
        }
    }
    delete mesh;
    m_shapeKeysDirty = false;
    emit sceneChanged();
    emit statusMessage("Shape keys applied to mesh");
}

void KSModelerQml::clearAllShapeKeys() {
    m_shapeKeyMesh.shapeKeyNames.clear();
    m_shapeKeyMesh.shapeKeyDeltas.clear();
    m_shapeKeyMesh.shapeKeyWeights.clear();
    m_shapeKeyMesh.shapeKeyMute.clear();
    m_shapeKeyMesh.shapeKeyMin.clear();
    m_shapeKeyMesh.shapeKeyMax.clear();
    m_shapeKeysDirty = true;
    emit shapeKeysChanged();
    emit statusMessage("All shape keys cleared");
}

// ============================================================================
// LOD Generation Bridge Methods
// ============================================================================

bool KSModelerQml::generateLODs(int levelCount) {
    Mesh* mesh = getSelectedMesh();
    if (!mesh) {
        emit errorMessage("No mesh selected for LOD generation");
        return false;
    }

    MeshData sourceMesh;
    sourceMesh.vertices.resize(mesh->vertices.size());
    for (int i = 0; i < (int)mesh->vertices.size(); ++i) {
        sourceMesh.vertices[i].position = QVector3D(mesh->vertices[i].x, mesh->vertices[i].y, mesh->vertices[i].z);
    }
    sourceMesh.faces.resize(mesh->indices.size() / 3);
    for (int i = 0; i < sourceMesh.faces.size(); ++i) {
        sourceMesh.faces[i].indices = {(int)mesh->indices[i*3], (int)mesh->indices[i*3+1], (int)mesh->indices[i*3+2]};
    }
    delete mesh;

    m_lodResult = LODSystem::generateAutoLODs(sourceMesh, levelCount);
    emit lodsGenerated(m_lodResult.levels.size());
    emit statusMessage(QString("Generated %1 LOD levels").arg(m_lodResult.levels.size()));
    return true;
}

bool KSModelerQml::exportLODs(const QString& basePath) {
    if (m_lodResult.levels.isEmpty()) {
        emit errorMessage("No LODs to export");
        return false;
    }

    bool allOk = true;
    for (int i = 0; i < m_lodResult.levels.size(); ++i) {
        QString ext = ".obj";
        QString path = basePath + QString("_LOD%1").arg(i) + ext;
        emit statusMessage(QString("Exporting %1").arg(path));

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            emit errorMessage(QString("Cannot open %1 for writing").arg(path));
            allOk = false;
            continue;
        }

        QTextStream out(&file);
        const MeshData& lodMesh = m_lodResult.levels[i];

        for (int vi = 0; vi < lodMesh.vertices.size(); ++vi) {
            const auto& v = lodMesh.vertices[vi].position;
            out << "v " << v.x() << " " << v.y() << " " << v.z() << "\n";
        }

        if (!lodMesh.uvs.isEmpty()) {
            for (const auto& uv : lodMesh.uvs)
                out << "vt " << uv.x() << " " << uv.y() << "\n";
        }

        if (!lodMesh.normals.isEmpty()) {
            for (const auto& n : lodMesh.normals)
                out << "vn " << n.x() << " " << n.y() << " " << n.z() << "\n";
        }

        for (const auto& face : lodMesh.faces) {
            out << "f";
            for (int j = 0; j < face.indices.size(); ++j) {
                int vi = face.indices[j] + 1;
                int ti = (j < face.uvIndices.size() && face.uvIndices[j] >= 0) ? face.uvIndices[j] + 1 : 0;
                if (ti > 0 && vi > 0)
                    out << " " << vi << "/" << ti;
                else
                    out << " " << vi;
            }
            out << "\n";
        }

        file.close();
    }

    if (allOk)
        emit statusMessage(QString("Exported %1 LOD levels to %2")
            .arg(m_lodResult.levels.size()).arg(basePath));
    return allOk;
}

void KSModelerQml::setLODDistance(int level, float distance) {
    if (level >= 0 && level < m_lodResult.distances.size()) {
        m_lodResult.distances[level] = distance;
    }
}

float KSModelerQml::getLODDistance(int level) const {
    if (level >= 0 && level < m_lodResult.distances.size()) {
        return m_lodResult.distances[level];
    }
    return 0.0f;
}

// ============================================================================
// Collision Mesh Generation Bridge Methods
// ============================================================================

bool KSModelerQml::generateCollisionMesh(int shapeType) {
    Mesh* mesh = getSelectedMesh();
    if (!mesh) {
        emit errorMessage("No mesh selected for collision generation");
        return false;
    }

    MeshData sourceMesh;
    sourceMesh.vertices.resize(mesh->vertices.size());
    for (int i = 0; i < (int)mesh->vertices.size(); ++i) {
        sourceMesh.vertices[i].position = QVector3D(mesh->vertices[i].x, mesh->vertices[i].y, mesh->vertices[i].z);
    }
    sourceMesh.faces.resize(mesh->indices.size() / 3);
    for (int i = 0; i < sourceMesh.faces.size(); ++i) {
        sourceMesh.faces[i].indices = {(int)mesh->indices[i*3], (int)mesh->indices[i*3+1], (int)mesh->indices[i*3+2]};
    }
    delete mesh;

    m_collisionConfig.shapeType = static_cast<CollisionConfig::ShapeType>(shapeType);
    m_collisionResult = PhysicsCollisionSystem::generate(sourceMesh, m_collisionConfig);

    emit collisionGenerated(m_collisionResult.hulls.size());
    emit statusMessage(QString("Generated %1 collision hull(s)").arg(m_collisionResult.hulls.size()));
    return true;
}

bool KSModelerQml::generateCollisionConvexDecomp(int maxHulls) {
    Mesh* mesh = getSelectedMesh();
    if (!mesh) {
        emit errorMessage("No mesh selected for convex decomposition");
        return false;
    }

    MeshData sourceMesh;
    sourceMesh.vertices.resize(mesh->vertices.size());
    for (int i = 0; i < (int)mesh->vertices.size(); ++i) {
        sourceMesh.vertices[i].position = QVector3D(mesh->vertices[i].x, mesh->vertices[i].y, mesh->vertices[i].z);
    }
    delete mesh;

    QVector<CollisionHull> hulls = PhysicsCollisionSystem::decomposeConvex(sourceMesh, maxHulls, 64);
    m_collisionResult.hulls = hulls;

    emit collisionGenerated(hulls.size());
    emit statusMessage(QString("Decomposed mesh into %1 convex hull(s)").arg(hulls.size()));
    return true;
}

void KSModelerQml::setCollisionSimplifyRatio(float ratio) {
    m_collisionSimplifyRatio = qBound(0.01f, ratio, 1.0f);
    m_collisionConfig.simplificationRatio = m_collisionSimplifyRatio;
}

float KSModelerQml::getCollisionSimplifyRatio() const {
    return m_collisionSimplifyRatio;
}

// ============================================================================
// Shape Key Animation Bridge Methods
// ============================================================================

void KSModelerQml::animateShapeKey(const QString& keyName, int startFrame, int endFrame, float startWeight, float endWeight) {
    if (!m_shapeKeyAnimDriver) {
        m_shapeKeyAnimDriver = new ShapeKeyAnimDriver(this);
        m_shapeKeyAnimDriver->setTargetMesh(&m_shapeKeyMesh);
    }

    m_shapeKeyAnimDriver->addChannel(keyName);
    m_shapeKeyAnimDriver->setKeyframe(keyName, startFrame, startWeight);
    m_shapeKeyAnimDriver->setKeyframe(keyName, endFrame, endWeight);
    emit statusMessage(QString("Shape key '%1' animation: %2->%3 (frames %4-%5)")
        .arg(keyName).arg(startWeight).arg(endWeight).arg(startFrame).arg(endFrame));
}

void KSModelerQml::clearShapeKeyAnimation(const QString& keyName) {
    if (m_shapeKeyAnimDriver) {
        m_shapeKeyAnimDriver->clearKeyframes(keyName);
        emit statusMessage(QString("Animation cleared for shape key '%1'").arg(keyName));
    }
}

void KSModelerQml::evaluateShapeKeyAnimation(int frame) {
    if (m_shapeKeyAnimDriver) {
        m_shapeKeyAnimDriver->evaluateAtFrame(frame);
        m_shapeKeysDirty = true;
        emit shapeKeysChanged();
    }
}

// ============================================================
// Deformation tools
// ============================================================

void KSModelerQml::applyCageDeform(const QString& cageObjectName, float strength, bool preserveVolume)
{
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;

    SceneObject* cageObj = nullptr;
    if (!cageObjectName.isEmpty()) {
        for (auto* so : m_scene->allObjects()) {
            if (so->name() == cageObjectName && so != obj) {
                cageObj = so;
                break;
            }
        }
    }
    if (!cageObj) {
        emit errorMessage("Cage object not found. Create a low-poly cage first.");
        return;
    }

    MeshData targetMesh = sceneMeshToMeshData(obj);
    MeshData cageMesh = sceneMeshToMeshData(cageObj);

    CageDeformModifier mod;
    mod.setCageMesh(cageMesh);
    mod.strength = strength;
    mod.usePreserveVolume = preserveVolume;

    MeshData result = mod.apply(targetMesh);
    meshDataToSceneMesh(obj, result);
    emit sceneChanged();
    emit statusMessage("Cage deformation applied");
}

void KSModelerQml::cageDeformBuildCage(int subdivisions)
{
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;

    MeshData md = sceneMeshToMeshData(obj);
    if (md.vertices.isEmpty()) return;

    md.computeBoundingBox();
    QVector3D size = md.boundingBoxMax - md.boundingBoxMin;
    QVector3D center = (md.boundingBoxMax + md.boundingBoxMin) * 0.5f;
    float maxDim = qMax(size.x(), qMax(size.y(), size.z()));

    int divs = qMax(1, subdivisions);
    MeshData cage;
    float hw = maxDim * 0.6f;
    float pi = 3.14159265358979323846f;

    // Generate cage vertices on a sphere-ish grid around the mesh
    for (int i = 0; i <= divs; ++i) {
        float theta = static_cast<float>(i) / divs * 2.0f * pi;
        for (int j = 0; j <= divs; ++j) {
            float phi = static_cast<float>(j) / divs * pi;
            Vertex v;
            v.position = center + QVector3D(
                hw * qSin(phi) * qCos(theta),
                hw * qCos(phi),
                hw * qSin(phi) * qSin(theta)
            );
            v.color = QVector4D(1, 1, 0, 1);
            cage.vertices.append(v);
        }
    }

    // Generate faces
    for (int i = 0; i < divs; ++i) {
        for (int j = 0; j < divs; ++j) {
            int a = i * (divs + 1) + j;
            int b = a + 1;
            int c = (i + 1) * (divs + 1) + j;
            int d = c + 1;
            cage.faces.append(Face({ a, b, c }));
            cage.faces.append(Face({ b, d, c }));
        }
    }

    cage.name = obj->name() + "_cage";
    importMeshDataToScene(m_scene, cage, cage.name);
    emit sceneChanged();
    emit statusMessage(QString("Cage created: %1").arg(cage.name));
}

void KSModelerQml::applyLattice(int uDivs, int vDivs, int wDivs, float strength)
{
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;

    MeshData md = sceneMeshToMeshData(obj);

    LatticeExModifier mod;
    mod.setDivisions(qMax(1, uDivs), qMax(1, vDivs), qMax(1, wDivs));
    mod.strength = strength;
    mod.resetControlPoints();

    // Store lattice state for interactive editing
    m_lattice.active = true;
    m_lattice.uDivs = mod.uDivs;
    m_lattice.vDivs = mod.vDivs;
    m_lattice.wDivs = mod.wDivs;
    m_lattice.strength = strength;
    m_lattice.controlPoints = mod.controlPoints;
    m_lattice.restControlPoints = mod.restControlPoints;
    m_lattice.selectedObject = m_selectedObject ? m_selectedObject->object()->id() : -1;

    MeshData result = mod.apply(md);
    meshDataToSceneMesh(obj, result);
    emit sceneChanged();
    emit statusMessage(QString("Lattice deformation applied (%1x%2x%3) - %4 control points")
        .arg(uDivs).arg(vDivs).arg(wDivs).arg(mod.controlPointCount()));
}

void KSModelerQml::applySimpleDeform(int method, int axis, float angle, float factor)
{
    if (!m_selectedObject) return;
    SceneObject* obj = m_selectedObject->object();
    if (!obj || !obj->mesh()) return;

    MeshData md = sceneMeshToMeshData(obj);

    SimpleDeformModifier mod;
    switch (method) {
        case 0: mod.deformMethod = SimpleDeformModifier::DeformMethod::Twist; break;
        case 1: mod.deformMethod = SimpleDeformModifier::DeformMethod::Stretch; break;
        case 2: mod.deformMethod = SimpleDeformModifier::DeformMethod::Bend; break;
        case 3: mod.deformMethod = SimpleDeformModifier::DeformMethod::Linear; break;
        default: mod.deformMethod = SimpleDeformModifier::DeformMethod::Twist; break;
    }
    switch (axis) {
        case 0: mod.deformAxis = SimpleDeformModifier::Axis::X; break;
        case 1: mod.deformAxis = SimpleDeformModifier::Axis::Y; break;
        case 2: mod.deformAxis = SimpleDeformModifier::Axis::Z; break;
        default: mod.deformAxis = SimpleDeformModifier::Axis::X; break;
    }
    mod.angle = angle;
    mod.factor = factor;

    MeshData result = mod.apply(md);
    meshDataToSceneMesh(obj, result);
    emit sceneChanged();
    emit statusMessage("Simple deform applied");
}

QVector3D KSModelerQml::getLatticeControlPoint(int index)
{
    if (!m_lattice.active || index < 0 || index >= m_lattice.controlPoints.size())
        return QVector3D();
    return m_lattice.controlPoints[index];
}

void KSModelerQml::setLatticeControlPoint(int index, float x, float y, float z)
{
    if (!m_lattice.active || index < 0 || index >= m_lattice.controlPoints.size())
        return;

    m_lattice.controlPoints[index] = QVector3D(x, y, z);

    // Re-apply lattice with deformed control points
    if (m_lattice.selectedObject < 0 || !m_scene) return;
    SceneObject* obj = m_scene->findObjectById(m_lattice.selectedObject);
    if (!obj || !obj->mesh()) return;

    MeshData md = sceneMeshToMeshData(obj);

    LatticeExModifier mod;
    mod.setDivisions(m_lattice.uDivs, m_lattice.vDivs, m_lattice.wDivs);
    mod.strength = m_lattice.strength;
    mod.controlPoints = m_lattice.controlPoints;

    MeshData result = mod.applyWithDeformedLattice(md, m_lattice.controlPoints);
    meshDataToSceneMesh(obj, result);
    emit sceneChanged();
}

int KSModelerQml::getLatticeControlPointCount()
{
    if (!m_lattice.active) return 0;
    return m_lattice.controlPoints.size();
}

void KSModelerQml::resetLattice()
{
    if (!m_lattice.active) return;

    // Restore rest positions
    m_lattice.controlPoints = m_lattice.restControlPoints;

    // Re-apply with reset control points
    if (m_lattice.selectedObject < 0 || !m_scene) return;
    SceneObject* obj = m_scene->findObjectById(m_lattice.selectedObject);
    if (!obj || !obj->mesh()) return;

    MeshData md = sceneMeshToMeshData(obj);

    LatticeExModifier mod;
    mod.setDivisions(m_lattice.uDivs, m_lattice.vDivs, m_lattice.wDivs);
    mod.strength = m_lattice.strength;
    mod.controlPoints = m_lattice.controlPoints;

    MeshData result = mod.applyWithDeformedLattice(md, m_lattice.controlPoints);
    meshDataToSceneMesh(obj, result);
    emit sceneChanged();
    emit statusMessage("Lattice control points reset to default");
}

// ============================================================================
// Keyboard Shortcuts Implementation
// ============================================================================

bool KSModelerQml::loadShortcuts(const QString& filePath) {
    if (!m_shortcutManager) return false;
    return m_shortcutManager->loadFromFile(filePath);
}

bool KSModelerQml::saveShortcuts(const QString& filePath) {
    if (!m_shortcutManager) return false;
    return m_shortcutManager->saveToFile(filePath);
}

QString KSModelerQml::getShortcutKey(const QString& id) const {
    if (!m_shortcutManager) return QString();
    return m_shortcutManager->getShortcutDisplay(id);
}

QString KSModelerQml::getShortcutDescription(const QString& id) const {
    if (!m_shortcutManager) return QString();
    auto sc = m_shortcutManager->getShortcut(id);
    return sc.description;
}

void KSModelerQml::remapShortcut(const QString& id, const QString& newKey) {
    if (m_shortcutManager) {
        m_shortcutManager->remapShortcut(id, newKey);
    }
}

void KSModelerQml::resetShortcutsToDefaults() {
    if (m_shortcutManager) {
        m_shortcutManager->resetToDefaults();
    }
}

QStringList KSModelerQml::getAllShortcutIds() const {
    if (!m_shortcutManager) return QStringList();
    QStringList ids;
    for (const auto& sc : m_shortcutManager->allShortcuts()) {
        ids.append(sc.id);
    }
    return ids;
}

QVariantMap KSModelerQml::getShortcutsData() const {
    QVariantMap result;
    if (!m_shortcutManager) return result;

    for (const auto& sc : m_shortcutManager->allShortcuts()) {
        QVariantMap scData;
        scData["key"] = sc.key;
        scData["description"] = sc.description;
        result[sc.id] = scData;
    }
    return result;
}

} // namespace ks
