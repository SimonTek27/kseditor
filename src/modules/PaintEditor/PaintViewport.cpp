#include "PaintViewport.h"
#include "PaintSystem.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileInfo>
#include <QDir>
#include <QColor>
#include <cmath>

namespace ks {
namespace paint {

PaintViewport::PaintViewport(QWidget* parent)
    : QWidget(parent)
    , m_paintEditor(ks::paint::PaintEditor::instance())
{
    setFocusPolicy(Qt::StrongFocus);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(2);

    QHBoxLayout* toolbar = new QHBoxLayout();
    QPushButton* resetBtn = new QPushButton(tr("Reset View"), this);
    resetBtn->setFixedHeight(24);
    connect(resetBtn, &QPushButton::clicked, this, &PaintViewport::resetCamera);
    toolbar->addWidget(resetBtn);
    toolbar->addStretch();
    mainLayout->addLayout(toolbar);

    m_viewport = new Viewport3DWidget(this);
    m_viewport->setMinimumHeight(250);
    m_viewport->setRenderMode(Viewport3DWidget::RenderMode::Lit);
    mainLayout->addWidget(m_viewport, 1);
}

PaintViewport::~PaintViewport()
{
    delete m_sceneRoot;
    delete m_meshRenderer;
}

void PaintViewport::setCarPath(const QString& path)
{
    m_carPath = path;
    m_paintEditor->setCarPath(path);

    QString meshPath;
    QDir dir(path);
    QStringList patterns = {"*.obj", "*.kn5", "*.gltf", "*.glb"};
    for (const auto& entry : dir.entryList(patterns, QDir::Files)) {
        meshPath = dir.absoluteFilePath(entry);
        break;
    }
    if (meshPath.isEmpty()) {
        QDir dataDir(path + "/data");
        if (dataDir.exists()) {
            for (const auto& entry : dataDir.entryList(patterns, QDir::Files)) {
                meshPath = dataDir.absoluteFilePath(entry);
                break;
            }
        }
    }

    if (!meshPath.isEmpty()) {
        loadMesh(meshPath);
    }
}

void PaintViewport::loadMesh(const QString& filePath)
{
    if (!m_meshRenderer) {
        m_meshRenderer = new MeshRenderer(this);
    }

    QString lower = filePath.toLower();
    bool loaded = false;
    if (lower.endsWith(".obj")) {
        loaded = m_meshRenderer->loadFromOBJ(filePath);
    } else if (lower.endsWith(".kn5")) {
        loaded = m_meshRenderer->loadFromKN5(filePath);
    } else if (lower.endsWith(".gltf") || lower.endsWith(".glb")) {
        loaded = m_meshRenderer->loadFromGLTF(filePath);
    }

    if (loaded) {
        convertToScene();
        focusOnModel();
    }
}

void PaintViewport::convertToScene()
{
    delete m_sceneRoot;
    m_sceneRoot = new SceneObject(0, "CarRoot", SceneObject::Type::Node);

    const auto& vertices = m_meshRenderer->getVertices();
    const auto& indices = m_meshRenderer->getIndices();

    if (vertices.isEmpty()) return;

    auto* meshObj = new SceneObject(1, m_meshRenderer->getName(), SceneObject::Type::Mesh);
    auto* sceneMesh = new SceneMesh();
    auto& sceneVerts = sceneMesh->geometry().vertices;
    auto& sceneIndices = sceneMesh->geometry().indices;

    sceneVerts.resize(vertices.size());
    for (int i = 0; i < vertices.size(); ++i) {
        sceneVerts[i].position = QVector3D(
            vertices[i].position.x(),
            vertices[i].position.y(),
            vertices[i].position.z()
        );
        sceneVerts[i].normal = QVector3D(
            vertices[i].normal.x(),
            vertices[i].normal.y(),
            vertices[i].normal.z()
        ).normalized();
        float r = vertices[i].color.x();
        float g = vertices[i].color.y();
        float b = vertices[i].color.z();
        if (r < 0.01f && g < 0.01f && b < 0.01f) {
            r = 0.7f; g = 0.7f; b = 0.7f;
        }
        sceneVerts[i].color = QVector4D(r, g, b, 1.0f);
        sceneVerts[i].uv = QVector2D(
            vertices[i].texCoord.x(),
            vertices[i].texCoord.y()
        );
    }

    sceneIndices.resize(indices.size());
    for (int i = 0; i < indices.size(); ++i)
        sceneIndices[i] = indices[i];

    meshObj->setMesh(sceneMesh);
    meshObj->setBaseColor(QColor(180, 180, 200));
    meshObj->setMetallic(0.3f);
    meshObj->setRoughness(0.6f);

    m_sceneRoot->addChild(meshObj);
    m_sceneRoot->setVisible(true);

    m_viewport->setScene(m_sceneRoot);
    m_viewport->focusOnPoint(QVector3D(0, 0.5f, 0), 5.0f);

    // Apply paint texture if available
    if (!m_paintTexture.isNull()) {
        applyPaintTexture(m_paintTexture);
    }
}

void PaintViewport::applyPaintTexture(const QImage& texture)
{
    m_paintTexture = texture;
    if (!m_sceneRoot) return;

    QImage sample = texture;
    if (sample.isNull()) return;

    // Sample the paint texture at each vertex's UV coordinates
    // and update vertex colors in the scene mesh
    for (auto* child : m_sceneRoot->children()) {
        if (!child->hasMesh() || !child->mesh()) continue;
        auto& verts = child->mesh()->geometry().vertices;
        bool anyChanged = false;

        for (int i = 0; i < verts.size(); ++i) {
            float u = verts[i].uv.x();
            float v = 1.0f - verts[i].uv.y(); // flip V for OpenGL

            // Wrap UVs
            u = u - std::floor(u);
            v = v - std::floor(v);

            int px = static_cast<int>(u * (sample.width() - 1));
            int py = static_cast<int>(v * (sample.height() - 1));
            px = qBound(0, px, sample.width() - 1);
            py = qBound(0, py, sample.height() - 1);

            QColor col = sample.pixelColor(px, py);
            QVector4D oldColor = verts[i].color;
            QVector4D newColor(col.redF(), col.greenF(), col.blueF(), 1.0f);
            if (qAbs(oldColor.x() - newColor.x()) > 0.001f ||
                qAbs(oldColor.y() - newColor.y()) > 0.001f ||
                qAbs(oldColor.z() - newColor.z()) > 0.001f) {
                verts[i].color = newColor;
                anyChanged = true;
            }
        }

        if (anyChanged) {
            // Push updated mesh to the viewport
            m_viewport->setScene(m_sceneRoot);
        }
    }
}

void PaintViewport::resetCamera()
{
    if (m_viewport)
        m_viewport->resetCamera();
}

void PaintViewport::focusOnModel()
{
    if (m_viewport)
        m_viewport->focusOnPoint(QVector3D(0, 0.5f, 0), 5.0f);
}

void PaintViewport::setViewMode(const QString& mode)
{
    m_viewMode = mode;
    if (m_viewport) {
        if (mode == "wireframe")
            m_viewport->setRenderMode(Viewport3DWidget::RenderMode::Wireframe);
        else
            m_viewport->setRenderMode(Viewport3DWidget::RenderMode::Lit);
    }
}

} // namespace paint
} // namespace ks