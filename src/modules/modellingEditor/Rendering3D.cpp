#include "Rendering3D.h"
#include <cmath>
#include <QColor>
#include <QPointF>
#include <QVector4D>
#include <algorithm>

namespace ks {
namespace rendering {

QMatrix4x4 Camera3D::viewMatrix() const {
    QMatrix4x4 view;
    view.lookAt(m_position, m_target, m_up);
    return view;
}

QMatrix4x4 Camera3D::projectionMatrix() const {
    QMatrix4x4 proj;
    proj.perspective(m_fov, 16.0f/9.0f, m_near, m_far);
    return proj;
}

void Camera3D::orbit(const QVector3D& center, float azimuth, float elevation) {
    m_target = center;
    QVector3D dir = m_position - center;
    float dist = dir.length();
    if (dist < 0.001f) dist = 1.0f;

    float theta = atan2f(dir.x(), dir.z()) + azimuth;
    float phi = asinf(qBound(-1.0f, dir.y() / dist, 1.0f)) + elevation;
    phi = qBound(-1.57f, phi, 1.57f);

    m_position = center + QVector3D(
        dist * sinf(theta) * cosf(phi),
        dist * sinf(phi),
        dist * cosf(theta) * cosf(phi)
    );
    emit cameraModified();
}

void Camera3D::pan(float dx, float dy) {
    QVector3D forward = (m_target - m_position).normalized();
    QVector3D right = QVector3D::crossProduct(forward, m_up).normalized();
    QVector3D up = QVector3D::crossProduct(right, forward).normalized();

    float speed = m_position.distanceToPoint(m_target) * 0.005f;
    m_position += right * (-dx * speed) + up * (dy * speed);
    m_target += right * (-dx * speed) + up * (dy * speed);
    emit cameraModified();
}

void Camera3D::zoom(float delta) {
    QVector3D dir = (m_target - m_position);
    float dist = dir.length();
    float newDist = dist * (1.0f - delta * 0.05f);
    newDist = qBound(0.1f, newDist, 1000.0f);
    m_position = m_target - dir.normalized() * newDist;
    emit cameraModified();
}

void RenderEngine::render(int width, int height) {
    emit renderStarted(width, height);
    m_width = width;
    m_height = height;
    m_result = QImage(width, height, QImage::Format_ARGB32);
    m_result.fill(QColor::fromRgbF(m_background.x(), m_background.y(), m_background.z()));

    if (!m_scene || !m_camera) {
        emit renderComplete();
        return;
    }

    QVector<float> depthBuffer(width * height, 1.0f);
    int totalObjects = m_scene->allObjects().size();
    int processed = 0;

    auto toScreen = [&](const QVector4D& clip) -> QPointF {
        return QPointF((clip.x() * 0.5f + 0.5f) * m_width,
                       (-clip.y() * 0.5f + 0.5f) * m_height);
    };

    auto edgeFunc = [](const QPointF& a, const QPointF& b, const QPointF& c) -> float {
        return (c.x() - a.x()) * (b.y() - a.y()) - (c.y() - a.y()) * (b.x() - a.x());
    };

    for (auto* obj : m_scene->allObjects()) {
        if (!obj->mesh || !obj->visible) {
            processed++;
            continue;
        }

        auto verts = obj->mesh->vertices();
        auto norms = obj->mesh->normals();
        auto indices = obj->mesh->indices();
        QMatrix4x4 mvp = m_camera->projectionMatrix() * m_camera->viewMatrix() * obj->transform;

        for (int i = 0; i + 2 < indices.size(); i += 3) {
            QVector<QVector4D> clip(3);
            QVector<QPointF> screen(3);
            bool clipped = false;
            for (int j = 0; j < 3; ++j) {
                QVector4D v(verts[indices[i + j]], 1.0f);
                clip[j] = mvp * v;
                if (clip[j].w() == 0) { clipped = true; break; }
                clip[j] /= clip[j].w();
                screen[j] = toScreen(clip[j]);
            }
            if (clipped) continue;

            float area = edgeFunc(screen[0], screen[1], screen[2]);
            if (area <= 0) continue;

            int minX = qMax(0, (int)std::min({screen[0].x(), screen[1].x(), screen[2].x()}));
            int maxX = qMin(width - 1, (int)std::max({screen[0].x(), screen[1].x(), screen[2].x()}));
            int minY = qMax(0, (int)std::min({screen[0].y(), screen[1].y(), screen[2].y()}));
            int maxY = qMin(height - 1, (int)std::max({screen[0].y(), screen[1].y(), screen[2].y()}));

            float invArea = 1.0f / area;
            QVector3D lightDir(0.0f, 0.5f, 1.0f);
            lightDir.normalize();

            for (int y = minY; y <= maxY; ++y) {
                for (int x = minX; x <= maxX; ++x) {
                    QPointF p(x + 0.5f, y + 0.5f);
                    float w0 = edgeFunc(screen[1], screen[2], p);
                    float w1 = edgeFunc(screen[2], screen[0], p);
                    float w2 = edgeFunc(screen[0], screen[1], p);
                    if (w0 < 0 || w1 < 0 || w2 < 0) continue;

                    float depth = clip[0].z() * w0 + clip[1].z() * w1 + clip[2].z() * w2;
                    depth *= invArea;
                    int idx = y * width + x;
                    if (depth >= depthBuffer[idx]) continue;
                    depthBuffer[idx] = depth;

                    float b0 = w0 * invArea, b1 = w1 * invArea, b2 = w2 * invArea;
                    QVector3D norm = norms[indices[i]] * b0 +
                                     norms[indices[i + 1]] * b1 +
                                     norms[indices[i + 2]] * b2;
                    norm.normalize();
                    float ndotl = qMax(0.15f, QVector3D::dotProduct(norm, lightDir));

                    QVector4D col(1.0f, 1.0f, 1.0f, 1.0f);
                    col.setX(qMin(1.0f, col.x() * ndotl));
                    col.setY(qMin(1.0f, col.y() * ndotl));
                    col.setZ(qMin(1.0f, col.z() * ndotl));

                    m_result.setPixelColor(x, y, QColor::fromRgbF(col.x(), col.y(), col.z(), col.w()));
                }
            }
        }

        processed++;
        emit renderProgress(processed * 100 / totalObjects);
    }

    emit renderComplete();
}

void Shader3D::addNode(const QString& nodeId, const QString& nodeType) {
    m_nodes[nodeId] = nodeType;
    emit shaderModified();
}

void Shader3D::removeNode(const QString& nodeId) {
    m_nodes.remove(nodeId);
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
            [&](const Connection& c) {
                return c.fromNode == nodeId || c.toNode == nodeId;
            }),
        m_connections.end());
    emit shaderModified();
}

void Shader3D::connectNodes(const QString& fromNode, const QString& toNode, const QString& fromSocket, const QString& toSocket) {
    Connection conn;
    conn.fromNode = fromNode;
    conn.toNode = toNode;
    conn.fromSocket = fromSocket;
    conn.toSocket = toSocket;
    m_connections.append(conn);
    emit shaderModified();
}

void Shader3D::disconnectNodes(const QString& fromNode, const QString& toNode, const QString& fromSocket, const QString& toSocket) {
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
            [&](const Connection& c) {
                return c.fromNode == fromNode && c.toNode == toNode &&
                       c.fromSocket == fromSocket && c.toSocket == toSocket;
            }),
        m_connections.end());
    emit shaderModified();
}

QString Shader3D::compile() const {
    QString glsl;
    glsl += "#version 450\n\n";

    if (m_type == Shader3D::PBR) {
        glsl += "layout(location = 0) in vec3 inPosition;\n";
        glsl += "layout(location = 1) in vec3 inNormal;\n";
        glsl += "layout(location = 2) in vec2 inUV;\n";
        glsl += "layout(location = 0) out vec4 outColor;\n\n";
        glsl += "void main() {\n";
        glsl += "    vec3 baseColor = vec3(" +
            QString::number(m_bsdf.baseColor[0]) + ", " +
            QString::number(m_bsdf.baseColor[1]) + ", " +
            QString::number(m_bsdf.baseColor[2]) + ");\n";
        glsl += "    float metallic = " + QString::number(m_bsdf.metallic) + ";\n";
        glsl += "    float roughness = " + QString::number(m_bsdf.roughness) + ";\n";
        glsl += "    float ao = 1.0;\n";
        glsl += "    outColor = vec4(baseColor, " + QString::number(m_bsdf.alpha) + ");\n";
        glsl += "}\n";
    } else {
        glsl += "void main() {\n";
        glsl += "    outColor = vec4(1.0);\n";
        glsl += "}\n";
    }

    return glsl;
}

} // namespace rendering
} // namespace ks
