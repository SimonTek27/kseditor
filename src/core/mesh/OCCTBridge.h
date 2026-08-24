#pragma once

#include "MeshOperations.h"

#ifdef HAS_OCCT
#include <opencascade/TopoDS_Shape.hxx>
#include <opencascade/TopoDS_Face.hxx>
#include <opencascade/TopoDS_Edge.hxx>
#include <opencascade/Geom_BSplineSurface.hxx>
#include <opencascade/Geom_BSplineCurve.hxx>
#include <opencascade/gp_Pnt.hxx>
#include <opencascade/gp_Vec.hxx>
#endif

namespace ks {

class OCCTBridge {
public:
    static bool isAvailable();

#ifdef HAS_OCCT
    // Convert kseditor MeshData to OCCT TopoDS_Shape (triangulated)
    static TopoDS_Shape meshToShape(const MeshData& mesh);

    // Convert OCCT TopoDS_Shape back to kseditor MeshData
    static MeshData shapeToMesh(const TopoDS_Shape& shape, int deflection = 0.1);

    // Convert NURBSSurface to OCCT Geom_BSplineSurface
    static Handle(Geom_BSplineSurface) nurbsToOCCT(const geometry::NURBSSurface& surf);

    // Convert OCCT Geom_BSplineSurface back to NURBSSurface
    static geometry::NURBSSurface occtToNurbs(const Handle(Geom_BSplineSurface)& surf);

    // Boolean operations using exact OCCT kernel
    static MeshData booleanUnionExact(const MeshData& a, const MeshData& b);
    static MeshData booleanDifferenceExact(const MeshData& a, const MeshData& b);
    static MeshData booleanIntersectionExact(const MeshData& a, const MeshData& b);

    // Offset surface using exact normal-based computation
    static MeshData offsetSurfaceExact(const MeshData& mesh, float distance);

    // Fillet/blend using rolling-ball algorithm
    static MeshData filletSurfaceExact(const MeshData& mesh, const MeshData& tool, float radius);

    // STEP import/export
    static bool importSTEPExact(const QString& path, QVector<MeshData>& meshes);
    static bool exportSTEPExact(const QString& path, const QVector<MeshData>& meshes);

    // BREP import
    static bool importBREPExact(const QString& path, QVector<MeshData>& meshes);

    // Fit mesh to NURBS surface using OCCT
    static geometry::NURBSSurface meshToNURBSFit(const MeshData& mesh, int uDegree = 3, int vDegree = 3,
                                                   int uSegments = 20, int vSegments = 20);
#endif
};

} // namespace ks
