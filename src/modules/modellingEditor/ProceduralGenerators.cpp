#include "ProceduralGenerators.h"
#include <cmath>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QColor>
#include <algorithm>

namespace ks {

// ---------------------------------------------------------------------------
// ProceduralTextureGenerator Implementation
// ---------------------------------------------------------------------------
ProceduralTextureGenerator::ProceduralTextureGenerator(QObject* parent) : QObject(parent) {}
ProceduralTextureGenerator::~ProceduralTextureGenerator() {}

float ProceduralTextureGenerator::fbm(float x, float y, int octaves, int seed) {
    float value = 0.0f, amplitude = 0.5f, frequency = 1.0f;
    for (int i = 0; i < octaves; ++i) {
        float nx = x * frequency + seed * 0.01f;
        float ny = y * frequency + seed * 0.01f;
        value += amplitude * (sinf(nx * 12.9898f + ny * 78.233f) * 43758.5453f);
        value -= floorf(value);
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return value;
}

float ProceduralTextureGenerator::turbulence(float x, float y, int octaves, int seed) {
    float value = 0.0f, scale = 1.0f;
    for (int i = 0; i < octaves; ++i) {
        float nx = x * scale + seed * 3.14159f;
        float ny = y * scale + seed * 2.71828f;
        value += fabsf(sinf(nx * 12.9898f + ny * 78.233f));
        scale *= 2.0f;
    }
    return value;
}

QVector3D ProceduralTextureGenerator::marblePattern(float x, float y, int seed, const TextureParams& params) {
    float t = turbulence(x * params.scale, y * params.scale, 6, seed);
    float sine = sinf((x * params.scale + t) * 3.14159f * 4.0f);
    float mix = sine * 0.5f + 0.5f;
    mix = qBound(0.0f, mix, 1.0f);
    return params.color1 * mix + params.color2 * (1.0f - mix);
}

QVector3D ProceduralTextureGenerator::woodPattern(float x, float y, int seed) {
    float rings = sinf(sqrtf(x * x + y * y) * 8.0f + seed * 0.1f) * 0.5f + 0.5f;
    return QVector3D(0.6f + rings * 0.3f, 0.3f + rings * 0.2f, 0.1f + rings * 0.1f);
}

QVector3D ProceduralTextureGenerator::concretePattern(float x, float y, int seed) {
    float n = fbm(x * 4.0f, y * 4.0f, 4, seed);
    float base = 0.5f + n * 0.1f;
    return QVector3D(base, base, base);
}

QVector3D ProceduralTextureGenerator::asphaltPattern(float x, float y, int seed) {
    float n = fbm(x * 8.0f, y * 8.0f, 5, seed);
    float base = 0.25f + n * 0.05f;
    float grain = fmodf(x * 100.0f + y * 100.0f + seed, 1.0f) * 0.02f;
    return QVector3D(base + grain, base + grain, base + grain);
}

QImage ProceduralTextureGenerator::generateTexture(const TextureParams& params) {
    QImage image(params.width, params.height, QImage::Format_RGB32);
    int seed = params.seed != 0 ? params.seed : 12345;

    for (int y = 0; y < params.height; ++y) {
        for (int x = 0; x < params.width; ++x) {
            float fx = (float)x / params.width;
            float fy = (float)y / params.height;
            QVector3D color;

            switch (params.type) {
                case Type_Marble:
                    color = marblePattern(fx, fy, seed, params);
                    break;
                case Type_Wood:
                    color = woodPattern(fx, fy, seed);
                    break;
                case Type_Concrete:
                    color = concretePattern(fx, fy, seed);
                    break;
                case Type_Asphalt:
                    color = asphaltPattern(fx, fy, seed);
                    break;
                case Type_Grass:
                    color = QVector3D(0.2f + fbm(fx * 6, fy * 6, 3, seed) * 0.3f, 0.5f + fbm(fx * 4, fy * 4, 3, seed + 1) * 0.3f, 0.1f);
                    break;
                case Type_Metal:
                    color = QVector3D(0.6f + fbm(fx * 10, fy * 10, 2, seed) * 0.2f, 0.6f + fbm(fx * 10, fy * 10, 2, seed + 1) * 0.2f, 0.6f + fbm(fx * 10, fy * 10, 2, seed + 2) * 0.2f);
                    break;
                case Type_Carbon:
                    color = QVector3D(0.1f + fmodf(fx * 50 + fy * 50 + seed, 1.0f) * 0.05f, 0.1f, 0.1f);
                    break;
                case Type_Plastic:
                    color = QVector3D(0.3f, 0.3f, 0.35f);
                    break;
                case Type_Rust:
                    color = QVector3D(0.5f + turbulence(fx, fy, 3, seed) * 0.3f, 0.2f + turbulence(fx, fy, 3, seed + 1) * 0.15f, 0.1f);
                    break;
                case Type_Grunge: {
                    float n = turbulence(fx, fy, 4, seed);
                    color = QVector3D(n, n, n) * 0.6f;
                    break;
                }
            }

            color = color * params.contrast + QVector3D(params.brightness, params.brightness, params.brightness);
            color.setX(qBound(0.0f, color.x(), 1.0f));
            color.setY(qBound(0.0f, color.y(), 1.0f));
            color.setZ(qBound(0.0f, color.z(), 1.0f));

            image.setPixelColor(x, y, QColor::fromRgbF(color.x(), color.y(), color.z()));
        }
    }

    emit generationComplete(image);
    return image;
}

QImage ProceduralTextureGenerator::generateNormalMap(const QImage& diffuse) {
    QImage normalMap(diffuse.size(), QImage::Format_RGB32);
    float strength = 1.0f;

    for (int y = 0; y < diffuse.height(); ++y) {
        for (int x = 0; x < diffuse.width(); ++x) {
            int left = qMax(0, x - 1);
            int right = qMin(diffuse.width() - 1, x + 1);
            int top = qMax(0, y - 1);
            int bottom = qMin(diffuse.height() - 1, y + 1);

            float hL = qGray(diffuse.pixel(left, y)) / 255.0f;
            float hR = qGray(diffuse.pixel(right, y)) / 255.0f;
            float hT = qGray(diffuse.pixel(x, top)) / 255.0f;
            float hB = qGray(diffuse.pixel(x, bottom)) / 255.0f;

            float dx = (hR - hL) * strength;
            float dy = (hT - hB) * strength;
            float dz = 1.0f / sqrtf(1.0f + dx * dx + dy * dy);

            normalMap.setPixelColor(x, y, QColor::fromRgbF(
                dx * 0.5f + 0.5f,
                dy * 0.5f + 0.5f,
                dz * 0.5f + 0.5f
            ));
        }
    }

    emit generationComplete(normalMap);
    return normalMap;
}

QString ProceduralTextureGenerator::textureTypeToString(TextureType type) {
    switch (type) {
        case Type_Marble: return "Marble";
        case Type_Wood: return "Wood";
        case Type_Concrete: return "Concrete";
        case Type_Asphalt: return "Asphalt";
        case Type_Grass: return "Grass";
        case Type_Metal: return "Metal";
        case Type_Carbon: return "Carbon";
        case Type_Plastic: return "Plastic";
        case Type_Rust: return "Rust";
        case Type_Grunge: return "Grunge";
        default: return "Unknown";
    }
}

ProceduralTextureGenerator::TextureType ProceduralTextureGenerator::stringToTextureType(const QString& str) {
    if (str == "Marble") return Type_Marble;
    if (str == "Wood") return Type_Wood;
    if (str == "Concrete") return Type_Concrete;
    if (str == "Asphalt") return Type_Asphalt;
    if (str == "Grass") return Type_Grass;
    if (str == "Metal") return Type_Metal;
    if (str == "Carbon") return Type_Carbon;
    if (str == "Plastic") return Type_Plastic;
    if (str == "Rust") return Type_Rust;
    if (str == "Grunge") return Type_Grunge;
    return Type_Marble;
}

// ---------------------------------------------------------------------------
// ProceduralMeshGenerator Implementation
// ---------------------------------------------------------------------------
ProceduralMeshGenerator::ProceduralMeshGenerator(QObject* parent) : QObject(parent) {}
ProceduralMeshGenerator::~ProceduralMeshGenerator() {}

void ProceduralMeshGenerator::generateBox(MeshData& mesh, const MeshParams& params) {
    float w = params.width / 2, h = params.height / 2, d = params.depth / 2;
    mesh.vertices = {
        {-w,-h,d}, {w,-h,d}, {w,h,d}, {-w,h,d},
        {-w,-h,-d}, {w,-h,-d}, {w,h,-d}, {-w,h,-d}
    };
    mesh.indices = {
        QVector3D(0,1,2), QVector3D(0,2,3), QVector3D(4,6,5), QVector3D(4,7,6),
        QVector3D(0,4,5), QVector3D(0,5,1), QVector3D(2,6,7), QVector3D(2,7,3),
        QVector3D(0,3,7), QVector3D(0,7,4), QVector3D(1,5,6), QVector3D(1,6,2)
    };
}

void ProceduralMeshGenerator::generateSphere(MeshData& mesh, const MeshParams& params) {
    float r = params.width / 2;
    int segs = qMax(3, params.segments);
    int rngs = qMax(3, params.rings);
    for (int i = 0; i <= rngs; ++i) {
        float phi = 3.14159f * i / rngs;
        for (int j = 0; j <= segs; ++j) {
            float theta = 2.0f * 3.14159f * j / segs;
            mesh.vertices.append(QVector3D(
                r * sinf(phi) * cosf(theta),
                r * cosf(phi),
                r * sinf(phi) * sinf(theta)
            ));
        }
    }
    for (int i = 0; i < rngs; ++i) {
        for (int j = 0; j < segs; ++j) {
            int a = i * (segs + 1) + j;
            int b = a + segs + 1;
            mesh.indices.append(QVector3D(a, b, b + 1));
            mesh.indices.append(QVector3D(a, b + 1, a + 1));
        }
    }
}

void ProceduralMeshGenerator::generateCylinder(MeshData& mesh, const MeshParams& params) {
    float r = params.width / 2, h = params.height / 2;
    int segs = qMax(3, params.segments);

    mesh.vertices.append(QVector3D(0, h, 0));
    mesh.vertices.append(QVector3D(0, -h, 0));

    for (int i = 0; i <= segs; ++i) {
        float a = 2.0f * 3.14159f * i / segs;
        float ca = cosf(a), sa = sinf(a);
        mesh.vertices.append(QVector3D(r * ca, h, r * sa));
        mesh.vertices.append(QVector3D(r * ca, -h, r * sa));
    }

    for (int i = 0; i < segs; ++i) {
        int b1 = 2 + i * 2, b2 = 2 + ((i + 1) % segs) * 2;
        mesh.indices.append(QVector3D(0, b1 + 0, b2 + 0));
        mesh.indices.append(QVector3D(1, b2 + 1, b1 + 1));
        mesh.indices.append(QVector3D(b1 + 0, b1 + 1, b2 + 1));
        mesh.indices.append(QVector3D(b1 + 0, b2 + 1, b2 + 0));
    }
}

void ProceduralMeshGenerator::generateTorus(MeshData& mesh, const MeshParams& params) {
    float majR = params.width / 2, minR = params.height / 4;
    int majS = qMax(3, params.segments), minS = qMax(3, params.rings);

    for (int i = 0; i <= majS; ++i) {
        float u = 2.0f * 3.14159f * i / majS;
        for (int j = 0; j <= minS; ++j) {
            float v = 2.0f * 3.14159f * j / minS;
            mesh.vertices.append(QVector3D(
                (majR + minR * cosf(v)) * cosf(u),
                minR * sinf(v),
                (majR + minR * cosf(v)) * sinf(u)
            ));
        }
    }

    for (int i = 0; i < majS; ++i) {
        for (int j = 0; j < minS; ++j) {
            int a = i * (minS + 1) + j;
            int b = a + minS + 1;
            mesh.indices.append(QVector3D(a, b, b + 1));
            mesh.indices.append(QVector3D(a, b + 1, a + 1));
        }
    }
}

float ProceduralMeshGenerator::heightMap(float x, float z, int seed) {
    float val = sinf(x * 0.1f + seed) * cosf(z * 0.15f + seed * 2);
    val += sinf(x * 0.05f + z * 0.08f + seed * 3) * 0.5f;
    val += sinf(x * 0.02f - z * 0.03f + seed * 5) * 0.25f;
    return val;
}

ProceduralMeshGenerator::MeshData ProceduralMeshGenerator::generateMesh(const MeshParams& params) {
    MeshData mesh;
    switch (params.primitive) {
        case MeshParams::Box: generateBox(mesh, params); break;
        case MeshParams::Sphere: generateSphere(mesh, params); break;
        case MeshParams::Cylinder: generateCylinder(mesh, params); break;
        case MeshParams::Torus: generateTorus(mesh, params); break;
        default: generateBox(mesh, params); break;
    }

    mesh.normals.resize(mesh.vertices.size());
    for (int i = 0; i + 2 < mesh.indices.size(); i += 3) {
        auto f = mesh.indices[i];
        QVector3D v0 = mesh.vertices[(int)f.x()];
        QVector3D v1 = mesh.vertices[(int)f.y()];
        QVector3D v2 = mesh.vertices[(int)f.z()];
        QVector3D n = QVector3D::crossProduct(v1 - v0, v2 - v0).normalized();
        mesh.normals[(int)f.x()] += n;
        mesh.normals[(int)f.y()] += n;
        mesh.normals[(int)f.z()] += n;
    }
    for (auto& n : mesh.normals) n = n.normalized();

    emit generationComplete(mesh);
    return mesh;
}

ProceduralMeshGenerator::MeshData ProceduralMeshGenerator::generateTerrain(int width, int height, float scale, int seed) {
    MeshData mesh;
    for (int z = 0; z <= height; ++z) {
        for (int x = 0; x <= width; ++x) {
            float y = heightMap(x * scale, z * scale, seed) * scale;
            mesh.vertices.append(QVector3D(x - width/2.0f, y, z - height/2.0f));
        }
    }

    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            int a = z * (width + 1) + x;
            int b = a + 1;
            int c = (z + 1) * (width + 1) + x;
            int d = c + 1;
            mesh.indices.append(QVector3D(a, c, b));
            mesh.indices.append(QVector3D(b, c, d));
        }
    }

    emit generationComplete(mesh);
    return mesh;
}

// ---------------------------------------------------------------------------
// ProceduralTrackGenerator Implementation
// ---------------------------------------------------------------------------
ProceduralTrackGenerator::ProceduralTrackGenerator(QObject* parent) : QObject(parent) {}
ProceduralTrackGenerator::~ProceduralTrackGenerator() {}

ProceduralTrackGenerator::TrackPoint ProceduralTrackGenerator::calculateTrackPoint(const QVector<QVector2D>& spline, float t) {
    int n = spline.size();
    float idx = t * (n - 1);
    int i0 = qMax(1, qMin(n - 2, (int)floorf(idx)));
    float frac = idx - i0;

    QVector2D pm = spline[i0 - 1];
    QVector2D p0 = spline[i0];
    QVector2D p1 = spline[qMin(n - 1, i0 + 1)];
    QVector2D p2 = spline[qMin(n - 1, i0 + 2)];

    float t2 = frac * frac, t3 = t2 * frac;
    QVector2D pt = 0.5f * ((2.0f * p0) + (-pm + p1) * frac
        + (2.0f * pm - 5.0f * p0 + 4.0f * p1 - p2) * t2
        + (-pm + 3.0f * p0 - 3.0f * p1 + p2) * t3);

    QVector2D dir = (p1 - pm).normalized();
    float cross = (p0 - pm).x() * (p1 - p0).y() - (p0 - pm).y() * (p1 - p0).x();

    TrackPoint tp;
    tp.position = QVector3D(pt.x(), 0, pt.y());
    tp.tangent = QVector3D(dir.x(), 0, dir.y()).normalized();
    tp.normal = QVector3D::crossProduct(tp.tangent, QVector3D(0, 1, 0)).normalized();
    tp.curvature = cross;
    tp.width = m_trackWidth;
    return tp;
}

ProceduralTrackGenerator::TrackData ProceduralTrackGenerator::generateTrack(const TrackParams& params) {
    TrackData data;
    int numPts = qMax(4, params.numPoints);
    m_trackWidth = params.width;

    QVector<QVector2D> splinePts;
    float angleStep = 2.0f * 3.14159f / numPts;
    for (int i = 0; i < numPts; ++i) {
        float angle = i * angleStep;
        float radius = params.minRadius + fabsf(sinf(angle * 2 + params.seed * 0.1f)) * (params.maxRadius - params.minRadius);
        splinePts.append(QVector2D(radius * cosf(angle), radius * sinf(angle)));
    }
    if (params.closed) splinePts.append(splinePts.first());

    int numTrackPts = 200;
    for (int i = 0; i < numTrackPts; ++i) {
        float t = (float)i / (numTrackPts - 1);
        TrackPoint tp = calculateTrackPoint(splinePts, t);
        data.points.append(tp);
        data.centerLine.append(tp.position);
        data.leftEdge.append(tp.position + tp.normal * tp.width * 0.5f);
        data.rightEdge.append(tp.position - tp.normal * tp.width * 0.5f);
    }

    if (params.includePitLane) {
        int pitStart = numTrackPts / 4;
        int pitEnd = numTrackPts * 3 / 4;
        float pitWidth = qMax(8.0f, params.width * 0.6f);
        for (int i = pitStart; i <= pitEnd; ++i) {
            if (i < data.points.size()) {
                QVector3D offset = data.points[i].normal * params.width * 1.2f;
                data.centerLine[i] += offset;
                data.leftEdge[i] += offset;
                data.rightEdge[i] += offset;
                data.points[i].position += offset;
                data.points[i].width = pitWidth;
            }
        }
    }

    data.totalLength = 0;
    for (int i = 1; i < data.centerLine.size(); ++i)
        data.totalLength += data.centerLine[i].distanceToPoint(data.centerLine[i - 1]);

    emit generationComplete(data);
    return data;
}

ProceduralTrackGenerator::TrackData ProceduralTrackGenerator::addChicanes(const TrackData& baseTrack, int count, float intensity) {
    TrackData data = baseTrack;
    if (count < 1 || data.points.size() < 10) return data;

    int step = data.points.size() / (count + 1);
    for (int c = 0; c < count; ++c) {
        int idx = step * (c + 1);
        if (idx >= data.points.size()) break;
        float offset = (c % 2 == 0) ? intensity : -intensity;
        for (int i = qMax(0, idx - 3); i <= qMin(data.points.size() - 1, idx + 3); ++i) {
            float fade = 1.0f - fabsf(i - idx) / 3.0f;
            data.points[i].position += data.points[i].normal * offset * fade;
            data.centerLine[i] = data.points[i].position;
            data.leftEdge[i] = data.points[i].position + data.points[i].normal * data.points[i].width * 0.5f;
            data.rightEdge[i] = data.points[i].position - data.points[i].normal * data.points[i].width * 0.5f;
        }
    }

    emit generationComplete(data);
    return data;
}

ProceduralTrackGenerator::TrackData ProceduralTrackGenerator::addCrest(const TrackData& baseTrack, float position, float height) {
    TrackData data = baseTrack;
    if (data.points.isEmpty()) return data;

    int idx = qBound(0, (int)(position * data.points.size()), data.points.size() - 1);
    for (int i = qMax(0, idx - 10); i <= qMin(data.points.size() - 1, idx + 10); ++i) {
        float fade = 1.0f - fabsf(i - idx) / 10.0f;
        float h = height * (1.0f - fade * fade);
        data.points[i].position.setY(h);
        data.centerLine[i].setY(h);
        data.leftEdge[i].setY(h);
        data.rightEdge[i].setY(h);
    }

    emit generationComplete(data);
    return data;
}

// ---------------------------------------------------------------------------
// ProceduralCarGenerator Implementation
// ---------------------------------------------------------------------------
ProceduralCarGenerator::ProceduralCarGenerator(QObject* parent) : QObject(parent) {}
ProceduralCarGenerator::~ProceduralCarGenerator() {}

ProceduralCarGenerator::CarModel ProceduralCarGenerator::generateCar(const CarParams& params) {
    CarModel model;
    ProceduralMeshGenerator gen;
    ProceduralMeshGenerator::MeshParams mp;
    mp.width = params.length;
    mp.height = params.height;
    mp.depth = params.width;
    mp.segments = 16 + params.detailLevel * 8;
    mp.rings = 8 + params.detailLevel * 4;

    switch (params.bodyStyle) {
        case CarParams::Sedan: mp.primitive = ProceduralMeshGenerator::MeshParams::Box; break;
        case CarParams::Coupe: mp.primitive = ProceduralMeshGenerator::MeshParams::Box; break;
        case CarParams::SUV: mp.primitive = ProceduralMeshGenerator::MeshParams::Box; break;
        case CarParams::Formula: mp.primitive = ProceduralMeshGenerator::MeshParams::Box; break;
        case CarParams::GT: mp.primitive = ProceduralMeshGenerator::MeshParams::Box; break;
    }

    model.body = gen.generateMesh(mp);

    for (int w = 0; w < 4; ++w) {
        ProceduralMeshGenerator::MeshData wheel;
        float wr = params.wheelRadius;
        float wt = wr * 0.4f;
        int segs = 12 + params.detailLevel * 4;
        for (int i = 0; i <= segs; ++i) {
            float a = 2.0f * 3.14159f * i / segs;
            wheel.vertices.append(QVector3D(0, wr * cosf(a), wr * sinf(a)));
            wheel.vertices.append(QVector3D(wt, wr * cosf(a), wr * sinf(a)));
        }
        model.wheels.append(wheel);
    }

    emit generationComplete(model);
    return model;
}

// ---------------------------------------------------------------------------
// DecalGenerator Implementation
// ---------------------------------------------------------------------------
DecalGenerator::DecalGenerator(QObject* parent) : QObject(parent) {}
DecalGenerator::~DecalGenerator() {}

QImage DecalGenerator::generateDecal(const DecalParams& params) {
    int w = qMax(64, (int)(params.width * 256));
    int h = qMax(64, (int)(params.height * 256));
    QImage image(w, h, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing);

    switch (params.type) {
        case DecalParams::Number:
        case DecalParams::Text: {
            if (!params.text.isEmpty()) {
                QFont font(params.fontFamily, params.fontSize);
                p.setFont(font);
                p.setPen(QPen(params.outlineColor, params.outlineWidth));
                QFontMetrics fm(font);
                QRect textRect = fm.boundingRect(params.text);
                QPoint textPos((w - textRect.width()) / 2, (h + textRect.height()) / 2);
                p.drawText(textPos, params.text);
                p.setPen(QPen(params.color, 1));
                textPos -= QPoint(params.outlineWidth, 0);
                p.drawText(QPoint(textPos.x() + (int)params.outlineWidth, textPos.y()), params.text);
            }
            break;
        }
        case DecalParams::Circle: {
            p.setBrush(params.color);
            p.setPen(QPen(params.outlineColor, params.outlineWidth));
            p.drawEllipse(QPointF(w/2, h/2), w/2 - 10, h/2 - 10);
            break;
        }
        case DecalParams::Rectangle: {
            p.setBrush(params.color);
            p.setPen(QPen(params.outlineColor, params.outlineWidth));
            p.drawRect(10, 10, w - 20, h - 20);
            break;
        }
        case DecalParams::Stripe: {
            p.fillRect(0, h/3, w, h/3, params.color);
            break;
        }
        case DecalParams::Logo: {
            p.setBrush(params.color);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(w/2, h/2), w/3, h/3);
            p.setBrush(params.outlineColor);
            QFont font(params.fontFamily, params.fontSize * 2);
            p.setFont(font);
            p.drawText(QRect(0, 0, w, h), Qt::AlignCenter, params.text.left(3));
            break;
        }
    }

    p.end();
    emit generationComplete(image);
    return image;
}

QImage DecalGenerator::generateNumberPlate(const QString& number, const QString& fontFamily) {
    DecalParams params;
    params.type = DecalParams::Text;
    params.text = number;
    params.fontFamily = fontFamily;
    params.fontSize = 72;
    params.width = 2.0f;
    params.height = 0.5f;
    params.color = QColor(30, 30, 30);
    params.outlineColor = Qt::white;
    params.outlineWidth = 0;
    return generateDecal(params);
}

} // namespace ks
