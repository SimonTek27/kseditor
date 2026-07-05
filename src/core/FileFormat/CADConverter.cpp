#include "CADConverter.h"
#include "../sys/LogManager.h"
#include "FBXParser.h"

namespace CAD {

Converter::Converter()
    : m_tessellationQuality(0.8f),
      m_chordalDeviation(0.01),
      m_angularTolerance(0.017453)  // ~1 degree in radians
{
}

ks::FBX::Scene Converter::toFBX(const File& cadFile)
{
    ks::FBX::Scene scene;
    scene.name = cadFile.assembly.name;
    
    // Convert each solid to an FBX mesh
    for (const Solid& solid : cadFile.assembly.rootComponent.solids) {
        ks::FBX::MeshData mesh;
        mesh.name = solid.name;
        
        // Create vertices from tessellated data
        for (const auto& v : solid.tessellatedVertices) {
            ks::FBX::Vertex fbxVert;
            fbxVert.position = v;
            fbxVert.normal = v.normalized();  // simple normal approximation
            fbxVert.uv = {0, 0};
            mesh.vertices.append(fbxVert);
        }
        
        // Create polygons from tessellated data
        for (const auto& poly : solid.tessellatedPolygons) {
            ks::FBX::Polygon fbxPoly;
            fbxPoly.indices = poly;
            mesh.polygons.append(fbxPoly);
        }
        
        scene.meshes.append(mesh);
    }
    
    return scene;
}

void Converter::setTessellationQuality(float quality)
{
    m_tessellationQuality = qBound(0.0f, quality, 1.0f);
}

void Converter::setChordalDeviation(double deviation)
{
    m_chordalDeviation = qMax(deviation, 0.00001);
}

void Converter::setAngularTolerance(double tolerance)
{
    m_angularTolerance = qMax(tolerance, 0.00001);
}

void Converter::convertSolid(const Solid& solid, ks::FBX::MeshData& mesh)
{
    mesh.name = solid.name;
    
    // Convert tessellated vertices
    for (const auto& v : solid.tessellatedVertices) {
        ks::FBX::Vertex fbxVert;
        fbxVert.position = v;
        fbxVert.normal = v.normalized();
        fbxVert.uv = {0, 0};
        mesh.vertices.append(fbxVert);
    }
    
    // Convert tessellated polygons
    for (const auto& poly : solid.tessellatedPolygons) {
        ks::FBX::Polygon fbxPoly;
        fbxPoly.indices = poly;
        mesh.polygons.append(fbxPoly);
    }
}

void Converter::convertComponent(const Component& component, ks::FBX::MeshData& mesh)
{
    for (const Solid& solid : component.solids) {
        convertSolid(solid, mesh);
    }
    
    for (const Component& child : component.children) {
        convertComponent(child, mesh);
    }
}

} // namespace CAD
