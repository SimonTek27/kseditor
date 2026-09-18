#include "TrackViewportWidget.h"
#include "TerrainEngine.h"
#include "RoadBuilder.h"
#include <QOpenGLContext>
#include <QDebug>
#include <cmath>
#include <vector>

namespace ks { namespace track {

// ═══════════════════════════════════════════════════════════════════════════════
// Shader sources
// ═══════════════════════════════════════════════════════════════════════════════

static const char* gridVertSrc = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    uniform mat4 uView;
    uniform mat4 uProj;
    void main() {
        gl_Position = uProj * uView * vec4(aPos, 1.0);
    }
)";

static const char* gridFragSrc = R"(
    #version 330 core
    out vec4 fragColor;
    uniform vec3 uColor;
    uniform float uAlpha;
    void main() {
        fragColor = vec4(uColor, uAlpha);
    }
)";

static const char* terrainVertSrc = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aNormal;
    uniform mat4 uView;
    uniform mat4 uProj;
    uniform vec3 uLightDir;
    out float vBrightness;
    void main() {
        gl_Position = uProj * uView * vec4(aPos, 1.0);
        float diff = max(dot(aNormal, normalize(uLightDir)), 0.15);
        vBrightness = diff;
    }
)";

static const char* terrainFragSrc = R"(
    #version 330 core
    in float vBrightness;
    out vec4 fragColor;
    uniform vec3 uColor;
    uniform bool uWireframe;
    void main() {
        if (uWireframe)
            fragColor = vec4(1.0, 1.0, 1.0, 1.0);
        else
            fragColor = vec4(uColor * vBrightness, 1.0);
    }
)";

static const char* roadVertSrc = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aColor;
    uniform mat4 uView;
    uniform mat4 uProj;
    out vec3 vColor;
    void main() {
        gl_Position = uProj * uView * vec4(aPos, 1.0);
        vColor = aColor;
    }
)";

static const char* roadFragSrc = R"(
    #version 330 core
    in vec3 vColor;
    out vec4 fragColor;
    void main() {
        fragColor = vec4(vColor, 1.0);
    }
)";

static const char* brushVertSrc = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    uniform mat4 uView;
    uniform mat4 uProj;
    uniform mat4 uModel;
    void main() {
        gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
    }
)";

static const char* brushFragSrc = R"(
    #version 330 core
    out vec4 fragColor;
    uniform vec3 uColor;
    void main() {
        fragColor = vec4(uColor, 0.6);
    }
)";

// ═══════════════════════════════════════════════════════════════════════════════
// Constructor / destructor
// ═══════════════════════════════════════════════════════════════════════════════

TrackViewportWidget::TrackViewportWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    m_fpsTimer.start();
    updateMatrices();
    m_proj.setToIdentity();
    m_proj.perspective(m_fov, 1.0f, 0.1f, 10000.0f);
}

TrackViewportWidget::~TrackViewportWidget()
{
    makeCurrent();
    if (m_gridVAO && _glDeleteVertexArrays) _glDeleteVertexArrays(1, &m_gridVAO);
    if (m_gridVBO) glDeleteBuffers(1, &m_gridVBO);
    teardownTerrain();
    teardownRoads();
    doneCurrent();
}

void TrackViewportWidget::setModule(TrackBuilderModule* module)
{
    m_module = module;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Terrain / road mesh rebuild
// ═══════════════════════════════════════════════════════════════════════════════

void TrackViewportWidget::rebuildTerrainMesh()
{
    makeCurrent();
    teardownTerrain();

    if (!m_module || !m_module->terrain()) { doneCurrent(); return; }

    TerrainEngine* engine = m_module->terrain();
    int gw = engine->gridW();
    int gh = engine->gridH();
    float ww = engine->worldW();
    float wh = engine->worldH();

    if (gw < 2 || gh < 2) { doneCurrent(); return; }

    const auto& heightmap = engine->heightmap();
    const auto& normals = engine->normals();

    std::vector<float> verts;
    std::vector<unsigned int> idx;

    verts.reserve((gw * gh) * 6);
    for (int z = 0; z < gh; ++z) {
        for (int x = 0; x < gw; ++x) {
            float wx = (float(x) / (gw - 1) - 0.5f) * ww;
            float wz = (float(z) / (gh - 1) - 0.5f) * wh;
            float h = (z * gw + x) < heightmap.size() ? heightmap[z * gw + x] : 0.f;
            verts.push_back(wx);
            verts.push_back(h);
            verts.push_back(wz);
            QVector3D n = (z * gw + x) < normals.size() ? normals[z * gw + x] : QVector3D(0, 1, 0);
            verts.push_back(n.x());
            verts.push_back(n.y());
            verts.push_back(n.z());
        }
    }

    idx.reserve((gw - 1) * (gh - 1) * 6);
    for (int z = 0; z < gh - 1; ++z) {
        for (int x = 0; x < gw - 1; ++x) {
            int topLeft = z * gw + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * gw + x;
            int bottomRight = bottomLeft + 1;
            idx.push_back(topLeft);
            idx.push_back(bottomLeft);
            idx.push_back(topRight);
            idx.push_back(topRight);
            idx.push_back(bottomLeft);
            idx.push_back(bottomRight);
        }
    }

    _glGenVertexArrays(1, &m_terrainGL.vao);
    glGenBuffers(1, &m_terrainGL.vbo);
    glGenBuffers(1, &m_terrainGL.ebo);

    _glBindVertexArray(m_terrainGL.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_terrainGL.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_terrainGL.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);

    _glBindVertexArray(0);

    m_terrainGL.indexCount = static_cast<int>(idx.size());
    m_terrainGL.vertexCount = static_cast<int>(verts.size() / 6);

    doneCurrent();
    update();
}

void TrackViewportWidget::rebuildRoadMeshes()
{
    makeCurrent();
    teardownRoads();

    if (!m_module) { doneCurrent(); return; }

    for (const auto& road : m_module->project().roads) {
        RoadMesh mesh = m_module->roadBuilder()->buildRoad(road);
        if (mesh.isEmpty()) continue;

        RoadGL gl;
        std::vector<float> verts;
        verts.reserve(mesh.vertices.size() * 6);
        for (const auto& v : mesh.vertices) {
            verts.push_back(v.position.x());
            verts.push_back(v.position.y());
            verts.push_back(v.position.z());
            // Color by surface type
            float r = 0.3f, g = 0.3f, b = 0.3f;
            switch (road.surface) {
                case SurfaceType::Asphalt:  r=0.25f; g=0.25f; b=0.27f; break;
                case SurfaceType::Concrete: r=0.5f;  g=0.5f;  b=0.5f;  break;
                case SurfaceType::Gravel:   r=0.5f;  g=0.35f; b=0.2f;  break;
                case SurfaceType::Dirt:     r=0.4f;  g=0.3f;  b=0.15f; break;
                case SurfaceType::Grass:    r=0.2f;  g=0.5f;  b=0.15f; break;
                case SurfaceType::Sand:     r=0.7f;  g=0.6f;  b=0.3f;  break;
                case SurfaceType::Ice:      r=0.7f;  g=0.8f;  b=0.9f;  break;
            }
            verts.push_back(r);
            verts.push_back(g);
            verts.push_back(b);
        }

        _glGenVertexArrays(1, &gl.vao);
        glGenBuffers(1, &gl.vbo);
        glGenBuffers(1, &gl.ebo);

        _glBindVertexArray(gl.vao);
        glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                              reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(int),
                     mesh.indices.constData(), GL_STATIC_DRAW);

        _glBindVertexArray(0);

        gl.indexCount = mesh.indices.size();
        gl.vertexCount = mesh.vertices.size();
        m_roadGL.append(gl);
    }

    doneCurrent();
    update();
}

// ═══════════════════════════════════════════════════════════════════════════════
// OpenGL initialization
// ═══════════════════════════════════════════════════════════════════════════════

static bool linkProgram(QOpenGLShaderProgram& prog,
                         const char* vertSrc, const char* fragSrc)
{
    if (!prog.addShaderFromSourceCode(QOpenGLShader::Vertex, vertSrc)) {
        qWarning() << "Vert shader error:" << prog.log();
        return false;
    }
    if (!prog.addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc)) {
        qWarning() << "Frag shader error:" << prog.log();
        return false;
    }
    return prog.link();
}

void TrackViewportWidget::setupShaders()
{
    linkProgram(m_gridShader, gridVertSrc, gridFragSrc);
    linkProgram(m_terrainShader, terrainVertSrc, terrainFragSrc);
    linkProgram(m_roadShader, roadVertSrc, roadFragSrc);
    linkProgram(m_brushShader, brushVertSrc, brushFragSrc);
}

void TrackViewportWidget::setupGrid()
{
    std::vector<float> vertices;
    int gridDivs = 20;
    float halfSize = 500.f;
    float step = halfSize * 2.0f / gridDivs;

    for (int i = 0; i <= gridDivs; ++i) {
        float pos = -halfSize + i * step;
        vertices.push_back(-halfSize); vertices.push_back(0.f); vertices.push_back(pos);
        vertices.push_back(halfSize);  vertices.push_back(0.f); vertices.push_back(pos);
        vertices.push_back(pos);  vertices.push_back(0.f); vertices.push_back(-halfSize);
        vertices.push_back(pos);  vertices.push_back(0.f); vertices.push_back(halfSize);
    }

    m_gridVertexCount = static_cast<int>(vertices.size() / 3);

    _glGenVertexArrays(1, &m_gridVAO);
    glGenBuffers(1, &m_gridVBO);
    _glBindVertexArray(m_gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    _glBindVertexArray(0);
}

void TrackViewportWidget::teardownTerrain()
{
    if (m_terrainGL.vao && _glDeleteVertexArrays)
        _glDeleteVertexArrays(1, &m_terrainGL.vao);
    if (m_terrainGL.vbo) glDeleteBuffers(1, &m_terrainGL.vbo);
    if (m_terrainGL.ebo) glDeleteBuffers(1, &m_terrainGL.ebo);
    m_terrainGL = {};
}

void TrackViewportWidget::teardownRoads()
{
    if (m_roadGL.isEmpty()) return;
    for (const auto& r : m_roadGL) {
        if (r.vao && _glDeleteVertexArrays) _glDeleteVertexArrays(1, &r.vao);
        if (r.vbo) glDeleteBuffers(1, &r.vbo);
        if (r.ebo) glDeleteBuffers(1, &r.ebo);
    }
    m_roadGL.clear();
}

// ═══════════════════════════════════════════════════════════════════════════════
// GL callbacks
// ═══════════════════════════════════════════════════════════════════════════════

void TrackViewportWidget::initializeGL()
{
    initializeOpenGLFunctions();

    auto ctx = QOpenGLContext::currentContext();
    _glGenVertexArrays = reinterpret_cast<pfn_glGenVertexArrays>(
        ctx->getProcAddress("glGenVertexArrays"));
    _glBindVertexArray = reinterpret_cast<pfn_glBindVertexArray>(
        ctx->getProcAddress("glBindVertexArray"));
    _glDeleteVertexArrays = reinterpret_cast<pfn_glDeleteVertexArrays>(
        ctx->getProcAddress("glDeleteVertexArrays"));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.12f, 0.12f, 0.13f, 1.0f);

    setupShaders();
    setupGrid();
    if (m_module && m_module->terrain()) {
        rebuildTerrainMesh();
        rebuildRoadMeshes();
    }
}

void TrackViewportWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    m_proj.setToIdentity();
    m_proj.perspective(m_fov, float(w) / float(std::max(h, 1)), 0.1f, 10000.0f);
}

void TrackViewportWidget::paintGL()
{
    updateMatrices();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_gridVisible) drawGrid(m_view, m_proj);
    drawAxes(m_view, m_proj);
    drawTerrain(m_view, m_proj);
    drawRoads(m_view, m_proj);
    if (m_brushVisible) drawBrush(m_view, m_proj);

    m_triCount = (m_terrainGL.indexCount / 3) + static_cast<int>(m_roadGL.size()) * 2;
    m_frameCount++;
    if (m_fpsTimer.elapsed() >= 1000) {
        m_fps = m_frameCount;
        m_frameCount = 0;
        m_fpsTimer.restart();
        emit frameRendered(m_triCount, m_fps);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Draw helpers
// ═══════════════════════════════════════════════════════════════════════════════

void TrackViewportWidget::drawGrid(const QMatrix4x4& view, const QMatrix4x4& proj)
{
    m_gridShader.bind();
    m_gridShader.setUniformValue("uView", view);
    m_gridShader.setUniformValue("uProj", proj);

    _glBindVertexArray(m_gridVAO);

    m_gridShader.setUniformValue("uColor", QVector3D(0.3f, 0.3f, 0.35f));
    m_gridShader.setUniformValue("uAlpha", 0.3f);
    glLineWidth(1.0f);
    for (int i = 0; i < m_gridVertexCount; i += 2) {
        glDrawArrays(GL_LINES, i, 2);
    }

    // Centre axes lines
    glLineWidth(2.0f);
    m_gridShader.setUniformValue("uColor", QVector3D(0.5f, 0.5f, 0.6f));
    m_gridShader.setUniformValue("uAlpha", 0.6f);
    int stride = (20 + 1) * 2;
    glDrawArrays(GL_LINES, 0, 2);
    glDrawArrays(GL_LINES, stride, 2);

    glLineWidth(1.0f);
    _glBindVertexArray(0);
    m_gridShader.release();
}

void TrackViewportWidget::drawTerrain(const QMatrix4x4& view, const QMatrix4x4& proj)
{
    if (!m_terrainGL.vao || m_terrainGL.indexCount == 0) return;

    m_terrainShader.bind();
    m_terrainShader.setUniformValue("uView", view);
    m_terrainShader.setUniformValue("uProj", proj);
    m_terrainShader.setUniformValue("uLightDir", QVector3D(0.5f, 0.8f, 0.3f));
    m_terrainShader.setUniformValue("uColor", QVector3D(0.3f, 0.5f, 0.25f));
    m_terrainShader.setUniformValue("uWireframe", false);

    _glBindVertexArray(m_terrainGL.vao);
    glDrawElements(GL_TRIANGLES, m_terrainGL.indexCount, GL_UNSIGNED_INT, nullptr);
    _glBindVertexArray(0);

    m_terrainShader.release();
}

void TrackViewportWidget::drawRoads(const QMatrix4x4& view, const QMatrix4x4& proj)
{
    if (m_roadGL.isEmpty()) return;

    m_roadShader.bind();
    m_roadShader.setUniformValue("uView", view);
    m_roadShader.setUniformValue("uProj", proj);

    for (const auto& road : m_roadGL) {
        _glBindVertexArray(road.vao);
        glDrawElements(GL_TRIANGLES, road.indexCount, GL_UNSIGNED_INT, nullptr);
    }

    _glBindVertexArray(0);
    m_roadShader.release();
}

void TrackViewportWidget::drawBrush(const QMatrix4x4& view, const QMatrix4x4& proj)
{
    if (!m_brushVisible || !_glGenVertexArrays || !_glDeleteVertexArrays) return;

    float h = 0.f;
    if (m_module && m_module->terrain())
        h = m_module->terrain()->getHeightWorld(m_brushWX, m_brushWZ) + 0.5f;

    const int segments = 64;
    std::vector<float> circle;
    circle.reserve(segments * 3);
    float twoPi = 2.f * 3.14159265f;
    for (int i = 0; i < segments; ++i) {
        float angle = float(i) / segments * twoPi;
        circle.push_back(m_brushWX + m_brushRadius * std::cos(angle));
        circle.push_back(h);
        circle.push_back(m_brushWZ + m_brushRadius * std::sin(angle));
    }

    GLuint vao = 0, vbo = 0;
    _glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    _glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, circle.size() * sizeof(float), circle.data(), GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    _glBindVertexArray(0);

    QMatrix4x4 identity;
    m_brushShader.bind();
    m_brushShader.setUniformValue("uView", view);
    m_brushShader.setUniformValue("uProj", proj);
    m_brushShader.setUniformValue("uModel", identity);
    m_brushShader.setUniformValue("uColor", QVector3D(1.0f, 0.3f, 0.1f));

    _glBindVertexArray(vao);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINE_LOOP, 0, segments);
    glLineWidth(1.0f);
    _glBindVertexArray(0);

    m_brushShader.release();

    _glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

void TrackViewportWidget::drawAxes(const QMatrix4x4& view, const QMatrix4x4& proj)
{
    if (!m_axesVisible || !_glGenVertexArrays || !_glDeleteVertexArrays) return;

    m_gridShader.bind();
    m_gridShader.setUniformValue("uView", view);
    m_gridShader.setUniformValue("uProj", proj);

    float len = 2.0f;
    float axisVerts[] = {
        0, 0, 0,  len, 0, 0,
        0, 0, 0,  0, len, 0,
        0, 0, 0,  0, 0, len,
    };

    GLuint axisVAO = 0, axisVBO = 0;
    _glGenVertexArrays(1, &axisVAO);
    glGenBuffers(1, &axisVBO);
    _glBindVertexArray(axisVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisVerts), axisVerts, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    _glBindVertexArray(0);

    glLineWidth(3.0f);
    m_gridShader.setUniformValue("uAlpha", 1.0f);

    _glBindVertexArray(axisVAO);
    m_gridShader.setUniformValue("uColor", QVector3D(1, 0.2f, 0.2f));
    glDrawArrays(GL_LINES, 0, 2);
    m_gridShader.setUniformValue("uColor", QVector3D(0.2f, 1, 0.2f));
    glDrawArrays(GL_LINES, 2, 2);
    m_gridShader.setUniformValue("uColor", QVector3D(0.2f, 0.2f, 1));
    glDrawArrays(GL_LINES, 4, 2);

    glLineWidth(1.0f);
    _glBindVertexArray(0);

    _glDeleteVertexArrays(1, &axisVAO);
    glDeleteBuffers(1, &axisVBO);
    m_gridShader.release();
}

void TrackViewportWidget::updateMatrices()
{
    float yawRad = qDegreesToRadians(m_camYaw);
    float pitchRad = qDegreesToRadians(m_camPitch);
    QVector3D eye(
        m_camTarget.x() + m_camDist * std::cos(pitchRad) * std::sin(yawRad),
        m_camTarget.y() + m_camDist * std::sin(pitchRad),
        m_camTarget.z() + m_camDist * std::cos(pitchRad) * std::cos(yawRad)
    );
    m_view.setToIdentity();
    m_view.lookAt(eye, m_camTarget, QVector3D(0, 1, 0));
}

}} // namespace ks::track
