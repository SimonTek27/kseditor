#pragma once

#include <QString>
#include <QVector3D>
#include <QVector2D>

#include "core/mesh/MeshOperations.h"

namespace ks {
namespace fileformat {

/// Parse a Grasshopper (.gh/.ghx) definition file.
/// Handles: geometry components, attributes, basic geometry generation.
/// Returns false on failure; optional error out.
bool importGrasshopperDefinition(const QByteArray& data, ks::MeshData& out, QString* error = nullptr);

/// Parse a Grasshopper definition and create a MeshData structure.
bool importGrasshopperDefinition(const QByteArray& data, ks::MeshData& out, QString* error = nullptr);

/// Create a GeometryNodes compound from a parsed Grasshopper definition.
/// This bridges Grasshopper definitions into ksEditor's node system.
bool importGrasshopperToScene(ks::Scene& scene, const QByteArray& data, QString* error = nullptr);

/// Import MeshData into the scene.
void importMeshDataToScene(ks::Scene& scene, const ks::MeshData& md, const QString& name);

} // namespace fileformat
} // namespace ks