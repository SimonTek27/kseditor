#include "AlembicParser.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <QDebug>
#include <cmath>

namespace ks {


// Note: Real Alembic support requires the Alembic C++ library (AbcCore)
// This is a simplified implementation for demonstration

bool AlembicParser::read(const QString& filePath, AlembicArchive& archive, QString* error) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = "Cannot open file: " + filePath;
        return false;
    }

    // Check magic bytes for Alembic (.abc) - HDF5 or Ogawa format
    QByteArray header = file.read(8);
    file.seek(0);
    
    if (header.size() < 8) {
        if (error) *error = "File too small to be valid Alembic";
        return false;
    }

    // For now, we'll parse a JSON representation or use a simplified format
    // Real implementation would link against Alembic library
    
    // Try to read as JSON (for testing/export from other tools)
    file.seek(0);
    QByteArray content = file.readAll();
    file.close();
    
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(content, &jsonError);
    
    if (!jsonError.error) {
        return parseJsonRepresentation(doc.object(), archive);
    }
    
    // If not JSON, try to parse binary format would need Alembic library
    if (error) *error = "Binary Alembic format requires Alembic C++ library. Use JSON representation for now.";
    return false;
}

bool AlembicParser::parseJsonRepresentation(const QJsonObject& obj, AlembicArchive& archive) {
    archive.filePath = obj["filePath"].toString();
    archive.startTime = obj["startTime"].toDouble(1.0);
    archive.endTime = obj["endTime"].toDouble(100.0);
    archive.timeSamplingRate = obj["timeSamplingRate"].toDouble(24.0);
    
    QJsonObject meta = obj["metadata"].toObject();
    for (auto it = meta.constBegin(); it != meta.constEnd(); ++it) {
        archive.metadata[it.key().toStdString()] = it.value().toVariant();
    }
    
    // Parse objects
    QJsonArray objectsArr = obj["objects"].toArray();
    for (const auto& v : objectsArr) {
        QJsonObject o = v.toObject();
        AlembicObject aobj;
        aobj.fullName = o["fullName"].toString().toStdString();
        aobj.typeName = o["typeName"].toString().toStdString();
        aobj.parent = o["parent"].toString().toStdString();
        
        QJsonArray childrenArr = o["children"].toArray();
        for (const auto& c : childrenArr) {
            aobj.children.append(c.toString().toStdString());
        }
        
        QJsonObject props = o["properties"].toObject();
        for (auto it = props.constBegin(); it != props.constEnd(); ++it) {
            aobj.properties[it.key().toStdString()] = it.value().toVariant();
        }
        
        archive.objects.append(aobj);
    }
    
    // Parse meshes
    QJsonArray meshesArr = obj["meshes"].toArray();
    for (const auto& v : meshesArr) {
        QJsonObject m = v.toObject();
        AlembicMesh mesh;
        mesh.objectPath = m["objectPath"].toString().toStdString();
        mesh.subdivisionScheme = m["subdivisionScheme"].toString().toStdString();
        
        QJsonArray samplesArr = m["samples"].toArray();
        for (const auto& s : samplesArr) {
            QJsonObject samp = s.toObject();
            AlembicMesh::Sample sample;
            sample.time = samp["time"].toDouble();
            
            // Positions
            QJsonArray posArr = samp["positions"].toArray();
            for (const auto& p : posArr) {
                QJsonArray v = p.toArray();
                sample.positions.append(QVector3D(v[0].toDouble(), v[1].toDouble(), v[2].toDouble()));
            }
            
            // Velocities
            QJsonArray velArr = samp["velocities"].toArray();
            for (const auto& p : velArr) {
                QJsonArray v = p.toArray();
                sample.velocities.append(QVector3D(v[0].toDouble(), v[1].toDouble(), v[2].toDouble()));
            }
            
            // Normals
            QJsonArray normArr = samp["normals"].toArray();
            for (const auto& p : normArr) {
                QJsonArray v = p.toArray();
                sample.normals.append(QVector3D(v[0].toDouble(), v[1].toDouble(), v[2].toDouble()));
            }
            
            // UVs
            QJsonArray uvArr = samp["uvs"].toArray();
            for (const auto& p : uvArr) {
                QJsonArray v = p.toArray();
                sample.uvs.append(QVector2D(v[0].toDouble(), v[1].toDouble()));
            }
            
            // Face counts
            QJsonArray fcArr = samp["faceCounts"].toArray();
            for (const auto& f : fcArr) {
                sample.faceCounts.append(f.toInt());
            }
            
            // Face indices
            QJsonArray fiArr = samp["faceIndices"].toArray();
            for (const auto& f : fiArr) {
                sample.faceIndices.append(f.toInt());
            }
            
            // Material IDs
            QJsonArray miArr = samp["materialIds"].toArray();
            for (const auto& f : miArr) {
                sample.materialIds.append(f.toInt());
            }
            
            mesh.samples.append(sample);
        }
        
        archive.meshes.append(mesh);
    }
    
    // Parse cameras
    QJsonArray camerasArr = obj["cameras"].toArray();
    for (const auto& v : camerasArr) {
        QJsonObject c = v.toObject();
        AlembicCamera cam;
        cam.objectPath = c["objectPath"].toString().toStdString();
        
        QJsonArray samplesArr = c["samples"].toArray();
        for (const auto& s : samplesArr) {
            QJsonObject samp = s.toObject();
            AlembicCamera::Sample sample;
            sample.time = samp["time"].toDouble();
            sample.fov = samp["fov"].toDouble();
            sample.focusDistance = samp["focusDistance"].toDouble();
            sample.aperture = samp["aperture"].toDouble();
            
            QJsonArray pos = samp["position"].toArray();
            sample.position = QVector3D(pos[0].toDouble(), pos[1].toDouble(), pos[2].toDouble());
            
            QJsonArray rot = samp["rotation"].toArray();
            sample.rotation = QQuaternion(rot[0].toDouble(), rot[1].toDouble(), rot[2].toDouble(), rot[3].toDouble());
            
            cam.samples.append(sample);
        }
        
        archive.cameras.append(cam);
    }
    
    return true;
}

bool AlembicParser::write(const QString& filePath, const AlembicArchive& archive, QString* error) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = "Cannot write file: " + filePath;
        return false;
    }
    
    QJsonObject obj;
    obj["filePath"] = archive.filePath;
    obj["startTime"] = archive.startTime;
    obj["endTime"] = archive.endTime;
    obj["timeSamplingRate"] = archive.timeSamplingRate;
    
    QJsonObject meta;
    for (auto it = archive.metadata.constBegin(); it != archive.metadata.constEnd(); ++it) {
        meta[QString::fromStdString(it.key())] = QJsonValue::fromVariant(it.value());
    }
    obj["metadata"] = meta;
    
    // Objects
    QJsonArray objectsArr;
    for (const auto& aobj : archive.objects) {
        QJsonObject o;
        o["fullName"] = QString::fromStdString(aobj.fullName);
        o["typeName"] = QString::fromStdString(aobj.typeName);
        o["parent"] = QString::fromStdString(aobj.parent);
        
        QJsonArray childrenArr;
        for (const auto& c : aobj.children) {
            childrenArr.append(QString::fromStdString(c));
        }
        o["children"] = childrenArr;
        
        QJsonObject props;
        for (auto it = aobj.properties.constBegin(); it != aobj.properties.constEnd(); ++it) {
            props[QString::fromStdString(it.key())] = QJsonValue::fromVariant(it.value());
        }
        o["properties"] = props;
        
        objectsArr.append(o);
    }
    obj["objects"] = objectsArr;
    
    // Meshes
    QJsonArray meshesArr;
    for (const auto& mesh : archive.meshes) {
        QJsonObject m;
        m["objectPath"] = QString::fromStdString(mesh.objectPath);
        m["subdivisionScheme"] = QString::fromStdString(mesh.subdivisionScheme);
        
        QJsonArray samplesArr;
        for (const auto& sample : mesh.samples) {
            QJsonObject s;
            s["time"] = sample.time;
            
            QJsonArray posArr;
            for (const auto& p : sample.positions) {
                posArr.append(QJsonArray{p.x(), p.y(), p.z()});
            }
            s["positions"] = posArr;
            
            QJsonArray velArr;
            for (const auto& p : sample.velocities) {
                velArr.append(QJsonArray{p.x(), p.y(), p.z()});
            }
            s["velocities"] = velArr;
            
            QJsonArray normArr;
            for (const auto& p : sample.normals) {
                normArr.append(QJsonArray{p.x(), p.y(), p.z()});
            }
            s["normals"] = normArr;
            
            QJsonArray uvArr;
            for (const auto& p : sample.uvs) {
                uvArr.append(QJsonArray{p.x(), p.y()});
            }
            s["uvs"] = uvArr;
            
            s["faceCounts"] = QJsonArray::fromVariantList(
                QVariant::fromValue(sample.faceCounts).toList());
            s["faceIndices"] = QJsonArray::fromVariantList(
                QVariant::fromValue(sample.faceIndices).toList());
            s["materialIds"] = QJsonArray::fromVariantList(
                QVariant::fromValue(sample.materialIds).toList());
            
            samplesArr.append(s);
        }
        m["samples"] = samplesArr;
        
        meshesArr.append(m);
    }
    obj["meshes"] = meshesArr;
    
    // Cameras
    QJsonArray camerasArr;
    for (const auto& cam : archive.cameras) {
        QJsonObject c;
        c["objectPath"] = QString::fromStdString(cam.objectPath);
        
        QJsonArray samplesArr;
        for (const auto& sample : cam.samples) {
            QJsonObject s;
            s["time"] = sample.time;
            s["fov"] = sample.fov;
            s["focusDistance"] = sample.focusDistance;
            s["aperture"] = sample.aperture;
            s["position"] = QJsonArray{sample.position.x(), sample.position.y(), sample.position.z()};
            s["rotation"] = QJsonArray{sample.rotation.x(), sample.rotation.y(), sample.rotation.z(), sample.rotation.scalar()};
            
            samplesArr.append(s);
        }
        c["samples"] = samplesArr;
        
        camerasArr.append(c);
    }
    obj["cameras"] = camerasArr;
    
    QJsonDocument doc(obj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    return true;
}

bool AlembicParser::alembicMeshToMeshData(const AlembicMesh& mesh, int frameIndex, MeshData& outData) {
    if (frameIndex < 0 || frameIndex >= mesh.samples.size()) return false;
    
    const auto& sample = mesh.samples[frameIndex];
    
    outData.vertices.reserve(sample.positions.size());
    for (int i = 0; i < sample.positions.size(); ++i) {
        MeshVertex v{};
        v.px = sample.positions[i].x();
        v.py = sample.positions[i].y();
        v.pz = sample.positions[i].z();
        if (i < sample.normals.size()) {
            v.nx = sample.normals[i].x();
            v.ny = sample.normals[i].y();
            v.nz = sample.normals[i].z();
        }
        if (i < sample.uvs.size()) {
            v.u0 = sample.uvs[i].x();
            v.v0 = sample.uvs[i].y();
        }
        outData.vertices.append(v);
    }
    
    // Build indices from face counts and face indices
    for (int idx : sample.faceIndices) {
        outData.indices.append(static_cast<uint32_t>(idx));
    }
    
    // Build submeshes from material IDs
    int indexOffset = 0;
    for (int i = 0; i < sample.faceCounts.size(); ++i) {
        int count = sample.faceCounts[i];
        if (i < sample.materialIds.size()) {
            MeshSubmesh sub;
            sub.indexOffset = indexOffset;
            sub.indexCount = count;
            outData.submeshes.append(sub);
        }
        indexOffset += count;
    }
    
    return true;
}

bool AlembicParser::meshDataToAlembicMesh(const MeshData& data, AlembicMesh& outMesh) {
    AlembicMesh::Sample sample;
    sample.time = 1.0;
    
    sample.positions.reserve(data.vertices.size());
    for (const auto& v : data.vertices) {
        sample.positions.append(QVector3D(v.px, v.py, v.pz));
        if (v.nx != 0 || v.ny != 0 || v.nz != 0) {
            sample.normals.append(QVector3D(v.nx, v.ny, v.nz));
        }
        if (v.u0 != 0 || v.v0 != 0) {
            sample.uvs.append(QVector2D(v.u0, v.v0));
        }
    }
    
    // Build face counts from submeshes
    for (const auto& sub : data.submeshes) {
        sample.faceCounts.append(sub.indexCount);
        for (uint32_t i = 0; i < sub.indexCount; ++i) {
            uint32_t idx = sub.indexOffset + i;
            if (idx < data.indices.size()) {
                sample.faceIndices.append(data.indices[idx]);
            }
        }
    }
    
    outMesh.samples.append(sample);
    return true;
}

bool AlembicParser::sampleMeshAtTime(const AlembicMesh& mesh, double time, MeshData& outData) {
    if (mesh.samples.isEmpty()) return false;
    
    // Find frame index
    int frameIndex = 0;
    for (int i = 1; i < mesh.samples.size(); ++i) {
        if (mesh.samples[i].time > time) break;
        frameIndex = i;
    }
    
    return alembicMeshToMeshData(mesh, frameIndex, outData);
}

bool AlembicParser::sampleCameraAtTime(const AlembicCamera& cam, double time,
                                        QVector3D& pos, QQuaternion& rot, float& fov) {
    if (cam.samples.isEmpty()) return false;
    
    int frameIndex = 0;
    for (int i = 1; i < cam.samples.size(); ++i) {
        if (cam.samples[i].time > time) break;
        frameIndex = i;
    }
    
    const auto& sample = cam.samples[frameIndex];
    pos = sample.position;
    rot = sample.rotation;
    fov = sample.fov;
    
    return true;
}

QVector<double> AlembicParser::getSampleTimes(const AlembicArchive& archive) {
    QVector<double> times;
    if (!archive.meshes.isEmpty() && !archive.meshes[0].samples.isEmpty()) {
        for (const auto& sample : archive.meshes[0].samples) {
            times.append(sample.time);
        }
    }
    return times;
}

int AlembicParser::findSampleIndex(const AlembicArchive& archive, double time) {
    if (archive.meshes.isEmpty() || archive.meshes[0].samples.isEmpty()) return 0;
    
    const auto& samples = archive.meshes[0].samples;
    for (int i = 1; i < samples.size(); ++i) {
        if (samples[i].time > time) return i - 1;
    }
    return samples.size() - 1;
}

double AlembicParser::getTimeForFrame(const AlembicArchive& archive, int frame) {
    return archive.startTime + frame / archive.timeSamplingRate;
}

} // namespace ks