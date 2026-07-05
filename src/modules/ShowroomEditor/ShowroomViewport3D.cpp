#include "ShowroomViewport3D.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QDebug>

namespace ks {

ShowroomViewport3D::ShowroomViewport3D(QWidget* parent)
    : QWidget(parent)
{
    m_meshRenderer = new MeshRenderer(this);
    buildUI();

    connect(m_meshRenderer, &MeshRenderer::meshLoaded,
            this, &ShowroomViewport3D::onMeshLoaded);
    connect(m_meshRenderer, &MeshRenderer::loadError,
            this, &ShowroomViewport3D::onMeshLoadError);
}

ShowroomViewport3D::~ShowroomViewport3D() {
    delete m_sceneRoot;
}

void ShowroomViewport3D::buildUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    buildControls();

    m_viewport = new Viewport3DWidget(this);
    m_viewport->setMinimumHeight(300);
    layout->addWidget(m_viewport, 1);

    m_statusLabel = new QLabel("No car loaded — use Open to load a 3D model", this);
    m_statusLabel->setStyleSheet("color: #888; padding: 4px;");
    layout->addWidget(m_statusLabel);
}

void ShowroomViewport3D::buildControls() {
    auto* controls = new QHBoxLayout;

    m_openBtn = new QPushButton("Open 3D Model...", this);
    m_resetViewBtn = new QPushButton("Reset View", this);

    m_renderModeCombo = new QComboBox(this);
    m_renderModeCombo->addItems({"Solid", "Wireframe"});
    m_renderModeCombo->setCurrentIndex(0);

    m_cameraModeCombo = new QComboBox(this);
    m_cameraModeCombo->addItems({"Perspective", "Top", "Front", "Right"});
    m_cameraModeCombo->setCurrentIndex(0);

    m_showGridCheck = new QCheckBox("Grid", this);
    m_showGridCheck->setChecked(true);
    m_showAxesCheck = new QCheckBox("Axes", this);
    m_showAxesCheck->setChecked(true);

    m_infoLabel = new QLabel("", this);
    m_infoLabel->setStyleSheet("color: #666;");

    controls->addWidget(m_openBtn);
    controls->addWidget(m_resetViewBtn);
    controls->addSpacing(8);
    controls->addWidget(new QLabel("Render:", this));
    controls->addWidget(m_renderModeCombo);
    controls->addWidget(new QLabel("Camera:", this));
    controls->addWidget(m_cameraModeCombo);
    controls->addSpacing(8);
    controls->addWidget(m_showGridCheck);
    controls->addWidget(m_showAxesCheck);
    controls->addStretch();
    controls->addWidget(m_infoLabel);

    auto* topLayout = qobject_cast<QVBoxLayout*>(layout());
    if (topLayout)
        topLayout->addLayout(controls);

    connect(m_openBtn, &QPushButton::clicked, this, &ShowroomViewport3D::onOpenFile);
    connect(m_resetViewBtn, &QPushButton::clicked, this, &ShowroomViewport3D::onResetView);
    connect(m_renderModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ShowroomViewport3D::onRenderModeChanged);
    connect(m_cameraModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ShowroomViewport3D::onCameraModeChanged);
    connect(m_showGridCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_viewport->setShowGrid(checked);
    });
    connect(m_showAxesCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_viewport->setShowAxes(checked);
    });
}

void ShowroomViewport3D::loadCarMesh(const QString& filePath) {
    if (filePath.isEmpty()) return;

    QString lower = filePath.toLower();
    bool loaded = false;

    if (lower.endsWith(".obj")) {
        loaded = m_meshRenderer->loadFromOBJ(filePath);
    } else if (lower.endsWith(".kn5")) {
        loaded = m_meshRenderer->loadFromKN5(filePath);
    } else if (lower.endsWith(".gltf") || lower.endsWith(".glb")) {
        loaded = m_meshRenderer->loadFromGLTF(filePath);
    } else {
        m_statusLabel->setText("Unsupported format: " + QFileInfo(filePath).suffix());
        return;
    }

    if (loaded) {
        convertToScene();
        QFileInfo fi(filePath);
        m_statusLabel->setText(QString("Loaded: %1 (%2 vertices, %3 faces)")
            .arg(fi.fileName())
            .arg(m_currentVertexCount)
            .arg(m_currentFaceCount));
    }
}

void ShowroomViewport3D::loadCarFromFolder(const QString& carFolder) {
    QDir dir(carFolder);
    if (!dir.exists()) return;

    // Try common car mesh locations
    QStringList patterns = {"*.obj", "*.kn5", "*.gltf", "*.glb"};
    QString found;

    // Check car folder root
    for (const auto& entry : dir.entryList(patterns, QDir::Files)) {
        found = dir.absoluteFilePath(entry);
        break;
    }

    // Check data/ subfolder
    if (found.isEmpty()) {
        QDir dataDir(carFolder + "/data");
        if (dataDir.exists()) {
            for (const auto& entry : dataDir.entryList(patterns, QDir::Files)) {
                found = dataDir.absoluteFilePath(entry);
                break;
            }
        }
    }

    if (!found.isEmpty()) {
        loadCarMesh(found);
    } else {
        m_statusLabel->setText("No 3D model found in " + carFolder);
    }
}

void ShowroomViewport3D::clearCar() {
    delete m_sceneRoot;
    m_sceneRoot = nullptr;
    m_viewport->setScene(nullptr);
    m_currentVertexCount = 0;
    m_currentFaceCount = 0;
    m_infoLabel->clear();
    m_statusLabel->setText("Car cleared");
}

void ShowroomViewport3D::convertToScene() {
    delete m_sceneRoot;
    m_sceneRoot = new SceneObject(0, "CarRoot", SceneObject::Type::Node);

    const auto& vertices = m_meshRenderer->getVertices();
    const auto& indices = m_meshRenderer->getIndices();

    if (vertices.isEmpty()) return;

    auto* meshObj = new SceneObject(1, m_meshRenderer->getName(), SceneObject::Type::Mesh);
    auto* sceneMesh = new SceneMesh();
    auto& sceneVerts = sceneMesh->vertices();
    auto& sceneIndices = sceneMesh->indices();

    sceneVerts.resize(vertices.size());
    for (int i = 0; i < vertices.size(); ++i) {
        sceneVerts[i].position = Vec3(
            vertices[i].position.x(),
            vertices[i].position.y(),
            vertices[i].position.z()
        );
        float brightness = 0.35f + 0.5f * std::abs(vertices[i].normal.z());
        sceneVerts[i].color = Vec3(
            vertices[i].color.x() * brightness,
            vertices[i].color.y() * brightness,
            vertices[i].color.z() * brightness
        );
    }

    sceneIndices.resize(indices.size());
    for (int i = 0; i < indices.size(); ++i)
        sceneIndices[i] = indices[i];

    meshObj->setMesh(sceneMesh);
    m_sceneRoot->addChild(meshObj);
    m_sceneRoot->setVisible(true);

    m_viewport->setScene(m_sceneRoot);
    m_viewport->focusOnPoint(QVector3D(0, 0, 0), 4.0f);

    m_currentVertexCount = vertices.size();
    m_currentFaceCount = indices.size() / 3;

    m_infoLabel->setText(QString("%1V %2F")
        .arg(m_currentVertexCount)
        .arg(m_currentFaceCount));
}

void ShowroomViewport3D::syncConfig(const ShowroomSystem::ShowroomConfig& config) {
    m_config = config;
    syncCamera(config.cameraDistance, config.cameraHeight,
               config.cameraAngle, config.cameraFov);
    syncLight(config.sunColor, config.sunIntensity,
              config.ambientColor, config.ambientIntensity);
}

void ShowroomViewport3D::syncCamera(double distance, double height,
                                     double angle, double fov) {
    if (!m_viewport) return;
    double rad = angle * 3.14159265 / 180.0;
    QVector3D pos(
        distance * std::cos(rad),
        height,
        distance * std::sin(rad)
    );
    Q_UNUSED(pos);
    m_viewport->focusOnPoint(QVector3D(0, 0, 0), distance);
}

void ShowroomViewport3D::syncLight(const QColor&, double, const QColor&, double) {
    applyColorsFromConfig();
}

void ShowroomViewport3D::applyColorsFromConfig() {
    if (!m_sceneRoot) return;

    float ambientR = (float)m_config.ambientColor.redF();
    float ambientG = (float)m_config.ambientColor.greenF();
    float ambientB = (float)m_config.ambientColor.blueF();
    float sunR = (float)m_config.sunColor.redF();
    float sunG = (float)m_config.sunColor.greenF();
    float sunB = (float)m_config.sunColor.blueF();

    for (auto* child : m_sceneRoot->children()) {
        if (child->hasMesh() && child->mesh()) {
            auto& verts = child->mesh()->vertices();
            for (auto& v : verts) {
                v.color = Vec3(
                    sunR * m_config.sunIntensity + ambientR * m_config.ambientIntensity,
                    sunG * m_config.sunIntensity + ambientG * m_config.ambientIntensity,
                    sunB * m_config.sunIntensity + ambientB * m_config.ambientIntensity
                );
            }
        }
    }
}

SceneObject* ShowroomViewport3D::createSceneNode(const QString& name,
    SceneObject::Type type, SceneObject* parent)
{
    static int nextId = 1;
    auto* obj = new SceneObject(nextId++, name, type);
    if (parent) parent->addChild(obj);
    return obj;
}

// ── Slots ──

void ShowroomViewport3D::onOpenFile() {
    QString filter = "3D Models (*.obj *.kn5 *.gltf *.glb);;OBJ (*.obj);;KN5 (*.kn5);;glTF (*.gltf *.glb);;All Files (*)";
    QString path = QFileDialog::getOpenFileName(this, "Open Car Model", QString(), filter);
    if (!path.isEmpty())
        loadCarMesh(path);
}

void ShowroomViewport3D::onResetView() {
    if (m_viewport)
        m_viewport->resetCamera();
}

void ShowroomViewport3D::onRenderModeChanged(int index) {
    if (m_viewport)
        m_viewport->setRenderMode(static_cast<Viewport3DWidget::RenderMode>(index));
}

void ShowroomViewport3D::onCameraModeChanged(int index) {
    if (m_viewport)
        m_viewport->setCameraMode(static_cast<Viewport3DWidget::CameraMode>(index));
}

void ShowroomViewport3D::onMeshLoaded(const QString& name, int vertexCount, int faceCount) {
    emit carLoaded(name, vertexCount, faceCount);
}

void ShowroomViewport3D::onMeshLoadError(const QString& error) {
    m_statusLabel->setText("Error: " + error);
    emit loadError(error);
}

} // namespace ks
