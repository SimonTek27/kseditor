#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

class MeshLoaderQML : public QObject
{
    Q_OBJECT

public:
    explicit MeshLoaderQML(QObject* parent = nullptr);
    ~MeshLoaderQML() override;

    Q_INVOKABLE bool loadKN5(const QString& path);
    Q_INVOKABLE bool loadOBJ(const QString& path);
    Q_INVOKABLE bool loadGLTF(const QString& path);
    Q_INVOKABLE QStringList getAvailableMeshes() const;
    Q_INVOKABLE int getCurrentVertexCount() const;
    Q_INVOKABLE int getCurrentFaceCount() const;
    Q_INVOKABLE QString getCurrentMeshName() const;
    Q_INVOKABLE QVariantList getVertexData() const;
    Q_INVOKABLE QVariantList getIndexData() const;

signals:
    void meshLoaded(const QString& name);
    void meshLoadError(const QString& error);
    void contentRefreshed();
};
