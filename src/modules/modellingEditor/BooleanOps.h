#pragma once

#include "core/mesh/GeometryTypes.h"
#include <vector>
#include <string>

namespace ks::geometry {

class BooleanOperations {
public:
    enum Operation { Union, Difference, Intersection, SymmetricDiff };

    static BoolOpResult performOperation(const GeoMeshData& meshA, const GeoMeshData& meshB, Operation op);
    static BoolOpResult performBatchOperations(const std::vector<GeoMeshData>& meshes, const std::vector<Operation>& operations);
    static std::string validateMesh(const GeoMeshData& mesh);
    static GeoMeshData repairMesh(const GeoMeshData& mesh);
    static GeoMeshData computeNormals(const GeoMeshData& mesh);

    static bool canPerform();
    static const char* operationName(Operation op);

private:
    static BoolOpResult performOperationImpl(const GeoMeshData& meshA, const GeoMeshData& meshB, Operation op);
    static void recalculateNormals(GeoMeshData& mesh);
};

} // namespace ks::geometry
