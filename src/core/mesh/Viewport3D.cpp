#include "Viewport3D.h"
#include <QDebug>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QPainter>
#include <QtMath>
#include <cmath>
#include <cfloat>

namespace ks {

static QVector3D applyTransformPosition(const QVariantMap& tf, const QVector3D& p);

static Viewport3D* s_viewport3D = nullptr;

Viewport3D::Viewport3D(QObject* parent)
    : QObject(parent)
    , m_backgroundColor(25, 25, 28)
{
}

Viewport3D::~Viewport3D() {
    s_viewport3D = nullptr;
}

Viewport3D* Viewport3D::instance() {
    if (!s_viewport3D) {
        s_viewport3D = new Viewport3D();
    }
    return s_viewport3D;
}

bool Viewport3D::initialize() {
    if (m_initialized) return true;
#if QT_CONFIG(vulkan)
    m_initialized = true;
    qInfo() << "[Viewport3D] Initialized successfully";
    emit initialized(true);
    emit initializedChanged(true);
    return true;
#else
    qWarning() << "[Viewport3D] Vulkan not supported";
    emit initialized(false);
    return false;
#endif
}



void Viewport3D::setBackgroundColor(const QColor& color) { m_backgroundColor = color; }
QColor Viewport3D::getBackgroundColor() const { return m_backgroundColor; }

void Viewport3D::setDisplayMode(const QString& mode) { m_displayMode = mode; emit cameraChanged(); }
QString Viewport3D::getDisplayMode() const { return m_displayMode; }

void Viewport3D::setShadingMode(const QString& mode) { m_shadingMode = mode; }
QString Viewport3D::getShadingMode() const { return m_shadingMode; }

void Viewport3D::setShowGrid(bool show) { m_showGrid = show; }
bool Viewport3D::getShowGrid() const { return m_showGrid; }
void Viewport3D::setShowAxes(bool show) { m_showAxes = show; }
bool Viewport3D::getShowAxes() const { return m_showAxes; }
void Viewport3D::setShowNormals(bool show) { m_showNormals = show; }
bool Viewport3D::getShowNormals() const { return m_showNormals; }
void Viewport3D::setShowWireframe(bool show) { m_showWireframe = show; }
bool Viewport3D::getShowWireframe() const { return m_showWireframe; }
void Viewport3D::setShowLighting(bool show) { m_showLighting = show; }
bool Viewport3D::getShowLighting() const { return m_showLighting; }
void Viewport3D::setShowShadows(bool show) { m_showShadows = show; }
bool Viewport3D::getShowShadows() const { return m_showShadows; }

void Viewport3D::setCameraSpeed(float speed) { m_cameraSpeed = speed; }
float Viewport3D::getCameraSpeed() const { return m_cameraSpeed; }
void Viewport3D::setCameraSensitivity(float sensitivity) { m_cameraSensitivity = sensitivity; }
float Viewport3D::getCameraSensitivity() const { return m_cameraSensitivity; }

void Viewport3D::orbit(float dx, float dy) {
    m_cameraYaw += dx * m_cameraSensitivity;
    m_cameraPitch = qBound(-89.0f, m_cameraPitch + dy * m_cameraSensitivity, 89.0f);
    emit cameraChanged();
}
void Viewport3D::pan(float dx, float dy) {
    m_cameraTarget += QVector3D(-dx * m_cameraSpeed * 0.01f, dy * m_cameraSpeed * 0.01f, 0);
    emit cameraChanged();
}
void Viewport3D::zoom(float delta) {
    m_cameraDistance = qMax(0.5f, m_cameraDistance - delta * m_cameraSpeed * 0.05f);
    emit cameraChanged();
}
void Viewport3D::focusOnSelection() {
    QVector3D bmin(FLT_MAX, FLT_MAX, FLT_MAX);
    QVector3D bmax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    bool found = false;

    for (const QString& id : m_selectedObjects) {
        if (!m_meshes.contains(id)) continue;
        const MeshData& md = m_meshes[id];
        QVariantMap tf;
        if (m_meshTransforms.contains(id))
            tf = m_meshTransforms[id].toMap();

        for (const auto& v : md.vertices) {
            QVector3D p = applyTransformPosition(tf, v.value<QVector3D>());
            bmin.setX(qMin(bmin.x(), p.x()));
            bmin.setY(qMin(bmin.y(), p.y()));
            bmin.setZ(qMin(bmin.z(), p.z()));
            bmax.setX(qMax(bmax.x(), p.x()));
            bmax.setY(qMax(bmax.y(), p.y()));
            bmax.setZ(qMax(bmax.z(), p.z()));
            found = true;
        }
    }

    if (found) {
        m_cameraTarget = (bmin + bmax) * 0.5f;
        float radius = qMax(bmax.x() - bmin.x(), qMax(bmax.y() - bmin.y(), bmax.z() - bmin.z())) * 0.5f;
        m_cameraDistance = qMax(radius * 2.5f, 1.0f);
    } else {
        m_cameraTarget = QVector3D(0, 0, 0);
        m_cameraDistance = 10.0f;
    }
    emit cameraChanged();
}

void Viewport3D::focusOnAll() {
    QVector3D bmin(FLT_MAX, FLT_MAX, FLT_MAX);
    QVector3D bmax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    bool found = false;

    for (auto it = m_meshes.constBegin(); it != m_meshes.constEnd(); ++it) {
        QVariantMap tf;
        if (m_meshTransforms.contains(it.key()))
            tf = m_meshTransforms[it.key()].toMap();

        for (const auto& v : it->vertices) {
            QVector3D p = applyTransformPosition(tf, v.value<QVector3D>());
            bmin.setX(qMin(bmin.x(), p.x()));
            bmin.setY(qMin(bmin.y(), p.y()));
            bmin.setZ(qMin(bmin.z(), p.z()));
            bmax.setX(qMax(bmax.x(), p.x()));
            bmax.setY(qMax(bmax.y(), p.y()));
            bmax.setZ(qMax(bmax.z(), p.z()));
            found = true;
        }
    }

    if (found) {
        m_cameraTarget = (bmin + bmax) * 0.5f;
        float radius = qMax(bmax.x() - bmin.x(), qMax(bmax.y() - bmin.y(), bmax.z() - bmin.z())) * 0.5f;
        m_cameraDistance = qMax(radius * 2.5f, 1.0f);
    } else {
        m_cameraTarget = QVector3D(0, 0, 0);
        m_cameraDistance = 50.0f;
    }
    emit cameraChanged();
}
void Viewport3D::resetCamera() {
    m_cameraYaw = 0.0f;
    m_cameraPitch = 0.0f;
    m_cameraPanX = 0.0f;
    m_cameraPanY = 0.0f;
    m_cameraDistance = 50.0f;
    m_cameraTarget = QVector3D(0, 0, 0);
    emit cameraChanged();
}

void Viewport3D::setGizmoMode(const QString& mode) { m_gizmoMode = mode; }
QString Viewport3D::getGizmoMode() const { return m_gizmoMode; }

void Viewport3D::setMeshData(const QVariant& meshData) {
    m_meshData = meshData.toMap();
    qDebug() << "[Viewport3D] Mesh data set with" << m_meshData.size() << "entries";
}

QVariant Viewport3D::getMeshData() const { return m_meshData; }

void Viewport3D::createMesh(const QString& name, const QVariant& vertices, const QVariant& indices) {
    MeshData md;
    md.vertices = vertices.toList();
    md.indices = indices.toList();
    m_meshes[name] = md;
    qDebug() << "[Viewport3D] Created mesh:" << name;
    emit meshCreated(name);
}

void Viewport3D::updateMesh(const QString& name, const QVariant& vertices, const QVariant& indices) {
    if (m_meshes.contains(name)) {
        MeshData& md = m_meshes[name];
        md.vertices = vertices.toList();
        md.indices = indices.toList();
    } else {
        createMesh(name, vertices, indices);
    }
}
void Viewport3D::destroyMesh(const QString& name) {
    m_meshes.remove(name);
    m_meshTransforms.remove(name);
    qDebug() << "[Viewport3D] Destroyed mesh:" << name;
    emit meshDestroyed(name);
}
void Viewport3D::setMeshTransform(const QString& name, const QVariant& transform) {
    m_meshTransforms[name] = transform;
    emit meshTransformChanged(name);
}

QVariant Viewport3D::getMeshTransform(const QString& name) const {
    if (m_meshTransforms.contains(name)) {
        return m_meshTransforms[name];
    }
    QVariantMap map;
    map["position"] = QVariantList() << 0 << 0 << 0;
    map["rotation"] = QVariantList() << 0 << 0 << 0;
    map["scale"] = QVariantList() << 1 << 1 << 1;
    return map;
}

void Viewport3D::clearMeshes() {
    m_meshData.clear();
    m_meshTransforms.clear();
    qDebug() << "[Viewport3D] Cleared all meshes";
}

void Viewport3D::setRenderSettings(const QVariant& settings) {
    QVariantMap s = settings.toMap();
    if (s.contains("displayMode")) m_displayMode = s["displayMode"].toString();
    if (s.contains("shadingMode")) m_shadingMode = s["shadingMode"].toString();
    if (s.contains("showGrid")) m_showGrid = s["showGrid"].toBool();
    if (s.contains("showAxes")) m_showAxes = s["showAxes"].toBool();
    if (s.contains("showWireframe")) m_showWireframe = s["showWireframe"].toBool();
    if (s.contains("showLighting")) m_showLighting = s["showLighting"].toBool();
    emit renderSettingsChanged();
}

void Viewport3D::takeScreenshot(const QString& path) {
    QImage img = getViewportImage();
    if (!img.isNull()) {
        img.save(path);
        qDebug() << "[Viewport3D] Screenshot saved to:" << path;
    }
    emit screenshotTaken(path);
}
QImage Viewport3D::getViewportImage() {
    QImage img(1920, 1080, QImage::Format_ARGB32);
    img.fill(m_backgroundColor);
    QPainter p(&img);
    // Render grid
    if (m_showGrid) {
        p.setPen(QPen(QColor(60, 60, 70), 1));
        for (int i = 0; i <= 20; ++i) {
            float t = (float)i / 20.0f;
            int x = (int)(t * 1920);
            int y = (int)(t * 1080);
            p.drawLine(x, 0, x, 1080);
            p.drawLine(0, y, 1920, y);
        }
    }
    p.end();
    return img;
}

QVector3D Viewport3D::screenToWorld(int x, int y, float depth)
{
    float fov = 45.0f;
    float aspect = 16.0f / 9.0f;
    float nearP = 0.1f;
    float farP = 1000.0f;

    QMatrix4x4 projection;
    projection.perspective(fov, aspect, nearP, farP);

    // Use actual camera state
    float yawRad = qDegreesToRadians(m_cameraYaw);
    float pitchRad = qDegreesToRadians(m_cameraPitch);
    QVector3D eye(
        m_cameraTarget.x() + m_cameraDistance * qCos(pitchRad) * qSin(yawRad),
        m_cameraTarget.y() + m_cameraDistance * qSin(pitchRad),
        m_cameraTarget.z() + m_cameraDistance * qCos(pitchRad) * qCos(yawRad)
    );
    QMatrix4x4 view;
    view.lookAt(eye, m_cameraTarget, QVector3D(0, 1, 0));

    float z = (depth > 0.0f) ? depth : 0.5f;

    // Use last known viewport size; fall back to 1920x1080
    int vpW = m_viewportWidth > 0 ? m_viewportWidth : 1920;
    int vpH = m_viewportHeight > 0 ? m_viewportHeight : 1080;
    float ndcX = (2.0f * x / vpW) - 1.0f;
    float ndcY = 1.0f - (2.0f * y / vpH);
    float ndcZ = 2.0f * z - 1.0f;

    QVector4D clipCoords(ndcX, ndcY, ndcZ, 1.0f);
    QMatrix4x4 invVP = (projection * view).inverted();
    QVector4D world = invVP * clipCoords;

    if (qFuzzyIsNull(world.w())) return QVector3D();
    return world.toVector3D();
}

QPoint Viewport3D::worldToScreen(const QVector3D& pos)
{
    float fov = 45.0f;
    float aspect = 16.0f / 9.0f;
    float nearP = 0.1f;
    float farP = 1000.0f;

    QMatrix4x4 projection;
    projection.perspective(fov, aspect, nearP, farP);

    // Use actual camera state
    float yawRad = qDegreesToRadians(m_cameraYaw);
    float pitchRad = qDegreesToRadians(m_cameraPitch);
    QVector3D eye(
        m_cameraTarget.x() + m_cameraDistance * qCos(pitchRad) * qSin(yawRad),
        m_cameraTarget.y() + m_cameraDistance * qSin(pitchRad),
        m_cameraTarget.z() + m_cameraDistance * qCos(pitchRad) * qCos(yawRad)
    );
    QMatrix4x4 view;
    view.lookAt(eye, m_cameraTarget, QVector3D(0, 1, 0));

    QVector4D clipPos = projection * view * QVector4D(pos, 1.0f);
    if (qFuzzyIsNull(clipPos.w())) return QPoint();

    QVector3D ndc = clipPos.toVector3D() / clipPos.w();
    int vpW = m_viewportWidth > 0 ? m_viewportWidth : 1920;
    int vpH = m_viewportHeight > 0 ? m_viewportHeight : 1080;
    int screenX = static_cast<int>((ndc.x() + 1.0f) * 0.5f * vpW);
    int screenY = static_cast<int>((1.0f - ndc.y()) * 0.5f * vpH);
    return QPoint(screenX, screenY);
}

float Viewport3D::getDepthAt(int x, int y)
{
    QVariantMap pick = rayPick(x, y);
    if (pick.value("valid").toBool()) {
        float dist = pick.value("distance").toFloat();
        float farP = 1000.0f;
        float nearP = 0.1f;
        float depth = qBound(0.0f, (dist - nearP) / (farP - nearP), 1.0f);
        return depth;
    }
    return 1.0f;
}

void Viewport3D::setSelection(const QStringList& objectIds)
{
    m_selectedObjects = objectIds;
    emit selectionChanged(m_selectedObjects);
}

QStringList Viewport3D::getSelection() const
{
    return m_selectedObjects;
}

void Viewport3D::clearSelection()
{
    m_selectedObjects.clear();
    emit selectionChanged(m_selectedObjects);
}

QVariant Viewport3D::getRenderSettings() const
{
    QVariantMap s;
    s["displayMode"] = m_displayMode;
    s["shadingMode"] = m_shadingMode;
    s["showGrid"] = m_showGrid;
    s["showAxes"] = m_showAxes;
    s["showWireframe"] = m_showWireframe;
    s["showLighting"] = m_showLighting;
    return s;
}

// ============================================================================
// Ray helpers
// ============================================================================

struct Ray {
    QVector3D origin;
    QVector3D direction;
};

static Ray cameraRayFromScreen(int x, int y,
                                float cameraYaw, float cameraPitch,
                                float cameraDistance, const QVector3D& cameraTarget,
                                int vpW, int vpH)
{
    float yawRad = qDegreesToRadians(cameraYaw);
    float pitchRad = qDegreesToRadians(cameraPitch);
    QVector3D eye(
        cameraTarget.x() + cameraDistance * qCos(pitchRad) * qSin(yawRad),
        cameraTarget.y() + cameraDistance * qSin(pitchRad),
        cameraTarget.z() + cameraDistance * qCos(pitchRad) * qCos(yawRad)
    );

    QMatrix4x4 projection;
    projection.perspective(45.0f, float(vpW) / float(vpH), 0.1f, 1000.0f);

    QMatrix4x4 view;
    view.lookAt(eye, cameraTarget, QVector3D(0, 1, 0));

    float ndcX = (2.0f * x / vpW) - 1.0f;
    float ndcY = 1.0f - (2.0f * y / vpH);

    QVector4D nearPoint(ndcX, ndcY, -1.0f, 1.0f);
    QVector4D farPoint(ndcX, ndcY, 1.0f, 1.0f);

    QMatrix4x4 invVP = (projection * view).inverted();
    QVector4D nearWorld = invVP * nearPoint;
    QVector4D farWorld = invVP * farPoint;

    if (!qFuzzyIsNull(nearWorld.w())) nearWorld /= nearWorld.w();
    if (!qFuzzyIsNull(farWorld.w())) farWorld /= farWorld.w();

    Ray ray;
    ray.origin = nearWorld.toVector3D();
    QVector3D farPos = farWorld.toVector3D();
    ray.direction = (farPos - ray.origin).normalized();
    return ray;
}

static bool rayIntersectsAABB(const Ray& ray, const QVector3D& bmin, const QVector3D& bmax)
{
    float tmin = 0.0f, tmax = 100000.0f;

    for (int i = 0; i < 3; ++i) {
        float invD = 1.0f / ray.direction[i];
        float t0 = (bmin[i] - ray.origin[i]) * invD;
        float t1 = (bmax[i] - ray.origin[i]) * invD;
        if (invD < 0.0f) qSwap(t0, t1);
        tmin = qMax(tmin, t0);
        tmax = qMin(tmax, t1);
        if (tmax < tmin) return false;
    }
    return true;
}

static bool rayTriangleIntersect(const Ray& ray,
                                  const QVector3D& v0, const QVector3D& v1, const QVector3D& v2,
                                  float& outT, float& outU, float& outV)
{
    const float EPSILON = 1e-8f;
    QVector3D edge1 = v1 - v0;
    QVector3D edge2 = v2 - v0;
    QVector3D h = QVector3D::crossProduct(ray.direction, edge2);
    float a = QVector3D::dotProduct(edge1, h);
    if (qAbs(a) < EPSILON) return false;
    float f = 1.0f / a;
    QVector3D s = ray.origin - v0;
    float u = f * QVector3D::dotProduct(s, h);
    if (u < 0.0f || u > 1.0f) return false;
    QVector3D q = QVector3D::crossProduct(s, edge1);
    float v = f * QVector3D::dotProduct(ray.direction, q);
    if (v < 0.0f || u + v > 1.0f) return false;
    float t = f * QVector3D::dotProduct(edge2, q);
    if (t > EPSILON) {
        outT = t;
        outU = u;
        outV = v;
        return true;
    }
    return false;
}

static QVector3D applyTransformPosition(const QVariantMap& tf, const QVector3D& p)
{
    QVector3D pos, rot, scale(1, 1, 1);
    if (tf.contains("position")) {
        QVariantList pl = tf["position"].toList();
        if (pl.size() >= 3) pos = QVector3D(pl[0].toFloat(), pl[1].toFloat(), pl[2].toFloat());
    }
    if (tf.contains("rotation")) {
        QVariantList rl = tf["rotation"].toList();
        if (rl.size() >= 3) rot = QVector3D(rl[0].toFloat(), rl[1].toFloat(), rl[2].toFloat());
    }
    if (tf.contains("scale")) {
        QVariantList sl = tf["scale"].toList();
        if (sl.size() >= 3) scale = QVector3D(sl[0].toFloat(), sl[1].toFloat(), sl[2].toFloat());
    }

    if (rot.lengthSquared() == 0.0f && scale.x() == 1.0f && scale.y() == 1.0f && scale.z() == 1.0f)
        return p + pos;

    // Full transform
    QMatrix4x4 m;
    m.translate(pos);
    m.rotate(rot.x(), 1, 0, 0);
    m.rotate(rot.y(), 0, 1, 0);
    m.rotate(rot.z(), 0, 0, 1);
    m.scale(scale);
    return m * p;
}

static bool meshIsWithinRay(const Ray& ray, const QVariantList& verts, const QVariantList& idxs,
                             const QVariantMap& transform,
                             float& outDist, QVector3D& outNormal)
{
    if (idxs.size() < 3 || verts.size() < 3) return false;

    // Compute AABB
    QVector3D bmin(FLT_MAX, FLT_MAX, FLT_MAX);
    QVector3D bmax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (const auto& v : verts) {
        QVector3D p = applyTransformPosition(transform, v.value<QVector3D>());
        bmin.setX(qMin(bmin.x(), p.x()));
        bmin.setY(qMin(bmin.y(), p.y()));
        bmin.setZ(qMin(bmin.z(), p.z()));
        bmax.setX(qMax(bmax.x(), p.x()));
        bmax.setY(qMax(bmax.y(), p.y()));
        bmax.setZ(qMax(bmax.z(), p.z()));
    }

    if (!rayIntersectsAABB(ray, bmin, bmax))
        return false;

    float closest = FLT_MAX;
    QVector3D closestNormal;
    bool found = false;

    for (int i = 0; i + 2 < idxs.size(); i += 3) {
        bool ok0, ok1, ok2;
        int i0 = idxs[i].toInt(&ok0);
        int i1 = idxs[i+1].toInt(&ok1);
        int i2 = idxs[i+2].toInt(&ok2);
        if (!ok0 || !ok1 || !ok2) continue;
        if (i0 >= verts.size() || i1 >= verts.size() || i2 >= verts.size()) continue;

        QVector3D v0 = applyTransformPosition(transform, verts[i0].value<QVector3D>());
        QVector3D v1 = applyTransformPosition(transform, verts[i1].value<QVector3D>());
        QVector3D v2 = applyTransformPosition(transform, verts[i2].value<QVector3D>());

        float t, u, v;
        if (rayTriangleIntersect(ray, v0, v1, v2, t, u, v)) {
            if (t < closest) {
                closest = t;
                // Barycentric normal
                QVector3D n = QVector3D::crossProduct(v1 - v0, v2 - v0).normalized();
                closestNormal = n;
                found = true;
            }
        }
    }

    if (found) {
        outDist = closest;
        outNormal = closestNormal;
        return true;
    }
    return false;
}

QVariantMap Viewport3D::rayPick(int x, int y)
{
    QVariantMap result;
    result["valid"] = false;

    int vpW = m_viewportWidth > 0 ? m_viewportWidth : 1920;
    int vpH = m_viewportHeight > 0 ? m_viewportHeight : 1080;
    if (x < 0 || x >= vpW || y < 0 || y >= vpH)
        return result;

    Ray ray = cameraRayFromScreen(x, y,
                                   m_cameraYaw, m_cameraPitch,
                                   m_cameraDistance, m_cameraTarget,
                                   vpW, vpH);

    QString closestName;
    float closestDist = FLT_MAX;
    QVector3D closestHit;
    QVector3D closestNormal;

    for (auto it = m_meshes.constBegin(); it != m_meshes.constEnd(); ++it) {
        QVariantMap tf;
        if (m_meshTransforms.contains(it.key()))
            tf = m_meshTransforms[it.key()].toMap();

        float dist;
        QVector3D normal;
        if (meshIsWithinRay(ray, it->vertices, it->indices, tf, dist, normal)) {
            if (dist < closestDist) {
                closestDist = dist;
                closestName = it.key();
                closestHit = ray.origin + ray.direction * dist;
                closestNormal = normal;
            }
        }
    }

    if (!closestName.isEmpty()) {
        result["valid"] = true;
        result["objectId"] = closestName;
        result["distance"] = closestDist;
        result["hitPoint"] = QVariantList{closestHit.x(), closestHit.y(), closestHit.z()};
        result["hitNormal"] = QVariantList{closestNormal.x(), closestNormal.y(), closestNormal.z()};
        result["rayOrigin"] = QVariantList{ray.origin.x(), ray.origin.y(), ray.origin.z()};
        result["rayDirection"] = QVariantList{ray.direction.x(), ray.direction.y(), ray.direction.z()};
    }

    return result;
}

QStringList Viewport3D::rayPickAll(int x, int y)
{
    int vpW = m_viewportWidth > 0 ? m_viewportWidth : 1920;
    int vpH = m_viewportHeight > 0 ? m_viewportHeight : 1080;
    if (x < 0 || x >= vpW || y < 0 || y >= vpH)
        return {};

    Ray ray = cameraRayFromScreen(x, y,
                                   m_cameraYaw, m_cameraPitch,
                                   m_cameraDistance, m_cameraTarget,
                                   vpW, vpH);

    QMap<float, QString> hits;
    for (auto it = m_meshes.constBegin(); it != m_meshes.constEnd(); ++it) {
        QVariantMap tf;
        if (m_meshTransforms.contains(it.key()))
            tf = m_meshTransforms[it.key()].toMap();

        float dist;
        QVector3D normal;
        if (meshIsWithinRay(ray, it->vertices, it->indices, tf, dist, normal)) {
            hits[dist] = it.key();
        }
    }

    return hits.values();
}

}