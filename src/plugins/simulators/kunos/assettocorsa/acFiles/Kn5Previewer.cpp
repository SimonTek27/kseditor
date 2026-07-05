#include "Kn5Previewer.h"
#include "KN5Parser.h"
#include <QDebug>
#include <QFileInfo>

Kn5Previewer::Kn5Previewer(QObject* parent)
    : QObject(parent)
{
}

void Kn5Previewer::loadKn5(const QString& path)
{
    if (!QFileInfo::exists(path)) {
        qWarning() << "Kn5Previewer: file not found:" << path;
        return;
    }

    auto data = KN5Parser::KN5ParserImpl::parse(path);
    if (!data.isValid()) {
        qWarning() << "Kn5Previewer: invalid KN5 file:" << path;
        return;
    }

    m_meshData.clear();
    for (const auto& mesh : data.meshes) {
        QVariantMap entry;
        entry["name"] = mesh.name;
        entry["vertices"] = static_cast<int>(mesh.getVertexCount());
        entry["faces"] = static_cast<int>(mesh.getTriangleCount());
        m_meshData.append(entry);
    }
    emit meshDataChanged(m_meshData);
    qInfo() << "Kn5Previewer: loaded" << data.meshes.size() << "meshes from" << path;
}
