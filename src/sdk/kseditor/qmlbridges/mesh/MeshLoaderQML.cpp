#include "MeshLoaderQML.h"

MeshLoaderQML::MeshLoaderQML(QObject* parent)
    : QObject(parent)
{
}

MeshLoaderQML::~MeshLoaderQML() = default;

bool MeshLoaderQML::loadKN5(const QString&)
{
    emit meshLoadError(QStringLiteral("KN5 loading not available"));
    return false;
}

bool MeshLoaderQML::loadOBJ(const QString&)
{
    emit meshLoadError(QStringLiteral("OBJ loading not available"));
    return false;
}

bool MeshLoaderQML::loadGLTF(const QString&)
{
    emit meshLoadError(QStringLiteral("glTF loading not available"));
    return false;
}

QStringList MeshLoaderQML::getAvailableMeshes() const
{
    return {};
}

int MeshLoaderQML::getCurrentVertexCount() const
{
    return 0;
}

int MeshLoaderQML::getCurrentFaceCount() const
{
    return 0;
}

QString MeshLoaderQML::getCurrentMeshName() const
{
    return {};
}

QVariantList MeshLoaderQML::getVertexData() const
{
    return {};
}

QVariantList MeshLoaderQML::getIndexData() const
{
    return {};
}
