#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector3D>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include "MeshRenderer.h"

class MeshLoaderQML : public QObject {
    Q_OBJECT
public:
    explicit MeshLoaderQML(QObject* parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE QVariantMap loadMeshFile(const QString& path) {
        QVariantMap result;
        QString localPath = QUrl(path).toLocalFile();
        if (localPath.isEmpty()) localPath = path;

        QFileInfo fi(localPath);
        result["path"] = localPath;
        result["name"] = fi.fileName();
        result["exists"] = fi.exists();
        result["size"] = fi.size();
        result["format"] = fi.suffix().toLower();

        if (!fi.exists()) {
            result["success"] = false;
            result["error"] = "File not found";
            emit meshLoaded(localPath, false);
            return result;
        }

        MeshRenderer loader;
        bool ok = false;
        QString ext = fi.suffix().toLower();
        if (ext == "kn5") {
            ok = loader.loadFromKN5(localPath);
        } else if (ext == "obj") {
            ok = loader.loadFromOBJ(localPath);
        } else if (ext == "gltf" || ext == "glb") {
            ok = loader.loadFromGLTF(localPath);
        }

        if (ok) {
            result["success"] = true;
            result["vertexCount"] = loader.getVertexCount();
            result["faceCount"] = loader.getFaceCount();
            result["meshName"] = loader.getName();

            QVariantList verts;
            const auto& vertices = loader.getVertices();
            for (const auto& v : vertices) {
                QVariantMap vm;
                vm["x"] = v.position.x();
                vm["y"] = v.position.y();
                vm["z"] = v.position.z();
                vm["nx"] = v.normal.x();
                vm["ny"] = v.normal.y();
                vm["nz"] = v.normal.z();
                vm["u"] = v.texCoord.x();
                vm["v"] = v.texCoord.y();
                verts.append(vm);
            }
            result["vertices"] = verts;

            QVariantList idxs;
            for (quint32 idx : loader.getIndices())
                idxs.append(static_cast<int>(idx));
            result["indices"] = idxs;
        } else {
            result["success"] = false;
            result["error"] = "Unsupported or invalid format";
        }

        emit meshLoaded(localPath, ok);
        return result;
    }

    Q_INVOKABLE QVariantList getAvailableMeshes(const QString& directory = "") {
        QVariantList meshes;
        QString scanDir = directory.isEmpty() ? QDir::currentPath() : directory;

        QStringList filters;
        filters << "*.kn5" << "*.obj" << "*.gltf" << "*.glb" << "*.fbx" << "*.stl";
        QStringList entries = QDir(scanDir).entryList(filters, QDir::Files, QDir::Name);

        for (const QString& entry : entries) {
            QVariantMap info;
            info["path"] = scanDir + "/" + entry;
            info["name"] = entry;
            info["format"] = QFileInfo(entry).suffix().toLower();
            meshes.append(info);
        }

        return meshes;
    }

    Q_INVOKABLE QVariantMap getMeshInfo(const QString& path) {
        QVariantMap info;
        QString localPath = QUrl(path).toLocalFile();
        if (localPath.isEmpty()) localPath = path;

        QFileInfo fi(localPath);
        info["path"] = localPath;
        info["name"] = fi.fileName();
        info["size"] = fi.size();
        info["format"] = fi.suffix().toLower();
        info["exists"] = fi.exists();

        emit meshInfoReady(info);
        return info;
    }

    Q_INVOKABLE bool isMeshFile(const QString& path) const {
        QString ext = QFileInfo(path).suffix().toLower();
        return ext == "kn5" || ext == "fbx" || ext == "obj" || ext == "glb"
            || ext == "gltf" || ext == "dae" || ext == "3ds" || ext == "stl";
    }

signals:
    void meshLoaded(const QString& path, bool success);
    void meshInfoReady(const QVariantMap& info);
};
