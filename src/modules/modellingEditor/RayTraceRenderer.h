#pragma once

#include <QImage>
#include <QVector3D>
#include <QVector>
#include <QColor>

namespace ks {

// A single world-space triangle fed to the ray tracer.
struct RTTriangle {
    QVector3D v0, v1, v2;
    QVector3D n0, n1, n2; // per-vertex normals (world space)
    QColor color;
    float metalness = 0.0f;
    float roughness = 0.6f;
};

struct RTCamera {
    QVector3D eye;
    QVector3D target;
    float fovDeg = 60.0f;
};

// A photometric light fed to the tracer. `type` matches LightSystem::LightDef:
// 0 = Directional (position unused), 1 = Point, 2 = Spot, 3 = Area (wide cone).
struct RTLight {
    int type = 0;
    QVector3D position;
    QVector3D direction;   // light forward axis (-Z), normalized
    QColor color = QColor(255, 244, 224);
    float intensity = 1.0f;
    float range = 30.0f;
    float spotAngleDeg = 45.0f;
    float spotPenumbraDeg = 10.0f;
    // Normalized vertical candela curve (index 0..90 == 0..90 deg from forward).
    // Empty disables IES shaping.
    QVector<float> iesCurve;
};

// Render elements (AOVs) produced by the CPU tracer. Color is the classic
// shaded preview; the others are compositing passes straight from the same
// primary intersection (no post-processing): a depth/Z pass (near = bright,
// non-mesh background = black), an ambient-occlusion pass (white = open space),
// a flat albedo/diffuse pass and a world-space normal pass (RGB = N*0.5+0.5).
enum class RenderPass {
    Color = 0,
    Depth,
    AmbientOcclusion,
    Diffuse,
    Normal
};

// Minimal CPU ray tracer used for the "Raytrace" viewport mode.
// Renders scene triangles against a sun light + ambient, with a
// procedural ground grid and gradient sky. Geometry is organised in a
// bounding-volume hierarchy built once per setObjects() call.
class RayTraceRenderer {
public:
    void setObjects(const QVector<RTTriangle>& tris);
    void setLights(const QVector<RTLight>& lights) { m_lights = lights; }
    void clear() { m_tris.clear(); m_bvh.clear(); m_lights.clear(); }
    bool hasLights() const { return !m_lights.isEmpty(); }
    // Preview render of the requested pass. `samples` supersamples primary
    // rays (used by pass `Color` and `AmbientOcclusion` for final outputs).
    QImage render(const RTCamera& cam, int width, int height,
                  RenderPass pass = RenderPass::Color, int samples = 1) const;
    // Mental-ray-style path-traced render: samples^2 rays/pixel, soft sun
    // shadow, one diffuse indirect bounce and ACES-ish tonemapping.
    QImage renderFinal(const RTCamera& cam, int width, int height, int samples) const;
    bool hasObjects() const { return !m_tris.isEmpty(); }

private:
    struct AABB { QVector3D min, max; };
    struct BVHNode {
        AABB box;
        int left = -1;
        int right = -1;
        int triStart = 0;
        int triCount = 0;
    };

    struct Hit {
        bool hit = false;
        float t = 0.0f;
        int tri = 0;
        float u = 0.0f;
        float v = 0.0f;
    };

    QVector<RTTriangle> m_tris;
    QVector<int> m_triIndices;
    QVector<BVHNode> m_bvh;
    QVector<RTLight> m_lights;

    int buildNode(int begin, int end);
    static AABB boundsOf(const RTTriangle& t);
    static AABB unionBox(const AABB& a, const AABB& b);
    bool rayBox(const QVector3D& o, const QVector3D& d, const AABB& b, float tMax) const;
    void intersect(const QVector3D& o, const QVector3D& d, float tMin, float tMax, Hit& out) const;
    void intersectNode(const QVector3D& o, const QVector3D& d, int node, float tMax, Hit& out) const;
    static bool intersectTri(const QVector3D& o, const QVector3D& d, const RTTriangle& t, float& tHit, float& u, float& v);
    QRgb shade(const QVector3D& o, const QVector3D& d, const Hit& h) const;
    QRgb shadeBackground(const QVector3D& d) const;
    QVector3D traceRay(const QVector3D& o, const QVector3D& d, int depth) const;
    QVector3D backgroundVec(const QVector3D& d) const;
    // Per-primary-ray evaluation of a non-color pass (returns linear 0..1).
    QVector3D evalPass(const QVector3D& o, const QVector3D& d, const Hit& h,
                       RenderPass pass, float farDist, unsigned& seed) const;
    float sampleAO(const QVector3D& P, const QVector3D& N, unsigned& seed) const;
    float sceneRadius() const;
    // Evaluates the direct lighting at P (ambient + every enabled light, with
    // shadow/cone/IES attenuation and Blinn-Phong specular), returning linear
    // 0..1 sRGB-ish RGB. Used by both the preview shade() and the path tracer.
    QVector3D shadePoint(const QVector3D& P, const QVector3D& N, const QVector3D& V,
                         float metalness, float roughness, const QColor& base) const;
};

} // namespace ks
