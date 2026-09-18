#include "OCCTBridge.h"
#include "GeometryTypes.h"

#ifdef HAS_OCCT
#include <opencascade/BRepBuilderAPI_MakeVertex.hxx>
#include <opencascade/BRepBuilderAPI_MakeEdge.hxx>
#include <opencascade/BRepBuilderAPI_MakeWire.hxx>
#include <opencascade/BRepBuilderAPI_MakeFace.hxx>
#include <opencascade/BRepBuilderAPI_MakePolygon.hxx>
#include <opencascade/BRepAlgoAPI_Fuse.hxx>
#include <opencascade/BRepAlgoAPI_Cut.hxx>
#include <opencascade/BRepAlgoAPI_Common.hxx>
#include <opencascade/BRepOffsetAPI_MakeOffsetShape.hxx>
#include <opencascade/BRepOffsetAPI_MakeThickSolid.hxx>
#include <opencascade/STEPControl_Reader.hxx>
#include <opencascade/STEPControl_Writer.hxx>
#include <opencascade/IFSelect_ReturnStatus.hxx>
#include <opencascade/GeomAPI_PointsToBSplineSurface.hxx>
#include <opencascade/TColgp_Array2OfPnt.hxx>
#include <opencascade/TopExp_Explorer.hxx>
#include <opencascade/TopoDS.hxx>
#include <opencascade/TopAbs_ShapeEnum.hxx>
#include <opencascade/BRep_Tool.hxx>
#include <opencascade/BRepMesh_IncrementalMesh.hxx>
#include <opencascade/TopLoc_Location.hxx>
#include <opencascade/gp_Trsf.hxx>
#include <opencascade/BRepBuilderAPI_Transform.hxx>
#include <opencascade/ShapeAnalysis_Surface.hxx>
#include <opencascade/GeomAPI_ProjectPointOnSurf.hxx>
#endif

namespace ks {

bool OCCTBridge::isAvailable() {
#ifdef HAS_OCCT
    return true;
#else
    return false;
#endif
}

#ifdef HAS_OCCT

TopoDS_Shape OCCTBridge::meshToShape(const MeshData& mesh) {
    // Build triangulated shape from mesh vertices/faces
    // Create vertices
    NCollection_Sequence<gp_Pnt> vertices;
    for (const auto& v : mesh.vertices) {
        vertices.Append(gp_Pnt(v.position.x(), v.position.y(), v.position.z()));
    }

    // Build faces
    TopoDS_Compound compound;
    BRep_Builder builder;
    builder.MakeCompound(compound);

    for (const auto& face : mesh.faces) {
        if (face.indices.size() < 3) continue;

        TopoDS_Wire wire;
        BRepBuilderAPI_MakeWire wireMaker;

        for (int i = 0; i < face.indices.size(); ++i) {
            int next = (i + 1) % face.indices.size();
            if (face.indices[i] < 0 || face.indices[i] >= vertices.Length()) continue;
            if (face.indices[next] < 0 || face.indices[next] >= vertices.Length()) continue;

            Handle(Geom_TrimmedCurve) edge = GC_MakeSegment(
                vertices.Value(face.indices[i] + 1),
                vertices.Value(face.indices[next] + 1)
            );
            wireMaker.Add(BRepBuilderAPI_MakeEdge(edge));
        }

        if (wireMaker.IsDone()) {
            wire = wireMaker.Wire();
            try {
                BRepBuilderAPI_MakeFace faceMaker(wire, /*onlyPlane=*/Standard_True);
                if (faceMaker.IsDone()) {
                    builder.Add(compound, faceMaker.Face());
                }
            } catch (...) {
                // Skip invalid faces
            }
        }
    }

    return compound;
}

MeshData OCCTBridge::shapeToMesh(const TopoDS_Shape& shape, int deflection) {
    MeshData mesh;

    // Triangulate the shape
    BRepMesh_IncrementalMesh meshGen(shape, deflection);

    // Extract triangulation from all faces
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        TopoDS_Face face = TopoDS::Face(exp.Current());
        TopLoc_Location location;
        Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);

        if (triangulation.IsNull()) continue;

        gp_Trsf trsf = location.Transformation();
        int vertexOffset = mesh.vertices.size();

        // Add vertices
        for (int i = 1; i <= triangulation->NbNodes(); ++i) {
            gp_Pnt pt = triangulation->Node(i).Transformed(trsf);
            Vertex v;
            v.position = QVector3D(pt.X(), pt.Y(), pt.Z());
            v.normal = QVector3D(0, 1, 0); // Will be computed later
            mesh.vertices.append(v);
        }

        // Add triangles
        for (int i = 1; i <= triangulation->NbTriangles(); ++i) {
            Poly_Tri tri = triangulation->Triangle(i);
            int n1, n2, n3;
            tri.Get(n1, n2, n3);

            Face f;
            f.indices = {vertexOffset + n1 - 1, vertexOffset + n2 - 1, vertexOffset + n3 - 1};
            mesh.faces.append(f);
        }
    }

    mesh.computeBoundingBox();
    mesh.computeNormals();
    return mesh;
}

Handle(Geom_BSplineSurface) OCCTBridge::nurbsToOCCT(const geometry::NURBSSurface& surf) {
    int nU = surf.controlPoints.size();
    int nV = surf.controlPoints.isEmpty() ? 0 : surf.controlPoints[0].size();

    TColgp_Array2OfPnt poles(1, nU, 1, nV);
    for (int i = 0; i < nU; ++i) {
        for (int j = 0; j < nV; ++j) {
            const QVector3D& p = surf.controlPoints[i][j];
            poles.SetValue(i + 1, j + 1, gp_Pnt(p.x(), p.y(), p.z()));
        }
    }

    TColStd_Array1OfReal uKnots(1, surf.knotVectorU.size());
    TColStd_Array1OfInteger uMults(1, surf.knotVectorU.size());
    for (int i = 0; i < surf.knotVectorU.size(); ++i) {
        uKnots.SetValue(i + 1, surf.knotVectorU[i]);
        uMults.SetValue(i + 1, 1);
    }

    TColStd_Array1OfReal vKnots(1, surf.knotVectorV.size());
    TColStd_Array1OfInteger vMults(1, surf.knotVectorV.size());
    for (int i = 0; i < surf.knotVectorV.size(); ++i) {
        vKnots.SetValue(i + 1, surf.knotVectorV[i]);
        vMults.SetValue(i + 1, 1);
    }

    return new Geom_BSplineSurface(poles, uKnots, vKnots, uMults, vMults,
                                    surf.degreeU, surf.degreeV);
}

geometry::NURBSSurface OCCTBridge::occtToNurbs(const Handle(Geom_BSplineSurface)& surf) {
    geometry::NURBSSurface result;

    result.degreeU = surf->DegreeU();
    result.degreeV = surf->DegreeV();

    // Extract poles (control points)
    int nU = surf->NbUPoles();
    int nV = surf->NbVPoles();
    result.controlPoints.resize(nU);
    for (int i = 0; i < nU; ++i) {
        result.controlPoints[i].resize(nV);
        for (int j = 0; j < nV; ++j) {
            gp_Pnt p = surf->Pole(i + 1, j + 1);
            result.controlPoints[i][j] = QVector3D(p.X(), p.Y(), p.Z());
        }
    }

    // Extract knot vectors
    result.knotVectorU.clear();
    result.knotVectorV.clear();
    for (int i = 1; i <= surf->NbUKnots(); ++i) {
        result.knotVectorU.append(surf->UKnot(i));
    }
    for (int i = 1; i <= surf->NbVKnots(); ++i) {
        result.knotVectorV.append(surf->VKnot(i));
    }

    return result;
}

MeshData OCCTBridge::booleanUnionExact(const MeshData& a, const MeshData& b) {
    TopoDS_Shape shapeA = meshToShape(a);
    TopoDS_Shape shapeB = meshToShape(b);

    BRepAlgoAPI_Fuse fuse(shapeA, shapeB);
    if (!fuse.IsDone()) {
        // Fallback to mesh boolean
        return MeshOperations::booleanUnion(a, b);
    }

    return shapeToMesh(fuse.Shape());
}

MeshData OCCTBridge::booleanDifferenceExact(const MeshData& a, const MeshData& b) {
    TopoDS_Shape shapeA = meshToShape(a);
    TopoDS_Shape shapeB = meshToShape(b);

    BRepAlgoAPI_Cut cut(shapeA, shapeB);
    if (!cut.IsDone()) {
        return MeshOperations::booleanDifference(a, b);
    }

    return shapeToMesh(cut.Shape());
}

MeshData OCCTBridge::booleanIntersectionExact(const MeshData& a, const MeshData& b) {
    TopoDS_Shape shapeA = meshToShape(a);
    TopoDS_Shape shapeB = meshToShape(b);

    BRepAlgoAPI_Common common(shapeA, shapeB);
    if (!common.IsDone()) {
        return MeshOperations::booleanIntersection(a, b);
    }

    return shapeToMesh(common.Shape());
}

MeshData OCCTBridge::offsetSurfaceExact(const MeshData& mesh, float distance) {
    TopoDS_Shape shape = meshToShape(mesh);

    BRepOffsetAPI_MakeOffsetShape offset(shape, distance, 0.01);
    if (!offset.IsDone()) {
        // Fallback: simple Y-axis translation
        MeshData result = mesh;
        for (auto& v : result.vertices) {
            v.position += QVector3D(0, distance, 0);
        }
        return result;
    }

    return shapeToMesh(offset.Shape());
}

MeshData OCCTBridge::filletSurfaceExact(const MeshData& mesh, const MeshData& tool, float radius) {
    // For now, use the mesh-based approach as a fallback
    // A proper rolling-ball implementation would require more complex OCCT usage
    return MeshOperations::filletSurface(mesh, radius, 16);
}

bool OCCTBridge::importSTEPExact(const QString& path, QVector<MeshData>& meshes) {
    STEPControl_Reader reader;
    IFSelect_ReturnStatus status = reader.ReadFile(path.toStdWString().c_str());

    if (status != IFSelect_RetDone) {
        return false;
    }

    reader.TransferRoots();
    TopoDS_Shape shape = reader.OneShape();

    if (shape.IsNull()) {
        return false;
    }

    meshes.append(shapeToMesh(shape));
    return true;
}

bool OCCTBridge::exportSTEPExact(const QString& path, const QVector<MeshData>& meshes) {
    STEPControl_Writer writer;

    for (const auto& mesh : meshes) {
        TopoDS_Shape shape = meshToShape(mesh);
        writer.Transfer(shape, STEPControl_AsIs);
    }

    IFSelect_ReturnStatus status = writer.Write(path.toStdWString().c_str());
    return status == IFSelect_RetDone;
}

bool OCCTBridge::importBREPExact(const QString& path, QVector<MeshData>& meshes) {
    // Use OCCT's BRep_Builder to read BREP files
    // This is a simplified version - full BREP parsing would use BinLDrivers
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    // For now, just try to read as STEP
    return importSTEPExact(path, meshes);
}

geometry::NURBSSurface OCCTBridge::meshToNURBSFit(const MeshData& mesh, int uDegree, int vDegree,
                                                     int uSegments, int vSegments) {
    // Create a grid of points from the mesh
    TColgp_Array2OfPnt points(1, uSegments, 1, vSegments);

    float minU = 0, maxU = 1, minV = 0, maxV = 1;
    for (int i = 0; i < uSegments; ++i) {
        for (int j = 0; j < vSegments; ++j) {
            float u = minU + (maxU - minU) * i / (uSegments - 1);
            float v = minV + (maxV - minV) * j / (vSegments - 1);

            // Sample mesh at UV coordinates (simplified - project onto bounding box)
            float x = mesh.boundingBoxMin.x() + (mesh.boundingBoxMax.x() - mesh.boundingBoxMin.x()) * u;
            float y = mesh.boundingBoxMin.y() + (mesh.boundingBoxMax.y() - mesh.boundingBoxMin.y()) * v;
            float z = (mesh.boundingBoxMin.z() + mesh.boundingBoxMax.z()) * 0.5f;

            points.SetValue(i + 1, j + 1, gp_Pnt(x, y, z));
        }
    }

    // Fit B-spline surface
    GeomAPI_PointsToBSplineSurface fitter(points, uDegree, vDegree,
                                            GeomAbs_C2, 0.01, 100);
    if (!fitter.IsDone()) {
        // Return empty surface
        geometry::NURBSSurface empty;
        return empty;
    }

    Handle(Geom_BSplineSurface) bspline = fitter.Surface();
    return occtToNurbs(bspline);
}

#endif // HAS_OCCT

} // namespace ks
