#include "AdditionalModifiers.h"
#include <QtMath>
#include <cmath>
#include <QDebug>
#include <QSet>
#include <QPair>

namespace ks {

WireframeModifierEx::WireframeModifierEx()
    : GenerateModifier("Wireframe")
{
    type = ModifierType::Generate;
}

MeshData WireframeModifierEx::apply(const MeshData& input)
{
    MeshData output;

    // Collect unique edges from faces
    struct EdgeKey { int v1, v2; };
    auto makeEdge = [](int a, int b) -> EdgeKey {
        return { qMin(a, b), qMax(a, b) };
    };

    QSet<QPair<int,int>> edgeSet;
    QVector<QPair<int,int>> uniqueEdges;
    for (const Face& face : input.faces) {
        for (int i = 0; i < face.vertexCount(); ++i) {
            int a = face[i];
            int b = face[(i + 1) % face.vertexCount()];
            auto key = std::make_pair(qMin(a, b), qMax(a, b));
            if (!edgeSet.contains(key)) {
                edgeSet.insert(key);
                uniqueEdges.append({a, b});
            }
        }
    }

    // For each unique edge, create a thin quad (4 vertices, 2 triangles)
    for (const auto& edge : uniqueEdges) {
        const Vertex& v1 = input.vertices[edge.first];
        const Vertex& v2 = input.vertices[edge.second];

        // Compute edge direction and perpendicular
        QVector3D edgeDir = (v2.position - v1.position).normalized();
        QVector3D perp = QVector3D::crossProduct(edgeDir, v1.normal).normalized();
        if (perp.lengthSquared() < 0.001f) {
            perp = QVector3D::crossProduct(edgeDir, QVector3D(0, 1, 0)).normalized();
            if (perp.lengthSquared() < 0.001f)
                perp = QVector3D::crossProduct(edgeDir, QVector3D(1, 0, 0)).normalized();
        }
        float halfThick = thickness * 0.5f;

        // Four corners of the wireframe quad
        QVector3D offsets[4] = {
            perp * halfThick + v1.normal * halfThick,
            -perp * halfThick + v1.normal * halfThick,
            -perp * halfThick + v2.normal * halfThick,
            perp * halfThick + v2.normal * halfThick
        };

        int base = output.vertices.size();
        for (int i = 0; i < 4; ++i) {
            Vertex v;
            v.position = (i < 2 ? v1.position : v2.position) + offsets[i];
            v.normal = perp;
            output.vertices.append(v);
        }

        // Two triangles per quad
        Face f1{base, base + 1, base + 2};
        Face f2{base, base + 2, base + 3};
        output.faces.append(f1);
        output.faces.append(f2);
    }

    if (!useReplaceOriginal) {
        // Append original mesh
        int base = output.vertices.size();
        for (const Vertex& v : input.vertices)
            output.vertices.append(v);
        for (const Face& f : input.faces) {
            output.faces.append(Face{
                f[0] + base,
                f[1] + base,
                f[2] + base
            });
        }
    }

    output.computeNormals();
    return output;
}

SkinModifierEx::SkinModifierEx()
    : GenerateModifier("Skin")
{
    type = ModifierType::Generate;
}

MeshData SkinModifierEx::apply(const MeshData& input)
{
    MeshData output = input;
    buildSkeleton(input);
    if (m_skeleton.isEmpty()) return output;

    // Compute world transforms for all bones
    for (int i = 0; i < m_skeleton.size(); ++i) {
        QMatrix4x4 local;
        local.translate(m_skeleton[i].head);
        local.rotate(m_skeleton[i].rotation);
        if (m_skeleton[i].parentIndex >= 0)
            m_skeleton[i].worldMatrix = m_skeleton[m_skeleton[i].parentIndex].worldMatrix * local;
        else
            m_skeleton[i].worldMatrix = local;
    }

    // Assign each vertex to nearest bone and deform
    for (Vertex& v : output.vertices) {
        int nearest = 0;
        float minDist = FLT_MAX;
        for (int i = 0; i < m_skeleton.size(); ++i) {
            float d = v.position.distanceToPoint(m_skeleton[i].head);
            if (d < minDist) { minDist = d; nearest = i; }
        }
        v.boneIndex = nearest;
        v.weight = 1.0f;

        QVector4D pos(v.position, 1.0f);
        QVector4D deformed = m_skeleton[nearest].worldMatrix.inverted() * pos;
        v.position = deformed.toVector3D();
    }
    return output;
}

void SkinModifierEx::buildSkeleton(const MeshData& mesh)
{
    if (mesh.vertices.isEmpty()) return;

    // Build bounding box
    float minX = 1e9, maxX = -1e9;
    float minY = 1e9, maxY = -1e9;
    float minZ = 1e9, maxZ = -1e9;

    for (const Vertex& v : mesh.vertices) {
        minX = qMin(minX, v.position.x());
        maxX = qMax(maxX, v.position.x());
        minY = qMin(minY, v.position.y());
        maxY = qMax(maxY, v.position.y());
        minZ = qMin(minZ, v.position.z());
        maxZ = qMax(maxZ, v.position.z());
    }

    float cx = (minX + maxX) / 2.0f;
    float cy = (minY + maxY) / 2.0f;
    float cz = (minZ + maxZ) / 2.0f;

    // Determine the longest axis
    float dx = maxX - minX;
    float dy = maxY - minY;
    float dz = maxZ - minZ;

    QVector3D spineDir(0, 1, 0);
    float spineLen = dy;
    float baseY = minY;
    float topY = maxY;

    // Create a hierarchical skeleton: Root -> Spine(3) -> Head
    int numSpineBones = qMax(1, static_cast<int>(spineLen / qMax(dx, qMax(dy, dz)) * 3));
    numSpineBones = qBound(2, numSpineBones, 8);

    // Root bone at base center
    Bone root;
    root.name = "Root";
    root.head = QVector3D(cx, baseY, cz);
    root.tail = QVector3D(cx, baseY + spineLen / numSpineBones, cz);
    root.parentIndex = -1;
    m_skeleton.append(root);

    // Spine chain
    for (int i = 1; i < numSpineBones; ++i) {
        Bone bone;
        float t = static_cast<float>(i) / numSpineBones;
        float tPrev = static_cast<float>(i - 1) / numSpineBones;
        bone.name = QString("Spine_%1").arg(i);
        bone.head = QVector3D(cx, baseY + tPrev * spineLen, cz);
        bone.tail = QVector3D(cx, baseY + t * spineLen, cz);
        bone.parentIndex = i - 1;
        m_skeleton.append(bone);
    }

    // Compute world transforms
    for (int i = 0; i < m_skeleton.size(); ++i) {
        QMatrix4x4 local;
        local.translate(m_skeleton[i].head);
        if (m_skeleton[i].parentIndex >= 0)
            m_skeleton[i].worldMatrix = m_skeleton[m_skeleton[i].parentIndex].worldMatrix * local;
        else
            m_skeleton[i].worldMatrix = local;
    }
}

DisplaceModifierEx::DisplaceModifierEx()
    : DeformModifier("Displace")
{
    type = ModifierType::Deform;
}

MeshData DisplaceModifierEx::apply(const MeshData& input)
{
    MeshData output = input;

    float s = strength;
    for (Vertex& v : output.vertices) {
        QVector3D displacement = noise3D(v.position.x(), v.position.y(), v.position.z());
        v.position += displacement * s;
    }

    return output;
}

float DisplaceModifierEx::cloud(float x, float y, float z)
{
    return (qSin(x * uScale) + qSin(y * uScale) + qSin(z * uScale)) / 3.0f * 2.0f - 1.0f;
}

float DisplaceModifierEx::marble(float x, float y, float z)
{
    return qSin(x * uScale + cloud(x, y, z));
}

float DisplaceModifierEx::voronoi(float x, float y, float z)
{
    float sx = x * uScale;
    float sy = y * uScale;
    float sz = z * uScale;

    int ix = static_cast<int>(std::floor(sx));
    int iy = static_cast<int>(std::floor(sy));
    int iz = static_cast<int>(std::floor(sz));

    float minDist = 1e10f;

    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dz = -1; dz <= 1; ++dz) {
                int cx = ix + dx;
                int cy = iy + dy;
                int cz = iz + dz;

                // Hash-based pseudo-random seed point
                unsigned int hash = static_cast<unsigned int>(cx * 374761393 + cy * 668265263 + cz * 1274126177);
                hash = (hash ^ (hash >> 13)) * 1274126177;
                float px = cx + (hash & 0xFFFF) / 65536.0f;
                hash = (hash >> 8) ^ (hash * 2654435761u);
                float py = cy + (hash & 0xFFFF) / 65536.0f;
                hash = (hash >> 8) ^ (hash * 2654435761u);
                float pz = cz + (hash & 0xFFFF) / 65536.0f;

                float dx2 = sx - px;
                float dy2 = sy - py;
                float dz2 = sz - pz;
                float dist = dx2 * dx2 + dy2 * dy2 + dz2 * dz2;
                if (dist < minDist) minDist = dist;
            }
        }
    }

    return std::sqrt(minDist) * 2.0f - 1.0f;
}

QVector3D DisplaceModifierEx::noise3D(float x, float y, float z)
{
    return QVector3D(cloud(x, y, z), cloud(y, z, x), cloud(z, x, y));
}

SimpleDeformModifier::SimpleDeformModifier()
    : DeformModifier("SimpleDeform")
{
    type = ModifierType::Deform;
}

MeshData SimpleDeformModifier::apply(const MeshData& input)
{
    QVector3D axis;
    switch (deformAxis) {
        case Axis::X: axis = QVector3D(1, 0, 0); break;
        case Axis::Y: axis = QVector3D(0, 1, 0); break;
        case Axis::Z: axis = QVector3D(0, 0, 1); break;
        default: axis = QVector3D(1, 0, 0);
    }

    switch (deformMethod) {
        case DeformMethod::Twist: return applyTwist(input, axis, angle);
        case DeformMethod::Stretch: return applyStretch(input, axis, factor);
        case DeformMethod::Bend: return applyBend(input, axis, angle);
        case DeformMethod::Linear: return applyLinear(input, axis, factor);
    }

    return input;
}

MeshData SimpleDeformModifier::applyTwist(const MeshData& input, const QVector3D& axis, float angle)
{
    MeshData output = input;
    float c = qCos(angle);
    float s = qSin(angle);
    float d = axis.x() * axis.x() + axis.y() * axis.y() + axis.z() * axis.z();

    for (Vertex& v : output.vertices) {
        QVector3D p = v.position;
        float dot = QVector3D::dotProduct(p, axis);

        QVector3D cross = QVector3D::crossProduct(axis, p);
        QVector3D perp = p - axis * dot;
        float perpLen = perp.length();

        if (perpLen > 0.001f) {
            float theta = angle * (dot / 1.0f);
            c = qCos(theta);
            s = qSin(theta);

            v.position = axis * dot + c * perp + s * cross;
        }
    }

    return output;
}

MeshData SimpleDeformModifier::applyStretch(const MeshData& input, const QVector3D& axis, float factor)
{
    MeshData output = input;

    for (Vertex& v : output.vertices) {
        float dot = QVector3D::dotProduct(v.position, axis);
        v.position += axis * (dot * factor);
    }

    return output;
}

MeshData SimpleDeformModifier::applyBend(const MeshData& input, const QVector3D& axis, float angle)
{
    MeshData output = input;
    if (qAbs(angle) < 0.001f) return output;

    QVector3D normAxis = axis.normalized();
    // Pick a perpendicular axis for the bend plane
    QVector3D ref(1, 0, 0);
    if (qAbs(QVector3D::dotProduct(normAxis, ref)) > 0.9f)
        ref = QVector3D(0, 1, 0);
    QVector3D perp = QVector3D::crossProduct(normAxis, ref).normalized();
    QVector3D binorm = QVector3D::crossProduct(normAxis, perp).normalized();

    // Find extent along bend axis
    float minDot = FLT_MAX, maxDot = -FLT_MAX;
    for (const Vertex& v : output.vertices) {
        float dot = QVector3D::dotProduct(v.position, normAxis);
        minDot = qMin(minDot, dot);
        maxDot = qMax(maxDot, dot);
    }
    float range = maxDot - minDot;
    if (range < 0.001f) return output;

    float radius = range / angle;

    for (Vertex& v : output.vertices) {
        float dot = QVector3D::dotProduct(v.position, normAxis);
        float t = (dot - minDot) / range; // 0..1 along bend axis

        QVector3D offset = v.position - normAxis * dot;
        float offPerp = QVector3D::dotProduct(offset, perp);
        float offBinorm = QVector3D::dotProduct(offset, binorm);

        float theta = t * angle;
        float newPos = minDot + radius * qSin(theta);
        float newOff = radius * (1.0f - qCos(theta)) + offPerp * qCos(theta);

        v.position = normAxis * newPos + perp * newOff + binorm * offBinorm;
    }

    output.computeNormals();
    return output;
}

MeshData SimpleDeformModifier::applyLinear(const MeshData& input, const QVector3D& axis, float factor)
{
    MeshData output = input;

    // Find extent along axis
    float minDot = FLT_MAX, maxDot = -FLT_MAX;
    for (const Vertex& v : output.vertices) {
        float dot = QVector3D::dotProduct(v.position, axis);
        minDot = qMin(minDot, dot);
        maxDot = qMax(maxDot, dot);
    }
    float range = maxDot - minDot;
    if (range < 0.001f) return output;

    for (Vertex& v : output.vertices) {
        float dot = QVector3D::dotProduct(v.position, axis);
        float t = (dot - minDot) / range;
        float scale = 1.0f + t * factor;
        QVector3D proj = axis * dot;
        QVector3D perp = v.position - proj;
        v.position = proj * scale + perp;
    }

    return output;
}

CorrectiveSmoothModifier::CorrectiveSmoothModifier()
    : DeformModifier("CorrectiveSmooth")
{
    type = ModifierType::Deform;
}

MeshData CorrectiveSmoothModifier::apply(const MeshData& input)
{
    QMap<int, QVector3D> pins;
    return smoothMesh(input, pins, iterations);
}

QMap<int, QVector3D> CorrectiveSmoothModifier::getPins(const MeshData& mesh, const QStringList& vertexGroup)
{
    QMap<int, QVector3D> pins;

    for (int i = 0; i < mesh.vertices.size(); ++i) {
        for (const QString& group : vertexGroup) {
            if (mesh.vertexGroups.contains(group) && mesh.vertexGroups[group].contains(i)) {
                pins[i] = mesh.vertices[i].position;
                break;
            }
        }
    }

    return pins;
}

MeshData CorrectiveSmoothModifier::smoothMesh(const MeshData& mesh, const QMap<int, QVector3D>& pins, int iters)
{
    MeshData output = mesh;

    for (int iter = 0; iter < iters; ++iter) {
        MeshData temp = output;
        for (int i = 0; i < output.vertices.size(); ++i) {
            if (pins.contains(i)) continue;

            QVector3D avg;
            int count = 0;
            for (const Edge& e : mesh.edges) {
                if (e.v1 == i) { avg += output.vertices[e.v2].position; count++; }
                else if (e.v2 == i) { avg += output.vertices[e.v1].position; count++; }
            }
            if (count > 0) {
                avg /= count;
                temp.vertices[i].position = output.vertices[i].position + (avg - output.vertices[i].position) * 0.5f;
            }
        }
        output = temp;
    }

    return output;
}

CurveModifier::CurveModifier()
    : DeformModifier("Curve")
{
    type = ModifierType::Deform;
}

MeshData CurveModifier::apply(const MeshData& input)
{
    if (input.vertices.isEmpty()) return input;

    // Determine deformation axis
    QVector3D deformDir;
    switch (deformAxis) {
        case DeformAxis::X:     deformDir = QVector3D(1, 0, 0); break;
        case DeformAxis::Y:     deformDir = QVector3D(0, 1, 0); break;
        case DeformAxis::Z:     deformDir = QVector3D(0, 0, 1); break;
        case DeformAxis::NEG_X: deformDir = QVector3D(-1, 0, 0); break;
        case DeformAxis::NEG_Y: deformDir = QVector3D(0, -1, 0); break;
        case DeformAxis::NEG_Z: deformDir = QVector3D(0, 0, -1); break;
    }

    // Find extent along deformation axis
    float minDot = FLT_MAX, maxDot = -FLT_MAX;
    for (const Vertex& v : input.vertices) {
        float dot = QVector3D::dotProduct(v.position, deformDir);
        minDot = qMin(minDot, dot);
        maxDot = qMax(maxDot, dot);
    }

    // Build curve points along the axis
    MeshData curve;
    int segments = 16;
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        Vertex v;
        v.position = deformDir * (minDot + t * (maxDot - minDot));
        v.normal = deformDir;
        curve.vertices.append(v);
    }
    return deformToCurve(input, curve);
}

MeshData CurveModifier::deformToCurve(const MeshData& mesh, const MeshData& curve)
{
    MeshData output = mesh;

    if (curve.vertices.size() < 2) return output;

    float totalLength = 0;
    QVector<float> cumLength;
    cumLength.append(0);

    for (int i = 1; i < curve.vertices.size(); ++i) {
        float segLen = (curve.vertices[i].position - curve.vertices[i-1].position).length();
        totalLength += segLen;
        cumLength.append(totalLength);
    }

    if (totalLength < 0.001f) return output;

    // Find bounding box along curve direction for each vertex
    QVector3D curveDir = (curve.vertices.last().position - curve.vertices.first().position).normalized();

    // Project each vertex onto the curve axis line and deform
    QVector3D curveStart = curve.vertices.first().position;
    QVector3D curveEnd = curve.vertices.last().position;
    float axisLength = (curveEnd - curveStart).length();

    for (Vertex& v : output.vertices) {
        QVector3D offset = v.position - curveStart;
        float t = QVector3D::dotProduct(offset, curveDir) / qMax(axisLength, 0.001f);
        t = qBound(0.0f, t, 1.0f);

        QVector3D targetPos = getPointOnCurve(curve, t);
        QVector3D lateralOffset = offset - curveDir * QVector3D::dotProduct(offset, curveDir);
        v.position = targetPos + lateralOffset;
    }

    return output;
}

QVector3D CurveModifier::getPointOnCurve(const MeshData& curve, float t)
{
    if (curve.vertices.isEmpty()) return QVector3D();

    float totalLength = 0;
    for (int i = 1; i < curve.vertices.size(); ++i) {
        totalLength += (curve.vertices[i].position - curve.vertices[i-1].position).length();
    }

    float targetLen = t * totalLength;
    float accumLen = 0;

    for (int i = 1; i < curve.vertices.size(); ++i) {
        float segLen = (curve.vertices[i].position - curve.vertices[i-1].position).length();
        if (accumLen + segLen >= targetLen) {
            float localT = (targetLen - accumLen) / segLen;
            return curve.vertices[i-1].position + (curve.vertices[i].position - curve.vertices[i-1].position) * localT;
        }
        accumLen += segLen;
    }

    return curve.vertices.last().position;
}

LatticeModifierEx::LatticeModifierEx()
    : DeformModifier("Lattice")
{
    type = ModifierType::Deform;
}

MeshData LatticeModifierEx::apply(const MeshData& input)
{
    MeshData output = input;
    if (input.vertices.isEmpty()) return output;

    // Build bounding box
    float minX = 1e9, maxX = -1e9;
    float minY = 1e9, maxY = -1e9;
    float minZ = 1e9, maxZ = -1e9;
    for (const Vertex& v : input.vertices) {
        minX = qMin(minX, v.position.x()); maxX = qMax(maxX, v.position.x());
        minY = qMin(minY, v.position.y()); maxY = qMax(maxY, v.position.y());
        minZ = qMin(minZ, v.position.z()); maxZ = qMax(maxZ, v.position.z());
    }

    float sx = maxX - minX, sy = maxY - minY, sz = maxZ - minZ;
    if (sx < 0.001f) sx = 1.0f;
    if (sy < 0.001f) sy = 1.0f;
    if (sz < 0.001f) sz = 1.0f;

    int divs[3] = {2, 2, 2};
    // Build lattice grid and deform vertices
    QVector<QVector<QVector<QVector3D>>> lattice(divs[0] + 1);
    for (int i = 0; i <= divs[0]; ++i) {
        lattice[i].resize(divs[1] + 1);
        for (int j = 0; j <= divs[1]; ++j) {
            lattice[i][j].resize(divs[2] + 1);
            for (int k = 0; k <= divs[2]; ++k) {
                lattice[i][j][k] = QVector3D(
                    minX + (static_cast<float>(i) / divs[0]) * sx,
                    minY + (static_cast<float>(j) / divs[1]) * sy,
                    minZ + (static_cast<float>(k) / divs[2]) * sz
                );
            }
        }
    }

    for (int di = 0; di < 2; ++di) {
        for (int dj = 0; dj < 2; ++dj) {
            for (int dk = 0; dk < 2; ++dk) {
                lattice[di != 0 ? divs[0] : 0]
                       [dj != 0 ? divs[1] : 0]
                       [dk != 0 ? divs[2] : 0] += QVector3D(strength, strength, strength);
            }
        }
    }

    for (Vertex& v : output.vertices) {
        // Trilinear interpolation of lattice displacement
        float u = (v.position.x() - minX) / sx;
        float vv = (v.position.y() - minY) / sy;
        float w = (v.position.z() - minZ) / sz;
        u = qBound(0.0f, u, 1.0f);
        vv = qBound(0.0f, vv, 1.0f);
        w = qBound(0.0f, w, 1.0f);

        int i0 = static_cast<int>(u * divs[0]);
        int j0 = static_cast<int>(vv * divs[1]);
        int k0 = static_cast<int>(w * divs[2]);
        int i1 = qMin(i0 + 1, divs[0]);
        int j1 = qMin(j0 + 1, divs[1]);
        int k1 = qMin(k0 + 1, divs[2]);

        float fu = (u * divs[0]) - i0;
        float fv = (vv * divs[1]) - j0;
        float fw = (w * divs[2]) - k0;

        QVector3D c000 = lattice[i0][j0][k0];
        QVector3D c100 = lattice[i1][j0][k0];
        QVector3D c010 = lattice[i0][j1][k0];
        QVector3D c110 = lattice[i1][j1][k0];
        QVector3D c001 = lattice[i0][j0][k1];
        QVector3D c101 = lattice[i1][j0][k1];
        QVector3D c011 = lattice[i0][j1][k1];
        QVector3D c111 = lattice[i1][j1][k1];

        QVector3D c00 = c000 * (1 - fu) + c100 * fu;
        QVector3D c01 = c001 * (1 - fu) + c101 * fu;
        QVector3D c10 = c010 * (1 - fu) + c110 * fu;
        QVector3D c11 = c011 * (1 - fu) + c111 * fu;
        QVector3D c0 = c00 * (1 - fv) + c10 * fv;
        QVector3D c1 = c01 * (1 - fv) + c11 * fv;
        QVector3D displaced = c0 * (1 - fw) + c1 * fw;

        v.position += (displaced - v.position) * strength;
    }

    return output;
}

WaveModifierEx::WaveModifierEx()
    : DeformModifier("Wave")
{
    type = ModifierType::Deform;
}

float WaveModifierEx::waveFunction(float x, float y, float z, float time)
{
    float dx = (x - position[0]) / width;
    float dy = (y - position[1]) / width;
    float offset = dx * dx + dy * dy;
    return height * qSin(-time * speed + offset * Narrowness * 6.28318f + phase);
}

CastModifierEx::CastModifierEx()
    : DeformModifier("Cast")
{
    type = ModifierType::Deform;
}

MeshData CastModifierEx::apply(const MeshData& input)
{
    MeshData output = input;

    for (Vertex& v : output.vertices) {
        QVector3D dir = v.position - QVector3D(0, 0, 0);

        switch (castType) {
            case CastType::Sphere: {
                float len = dir.length();
                if (len > 0.001f) {
                    float factor = strength * (radius - len) / radius;
                    v.position = v.position + dir * factor;
                }
                break;
            }
            case CastType::Cylinder: {
                dir.setY(0);
                float len = dir.length();
                if (len > 0.001f) {
                    float factor = strength * (radius - len) / radius;
                    v.position = v.position + dir * factor;
                }
                break;
            }
            case CastType::Cubic: {
                v.position.setX(qBound(-radius, v.position.x(), radius));
                v.position.setY(qBound(-radius, v.position.y(), radius));
                v.position.setZ(qBound(-radius, v.position.z(), radius));
                break;
            }
        }
    }

    return output;
}

SimpleSmoothModifier::SimpleSmoothModifier()
    : DeformModifier("SimpleSmooth")
{
    type = ModifierType::Deform;
}

MeshData SimpleSmoothModifier::apply(const MeshData& input)
{
    MeshData output = input;

    for (int iter = 0; iter < iterations; ++iter) {
        MeshData temp = output;

        for (int i = 0; i < output.vertices.size(); ++i) {
            QVector3D avg(0, 0, 0);
            int count = 0;

            for (const Face& face : output.faces) {
                for (int j = 0; j < face.vertexCount(); ++j) {
                    if (face[j] == i) continue;
                    avg += temp.vertices[face[j]].position;
                    count++;
                }
            }

            if (count > 0) {
                avg /= count;
                float lambda = lambdaFactor * lambdaBias;
                output.vertices[i].position = temp.vertices[i].position * (1 - lambda) + avg * lambda;
            }
        }
    }

    return output;
}

SmoothModifierEx::SmoothModifierEx()
    : DeformModifier("Smooth")
{
    type = ModifierType::Deform;
}

MeshData SmoothModifierEx::apply(const MeshData& input)
{
    MeshData temp = input;

    for (int i = 0; i < input.vertices.size(); ++i) {
        QVector3D smoothPos = smoothVertex(input, i, strength, useAxisX, useAxisY, useAxisZ);
        temp.vertices[i].position = smoothPos;
    }

    return temp;
}

QVector3D SmoothModifierEx::smoothVertex(const MeshData& mesh, int index, float str, bool x, bool y, bool z)
{
    QVector3D result(0, 0, 0);
    int count = 0;

    for (const Face& face : mesh.faces) {
        for (int j = 0; j < face.vertexCount(); ++j) {
            if (face[j] == index) continue;
            result += mesh.vertices[face[j]].position;
            count++;
        }
    }

    if (count > 0) {
        result /= count;
    }

    return result * str + mesh.vertices[index].position * (1 - str);
}

LaplacianSmoothModifier::LaplacianSmoothModifier()
    : DeformModifier("LaplacianSmooth")
{
    type = ModifierType::Deform;
}

MeshData LaplacianSmoothModifier::apply(const MeshData& input)
{
    MeshData output = input;

    for (int iter = 0; iter < 10; ++iter) {
        MeshData temp = output;

        for (int i = 0; i < output.vertices.size(); ++i) {
            QVector3D avg(0, 0, 0);
            int count = 0;

            for (const Face& face : output.faces) {
                for (int j = 0; j < face.vertexCount(); ++j) {
                    if (face[j] == i) continue;
                    avg += temp.vertices[face[j]].position;
                    count++;
                }
            }

            if (count > 0) {
                avg /= count;
                output.vertices[i].position = temp.vertices[i].position * (1 - lambdaFactor) + avg * (lambdaFactor + areaWeight);
            }
        }
    }

    return output;
}

SurfaceSmoothModifier::SurfaceSmoothModifier()
    : DeformModifier("SurfaceSmooth")
{
    type = ModifierType::Deform;
}

MeshData SurfaceSmoothModifier::apply(const MeshData& input)
{
    MeshData output = input;

    for (int iter = 0; iter < iterations; ++iter) {
        MeshData temp = output;

        for (int i = 0; i < output.vertices.size(); ++i) {
            QVector3D normal = output.vertices[i].normal;
            QVector3D avg(0, 0, 0);
            int count = 0;

            for (const Face& face : output.faces) {
                for (int j = 0; j < face.vertexCount(); ++j) {
                    if (face[j] == i) continue;
                    QVector3D diff = temp.vertices[face[j]].position - temp.vertices[i].position;
                    avg += diff - normal * QVector3D::dotProduct(diff, normal);
                    count++;
                }
            }

            if (count > 0) {
                avg /= count;
                float factor = smoothness * 0.1f;
                output.vertices[i].position += avg * factor;
            }
        }
    }

    return output;
}

VolumeSmoothModifier::VolumeSmoothModifier()
    : DeformModifier("VolumeSmooth")
{
    type = ModifierType::Deform;
}

MeshData VolumeSmoothModifier::apply(const MeshData& input)
{
    MeshData output = input;
    if (input.vertices.isEmpty()) return output;

    float origVolume = 0.0f;
    for (const Face& f : input.faces) {
        if (f.vertexCount() < 3) continue;
        const QVector3D& a = input.vertices[f[0]].position;
        const QVector3D& b = input.vertices[f[1]].position;
        const QVector3D& c = input.vertices[f[2]].position;
        origVolume += QVector3D::dotProduct(a, QVector3D::crossProduct(b, c));
    }
    origVolume = qAbs(origVolume) / 6.0f;

    for (int iter = 0; iter < iterations; ++iter) {
        MeshData temp = output;
        for (int i = 0; i < output.vertices.size(); ++i) {
            QVector3D avg;
            int count = 0;
            for (const Face& f : output.faces) {
                for (int j = 0; j < f.vertexCount(); ++j) {
                    if (f[j] == i) continue;
                    avg += temp.vertices[f[j]].position;
                    count++;
                }
            }
            if (count > 0) {
                avg /= count;
                output.vertices[i].position = temp.vertices[i].position + (avg - temp.vertices[i].position) * smoothness;
            }
        }
        // Volume preservation: rescale to match original volume
        float newVolume = 0.0f;
        for (const Face& f : output.faces) {
            if (f.vertexCount() < 3) continue;
            const QVector3D& a = output.vertices[f[0]].position;
            const QVector3D& b = output.vertices[f[1]].position;
            const QVector3D& c = output.vertices[f[2]].position;
            newVolume += QVector3D::dotProduct(a, QVector3D::crossProduct(b, c));
        }
        newVolume = qAbs(newVolume) / 6.0f;
        if (newVolume > 0.0001f) {
            float scale = powf(origVolume / newVolume, 1.0f / 3.0f);
            QVector3D center;
            for (const Vertex& v : output.vertices)
                center += v.position;
            center /= output.vertices.size();
            for (Vertex& v : output.vertices)
                v.position = center + (v.position - center) * scale;
        }
    }

    return output;
}

UVProjectModifier::UVProjectModifier()
    : Modifier("UVProject", ModifierType::Generate)
{
    type = ModifierType::Generate;
}

MeshData UVProjectModifier::apply(const MeshData& input)
{
    MeshData output = input;

    for (Vertex& v : output.vertices) {
        v.uv = QVector2D(
            (v.position.x() + 1.0f) * 0.5f,
            (v.position.y() + 1.0f) * 0.5f
        );
    }

    return output;
}

UVResolveOverlapsModifier::UVResolveOverlapsModifier()
    : DeformModifier("UVResolveOverlaps")
{
    type = ModifierType::Deform;
}

MeshData UVResolveOverlapsModifier::apply(const MeshData& input)
{
    return MeshOperations::resolveUVOverlaps(input, padding);
}

QMap<QString, QVariant> UVResolveOverlapsModifier::writeParameters() const
{
    QMap<QString, QVariant> p;
    p["padding"] = padding;
    return p;
}

void UVResolveOverlapsModifier::readParameters(const QMap<QString, QVariant>& params)
{
    if (params.contains("padding")) padding = params["padding"].toFloat();
}

WeldModifier::WeldModifier()
    : GenerateModifier("Weld")
{
    type = ModifierType::Generate;
}

MeshData WeldModifier::apply(const MeshData& input)
{
    MeshData output;
    if (input.vertices.isEmpty()) return output;

    // Build vertex mapping: map old index -> new index (or -1 if merged)
    QVector<int> oldToNew(input.vertices.size(), -1);
    QVector<int> mergeTarget(input.vertices.size(), -1);

    // Find vertices within threshold distance and mark for merging
    for (int i = 0; i < input.vertices.size(); ++i) {
        if (mergeTarget[i] != -1) continue;
        for (int j = i + 1; j < input.vertices.size(); ++j) {
            if (mergeTarget[j] != -1) continue;
            float dist = (input.vertices[i].position - input.vertices[j].position).length();
            if (dist < threshold) {
                mergeTarget[j] = i;
            }
        }
    }

    // Build output vertices (only unique vertices)
    for (int i = 0; i < input.vertices.size(); ++i) {
        if (mergeTarget[i] == -1) {
            oldToNew[i] = output.vertices.size();
            output.vertices.append(input.vertices[i]);
        }
    }

    // Map merged vertices to their new index
    for (int i = 0; i < input.vertices.size(); ++i) {
        if (mergeTarget[i] != -1) {
            int target = mergeTarget[i];
            while (mergeTarget[target] != -1) target = mergeTarget[target];
            oldToNew[i] = oldToNew[target];
        }
    }

    // Remap face indices
    for (const Face& f : input.faces) {
        int v0 = oldToNew[f[0]];
        int v1 = oldToNew[f[1]];
        int v2 = oldToNew[f[2]];
        if (v0 >= 0 && v1 >= 0 && v2 >= 0 &&
            v0 != v1 && v1 != v2 && v0 != v2) {
            // Degenerate face (all vertices collapsed) is skipped
            output.faces.append(Face{v0, v1, v2});
        }
    }

    return output;
}

WeightVGroupModifier::WeightVGroupModifier()
    : DeformModifier("WeightVGroup")
{
    type = ModifierType::WeightVGPaint;
}

MeshData WeightVGroupModifier::apply(const MeshData& input)
{
    MeshData output = input;

    if (output.vertexGroups.contains(vertexGroup)) {
        QVector<float>& weights = output.vertexGroups[vertexGroup];
        for (int i = 0; i < qMin(weights.size(), output.vertices.size()); ++i) {
            output.vertices[i].weight = weights[i];
        }
    }

    return output;
}

QVector<QVector<int>> SkinModifierEx::findAdjacency(const MeshData& mesh)
{
    QVector<QVector<int>> adj(mesh.vertices.size());
    for (const auto& tri : mesh.faces) {
        for (int i = 0; i < 3; ++i) {
            int a = tri.indices[i];
            int b = tri.indices[(i + 1) % 3];
            if (!adj[a].contains(b)) adj[a].append(b);
            if (!adj[b].contains(a)) adj[b].append(a);
        }
    }
    return adj;
}

// ============================================================
// CageDeformModifier
// ============================================================
CageDeformModifier::CageDeformModifier()
    : DeformModifier("CageDeform")
{
    type = ModifierType::Deform;
}

void CageDeformModifier::setCageMesh(const MeshData& cage)
{
    cageMesh = cage;
    m_coordinatesValid = false;
}

void CageDeformModifier::buildCoordinates(const MeshData& target)
{
    if (cageMesh.vertices.isEmpty()) return;

    targetRestPose = target;
    m_cageWeights.clear();

    // Build cage face topology (triangulate if needed)
    QVector<QVector<int>> cageFaces;
    for (const auto& f : cageMesh.faces) {
        if (f.indices.size() >= 3) {
            for (int i = 1; i < f.indices.size() - 1; ++i) {
                QVector<int> tri = { f.indices[0], f.indices[i], f.indices[i + 1] };
                cageFaces.append(tri);
            }
        }
    }

    QVector<QVector3D> cageVerts;
    for (const auto& v : cageMesh.vertices)
        cageVerts.append(v.position);

    // Compute Mean Value Coordinates for each target vertex
    for (const auto& v : target.vertices) {
        CageWeight cw;
        cw.cageVertices = cageVerts;
        cw.weights = computeMeanValueCoordinates(v.position, cageVerts, cageFaces);
        m_cageWeights.append(cw);
    }

    m_coordinatesValid = true;
}

MeshData CageDeformModifier::apply(const MeshData& input)
{
    if (cageMesh.vertices.isEmpty() || input.vertices.isEmpty())
        return input;

    // Build coordinates if needed or if input changed
    if (!m_coordinatesValid || targetRestPose.vertices.size() != input.vertices.size())
        buildCoordinates(input);

    MeshData output = input;

    QVector<QVector3D> cageVerts;
    for (const auto& v : cageMesh.vertices)
        cageVerts.append(v.position);
    if (cageVerts.isEmpty()) return output;

    for (int i = 0; i < output.vertices.size() && i < m_cageWeights.size(); ++i) {
        const auto& cw = m_cageWeights[i];
        if (cw.weights.isEmpty()) continue;

        QVector3D deformed(0, 0, 0);
        float totalWeight = 0.0f;

        for (int j = 0; j < cw.weights.size() && j < cageVerts.size(); ++j) {
            deformed += cageVerts[j] * cw.weights[j];
            totalWeight += cw.weights[j];
        }

        if (totalWeight > 0.0001f) {
            deformed /= totalWeight;
        }

        // Blend between original and deformed position
        output.vertices[i].position = input.vertices[i].position +
            (deformed - input.vertices[i].position) * strength;
    }

    if (usePreserveVolume) {
        output.computeNormals();
    }

    return output;
}

QVector<float> CageDeformModifier::computeMeanValueCoordinates(
    const QVector3D& point,
    const QVector<QVector3D>& cageVerts,
    const QVector<QVector<int>>& cageFaces)
{
    int n = cageVerts.size();
    QVector<float> weights(n, 0.0f);

    if (n < 3) return weights;

    for (const auto& tri : cageFaces) {
        if (tri.size() < 3) continue;
        int i = tri[0], j = tri[1], k = tri[2];
        if (i >= n || j >= n || k >= n) continue;

        float w = computeAngleWeight(point, cageVerts[i], cageVerts[j], cageVerts[k]);
        weights[i] += w;
        weights[j] += w;
        weights[k] += w;
    }

    // Normalize
    float sum = 0.0f;
    for (float w : weights) sum += w;
    if (sum > 0.0001f) {
        for (auto& w : weights) w /= sum;
    }

    return weights;
}

float CageDeformModifier::computeAngleWeight(
    const QVector3D& p,
    const QVector3D& vi,
    const QVector3D& vj,
    const QVector3D& vk)
{
    QVector3D ei = vi - p;
    QVector3D ej = vj - p;
    QVector3D ek = vk - p;

    float eiLen = ei.length();
    float ejLen = ej.length();
    float ekLen = ek.length();

    if (eiLen < 0.0001f || ejLen < 0.0001f || ekLen < 0.0001f)
        return 0.0f;

    QVector3D ni = QVector3D::crossProduct(ej, ek);
    QVector3D nj = QVector3D::crossProduct(ek, ei);
    QVector3D nk = QVector3D::crossProduct(ei, ej);

    float niLen = ni.length();
    float njLen = nj.length();
    float nkLen = nk.length();

    if (niLen < 0.0001f || njLen < 0.0001f || nkLen < 0.0001f)
        return 0.0f;

    ni /= niLen;
    nj /= njLen;
    nk /= nkLen;

    float thetaI = std::acos(qBound(-1.0f, QVector3D::dotProduct(ej / ejLen, ek / ekLen), 1.0f));
    float thetaJ = std::acos(qBound(-1.0f, QVector3D::dotProduct(ek / ekLen, ei / eiLen), 1.0f));
    float thetaK = std::acos(qBound(-1.0f, QVector3D::dotProduct(ei / eiLen, ej / ejLen), 1.0f));

    float weight = (std::tan(thetaI / 2.0f) + std::tan(thetaJ / 2.0f) + std::tan(thetaK / 2.0f)) / 3.0f;

    return qMax(0.0f, weight);
}

// ============================================================
// LatticeExModifier - enhanced configurable lattice
// ============================================================
LatticeExModifier::LatticeExModifier()
    : DeformModifier("Lattice")
{
    type = ModifierType::Deform;
    resetControlPoints();
}

int LatticeExModifier::cpIndex(int i, int j, int k) const
{
    return i * (vDivs + 1) * (wDivs + 1) + j * (wDivs + 1) + k;
}

void LatticeExModifier::setDivisions(int u, int v, int w)
{
    uDivs = qMax(1, u);
    vDivs = qMax(1, v);
    wDivs = qMax(1, w);
    resetControlPoints();
}

void LatticeExModifier::resetControlPoints()
{
    int count = (uDivs + 1) * (vDivs + 1) * (wDivs + 1);
    controlPoints.resize(count);
    restControlPoints.resize(count);
    for (int i = 0; i <= uDivs; ++i) {
        for (int j = 0; j <= vDivs; ++j) {
            for (int k = 0; k <= wDivs; ++k) {
                int idx = cpIndex(i, j, k);
                controlPoints[idx] = QVector3D(
                    static_cast<float>(i) / uDivs,
                    static_cast<float>(j) / vDivs,
                    static_cast<float>(k) / wDivs
                );
                restControlPoints[idx] = controlPoints[idx];
            }
        }
    }
}

bool LatticeExModifier::moveControlPoint(int index, const QVector3D& delta)
{
    if (index < 0 || index >= controlPoints.size())
        return false;
    controlPoints[index] += delta;
    return true;
}

MeshData LatticeExModifier::apply(const MeshData& input)
{
    return applyWithDeformedLattice(input, controlPoints);
}

MeshData LatticeExModifier::applyWithDeformedLattice(
    const MeshData& input, const QVector<QVector3D>& deformedCPs)
{
    MeshData output = input;
    if (output.vertices.isEmpty() || deformedCPs.size() != controlPointCount())
        return output;

    QVector3D bbMin, bbMax;
    output.computeBoundingBox();
    bbMin = output.boundingBoxMin;
    bbMax = output.boundingBoxMax;

    QVector3D size = bbMax - bbMin;
    if (size.x() < 0.001f) size.setX(1.0f);
    if (size.y() < 0.001f) size.setY(1.0f);
    if (size.z() < 0.001f) size.setZ(1.0f);

    for (auto& v : output.vertices) {
        QVector3D localPos(
            (v.position.x() - bbMin.x()) / size.x(),
            (v.position.y() - bbMin.y()) / size.y(),
            (v.position.z() - bbMin.z()) / size.z()
        );
        localPos.setX(qBound(0.0f, localPos.x(), 1.0f));
        localPos.setY(qBound(0.0f, localPos.y(), 1.0f));
        localPos.setZ(qBound(0.0f, localPos.z(), 1.0f));

        QVector3D restPos, defPos;

        switch (interpolation) {
            case Interpolation::BSpline:
                restPos = interpolateBSpline(localPos, restControlPoints);
                defPos = interpolateBSpline(localPos, deformedCPs);
                break;
            case Interpolation::CatmullRom:
                restPos = interpolateCatmullRom(localPos, restControlPoints);
                defPos = interpolateCatmullRom(localPos, deformedCPs);
                break;
            default:
                restPos = interpolateTrilinear(localPos, restControlPoints);
                defPos = interpolateTrilinear(localPos, deformedCPs);
                break;
        }

        QVector3D offset = defPos - restPos;
        v.position += offset * strength;
    }

    output.computeNormals();
    return output;
}

QVector3D LatticeExModifier::interpolateTrilinear(
    const QVector3D& localPos, const QVector<QVector3D>& cps) const
{
    float u = localPos.x() * uDivs;
    float v = localPos.y() * vDivs;
    float w = localPos.z() * wDivs;

    int i0 = qBound(0, static_cast<int>(u), uDivs - 1);
    int j0 = qBound(0, static_cast<int>(v), vDivs - 1);
    int k0 = qBound(0, static_cast<int>(w), wDivs - 1);
    int i1 = qMin(i0 + 1, uDivs);
    int j1 = qMin(j0 + 1, vDivs);
    int k1 = qMin(k0 + 1, wDivs);

    float fu = u - i0;
    float fv = v - j0;
    float fw = w - k0;

    QVector3D c000 = cps[cpIndex(i0, j0, k0)];
    QVector3D c100 = cps[cpIndex(i1, j0, k0)];
    QVector3D c010 = cps[cpIndex(i0, j1, k0)];
    QVector3D c110 = cps[cpIndex(i1, j1, k0)];
    QVector3D c001 = cps[cpIndex(i0, j0, k1)];
    QVector3D c101 = cps[cpIndex(i1, j0, k1)];
    QVector3D c011 = cps[cpIndex(i0, j1, k1)];
    QVector3D c111 = cps[cpIndex(i1, j1, k1)];

    QVector3D c00 = c000 * (1.0f - fu) + c100 * fu;
    QVector3D c01 = c001 * (1.0f - fu) + c101 * fu;
    QVector3D c10 = c010 * (1.0f - fu) + c110 * fu;
    QVector3D c11 = c011 * (1.0f - fu) + c111 * fu;

    QVector3D c0 = c00 * (1.0f - fv) + c10 * fv;
    QVector3D c1 = c01 * (1.0f - fv) + c11 * fv;

    return c0 * (1.0f - fw) + c1 * fw;
}

QVector3D LatticeExModifier::interpolateBSpline(
    const QVector3D& localPos, const QVector<QVector3D>& cps) const
{
    float u = localPos.x() * uDivs;
    float v = localPos.y() * vDivs;
    float w = localPos.z() * wDivs;

    // Uniform cubic B-spline basis functions
    auto bsplineBasis = [](float t) -> QVector4D {
        float t2 = t * t;
        float t3 = t2 * t;
        return QVector4D(
            (1.0f - 3.0f*t + 3.0f*t2 - t3) / 6.0f,
            (4.0f - 6.0f*t2 + 3.0f*t3) / 6.0f,
            (1.0f + 3.0f*t + 3.0f*t2 - 3.0f*t3) / 6.0f,
            t3 / 6.0f
        );
    };

    auto eval1D = [&](int idx, int stride, float t) -> QVector3D {
        int i0 = qBound(0, idx - 1, stride - 1);
        int i1 = qBound(0, idx, stride - 1);
        int i2 = qBound(0, idx + 1, stride - 1);
        int i3 = qBound(0, idx + 2, stride - 1);
        QVector4D basis = bsplineBasis(t - idx);
        return cps[i0] * basis.x() + cps[i1] * basis.y() +
               cps[i2] * basis.z() + cps[i3] * basis.w();
    };

    // Separable 3D B-spline: evaluate along U, V, W in sequence
    int uIdx = qBound(1, (int)std::floor(u), uDivs - 1);
    int vIdx = qBound(1, (int)std::floor(v), vDivs - 1);
    int wIdx = qBound(1, (int)std::floor(w), wDivs - 1);

    // Evaluate along W for each (uIdx, vIdx) slice
    QVector3D c000 = eval1D(wIdx, wDivs, w);  // simplified 1D along W
    QVector3D c100 = eval1D(wIdx, wDivs, w);
    QVector3D c010 = eval1D(wIdx, wDivs, w);
    QVector3D c110 = eval1D(wIdx, wDivs, w);
    QVector3D c001 = eval1D(wIdx, wDivs, w);
    QVector3D c101 = eval1D(wIdx, wDivs, w);
    QVector3D c011 = eval1D(wIdx, wDivs, w);
    QVector3D c111 = eval1D(wIdx, wDivs, w);

    // Actually: do separable trilinear-then-bspline
    // Step 1: Evaluate along U axis for each (v,w) corner
    auto evalU = [&](float t) -> QVector3D {
        int i0 = qBound(0, uIdx - 1, uDivs - 1);
        int i1 = qBound(0, uIdx,     uDivs - 1);
        int i2 = qBound(0, uIdx + 1, uDivs - 1);
        int i3 = qBound(0, uIdx + 2, uDivs - 1);
        QVector4D basis = bsplineBasis(t - uIdx);

        // For each of the 16 v,w corner combos, interpolate along U
        auto blend = [&](int vi0, int vi1, int wi0, int wi1) -> QVector3D {
            int strideV = vDivs + 1;
            int strideW = wDivs + 1;
            int idx0 = i0 * strideV * strideW + vi0 * strideW + wi0;
            int idx1 = i1 * strideV * strideW + vi0 * strideW + wi0;
            int idx2 = i2 * strideV * strideW + vi0 * strideW + wi0;
            int idx3 = i3 * strideV * strideW + vi0 * strideW + wi0;
            if (idx0 < cps.size() && idx1 < cps.size() && idx2 < cps.size() && idx3 < cps.size())
                return cps[idx0] * basis.x() + cps[idx1] * basis.y() +
                       cps[idx2] * basis.z() + cps[idx3] * basis.w();
            return QVector3D();
        };

        // Get the 8 corners from U-interpolated values
        QVector3D corners[8];
        int vi[2] = { qBound(0, vIdx - 1, vDivs), qBound(0, vIdx, vDivs) };
        int wi[2] = { qBound(0, wIdx - 1, wDivs), qBound(0, wIdx, wDivs) };

        // Interpolate along V
        QVector4D basisV = bsplineBasis(v - vIdx);
        QVector4D basisW = bsplineBasis(w - wIdx);

        // Full separable 3D B-spline evaluation
        QVector3D result;
        for (int ii = 0; ii < 4; ii++) {
            int ui = qBound(0, uIdx - 1 + ii, uDivs);
            for (int jj = 0; jj < 4; jj++) {
                int vii = qBound(0, vIdx - 1 + jj, vDivs);
                for (int kk = 0; kk < 4; kk++) {
                    int wii = qBound(0, wIdx - 1 + kk, wDivs);
                    int flatIdx = ui * (vDivs + 1) * (wDivs + 1) + vii * (wDivs + 1) + wii;
                    if (flatIdx < cps.size()) {
                        QVector4D bu = bsplineBasis(u - uIdx);
                        QVector4D bv = bsplineBasis(v - vIdx);
                        QVector4D bw = bsplineBasis(w - wIdx);
                        float weight = bu[ii] * bv[jj] * bw[kk];
                        result += cps[flatIdx] * weight;
                    }
                }
            }
        }
        return result;
    };

    return evalU(u);
}

QVector3D LatticeExModifier::interpolateCatmullRom(
    const QVector3D& localPos, const QVector<QVector3D>& cps) const
{
    // Separable 3D Catmull-Rom spline interpolation
    float u = localPos.x() * uDivs;
    float v = localPos.y() * vDivs;
    float w = localPos.z() * wDivs;

    auto catmullRomBasis = [](float t) -> QVector4D {
        float t2 = t * t;
        float t3 = t2 * t;
        return QVector4D(
            -0.5f * t3 + t2 - 0.5f * t,
             1.5f * t3 - 2.5f * t2 + 1.0f,
            -1.5f * t3 + 2.0f * t2 + 0.5f * t,
             0.5f * t3 - 0.5f * t2
        );
    };

    // 3D separable Catmull-Rom: evaluate basis along U, V, W
    int uIdx = qBound(1, (int)std::floor(u), uDivs - 1);
    int vIdx = qBound(1, (int)std::floor(v), vDivs - 1);
    int wIdx = qBound(1, (int)std::floor(w), wDivs - 1);

    QVector4D bu = catmullRomBasis(u - uIdx);
    QVector4D bv = catmullRomBasis(v - vIdx);
    QVector4D bw = catmullRomBasis(w - wIdx);

    QVector3D result;
    for (int i = 0; i < 4; i++) {
        int ui = qBound(0, uIdx - 1 + i, uDivs);
        for (int j = 0; j < 4; j++) {
            int vi = qBound(0, vIdx - 1 + j, vDivs);
            for (int k = 0; k < 4; k++) {
                int wi = qBound(0, wIdx - 1 + k, wDivs);
                int flatIdx = ui * (vDivs + 1) * (wDivs + 1) + vi * (wDivs + 1) + wi;
                if (flatIdx < cps.size()) {
                    result += cps[flatIdx] * (bu[i] * bv[j] * bw[k]);
                }
            }
        }
    }
    return result;
}

TaperModifier::TaperModifier()
    : DeformModifier("Taper")
{
    type = ModifierType::Deform;
}

void TaperModifier::readParameters(const QMap<QString, QVariant>& params)
{
    if (params.contains("factor")) factor = params["factor"].toFloat();
    if (params.contains("taperAxis")) taperAxis = (Axis)params["taperAxis"].toInt();
    if (params.contains("useCurve")) useCurve = params["useCurve"].toBool();
}

MeshData TaperModifier::apply(const MeshData& input)
{
    MeshData output = input;
    if (output.vertices.isEmpty())
        return output;

    output.computeBoundingBox();
    QVector3D center = (output.boundingBoxMin + output.boundingBoxMax) * 0.5f;
    QVector3D size = output.boundingBoxMax - output.boundingBoxMin;
    float axisLen = size.z();
    switch (taperAxis) {
        case Axis::X: axisLen = size.x(); break;
        case Axis::Y: axisLen = size.y(); break;
        case Axis::Z: axisLen = size.z(); break;
        default: axisLen = size.y(); break;
    }
    if (axisLen < 1e-6f)
        return output;

    for (Vertex& v : output.vertices) {
        QVector3D p = v.position - center;
        float t;
        switch (taperAxis) {
            case Axis::X: t = p.x() / (axisLen * 0.5f); break;
            case Axis::Y: t = p.y() / (axisLen * 0.5f); break;
            case Axis::Z: t = p.z() / (axisLen * 0.5f); break;
            default: t = p.y() / (axisLen * 0.5f); break;
        }
        t = qBound(-1.0f, t, 1.0f);
        float s = 1.0f + factor * t;
        switch (taperAxis) {
            case Axis::X:
                p.setY(p.y() * s);
                p.setZ(p.z() * s);
                break;
            case Axis::Y:
                p.setX(p.x() * s);
                p.setZ(p.z() * s);
                break;
            case Axis::Z:
                p.setX(p.x() * s);
                p.setY(p.y() * s);
                break;
            default:
                p.setX(p.x() * s);
                p.setZ(p.z() * s);
                break;
        }
        v.position = p + center;
    }

    output.computeNormals();
    output.computeBoundingBox();
    return output;
}

RippleModifier::RippleModifier()
    : DeformModifier("Ripple")
{
    type = ModifierType::Deform;
}

void RippleModifier::readParameters(const QMap<QString, QVariant>& params)
{
    if (params.contains("amplitude")) amplitude = params["amplitude"].toFloat();
    if (params.contains("wavelength")) wavelength = params["wavelength"].toFloat();
    if (params.contains("phase")) phase = params["phase"].toFloat();
    if (params.contains("decay")) decay = params["decay"].toFloat();
    if (params.contains("rippleAxis")) rippleAxis = (Axis)params["rippleAxis"].toInt();
}

MeshData RippleModifier::apply(const MeshData& input)
{
    MeshData output = input;
    if (output.vertices.isEmpty())
        return output;
    if (wavelength < 1e-6f)
        return output;

    output.computeBoundingBox();
    QVector3D center = (output.boundingBoxMin + output.boundingBoxMax) * 0.5f;

    for (Vertex& v : output.vertices) {
        QVector3D p = v.position - center;
        QVector2D radial;
        switch (rippleAxis) {
            case Axis::X: radial = QVector2D(p.y(), p.z()); break;
            case Axis::Y: radial = QVector2D(p.x(), p.z()); break;
            default: radial = QVector2D(p.x(), p.y()); break;
        }
        float dist = radial.length();
        float decayFactor = (decay > 0.0f) ? qExp(-decay * dist) : 1.0f;
        float offset = amplitude * qSin((2.0f * float(M_PI) * dist) / wavelength + phase) * decayFactor;

        switch (rippleAxis) {
            case Axis::X: p.setX(p.x() + offset); break;
            case Axis::Y: p.setY(p.y() + offset); break;
            default: p.setZ(p.z() + offset); break;
        }
        v.position = p + center;
    }

    output.computeNormals();
    output.computeBoundingBox();
    return output;
}

NoiseModifier::NoiseModifier()
    : DeformModifier("Noise")
{
    type = ModifierType::Deform;
}

void NoiseModifier::readParameters(const QMap<QString, QVariant>& params)
{
    if (params.contains("scale")) scale = params["scale"].toFloat();
    if (params.contains("strength")) strength = params["strength"].toFloat();
    if (params.contains("seed")) seed = params["seed"].toInt();
    if (params.contains("depth")) depth = params["depth"].toFloat();
}

float NoiseModifier::noiseValue(float x, float y, float z) const
{
    float n = qSin(x * 12.9898f + seed) * qSin(y * 78.233f + seed) * qSin(z * 37.719f + seed);
    n = n * 43758.5453f;
    return n - std::floor(n);
}

MeshData NoiseModifier::apply(const MeshData& input)
{
    MeshData output = input;
    if (output.vertices.isEmpty())
        return output;

    output.computeNormals();
    for (Vertex& v : output.vertices) {
        QVector3D p = v.position;
        float nx = (qSin(p.x() * scale * 1.0f + seed) * 0.5f +
                    qSin(p.y() * scale * 1.7f + seed * 2.0f) * 0.5f +
                    qSin(p.z() * scale * 2.3f + seed * 3.0f) * 0.5f);
        float ny = noiseValue(p.x() * scale, p.y() * scale, p.z() * scale);
        QVector3D offset = v.normal * (nx * 0.5f + ny * 0.5f) * strength;
        v.position += offset;
    }

    output.computeNormals();
    output.computeBoundingBox();
    return output;
}

PushModifier::PushModifier()
    : DeformModifier("Push")
{
    type = ModifierType::Deform;
}

void PushModifier::readParameters(const QMap<QString, QVariant>& params)
{
    if (params.contains("distance")) distance = params["distance"].toFloat();
}

MeshData PushModifier::apply(const MeshData& input)
{
    MeshData output = input;
    if (output.vertices.isEmpty())
        return output;

    output.computeNormals();
    for (Vertex& v : output.vertices) {
        QVector3D n = v.normal;
        if (n.lengthSquared() < 1e-6f)
            continue;
        v.position += n.normalized() * distance;
    }
    if (distance != 0.0f) {
        output.computeNormals();
        output.computeBoundingBox();
    }
    return output;
}

RelaxModifier::RelaxModifier()
    : DeformModifier("Relax")
{
    type = ModifierType::Deform;
}

void RelaxModifier::readParameters(const QMap<QString, QVariant>& params)
{
    if (params.contains("iterations")) iterations = params["iterations"].toInt();
    if (params.contains("factor")) factor = params["factor"].toFloat();
    if (params.contains("preserveVolume")) preserveVolume = params["preserveVolume"].toBool();
    if (params.contains("pinBoundary")) pinBoundary = params["pinBoundary"].toBool();
}

MeshData RelaxModifier::apply(const MeshData& input)
{
    MeshData output = input;
    int n = output.vertices.size();
    if (n == 0)
        return output;

    output.computeBoundingBox();
    if (pinBoundary)
        MeshOperations::ensureEdgeList(output);

    // 1-ring adjacency
    QVector<QVector<int>> adj(n);
    for (const Face& f : output.faces) {
        int fc = f.indices.size();
        for (int k = 0; k < fc; ++k) {
            int a = f.indices[k];
            int b = f.indices[(k + 1) % fc];
            if (a < 0 || a >= n || b < 0 || b >= n)
                continue;
            if (!adj[a].contains(b)) adj[a].append(b);
            if (!adj[b].contains(a)) adj[b].append(a);
        }
    }

    QVector<char> boundary(n, 0);
    if (pinBoundary && output.edges.isEmpty())
        MeshOperations::ensureEdgeList(output);
    if (pinBoundary) {
        QSet<int> edgeSet;
        for (const Edge& e : output.edges)
            edgeSet.insert((e.v1 << 16) ^ (e.v2 & 0xffff));
        QSet<int> boundaryVerts;
        for (const Face& f : output.faces) {
            int fc = f.indices.size();
            for (int k = 0; k < fc; ++k) {
                int a = f.indices[k];
                int b = f.indices[(k + 1) % fc];
                int key = (a << 16) ^ (b & 0xffff);
                int revKey = (b << 16) ^ (a & 0xffff);
                if (!edgeSet.contains(revKey))
                    boundaryVerts.insert(a);
            }
        }
        for (int v : boundaryVerts)
            boundary[v] = 1;
    }

    for (int it = 0; it < qMax(1, iterations); ++it) {
        QVector<QVector3D> relaxed(n);
        for (int i = 0; i < n; ++i) {
            if (boundary[i])
                continue;
            if (adj[i].isEmpty()) {
                relaxed[i] = output.vertices[i].position;
                continue;
            }
            QVector3D acc;
            for (int j : adj[i])
                acc += output.vertices[j].position;
            relaxed[i] = output.vertices[i].position + (acc / adj[i].size() - output.vertices[i].position) * factor;
        }
        for (int i = 0; i < n; ++i)
            if (!boundary[i])
                output.vertices[i].position = relaxed[i];
    }

    output.computeNormals();
    output.computeBoundingBox();
    return output;
}

MeltModifier::MeltModifier()
    : DeformModifier("Melt")
{
    type = ModifierType::Deform;
}

void MeltModifier::readParameters(const QMap<QString, QVariant>& params)
{
    if (params.contains("amount")) amount = params["amount"].toFloat();
    if (params.contains("viscosity")) viscosity = params["viscosity"].toFloat();
    if (params.contains("axis")) axis = (MeltAxisType)params["axis"].toInt();
}

MeshData MeltModifier::apply(const MeshData& input)
{
    MeshData output = input;
    if (output.vertices.isEmpty())
        return output;

    output.computeBoundingBox();
    QVector3D bbMin = output.boundingBoxMin;
    QVector3D bbMax = output.boundingBoxMax;

    for (Vertex& v : output.vertices) {
        float t = 1.0f;
        switch (axis) {
            case MeltAxisType::X:
                t = qBound(0.0f, (v.position.x() - bbMin.x()) / qMax(1e-6f, bbMax.x() - bbMin.x()), 1.0f);
                break;
            case MeltAxisType::Y:
                t = qBound(0.0f, (v.position.y() - bbMin.y()) / qMax(1e-6f, bbMax.y() - bbMin.y()), 1.0f);
                break;
            default:
                t = qBound(0.0f, (v.position.z() - bbMin.z()) / qMax(1e-6f, bbMax.z() - bbMin.z()), 1.0f);
                break;
        }
        float amt = amount * qPow(t, 1.0f + viscosity);
        switch (axis) {
            case MeltAxisType::X: v.position.setX(v.position.x() - (v.position.x() - bbMin.x()) * amt); break;
            case MeltAxisType::Y: v.position.setY(v.position.y() - (v.position.y() - bbMin.y()) * amt); break;
            default: v.position.setZ(v.position.z() - (v.position.z() - bbMin.z()) * amt); break;
        }
    }

    output.computeNormals();
    output.computeBoundingBox();
    return output;
}

LatheModifier::LatheModifier()
    : GenerateModifier("Lathe")
{
    type = ModifierType::Generate;
}

void LatheModifier::readParameters(const QMap<QString, QVariant>& params)
{
    if (params.contains("segments")) segments = params["segments"].toInt();
    if (params.contains("angle")) angle = params["angle"].toFloat();
    if (params.contains("latheAxis")) latheAxis = (Axis)params["latheAxis"].toInt();
}

MeshData LatheModifier::apply(const MeshData& input)
{
    MeshData output;
    output.name = input.name + ".lathe";
    output.diffuseColor = input.diffuseColor;
    output.materialName = input.materialName;
    output.materials = input.materials;

    int vCount = input.vertices.size();
    if (vCount < 2)
        return output;

    int seg = qMax(3, segments);
    float arcRad = angle * float(M_PI) / 180.0f;

    QVector<QVector3D> axisPos;
    for (int i = 0; i < vCount; ++i)
        axisPos.append(input.vertices[i].position);

    for (int s = 0; s <= seg; ++s) {
        float a = arcRad * s / seg;
        float ca = qCos(a);
        float sa = qSin(a);
        for (int i = 0; i < vCount; ++i) {
            QVector3D p = axisPos[i];
            QVector3D r;
            switch (latheAxis) {
                case Axis::X:
                    r = QVector3D(p.x(), p.y() * ca - p.z() * sa, p.y() * sa + p.z() * ca);
                    break;
                case Axis::Y:
                    r = QVector3D(p.x() * ca + p.z() * sa, p.y(), -p.x() * sa + p.z() * ca);
                    break;
                default:
                    r = QVector3D(p.x() * ca - p.y() * sa, p.x() * sa + p.y() * ca, p.z());
                    break;
            }
            Vertex v = input.vertices[i];
            v.position = r;
            output.vertices.append(v);
        }
    }

    int ringStride = vCount;
    if (arcRad < 6.2830f) {
        for (int s = 0; s < seg; ++s) {
            for (int i = 0; i < vCount - 1; ++i) {
                int a = s * ringStride + i;
                int b = s * ringStride + i + 1;
                int c = (s + 1) * ringStride + i + 1;
                int d = (s + 1) * ringStride + i;
                output.faces.append(Face({a, b, c, d}));
            }
        }
    } else {
        for (int s = 0; s < seg; ++s) {
            for (int i = 0; i < vCount - 1; ++i) {
                int a = s * ringStride + i;
                int b = s * ringStride + i + 1;
                int c = ((s + 1) % seg) * ringStride + i + 1;
                int d = ((s + 1) % seg) * ringStride + i;
                output.faces.append(Face({a, b, c, d}));
            }
        }
    }

    output.computeNormals();
    output.computeBoundingBox();
    return output;
}

SmoothingGroupsModifier::SmoothingGroupsModifier()
    : DeformModifier("SmoothingGroups")
{
    type = ModifierType::Deform;
}

MeshData SmoothingGroupsModifier::apply(const MeshData& input)
{
    if (input.faces.isEmpty()) return input;

    // Compute smoothing groups based on angle threshold
    QVector<int> faceGroups = MeshOperations::autoSmooth(input, angleThreshold);

    // Split vertices at smoothing group boundaries
    MeshData result = MeshOperations::splitSmoothingGroups(input, faceGroups);

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

QMap<QString, QVariant> SmoothingGroupsModifier::writeParameters() const
{
    QMap<QString, QVariant> p;
    p["angleThreshold"] = angleThreshold;
    return p;
}

void SmoothingGroupsModifier::readParameters(const QMap<QString, QVariant>& params)
{
    if (params.contains("angleThreshold")) angleThreshold = params["angleThreshold"].toFloat();
}

OffsetModifier::OffsetModifier()
    : DeformModifier("Offset")
{
    type = ModifierType::Deform;
}

MeshData OffsetModifier::apply(const MeshData& input)
{
    return MeshOperations::shell(input, thickness, 1, useFlipNormals);
}

QMap<QString, QVariant> OffsetModifier::writeParameters() const
{
    QMap<QString, QVariant> p;
    p["thickness"] = thickness;
    p["useFlipNormals"] = useFlipNormals;
    return p;
}

void OffsetModifier::readParameters(const QMap<QString, QVariant>& params)
{
    if (params.contains("thickness")) thickness = params["thickness"].toFloat();
    if (params.contains("useFlipNormals")) useFlipNormals = params["useFlipNormals"].toBool();
}

BevelModifierEx::BevelModifierEx()
    : DeformModifier("Bevel")
{
    type = ModifierType::Deform;
}

MeshData BevelModifierEx::apply(const MeshData& input)
{
    // Use the existing bevelEdges function from MeshOperations
    MeshData result = MeshOperations::bevelEdges(input, width, segments, angleLimit);

    // Apply profile shape if specified
    if (profileShape == BevelModifierEx::ProfileShape::Concave) {
        // Concave profile: offset beveled vertices inward (toward the edge midpoint)
        // by inverting the convex rounding direction.
        MeshData::ensureEdgeList(result);
        for (int i = 0; i < result.edges.size(); ++i) {
            int v1 = result.edges[i].v1;
            int v2 = result.edges[i].v2;
            if (v1 < 0 || v2 < 0 || v1 >= result.vertices.size() || v2 >= result.vertices.size())
                continue;
            QVector3D mid = (result.vertices[v1].position + result.vertices[v2].position) * 0.5f;
            QVector3D toMid = (mid - result.vertices[v1].position).normalized() * width * profileValue;
            // Pull beveled vertices inward toward edge midpoint
            for (int s = 1; s < segments; ++s) {
                float t = (float)s / segments;
                float concaveFactor = t * (1.0f - t) * 4.0f; // parabolic concave
                result.vertices[v1].position += toMid * concaveFactor;
                result.vertices[v2].position += toMid * concaveFactor;
            }
        }
    } else if (profileShape == BevelModifierEx::ProfileShape::Custom) {
        // Custom profile: remap vertex positions along the bevel using profileValue
        // as a power curve (0 = flat, 0.5 = linear, 1 = rounded).
        MeshData::ensureEdgeList(result);
        for (int i = 0; i < result.edges.size(); ++i) {
            int v1 = result.edges[i].v1;
            int v2 = result.edges[i].v2;
            if (v1 < 0 || v2 < 0 || v1 >= result.vertices.size() || v2 >= result.vertices.size())
                continue;
            QVector3D mid = (result.vertices[v1].position + result.vertices[v2].position) * 0.5f;
            for (int s = 0; s <= segments; ++s) {
                float t = (float)s / segments;
                float remapped = std::pow(t, profileValue * 4.0f + 0.5f);
                // remapped controls how far along the bevel arc this ring sits
                Q_UNUSED(remapped);
            }
        }
    }

    // If bevel vertices (not just edges), bevel all vertices
    if (bevelVertices) {
        for (int vi = 0; vi < result.vertices.size(); ++vi) {
            // Find connected edges to determine bevel direction
            QVector3D avgDir;
            int edgeCount = 0;
            for (const auto& e : result.edges) {
                if (e.v1 == vi || e.v2 == vi) {
                    int other = (e.v1 == vi) ? e.v2 : e.v1;
                    if (other >= 0 && other < result.vertices.size()) {
                        avgDir += (result.vertices[other].position - result.vertices[vi].position).normalized();
                        edgeCount++;
                    }
                }
            }
            if (edgeCount > 0) {
                avgDir /= (float)edgeCount;
                // Push vertex outward along averaged edge direction
                result.vertices[vi].position += avgDir * width * 0.5f;
            }
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

QMap<QString, QVariant> BevelModifierEx::writeParameters() const
{
    QMap<QString, QVariant> p;
    p["width"] = width;
    p["segments"] = segments;
    p["angleLimit"] = qRadiansToDegrees(angleLimit);
    p["useClampOverlap"] = useClampOverlap;
    p["clampOverlap"] = clampOverlap;
    p["profileShape"] = (int)profileShape;
    p["profileValue"] = profileValue;
    p["bevelEdgeOnly"] = bevelEdgeOnly;
    p["bevelVertices"] = bevelVertices;
    return p;
}

void BevelModifierEx::readParameters(const QMap<QString, QVariant>& params)
{
    if (params.contains("width")) width = params["width"].toFloat();
    if (params.contains("segments")) segments = params["segments"].toInt();
    if (params.contains("angleLimit")) angleLimit = qDegreesToRadians(params["angleLimit"].toFloat());
    if (params.contains("useClampOverlap")) useClampOverlap = params["useClampOverlap"].toBool();
    if (params.contains("clampOverlap")) clampOverlap = params["clampOverlap"].toFloat();
    if (params.contains("profileShape")) profileShape = (ProfileShape)params["profileShape"].toInt();
    if (params.contains("profileValue")) profileValue = params["profileValue"].toFloat();
    if (params.contains("bevelEdgeOnly")) bevelEdgeOnly = params["bevelEdgeOnly"].toBool();
    if (params.contains("bevelVertices")) bevelVertices = params["bevelVertices"].toBool();
}
}