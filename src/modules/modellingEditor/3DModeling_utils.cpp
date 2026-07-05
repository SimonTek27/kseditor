#include "3DModeling_utils.h"
#include <QDebug>
#include <QFile>
#include <cmath>

namespace ks {
namespace modeler {

// ============================================================================
// ParametricObject Implementation
// ============================================================================

ParametricObject::ParametricObject(const QString& name, QObject* parent)
    : QObject(parent), m_name(name)
{
}

void ParametricObject::addParameter(const QString& key, ParamType type,
                                    const QVariant& defaultValue, const QString& description)
{
    ParamValue pv;
    pv.type = type;
    pv.value = defaultValue;
    pv.description = description;
    m_params[key] = pv;
}

void ParametricObject::setParameter(const QString& key, const QVariant& value)
{
    if (m_params.contains(key)) {
        m_params[key].value = value;
        emit parameterChanged(key);
        rebuild();
    }
}

ParamValue ParametricObject::getParameter(const QString& key) const
{
    return m_params.value(key);
}

// ============================================================================
// ParametricWheel Implementation
// ============================================================================

ParametricWheel::ParametricWheel(QObject* parent)
    : ParametricObject("Wheel", parent)
{
    addParameter("rimDiameter", ParamFloat, 18.0f, "Rim diameter in inches");
    addParameter("rimWidth", ParamFloat, 9.5f, "Rim width in inches");
    addParameter("tireWidth", ParamFloat, 245.0f, "Tire width in mm");
    addParameter("tireProfile", ParamFloat, 40.0f, "Tire profile (%");
    addParameter("numBolts", ParamInt, 5, "Number of bolts");
    addParameter("boltPattern", ParamFloat, 112.0f, "Bolt pattern diameter in mm");
    addParameter("offset", ParamFloat, 45.0f, "Wheel offset in mm");
}

void ParametricWheel::rebuild()
{
    generateWheelMesh();
    emit meshRegenerated();
}

QByteArray ParametricWheel::getMeshData() const
{
    return m_meshData;
}

void ParametricWheel::generateWheelMesh()
{
    m_meshData.clear();

    float rimDia = m_params["rimDiameter"].toFloat() * 25.4f;
    float rimWidth = m_params["rimWidth"].toFloat() * 25.4f;
    float tireWidth = m_params["tireWidth"].toFloat();
    float tireProfile = m_params["tireProfile"].toFloat() / 100.0f;
    int numBolts = m_params["numBolts"].toInt();
    float boltPattern = m_params["boltPattern"].toFloat();

    float rimRadius = rimDia * 0.5f;
    float tireOuterRadius = rimRadius + tireWidth * tireProfile;
    float tireInnerRadius = rimRadius;
    float halfWidth = rimWidth * 0.5f;

    const int rimSegments = 48;
    const int tireSegments = 24;

    struct WVertex { float x, y, z, nx, ny, nz, u, v, r, g, b, a; };
    QVector<WVertex> verts;
    QVector<uint32_t> indices;

    auto addVert = [&](float x, float y, float z, float nx, float ny, float nz, float u, float v) {
        verts.append({x, y, z, nx, ny, nz, u, v, 0.7f, 0.7f, 0.7f, 1.0f});
    };

    // Rim outer cylinder
    int rimBase = verts.size();
    for (int i = 0; i <= rimSegments; ++i) {
        float a = (float)i / rimSegments * 2.0f * M_PI;
        float c = std::cos(a), s = std::sin(a);
        addVert(c * rimRadius, s * rimRadius, -halfWidth, c, s, 0, (float)i / rimSegments, 0);
        addVert(c * rimRadius, s * rimRadius,  halfWidth, c, s, 0, (float)i / rimSegments, 1);
    }
    for (int i = 0; i < rimSegments; ++i) {
        int b = rimBase + i * 2;
        indices.append(b); indices.append(b + 1); indices.append(b + 2);
        indices.append(b + 1); indices.append(b + 3); indices.append(b + 2);
    }

    // Rim inner barrel (lip)
    float lipRadius = rimRadius - 5.0f;
    if (lipRadius < 10.0f) lipRadius = rimRadius * 0.85f;
    int lipBase = verts.size();
    for (int i = 0; i <= rimSegments; ++i) {
        float a = (float)i / rimSegments * 2.0f * M_PI;
        float c = std::cos(a), s = std::sin(a);
        addVert(c * lipRadius, s * lipRadius, -halfWidth + 3.0f, -c, -s, 0, (float)i / rimSegments, 0);
        addVert(c * rimRadius, s * rimRadius, -halfWidth, -c, -s, 0, (float)i / rimSegments, 1);
    }
    for (int i = 0; i < rimSegments; ++i) {
        int b = lipBase + i * 2;
        indices.append(b); indices.append(b + 2); indices.append(b + 1);
        indices.append(b + 1); indices.append(b + 2); indices.append(b + 3);
    }

    // Tire cross-section (torus-like)
    int tireBase = verts.size();
    float tireRadialWidth = (tireOuterRadius - tireInnerRadius);
    for (int i = 0; i <= rimSegments; ++i) {
        float a = (float)i / rimSegments * 2.0f * M_PI;
        float c = std::cos(a), s = std::sin(a);
        for (int j = 0; j <= tireSegments; ++j) {
            float t = (float)j / tireSegments;
            float angle = t * M_PI * 0.5f;
            float rr = tireInnerRadius + tireRadialWidth * std::sin(angle);
            float zz = halfWidth + tireRadialWidth * std::cos(angle) * 0.6f;
            float nx = c * std::cos(angle);
            float ny = s * std::cos(angle);
            float nz = std::sin(angle) * 0.6f;
            addVert(c * rr, s * rr, zz, nx, ny, nz, (float)i / rimSegments, t);
        }
    }
    for (int i = 0; i < rimSegments; ++i) {
        for (int j = 0; j < tireSegments; ++j) {
            int b = tireBase + i * (tireSegments + 1) + j;
            int n = b + tireSegments + 1;
            indices.append(b); indices.append(n); indices.append(b + 1);
            indices.append(b + 1); indices.append(n); indices.append(n + 1);
        }
    }
    // Inner tire wall
    for (int i = 0; i <= rimSegments; ++i) {
        float a = (float)i / rimSegments * 2.0f * M_PI;
        float c = std::cos(a), s = std::sin(a);
        for (int j = 0; j <= tireSegments; ++j) {
            float t = (float)j / tireSegments;
            float angle = t * M_PI * 0.5f;
            float rr = tireInnerRadius + tireRadialWidth * std::sin(angle);
            float zz = -(halfWidth + tireRadialWidth * std::cos(angle) * 0.6f);
            float nx = -c * std::cos(angle);
            float ny = -s * std::cos(angle);
            float nz = -std::sin(angle) * 0.6f;
            addVert(c * rr, s * rr, zz, nx, ny, nz, (float)i / rimSegments, t);
        }
    }
    int innerBase = tireBase + (rimSegments + 1) * (tireSegments + 1);
    for (int i = 0; i < rimSegments; ++i) {
        for (int j = 0; j < tireSegments; ++j) {
            int b = innerBase + i * (tireSegments + 1) + j;
            int n = b + tireSegments + 1;
            indices.append(b); indices.append(b + 1); indices.append(n);
            indices.append(b + 1); indices.append(n + 1); indices.append(n);
        }
    }

    // Bolt holes
    if (numBolts > 0 && boltPattern > 0) {
        float boltRadius = boltPattern * 0.5f;
        float holeRadius = 6.0f;
        for (int b = 0; b < numBolts; ++b) {
            float ba = (float)b / numBolts * 2.0f * M_PI;
            float bx = std::cos(ba) * boltRadius;
            float by = std::sin(ba) * boltRadius;
            int holeBase = verts.size();
            for (int i = 0; i <= 16; ++i) {
                float ha = (float)i / 16 * 2.0f * M_PI;
                float hc = std::cos(ha), hs = std::sin(ha);
                addVert(bx + hc * holeRadius, by + hs * holeRadius, -halfWidth - 1.0f, 0, 0, -1, (float)i / 16, 0);
                addVert(bx + hc * holeRadius, by + hs * holeRadius, -halfWidth + 2.0f, 0, 0, -1, (float)i / 16, 1);
            }
            for (int i = 0; i < 16; ++i) {
                int vb = holeBase + i * 2;
                indices.append(vb); indices.append(vb + 2); indices.append(vb + 1);
                indices.append(vb + 1); indices.append(vb + 2); indices.append(vb + 3);
            }
        }
    }

    // Serialize to QByteArray
    QDataStream stream(&m_meshData, QIODevice::WriteOnly);
    stream << (uint32_t)verts.size();
    stream << (uint32_t)indices.size();
    for (const auto& v : verts) {
        stream << v.x << v.y << v.z << v.nx << v.ny << v.nz << v.u << v.v << v.r << v.g << v.b << v.a;
    }
    for (uint32_t idx : indices) {
        stream << idx;
    }

    qDebug() << "Generated wheel mesh:" << verts.size() << "vertices," << indices.size() / 3 << "triangles";
}

// ============================================================================
// ParametricBrakeDisc Implementation
// ============================================================================

ParametricBrakeDisc::ParametricBrakeDisc(QObject* parent)
    : ParametricObject("BrakeDisc", parent)
{
    addParameter("discDiameter", ParamFloat, 280.0f, "Disc diameter in mm");
    addParameter("discThickness", ParamFloat, 25.0f, "Disc thickness in mm");
    addParameter("hatDiameter", ParamFloat, 150.0f, "Hat diameter in mm");
    addParameter("numBoltHoles", ParamInt, 5, "Number of bolt holes");
    addParameter("coolingVanes", ParamBool, true, "Add cooling vanes");
}

void ParametricBrakeDisc::rebuild()
{
    generateBrakeDiscMesh();
    emit meshRegenerated();
}

QByteArray ParametricBrakeDisc::getMeshData() const
{
    return m_meshData;
}

void ParametricBrakeDisc::generateBrakeDiscMesh()
{
    m_meshData.clear();
    float discDia = m_params["discDiameter"].toFloat();
    float discThickness = m_params["discThickness"].toFloat();
    float hatDia = m_params["hatDiameter"].toFloat();
    int numBoltHoles = m_params["numBoltHoles"].toInt();
    bool coolingVanes = m_params["coolingVanes"].toBool();

    float discRadius = discDia * 0.5f;
    float hatRadius = hatDia * 0.5f;
    float halfThick = discThickness * 0.5f;
    float hatHeight = discThickness * 1.5f;

    const int segments = 64;
    const int vaneCount = coolingVanes ? 12 : 0;

    struct BVertex { float x, y, z, nx, ny, nz, u, v, r, g, b, a; };
    QVector<BVertex> verts;
    QVector<uint32_t> indices;

    auto addVert = [&](float x, float y, float z, float nx, float ny, float nz, float u, float v) {
        verts.append({x, y, z, nx, ny, nz, u, v, 0.6f, 0.6f, 0.65f, 1.0f});
    };

    // Disc top face
    int topBase = verts.size();
    addVert(0, 0, halfThick, 0, 0, 1, 0.5f, 0.5f);
    for (int i = 0; i <= segments; ++i) {
        float a = (float)i / segments * 2.0f * M_PI;
        float c = std::cos(a), s = std::sin(a);
        addVert(c * discRadius, s * discRadius, halfThick, 0, 0, 1, c * 0.5f + 0.5f, s * 0.5f + 0.5f);
    }
    for (int i = 0; i < segments; ++i) {
        indices.append(topBase);
        indices.append(topBase + 1 + i);
        indices.append(topBase + 2 + i);
    }

    // Disc bottom face
    int botBase = verts.size();
    addVert(0, 0, -halfThick, 0, 0, -1, 0.5f, 0.5f);
    for (int i = 0; i <= segments; ++i) {
        float a = (float)i / segments * 2.0f * M_PI;
        float c = std::cos(a), s = std::sin(a);
        addVert(c * discRadius, s * discRadius, -halfThick, 0, 0, -1, c * 0.5f + 0.5f, s * 0.5f + 0.5f);
    }
    for (int i = 0; i < segments; ++i) {
        indices.append(botBase);
        indices.append(botBase + 2 + i);
        indices.append(botBase + 1 + i);
    }

    // Disc outer rim
    int rimBase = verts.size();
    for (int i = 0; i <= segments; ++i) {
        float a = (float)i / segments * 2.0f * M_PI;
        float c = std::cos(a), s = std::sin(a);
        addVert(c * discRadius, s * discRadius, -halfThick, c, s, 0, (float)i / segments, 0);
        addVert(c * discRadius, s * discRadius,  halfThick, c, s, 0, (float)i / segments, 1);
    }
    for (int i = 0; i < segments; ++i) {
        int b = rimBase + i * 2;
        indices.append(b); indices.append(b + 1); indices.append(b + 2);
        indices.append(b + 1); indices.append(b + 3); indices.append(b + 2);
    }

    // Hat (center hub)
    int hatBase = verts.size();
    for (int i = 0; i <= segments; ++i) {
        float a = (float)i / segments * 2.0f * M_PI;
        float c = std::cos(a), s = std::sin(a);
        addVert(c * hatRadius, s * hatRadius, halfThick, c, s, 0, (float)i / segments, 0);
        addVert(c * hatRadius, s * hatRadius, halfThick + hatHeight, c, s, 0, (float)i / segments, 1);
    }
    for (int i = 0; i < segments; ++i) {
        int b = hatBase + i * 2;
        indices.append(b); indices.append(b + 2); indices.append(b + 1);
        indices.append(b + 1); indices.append(b + 2); indices.append(b + 3);
    }

    // Hat top cap
    int hatTopBase = verts.size();
    addVert(0, 0, halfThick + hatHeight, 0, 0, 1, 0.5f, 0.5f);
    for (int i = 0; i <= segments; ++i) {
        float a = (float)i / segments * 2.0f * M_PI;
        float c = std::cos(a), s = std::sin(a);
        addVert(c * hatRadius, s * hatRadius, halfThick + hatHeight, 0, 0, 1, c * 0.5f + 0.5f, s * 0.5f + 0.5f);
    }
    for (int i = 0; i < segments; ++i) {
        indices.append(hatTopBase);
        indices.append(hatTopBase + 2 + i);
        indices.append(hatTopBase + 1 + i);
    }

    // Bolt holes in hat
    if (numBoltHoles > 0) {
        float boltRadius = hatRadius * 0.6f;
        float holeRadius = 4.0f;
        for (int b = 0; b < numBoltHoles; ++b) {
            float ba = (float)b / numBoltHoles * 2.0f * M_PI;
            float bx = std::cos(ba) * boltRadius;
            float by = std::sin(ba) * boltRadius;
            int holeBase = verts.size();
            for (int i = 0; i <= 12; ++i) {
                float ha = (float)i / 12 * 2.0f * M_PI;
                float hc = std::cos(ha), hs = std::sin(ha);
                addVert(bx + hc * holeRadius, by + hs * holeRadius, halfThick - 1.0f, 0, 0, -1, (float)i / 12, 0);
                addVert(bx + hc * holeRadius, by + hs * holeRadius, halfThick + hatHeight + 1.0f, 0, 0, 1, (float)i / 12, 1);
            }
            for (int i = 0; i < 12; ++i) {
                int vb = holeBase + i * 2;
                indices.append(vb); indices.append(vb + 1); indices.append(vb + 2);
                indices.append(vb + 1); indices.append(vb + 3); indices.append(vb + 2);
            }
        }
    }

    // Cooling vanes (radial ridges on disc surface)
    if (coolingVanes && vaneCount > 0) {
        float vaneHeight = 3.0f;
        float vaneWidth = 2.0f;
        for (int v = 0; v < vaneCount; ++v) {
            float va = (float)v / vaneCount * 2.0f * M_PI;
            float vc = std::cos(va), vs = std::sin(va);
            int vaneBase = verts.size();
            float innerR = hatRadius + 5.0f;
            float outerR = discRadius - 5.0f;
            addVert(vc * innerR, vs * innerR, halfThick, 0, 0, 1, 0, 0);
            addVert(vc * outerR, vs * outerR, halfThick, 0, 0, 1, 1, 0);
            addVert(vc * innerR, vs * innerR, halfThick + vaneHeight, 0, 0, 1, 0, 1);
            addVert(vc * outerR, vs * outerR, halfThick + vaneHeight, 0, 0, 1, 1, 1);
            indices.append(vaneBase); indices.append(vaneBase + 1); indices.append(vaneBase + 2);
            indices.append(vaneBase + 1); indices.append(vaneBase + 3); indices.append(vaneBase + 2);
        }
    }

    // Serialize
    QDataStream stream(&m_meshData, QIODevice::WriteOnly);
    stream << (uint32_t)verts.size();
    stream << (uint32_t)indices.size();
    for (const auto& v : verts) {
        stream << v.x << v.y << v.z << v.nx << v.ny << v.nz << v.u << v.v << v.r << v.g << v.b << v.a;
    }
    for (uint32_t idx : indices) {
        stream << idx;
    }

    qDebug() << "Generated brake disc mesh:" << verts.size() << "vertices," << indices.size() / 3 << "triangles";
}

// ============================================================================
// ParametricSuspensionArm Implementation
// ============================================================================

ParametricSuspensionArm::ParametricSuspensionArm(QObject* parent)
    : ParametricObject("SuspensionArm", parent)
{
    addParameter("armLength", ParamFloat, 300.0f, "Arm length in mm");
    addParameter("armWidth", ParamFloat, 40.0f, "Arm width in mm");
    addParameter("armThickness", ParamFloat, 25.0f, "Arm thickness in mm");
    addParameter("bushingDiameter", ParamFloat, 45.0f, "Bushing diameter in mm");
    addParameter("armType", ParamEnum, 0, "Type of arm");
    m_params["armType"].enumValues = {"A-Arm", "L-Arm", "Multi-Link"};
}

void ParametricSuspensionArm::rebuild()
{
    generateArmMesh();
    emit meshRegenerated();
}

QByteArray ParametricSuspensionArm::getMeshData() const
{
    return m_meshData;
}

void ParametricSuspensionArm::generateArmMesh()
{
    m_meshData.clear();
    float length = m_params["armLength"].toFloat();
    float width = m_params["armWidth"].toFloat();
    float thickness = m_params["armThickness"].toFloat();
    float bushingDia = m_params["bushingDiameter"].toFloat();
    int armType = m_params["armType"].toInt();

    float halfLen = length * 0.5f;
    float halfW = width * 0.5f;
    float halfT = thickness * 0.5f;
    float bushRadius = bushingDia * 0.5f;

    struct SVertex { float x, y, z, nx, ny, nz, u, v, r, g, b, a; };
    QVector<SVertex> verts;
    QVector<uint32_t> indices;

    auto addVert = [&](float x, float y, float z, float nx, float ny, float nz, float u, float v) {
        verts.append({x, y, z, nx, ny, nz, u, v, 0.5f, 0.55f, 0.6f, 1.0f});
    };

    if (armType == 0) {
        // A-Arm: triangular shape with two inner mounting points and one outer ball joint
        float innerSpread = halfLen * 0.6f;
        float pivotX = -halfLen;
        float ballX = halfLen;

        // Top face
        int topBase = verts.size();
        addVert(pivotX, 0, halfT, 0, 0, 1, 0, 0.5f);
        addVert(pivotX, -innerSpread, halfT, 0, 0, 1, 0, 0);
        addVert(pivotX,  innerSpread, halfT, 0, 0, 1, 0, 1);
        addVert(ballX, 0, halfT, 0, 0, 1, 1, 0.5f);
        indices.append(topBase); indices.append(topBase + 1); indices.append(topBase + 3);
        indices.append(topBase); indices.append(topBase + 3); indices.append(topBase + 2);

        // Bottom face
        int botBase = verts.size();
        addVert(pivotX, 0, -halfT, 0, 0, -1, 0, 0.5f);
        addVert(pivotX, -innerSpread, -halfT, 0, 0, -1, 0, 0);
        addVert(pivotX,  innerSpread, -halfT, 0, 0, -1, 0, 1);
        addVert(ballX, 0, -halfT, 0, 0, -1, 1, 0.5f);
        indices.append(botBase); indices.append(botBase + 3); indices.append(botBase + 1);
        indices.append(botBase); indices.append(botBase + 2); indices.append(botBase + 3);

        // Side faces
        auto addSideQuad = [&](int i0, int i1, int i2, int i3) {
            int b = verts.size();
            for (int idx : {i0, i1, i2, i3}) {
                const auto& sv = (idx < topBase ? verts[idx] : verts[idx]);
                addVert(sv.x, sv.y, sv.z, 0, 0, 0, 0, 0);
            }
            // Compute normal from edge vectors
            QVector3D e1(verts[i1].x - verts[i0].x, verts[i1].y - verts[i0].y, verts[i1].z - verts[i0].z);
            QVector3D e2(verts[i3].x - verts[i0].x, verts[i3].y - verts[i0].y, verts[i3].z - verts[i0].z);
            QVector3D n = QVector3D::crossProduct(e1, e2).normalized();
            for (int j = b; j < b + 4; ++j) { verts[j].nx = n.x(); verts[j].ny = n.y(); verts[j].nz = n.z(); }
            indices.append(b); indices.append(b + 1); indices.append(b + 2);
            indices.append(b); indices.append(b + 2); indices.append(b + 3);
        };

        // Outer edge: pivot to ball
        addSideQuad(topBase, topBase + 3, botBase + 3, botBase);
        // Inner edge 1: pivot to inner1
        addSideQuad(topBase, topBase + 1, botBase + 1, botBase);
        // Inner edge 2: pivot to inner2
        addSideQuad(topBase + 2, topBase, botBase, botBase + 2);
        // Front edge: inner1 to ball
        addSideQuad(topBase + 1, topBase + 3, botBase + 3, botBase + 1);
        // Back edge: inner2 to ball
        addSideQuad(topBase + 3, topBase + 2, botBase + 2, botBase + 3);

        // Bushing cylinders at inner mount points
        for (int side = -1; side <= 1; side += 2) {
            int bushBase = verts.size();
            for (int i = 0; i <= 16; ++i) {
                float a = (float)i / 16 * 2.0f * M_PI;
                float c = std::cos(a), s = std::sin(a);
                addVert(pivotX, side * innerSpread + c * bushRadius, s * bushRadius, 0, 0, 0, (float)i / 16, 0);
                addVert(pivotX - 5.0f, side * innerSpread + c * bushRadius, s * bushRadius, -1, 0, 0, (float)i / 16, 1);
            }
            for (int i = 0; i < 16; ++i) {
                int b = bushBase + i * 2;
                indices.append(b); indices.append(b + 1); indices.append(b + 2);
                indices.append(b + 1); indices.append(b + 3); indices.append(b + 2);
            }
        }

        // Ball joint sphere at outer end
        int ballBase = verts.size();
        const int ballSeg = 12, ballRings = 8;
        for (int ring = 0; ring <= ballRings; ++ring) {
            float theta = (float)ring / ballRings * M_PI;
            float st = std::sin(theta), ct = std::cos(theta);
            for (int seg = 0; seg <= ballSeg; ++seg) {
                float phi = (float)seg / ballSeg * 2.0f * M_PI;
                float x = std::cos(phi) * st, y = std::sin(phi) * st, z = ct;
                addVert(ballX + x * bushRadius * 0.8f, y * bushRadius * 0.8f, z * bushRadius * 0.8f, x, y, z, (float)seg / ballSeg, (float)ring / ballRings);
            }
        }
        for (int ring = 0; ring < ballRings; ++ring) {
            for (int seg = 0; seg < ballSeg; ++seg) {
                int b = ballBase + ring * (ballSeg + 1) + seg;
                int n = b + ballSeg + 1;
                indices.append(b); indices.append(n); indices.append(b + 1);
                indices.append(b + 1); indices.append(n); indices.append(n + 1);
            }
        }

    } else if (armType == 1) {
        // L-Arm: L-shaped arm
        float cornerX = 0;
        float armLen2 = halfLen * 0.5f;

        // Main beam (along X)
        int mainBase = verts.size();
        addVert(-halfLen, 0, halfT, 0, 0, 1, 0, 0);
        addVert(-halfLen, 0, -halfT, 0, 0, -1, 0, 1);
        addVert( cornerX, 0, halfT, 0, 0, 1, 0.5f, 0);
        addVert( cornerX, 0, -halfT, 0, 0, -1, 0.5f, 1);
        addVert( cornerX, armLen2, halfT, 0, 0, 1, 1, 0);
        addVert( cornerX, armLen2, -halfT, 0, 0, -1, 1, 1);
        addVert( halfLen, 0, halfT, 0, 0, 1, 0.75f, 0);
        addVert( halfLen, 0, -halfT, 0, 0, -1, 0.75f, 1);

        indices.append(mainBase); indices.append(mainBase+2); indices.append(mainBase+4);
        indices.append(mainBase+2); indices.append(mainBase+6); indices.append(mainBase+4);
        indices.append(mainBase+1); indices.append(mainBase+5); indices.append(mainBase+3);
        indices.append(mainBase+3); indices.append(mainBase+5); indices.append(mainBase+7);
        // Top
        indices.append(mainBase); indices.append(mainBase+4); indices.append(mainBase+6);
        // Bottom
        indices.append(mainBase+1); indices.append(mainBase+7); indices.append(mainBase+5);

        // Bushing at rear (-X end)
        int bushBase = verts.size();
        for (int i = 0; i <= 16; ++i) {
            float a = (float)i / 16 * 2.0f * M_PI;
            float c = std::cos(a), s = std::sin(a);
            addVert(-halfLen - 3.0f, c * bushRadius, s * bushRadius, -1, 0, 0, (float)i / 16, 0);
            addVert(-halfLen, c * bushRadius, s * bushRadius, 0, c, s, (float)i / 16, 1);
        }
        for (int i = 0; i < 16; ++i) {
            int b = bushBase + i * 2;
            indices.append(b); indices.append(b + 1); indices.append(b + 2);
            indices.append(b + 1); indices.append(b + 3); indices.append(b + 2);
        }
    } else {
        // Multi-Link: rectangular bar with mounting holes
        int barBase = verts.size();
        addVert(-halfLen, -halfW, halfT, 0, 0, 1, 0, 0);
        addVert(-halfLen, -halfW, -halfT, 0, 0, -1, 0, 1);
        addVert(-halfLen,  halfW, halfT, 0, 0, 1, 0.2f, 0);
        addVert(-halfLen,  halfW, -halfT, 0, 0, -1, 0.2f, 1);
        addVert( halfLen, -halfW, halfT, 0, 0, 1, 0.8f, 0);
        addVert( halfLen, -halfW, -halfT, 0, 0, -1, 0.8f, 1);
        addVert( halfLen,  halfW, halfT, 0, 0, 1, 1, 0);
        addVert( halfLen,  halfW, -halfT, 0, 0, -1, 1, 1);

        // Top
        indices.append(barBase); indices.append(barBase+4); indices.append(barBase+6);
        indices.append(barBase); indices.append(barBase+6); indices.append(barBase+2);
        // Bottom
        indices.append(barBase+1); indices.append(barBase+3); indices.append(barBase+7);
        indices.append(barBase+1); indices.append(barBase+7); indices.append(barBase+5);
        // Front
        indices.append(barBase); indices.append(barBase+1); indices.append(barBase+5);
        indices.append(barBase); indices.append(barBase+5); indices.append(barBase+4);
        // Back
        indices.append(barBase+2); indices.append(barBase+6); indices.append(barBase+7);
        indices.append(barBase+2); indices.append(barBase+7); indices.append(barBase+3);
        // Left
        indices.append(barBase); indices.append(barBase+2); indices.append(barBase+3);
        indices.append(barBase); indices.append(barBase+3); indices.append(barBase+1);
        // Right
        indices.append(barBase+4); indices.append(barBase+5); indices.append(barBase+7);
        indices.append(barBase+4); indices.append(barBase+7); indices.append(barBase+6);

        // Mounting holes along the bar
        int numHoles = 3;
        float holeRadius = bushRadius * 0.6f;
        for (int h = 0; h < numHoles; ++h) {
            float hx = -halfLen + (float)(h + 1) / (numHoles + 1) * length;
            int holeBase = verts.size();
            for (int i = 0; i <= 12; ++i) {
                float a = (float)i / 12 * 2.0f * M_PI;
                float c = std::cos(a), s = std::sin(a);
                addVert(hx, c * holeRadius, halfT + 1.0f, 0, 0, 1, (float)i / 12, 0);
                addVert(hx, c * holeRadius, -halfT - 1.0f, 0, 0, -1, (float)i / 12, 1);
            }
            for (int i = 0; i < 12; ++i) {
                int b = holeBase + i * 2;
                indices.append(b); indices.append(b + 2); indices.append(b + 1);
                indices.append(b + 1); indices.append(b + 2); indices.append(b + 3);
            }
        }
    }

    // Serialize
    QDataStream stream(&m_meshData, QIODevice::WriteOnly);
    stream << (uint32_t)verts.size();
    stream << (uint32_t)indices.size();
    for (const auto& v : verts) {
        stream << v.x << v.y << v.z << v.nx << v.ny << v.nz << v.u << v.v << v.r << v.g << v.b << v.a;
    }
    for (uint32_t idx : indices) {
        stream << idx;
    }

    qDebug() << "Generated suspension arm mesh:" << verts.size() << "vertices," << indices.size() / 3 << "triangles";
}

// ============================================================================
// ParametricFactory Implementation
// ============================================================================

ParametricFactory* ParametricFactory::s_instance = nullptr;

ParametricFactory::ParametricFactory(QObject* parent) : QObject(parent)
{
}

ParametricFactory::~ParametricFactory()
{
}

ParametricFactory* ParametricFactory::instance()
{
    if (!s_instance) s_instance = new ParametricFactory();
    return s_instance;
}

ParametricObject* ParametricFactory::createWheel(const QString& name)
{
    auto* obj = new ParametricWheel();
    obj->setName(name.isEmpty() ? "Wheel" : name);
    return obj;
}

ParametricObject* ParametricFactory::createBrakeDisc(const QString& name)
{
    auto* obj = new ParametricBrakeDisc();
    obj->setName(name.isEmpty() ? "BrakeDisc" : name);
    return obj;
}

ParametricObject* ParametricFactory::createSuspensionArm(const QString& name)
{
    auto* obj = new ParametricSuspensionArm();
    obj->setName(name.isEmpty() ? "SuspensionArm" : name);
    return obj;
}

QVector<QString> ParametricFactory::availableTypes() const
{
    return {"Wheel", "BrakeDisc", "SuspensionArm"};
}

} // namespace modeler

} // namespace ks