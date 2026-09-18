#include "QmlBridges.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>

namespace ks {

// ─── CspConfigBridge ─────────────────────────────────────────────────────────

QStringList CspConfigBridge::s_conditionInputs = {
    "ONE", "NONE", "TIME", "AMBIENT", "FOG", "SUN", "RACING_FLAG", "YEAR_PROGRESS", "FLAG_TYPE"
};

QStringList CspConfigBridge::s_materialKeys = {
    "ksEmissive", "ksAlphaRef", "ksSpecular", "ksSpecularEXP",
    "ksDiffuse", "ksAmbient", "fresnelC", "fresnelMaxLevel", "fresnelEXP"
};

QStringList CspConfigBridge::s_conditionSmoothnessOptions = {
    "LINEAR", "0.3"
};

CspConfigBridge::CspConfigBridge(QObject* parent)
    : QObject(parent) {}

QVariantMap CspConfigBridge::loadCarConfig(const QString& path) {
    QVariantMap data = CspConfigParser::instance()->loadCarConfig(path);
    emit configLoaded(path, data);
    return data;
}

QVariantMap CspConfigBridge::loadTrackConfig(const QString& path) {
    QVariantMap data = CspConfigParser::instance()->loadTrackConfig(path);
    emit configLoaded(path, data);
    return data;
}

bool CspConfigBridge::saveCarConfig(const QString& path, const QVariantMap& data) {
    bool success = CspConfigParser::instance()->saveCarConfig(path, data);
    emit configSaved(path, success);
    return success;
}

bool CspConfigBridge::saveTrackConfig(const QString& path, const QVariantMap& data) {
    bool success = CspConfigParser::instance()->saveTrackConfig(path, data);
    emit configSaved(path, success);
    return success;
}

QVariantList CspConfigBridge::parseEmissives(const QVariantMap& sections) {
    return CspConfigParser::instance()->parseEmissives(sections);
}

QVariantList CspConfigBridge::parseBrakeDiscs(const QVariantMap& sections) {
    return CspConfigParser::instance()->parseBrakeDiscs(sections);
}

QVariantList CspConfigBridge::parseTrackLights(const QVariantMap& sections) {
    return CspConfigParser::instance()->parseTrackLights(sections);
}

QVariantList CspConfigBridge::parseMaterialAdjustments(const QVariantMap& sections) {
    return CspConfigParser::instance()->parseMaterialAdjustments(sections);
}

QVariantList CspConfigBridge::parseConditions(const QVariantMap& sections) {
    return CspConfigParser::instance()->parseConditions(sections);
}

QVariantMap CspConfigBridge::serializeEmissives(const QVariantList& emissives) {
    return CspConfigParser::instance()->serializeEmissives(emissives);
}

QVariantMap CspConfigBridge::serializeBrakeDiscs(const QVariantList& brakeDiscs) {
    return CspConfigParser::instance()->serializeBrakeDiscs(brakeDiscs);
}

QVariantMap CspConfigBridge::serializeTrackLights(const QVariantList& trackLights) {
    return CspConfigParser::instance()->serializeTrackLights(trackLights);
}

QVariantMap CspConfigBridge::serializeMaterialAdjustments(const QVariantList& adjustments) {
    return CspConfigParser::instance()->serializeMaterialAdjustments(adjustments);
}

QVariantMap CspConfigBridge::serializeConditions(const QVariantList& conditions) {
    return CspConfigParser::instance()->serializeConditions(conditions);
}

QVariantMap CspConfigBridge::getCarSchema() {
    return CspConfigParser::instance()->getCarSchema();
}

QVariantMap CspConfigBridge::getTrackSchema() {
    return CspConfigParser::instance()->getTrackSchema();
}

QVariantMap CspConfigBridge::createDefaultCarConfig() {
    QVariantMap config;
    QVariantMap basic;
    basic.insert("OPEN_WHEELER", "1");
    basic.insert("BOUNDING_SPHERE_RADIUS", "3");
    config.insert("BASIC", basic);

    QVariantMap brakeFx;
    brakeFx.insert("AMBIENT_MULT", "0.6");
    brakeFx.insert("REFLECTION_MULT", "1.0");
    brakeFx.insert("DISC_INTERNAL_RADIUS", "0.126");
    brakeFx.insert("RIM_INTERNAL_RADIUS", "0.06");
    config.insert("BRAKEDISC_FX", brakeFx);

    QVariantMap brakeFxFront;
    brakeFxFront.insert("AMBIENT_MULT", "1.6");
    brakeFxFront.insert("REFLECTION_MULT", "2.0");
    brakeFxFront.insert("DISC_INTERNAL_RADIUS", "0.105");
    brakeFxFront.insert("RIM_INTERNAL_RADIUS", "0.08");
    config.insert("BRAKEDISC_FX_FRONT", brakeFxFront);

    QVariantMap brakeFxRear;
    brakeFxRear.insert("AMBIENT_MULT", "1.6");
    brakeFxRear.insert("REFLECTION_MULT", "2.0");
    brakeFxRear.insert("DISC_INTERNAL_RADIUS", "0.101");
    brakeFxRear.insert("RIM_INTERNAL_RADIUS", "0.08");
    config.insert("BRAKEDISC_FX_REAR", brakeFxRear);

    return config;
}

QVariantMap CspConfigBridge::createDefaultTrackConfig() {
    QVariantMap config;

    QVariantMap lighting;
    lighting.insert("BOUNCED_LIGHT_MULT", "1,1,1,0.1");
    lighting.insert("CAR_LIGHTS_LIT_MULT", "1.0");
    lighting.insert("LIT_MULT", "1.0");
    lighting.insert("SPECULAR_MULT", "1.0");
    config.insert("LIGHTING", lighting);

    QVariantMap condition;
    condition.insert("NAME", "NIGHT_SMOOTH");
    condition.insert("INPUT", "ONE");
    condition.insert("FLASHING_FREQUENCY", "0");
    condition.insert("FLASHING_SMOOTHNESS", "LINEAR");
    config.insert("CONDITION_0", condition);

    return config;
}

QStringList CspConfigBridge::getConditionInputs() {
    return s_conditionInputs;
}

QStringList CspConfigBridge::getMaterialKeys() {
    return s_materialKeys;
}

QStringList CspConfigBridge::getConditionSmoothnessOptions() {
    return s_conditionSmoothnessOptions;
}

QVariantMap CspConfigBridge::addEmissive(QVariantMap sections, const QVariantMap& emissive) {
    return CspConfigParser::instance()->addEmissive(sections, emissive);
}

QVariantMap CspConfigBridge::removeEmissive(QVariantMap sections, const QString& sectionName) {
    return CspConfigParser::instance()->removeEmissive(sections, sectionName);
}

QVariantMap CspConfigBridge::addTrackLight(QVariantMap sections, const QVariantMap& light) {
    return CspConfigParser::instance()->addTrackLight(sections, light);
}

QVariantMap CspConfigBridge::removeTrackLight(QVariantMap sections, const QString& sectionName) {
    return CspConfigParser::instance()->removeTrackLight(sections, sectionName);
}

QVariantMap CspConfigBridge::addMaterialAdjustment(QVariantMap sections, const QVariantMap& adjustment) {
    return CspConfigParser::instance()->addMaterialAdjustment(sections, adjustment);
}

QVariantMap CspConfigBridge::removeMaterialAdjustment(QVariantMap sections, const QString& sectionName) {
    return CspConfigParser::instance()->removeMaterialAdjustment(sections, sectionName);
}

QVariantMap CspConfigBridge::addCondition(QVariantMap sections, const QVariantMap& condition) {
    return CspConfigParser::instance()->addCondition(sections, condition);
}

QVariantMap CspConfigBridge::removeCondition(QVariantMap sections, const QString& sectionName) {
    return CspConfigParser::instance()->removeCondition(sections, sectionName);
}

// ─── MeshLoaderBridge ────────────────────────────────────────────────────────

MeshLoaderBridge::MeshLoaderBridge(QObject *parent)
    : QObject(parent)
    , m_meshRenderer(new MeshRenderer(this))
    , m_contentFinder(new KsContentFinder(this))
{
    connect(m_meshRenderer, &MeshRenderer::meshLoaded,
            this, &MeshLoaderBridge::meshLoaded);
    connect(m_meshRenderer, &MeshRenderer::loadError,
            this, &MeshLoaderBridge::meshLoadError);
    connect(m_contentFinder, &KsContentFinder::contentChanged,
            this, &MeshLoaderBridge::contentRefreshed);
}

bool MeshLoaderBridge::loadKN5(const QString &filePath)
{
    return m_meshRenderer->loadFromKN5(filePath);
}

bool MeshLoaderBridge::loadOBJ(const QString &filePath)
{
    return m_meshRenderer->loadFromOBJ(filePath);
}

bool MeshLoaderBridge::loadGLTF(const QString &filePath)
{
    return m_meshRenderer->loadFromGLTF(filePath);
}

QStringList MeshLoaderBridge::getAvailableMeshes() const
{
    if (!m_meshRenderer) return {};
    QString name = m_meshRenderer->getName();
    if (name.isEmpty()) return {};
    return QStringList() << name;
}

int MeshLoaderBridge::getCurrentVertexCount() const
{
    return m_meshRenderer->getVertexCount();
}

int MeshLoaderBridge::getCurrentFaceCount() const
{
    return m_meshRenderer->getFaceCount();
}

QString MeshLoaderBridge::getCurrentMeshName() const
{
    return m_meshRenderer->getName();
}

QString MeshLoaderBridge::getKsRoot() const
{
    return m_contentFinder->findKsRoot();
}

QStringList MeshLoaderBridge::getCarList() const
{
    return m_contentFinder->findCars();
}

QStringList MeshLoaderBridge::getTrackList() const
{
    return m_contentFinder->findTracks();
}

QStringList MeshLoaderBridge::getSkinList(const QString &carName) const
{
    if (carName.isEmpty()) return QStringList();

    QStringList skins;
    QString contentPath = m_contentFinder->findContentFolder();
    if (contentPath.isEmpty()) return skins;

    QString skinsPath = contentPath + "/cars/" + carName + "/skins";
    QDir skinsDir(skinsPath);

    if (skinsDir.exists()) {
        skins = skinsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    }

    return skins;
}

QString MeshLoaderBridge::getMeshInfo(const QString &carOrTrack) const
{
    if (carOrTrack.isEmpty()) return QString();

    QString contentPath = m_contentFinder->findContentFolder();
    if (contentPath.isEmpty()) return QString();

    QStringList kn5Files;
    QString carPath = contentPath + "/cars/" + carOrTrack;
    QString trackPath = contentPath + "/tracks/" + carOrTrack;

    QDir carDir(carPath);
    QDir trackDir(trackPath);

    if (carDir.exists()) {
        kn5Files = carDir.entryList(QStringList() << "*.kn5", QDir::Files);
    } else if (trackDir.exists()) {
        kn5Files = trackDir.entryList(QStringList() << "*.kn5", QDir::Files);
    }

    if (kn5Files.isEmpty()) return QString();

    QString kn5Path = kn5Files.first().startsWith(carPath) ? carPath : trackPath;
    kn5Path += "/" + kn5Files.first();

    MeshRenderer renderer;
    if (renderer.loadFromKN5(kn5Path)) {
        return QString("Vertices: %1, Faces: %2").arg(renderer.getVertexCount()).arg(renderer.getFaceCount());
    }

    return QString();
}

QString MeshLoaderBridge::getCarKn5Path(const QString &carName) const
{
    if (carName.isEmpty()) return QString();

    QString contentPath = m_contentFinder->findContentFolder();
    if (contentPath.isEmpty()) return QString();

    QString carPath = contentPath + "/cars/" + carName;
    QDir dir(carPath);

    if (dir.exists()) {
        QStringList kn5Files = dir.entryList(QStringList() << "*.kn5", QDir::Files);
        if (!kn5Files.isEmpty()) {
            return carPath + "/" + kn5Files.first();
        }
    }

    return QString();
}

QString MeshLoaderBridge::getTrackKn5Path(const QString &trackName) const
{
    if (trackName.isEmpty()) return QString();

    QString contentPath = m_contentFinder->findContentFolder();
    if (contentPath.isEmpty()) return QString();

    QString trackPath = contentPath + "/tracks/" + trackName;
    QDir dir(trackPath);

    if (dir.exists()) {
        QStringList kn5Files = dir.entryList(QStringList() << "*.kn5", QDir::Files);
        if (!kn5Files.isEmpty()) {
            return trackPath + "/" + kn5Files.first();
        }
    }

    return QString();
}

QVariantList MeshLoaderBridge::getVertexData() const
{
    QVariantList result;
    for (const MeshVertex &v : m_meshRenderer->getVertices()) {
        QVariantMap vertex;
        vertex["position"] = QVariant::fromValue(v.position);
        vertex["normal"] = QVariant::fromValue(v.normal);
        vertex["texCoord"] = QVariant::fromValue(v.texCoord);
        vertex["color"] = QVariant::fromValue(v.color);
        result.append(vertex);
    }
    return result;
}

QVariantList MeshLoaderBridge::getIndexData() const
{
    QVariantList result;
    for (quint32 idx : m_meshRenderer->getIndices()) {
        result.append(idx);
    }
    return result;
}

// ─── MeshOpsBridge ───────────────────────────────────────────────────────────

QVector3D MeshOpsBridge::parseVector3D(const QVariant& v) {
    if (v.canConvert<QVector3D>()) {
        return v.value<QVector3D>();
    }
    QVariantList list = v.toList();
    if (list.size() >= 3) {
        return QVector3D(list[0].toFloat(), list[1].toFloat(), list[2].toFloat());
    }
    return QVector3D();
}

QVector3D MeshOpsBridge::variantToVector3D(const QVariant& v) {
    return parseVector3D(v);
}

QVariant MeshOpsBridge::vector3DToVariant(const QVector3D& v) {
    return QVariant::fromValue(v);
}

MeshData MeshOpsBridge::variantToMesh(const QVariant& data) {
    MeshData mesh;
    QVariantMap map = data.toMap();

    QVariantList vertices = map["vertices"].toList();
    for (const QVariant& v : vertices) {
        QVariantMap vm = v.toMap();
        Vertex vert;
        vert.position = variantToVector3D(vm["position"]);
        vert.normal = variantToVector3D(vm["normal"]);
        if (vm.contains("uv")) {
            QVariant uv = vm["uv"];
            if (uv.canConvert<QVector2D>()) {
                vert.uv = uv.value<QVector2D>();
            } else {
                QVariantList uvList = uv.toList();
                if (uvList.size() >= 2) {
                    vert.uv = QVector2D(uvList[0].toFloat(), uvList[1].toFloat());
                }
            }
        }
        mesh.vertices.append(vert);
    }

    QVariantList faces = map["faces"].toList();
    for (const QVariant& f : faces) {
        QVariantList indices = f.toList();
        Face face;
        for (const QVariant& v : indices) {
            face.indices.append(v.toInt());
        }
        mesh.faces.append(face);
    }

    mesh.computeBoundingBox();
    return mesh;
}

QVariant MeshOpsBridge::meshToVariant(const MeshData& mesh) {
    QVariantMap map;

    QVariantList vertices;
    for (const Vertex& v : mesh.vertices) {
        QVariantMap vm;
        vm["position"] = QVariant::fromValue(v.position);
        vm["normal"] = QVariant::fromValue(v.normal);
        vm["uv"] = QVariant::fromValue(v.uv);
        vertices.append(vm);
    }
    map["vertices"] = vertices;

    QVariantList faces;
    for (const Face& f : mesh.faces) {
        QVariantList indices;
        for (int idx : f.indices) {
            indices.append(idx);
        }
        faces.append(indices);
    }
    map["faces"] = faces;

    map["vertexCount"] = mesh.vertices.size();
    map["faceCount"] = mesh.faces.size();

    return map;
}

QVariantMap MeshOpsBridge::loopCut(const QVariant& meshData, int cuts, const QVariant& center, const QVariant& normal) {
    MeshData mesh = variantToMesh(meshData);
    QVector3D c = variantToVector3D(center);
    QVector3D n = variantToVector3D(normal);

    MeshData result = LoopCut::cut(mesh, cuts, c, n);

    QVariantMap map;
    map["mesh"] = meshToVariant(result);
    map["vertexCount"] = result.vertices.size();
    map["faceCount"] = result.faces.size();
    return map;
}

QVariantMap MeshOpsBridge::knifeCut(const QVariant& meshData, const QVariant& start, const QVariant& end, bool snapToVertex) {
    MeshData mesh = variantToMesh(meshData);
    QVector3D s = variantToVector3D(start);
    QVector3D e = variantToVector3D(end);

    MeshData result = KnifeTool::cut(mesh, s, e, snapToVertex);

    QVariantMap map;
    map["mesh"] = meshToVariant(result);
    map["vertexCount"] = result.vertices.size();
    map["faceCount"] = result.faces.size();
    return map;
}

QVariant MeshOpsBridge::knifeIntersectWithPlane(const QVariant& meshData, const QVariant& point, const QVariant& normal) {
    MeshData mesh = variantToMesh(meshData);
    QVector3D p = variantToVector3D(point);
    QVector3D n = variantToVector3D(normal);

    QVector<KnifeTool::CutPoint> cutPoints = KnifeTool::intersectWithPlane(mesh, p, n);

    QVariantList result;
    for (const KnifeTool::CutPoint& cp : cutPoints) {
        QVariantMap pm;
        pm["position"] = vector3DToVariant(cp.position);
        pm["vertexIndex"] = cp.vertexIndex;
        pm["isNewVertex"] = cp.isNewVertex;
        pm["t"] = cp.t;
        result.append(pm);
    }
    return result;
}

int MeshOpsBridge::splitEdge(QVariant& meshData, int v1, int v2, float t) {
    MeshData mesh = variantToMesh(meshData);
    int newVertexIndex = KnifeTool::splitEdge(mesh, v1, v2, t);
    meshData = meshToVariant(mesh);
    return newVertexIndex;
}

QVariant MeshOpsBridge::splitFace(QVariant& meshData, int faceIndex, const QVariant& point) {
    MeshData mesh = variantToMesh(meshData);
    QVector3D p = variantToVector3D(point);

    QVector<int> newVertexIndices = KnifeTool::splitFace(mesh, faceIndex, p);

    QVariantList result;
    for (int idx : newVertexIndices) {
        result.append(idx);
    }
    meshData = meshToVariant(mesh);
    return result;
}

QVariantList MeshOpsBridge::findEdgeLoops(const QVariant& meshData, int startEdge) {
    MeshData mesh = variantToMesh(meshData);
    QVector<QVector<int>> loops = LoopCut::findEdgeLoops(mesh, startEdge);

    QVariantList result;
    for (const QVector<int>& loop : loops) {
        QVariantList l;
        for (int idx : loop) {
            l.append(idx);
        }
        result.append(l);
    }
    return result;
}

QVariantList MeshOpsBridge::findFaceLoops(const QVariant& meshData, const QVariant& edgeLoop) {
    MeshData mesh = variantToMesh(meshData);
    QVariantList list = edgeLoop.toList();
    QVector<int> loop;
    for (const QVariant& v : list) {
        loop.append(v.toInt());
    }

    QVector<QVector<int>> faceLoops = LoopCut::findFaceLoops(mesh, loop);

    QVariantList result;
    for (const QVector<int>& fl : faceLoops) {
        QVariantList l;
        for (int idx : fl) {
            l.append(idx);
        }
        result.append(l);
    }
    return result;
}

QVariantList MeshOpsBridge::bisect(const QVariant& meshData, const QVariant& planePoint, const QVariant& planeNormal) {
    MeshData mesh = variantToMesh(meshData);
    QVector3D p = variantToVector3D(planePoint);
    QVector3D n = variantToVector3D(planeNormal);

    auto [mesh1, mesh2] = Bisect::split(mesh, p, n);

    QVariantList result;
    result.append(meshToVariant(mesh1));
    result.append(meshToVariant(mesh2));
    return result;
}

QVariantMap MeshOpsBridge::bisectCut(const QVariant& meshData, const QVariant& planePoint, const QVariant& planeNormal, bool cutCenter) {
    MeshData mesh = variantToMesh(meshData);
    QVector3D p = variantToVector3D(planePoint);
    QVector3D n = variantToVector3D(planeNormal);

    MeshData result = Bisect::cut(mesh, p, n, cutCenter);

    QVariantMap map;
    map["mesh"] = meshToVariant(result);
    map["vertexCount"] = result.vertices.size();
    map["faceCount"] = result.faces.size();
    return map;
}

QVariantList MeshOpsBridge::findConnectedVertices(const QVariant& meshData, int startVertex) {
    MeshData mesh = variantToMesh(meshData);
    QVector<QVector<int>> connected = VertexConnectivity::findConnectedVertices(mesh, startVertex);

    QVariantList result;
    for (const QVector<int>& group : connected) {
        QVariantList g;
        for (int idx : group) {
            g.append(idx);
        }
        result.append(g);
    }
    return result;
}

QVariantList MeshOpsBridge::findVertexRings(const QVariant& meshData, int vertexIndex) {
    MeshData mesh = variantToMesh(meshData);
    QVector<QVector<int>> rings = VertexConnectivity::findVertexRings(mesh, vertexIndex);

    QVariantList result;
    for (const QVector<int>& ring : rings) {
        QVariantList r;
        for (int idx : ring) {
            r.append(idx);
        }
        result.append(r);
    }
    return result;
}

int MeshOpsBridge::getVertexValence(const QVariant& meshData, int vertexIndex) {
    MeshData mesh = variantToMesh(meshData);
    return VertexConnectivity::getVertexValence(mesh, vertexIndex);
}

QVariantMap MeshOpsBridge::convexHull(const QVariant& meshData) {
    MeshData mesh = variantToMesh(meshData);
    MeshData hull = ConvexHull::compute(mesh);

    QVariantMap map;
    map["mesh"] = meshToVariant(hull);
    map["vertexCount"] = hull.vertices.size();
    map["faceCount"] = hull.faces.size();
    return map;
}

QVariantList MeshOpsBridge::findHoles(const QVariant& meshData) {
    MeshData mesh = variantToMesh(meshData);
    QVector<QVector<int>> holes = PolygonOperations::findHoles(mesh);

    QVariantList result;
    for (const QVector<int>& hole : holes) {
        QVariantList h;
        for (int idx : hole) {
            h.append(idx);
        }
        result.append(h);
    }
    return result;
}

QVariantMap MeshOpsBridge::fillHoles(const QVariant& meshData, int maxHoleSize) {
    MeshData mesh = variantToMesh(meshData);
    MeshData result = PolygonOperations::fillHoles(mesh, maxHoleSize);

    QVariantMap map;
    map["mesh"] = meshToVariant(result);
    map["filledFaces"] = result.faces.size() - mesh.faces.size();
    return map;
}

QVariantMap MeshOpsBridge::planarFaces(const QVariant& meshData, float threshold) {
    MeshData mesh = variantToMesh(meshData);
    MeshData result = PolygonOperations::planarFaces(mesh, threshold);

    QVariantMap map;
    map["mesh"] = meshToVariant(result);
    return map;
}

QVariantMap MeshOpsBridge::triRemesh(const QVariant& meshData) {
    MeshData mesh = variantToMesh(meshData);
    MeshData result = Remeshing::triRemesh(mesh);

    QVariantMap map;
    map["mesh"] = meshToVariant(result);
    map["vertexCount"] = result.vertices.size();
    map["faceCount"] = result.faces.size();
    return map;
}

QVariantMap MeshOpsBridge::quadRemesh(const QVariant& meshData, int targetCount) {
    MeshData mesh = variantToMesh(meshData);
    MeshData result = Remeshing::quadRemesh(mesh, targetCount);

    QVariantMap map;
    map["mesh"] = meshToVariant(result);
    map["vertexCount"] = result.vertices.size();
    map["faceCount"] = result.faces.size();
    return map;
}

QVariantMap MeshOpsBridge::decimate(const QVariant& meshData, float targetRatio) {
    MeshData mesh = variantToMesh(meshData);
    MeshData result = Decimation::simplify(mesh, targetRatio);

    QVariantMap map;
    map["mesh"] = meshToVariant(result);
    map["originalFaces"] = mesh.faces.size();
    map["newFaces"] = result.faces.size();
    map["reduction"] = 1.0f - (float)result.faces.size() / mesh.faces.size();
    return map;
}

QVariant MeshOpsBridge::sculptDraw(const QVariant& meshData, const QVariant& brushPos, float strength, float radius) {
    MeshData mesh = variantToMesh(meshData);
    QVector3D center = variantToVector3D(brushPos);
    float radiusSq = radius * radius;

    for (auto& vert : mesh.vertices) {
        QVector3D diff = vert.position - center;
        if (diff.lengthSquared() < radiusSq) {
            float falloff = 1.0f - (diff.length() / radius);
            vert.position += diff.normalized() * strength * falloff;
        }
    }

    QVariantMap map;
    map["mesh"] = meshToVariant(mesh);
    map["modified"] = true;
    return map;
}

QVariant MeshOpsBridge::sculptSmooth(const QVariant& meshData, const QVariant& brushPos, float strength, float radius) {
    MeshData mesh = variantToMesh(meshData);
    QVector3D center = variantToVector3D(brushPos);
    float radiusSq = radius * radius;

    QMap<int, QVector3D> avgPositions;
    for (int i = 0; i < mesh.faces.size(); ++i) {
        for (int j = 0; j < qMin(3, mesh.faces[i].vertexCount()); ++j) {
            int idx = mesh.faces[i].indices[j];
            QVector3D diff = mesh.vertices[idx].position - center;
            if (diff.lengthSquared() < radiusSq && !avgPositions.contains(idx)) {
                QVector3D avg;
                int count = 0;
                for (int k = 0; k < mesh.faces.size(); ++k) {
                    int kc = mesh.faces[k].vertexCount();
                    for (int l = 0; l < kc; ++l) {
                        int vi = mesh.faces[k].indices[l];
                        if (vi == idx) {
                            for (int m = 0; m < kc; ++m) {
                                avg += mesh.vertices[mesh.faces[k].indices[m]].position;
                            }
                            avg /= kc;
                            count++;
                            break;
                        }
                    }
                }
                if (count > 0) avgPositions[idx] = avg / count;
            }
        }
    }

    for (auto it = avgPositions.begin(); it != avgPositions.end(); ++it) {
        QVector3D diff = it.value() - mesh.vertices[it.key()].position;
        float falloff = 1.0f - (diff.length() / radius);
        mesh.vertices[it.key()].position += diff * strength * falloff;
    }

    QVariantMap map;
    map["mesh"] = meshToVariant(mesh);
    map["modified"] = true;
    return map;
}

QVariant MeshOpsBridge::sculptFlatten(const QVariant& meshData, const QVariant& brushPos, float strength, float radius) {
    MeshData mesh = variantToMesh(meshData);
    QVector3D center = variantToVector3D(brushPos);
    float radiusSq = radius * radius;
    float avgHeight = 0;
    int count = 0;

    for (const auto& vert : mesh.vertices) {
        QVector3D diff = vert.position - center;
        if (diff.lengthSquared() < radiusSq) {
            avgHeight += vert.position.y();
            count++;
        }
    }
    if (count > 0) avgHeight /= count;

    for (auto& vert : mesh.vertices) {
        QVector3D diff = vert.position - center;
        if (diff.lengthSquared() < radiusSq) {
            float falloff = 1.0f - (diff.length() / radius);
            vert.position.setY(vert.position.y() + (avgHeight - vert.position.y()) * strength * falloff);
        }
    }

    QVariantMap map;
    map["mesh"] = meshToVariant(mesh);
    map["modified"] = true;
    return map;
}

QVariant MeshOpsBridge::sculptInflate(const QVariant& meshData, const QVariant& brushPos, float strength, float radius) {
    MeshData mesh = variantToMesh(meshData);
    QVector3D center = variantToVector3D(brushPos);
    float radiusSq = radius * radius;

    for (auto& vert : mesh.vertices) {
        QVector3D diff = vert.position - center;
        if (diff.lengthSquared() < radiusSq) {
            float falloff = 1.0f - (diff.length() / radius);
            QVector3D normal = vert.normal.isNull() ? QVector3D(0, 1, 0) : vert.normal;
            vert.position += normal * strength * falloff;
        }
    }

    QVariantMap map;
    map["mesh"] = meshToVariant(mesh);
    map["modified"] = true;
    return map;
}

QVariantList MeshOpsBridge::selectSimilar(const QVariant& meshData, const QVariantList& selected, const QString& property, float tolerance) {
    MeshData mesh = variantToMesh(meshData);
    QSet<int> selectedSet;
    for (const QVariant& v : selected) selectedSet.insert(v.toInt());

    if (selectedSet.isEmpty()) return {};

    // Compute average property value from selection
    float avgVal = 0;
    for (int idx : selectedSet) {
        if (idx >= 0 && idx < mesh.vertices.size()) {
            if (property == "x") avgVal += mesh.vertices[idx].position.x();
            else if (property == "y") avgVal += mesh.vertices[idx].position.y();
            else if (property == "z") avgVal += mesh.vertices[idx].position.z();
        }
    }
    avgVal /= selectedSet.size();

    QVariantList result;
    for (int i = 0; i < mesh.vertices.size(); ++i) {
        float val = 0;
        if (property == "x") val = mesh.vertices[i].position.x();
        else if (property == "y") val = mesh.vertices[i].position.y();
        else if (property == "z") val = mesh.vertices[i].position.z();
        if (qAbs(val - avgVal) < tolerance) result.append(i);
    }
    return result;
}

QVariantList MeshOpsBridge::selectByNormal(const QVariant& meshData, const QVariant& normal, float angleDegrees) {
    MeshData mesh = variantToMesh(meshData);
    QVector3D refNormal = variantToVector3D(normal).normalized();
    float cosAngle = qCos(qDegreesToRadians(angleDegrees));

    QVariantList result;
    for (int i = 0; i < mesh.vertices.size(); ++i) {
        QVector3D n = mesh.vertices[i].normal.normalized();
        if (QVector3D::dotProduct(n, refNormal) >= cosAngle)
            result.append(i);
    }
    return result;
}

QVariantList MeshOpsBridge::growSelection(const QVariant& meshData, const QVariantList& selected) {
    MeshData mesh = variantToMesh(meshData);
    QSet<int> selectedSet;
    for (const QVariant& v : selected) selectedSet.insert(v.toInt());

    QSet<int> grown = selectedSet;
    for (const auto& face : mesh.faces) {
        bool faceHasSelected = false;
        for (int idx : face.indices) {
            if (selectedSet.contains(idx)) { faceHasSelected = true; break; }
        }
        if (faceHasSelected) {
            for (int idx : face.indices) grown.insert(idx);
        }
    }

    QVariantList result;
    for (int idx : grown) result.append(idx);
    return result;
}

QVariantList MeshOpsBridge::shrinkSelection(const QVariant& meshData, const QVariantList& selected) {
    MeshData mesh = variantToMesh(meshData);
    QSet<int> selectedSet;
    for (const QVariant& v : selected) selectedSet.insert(v.toInt());

    QSet<int> boundary;
    for (const auto& face : mesh.faces) {
        bool hasSelected = false, hasUnselected = false;
        for (int idx : face.indices) {
            if (selectedSet.contains(idx)) hasSelected = true;
            else hasUnselected = true;
        }
        if (hasSelected && hasUnselected) {
            for (int idx : face.indices) boundary.insert(idx);
        }
    }

    QVariantList result;
    for (int idx : selectedSet) {
        if (!boundary.contains(idx)) result.append(idx);
    }
    return result;
}

// ─── LiveryEditorBridge ──────────────────────────────────────────────────────

LiveryEditorBridge::LiveryEditorBridge(QObject* parent)
    : QObject(parent)
{
}

void LiveryEditorBridge::loadSkins(const QString& carPath)
{
    if (m_manager) {
        delete m_manager;
    }
    m_manager = new paint::PaintManager(carPath);
    m_manager->loadSkins();

    m_skins.clear();
    for (const auto& skin : m_manager->getSkins()) {
        m_skins.append(skin.name);
    }
    emit skinsChanged();
}

bool LiveryEditorBridge::createSkin(const QString& carPath, const QString& skinName)
{
    bool ok = paint::PaintSystem::createSkin(carPath, skinName);
    if (ok) {
        loadSkins(carPath);
        emit skinCreated(skinName);
    }
    return ok;
}

bool LiveryEditorBridge::deleteSkin(const QString& carPath, const QString& skinName)
{
    bool ok = paint::PaintSystem::deleteSkin(carPath, skinName);
    if (ok) {
        loadSkins(carPath);
        emit skinDeleted(skinName);
    }
    return ok;
}

bool LiveryEditorBridge::duplicateSkin(const QString& carPath, const QString& sourceName, const QString& destName)
{
    bool ok = paint::PaintSystem::duplicateSkin(carPath, sourceName, destName);
    if (ok) {
        loadSkins(carPath);
    }
    return ok;
}

bool LiveryEditorBridge::selectSkin(const QString& carPath, const QString& skinName)
{
    if (!m_manager) {
        m_manager = new paint::PaintManager(carPath);
        m_manager->loadSkins();
    }

    bool ok = m_manager->setCurrentSkin(skinName);
    if (ok) {
        m_currentSkin = skinName;
        emit currentSkinChanged();
    }
    return ok;
}

QVariantMap LiveryEditorBridge::getSkinConfig(const QString& skinPath)
{
    paint::PaintSystem::SkinConfig config = paint::PaintSystem::loadSkinConfig(skinPath);

    QVariantMap map;
    map["name"] = config.name;
    map["baseColor"] = config.baseColor;
    map["licensePlateText"] = config.licensePlateText;
    map["licensePlateCountry"] = config.licensePlateCountry;
    map["hasNumber"] = config.hasNumber;
    map["carNumber"] = config.carNumber;
    map["driverName"] = config.driverName;
    map["teamName"] = config.teamName;

    QVariantList layersList;
    for (const auto& layer : config.layers) {
        QVariantMap layerMap;
        layerMap["name"] = layer.name;
        layerMap["type"] = layer.type;
        layerMap["opacity"] = static_cast<double>(layer.opacity);
        layerMap["posX"] = static_cast<double>(layer.position[0]);
        layerMap["posY"] = static_cast<double>(layer.position[1]);
        layerMap["sizeW"] = static_cast<double>(layer.size[0]);
        layerMap["sizeH"] = static_cast<double>(layer.size[1]);
        layerMap["rotation"] = static_cast<double>(layer.rotation);
        layerMap["texturePath"] = layer.texturePath;
        layerMap["visible"] = layer.visible;
        layersList.append(layerMap);
    }
    map["layers"] = layersList;

    return map;
}

bool LiveryEditorBridge::saveSkinConfig(const QString& skinPath, const QVariantMap& config)
{
    paint::PaintSystem::SkinConfig sc;
    sc.name = config["name"].toString();
    sc.baseColor = config["baseColor"].toString();
    sc.licensePlateText = config["licensePlateText"].toString();
    sc.licensePlateCountry = config["licensePlateCountry"].toString();
    sc.hasNumber = config["hasNumber"].toBool();
    sc.carNumber = config["carNumber"].toInt();
    sc.driverName = config["driverName"].toString();
    sc.teamName = config["teamName"].toString();

    QVariantList layersList = config["layers"].toList();
    for (const QVariant& v : layersList) {
        QVariantMap lm = v.toMap();
        paint::PaintSystem::PaintLayer layer;
        layer.name = lm["name"].toString();
        layer.type = lm["type"].toString();
        if (layer.type.isEmpty()) layer.type = "decal";
        layer.opacity = lm["opacity"].toFloat();
        if (layer.opacity == 0.0f && !lm.contains("opacity")) layer.opacity = 1.0f;
        layer.position[0] = lm["posX"].toFloat();
        layer.position[1] = lm["posY"].toFloat();
        layer.size[0] = lm["sizeW"].toFloat();
        if (layer.size[0] == 0.0f && !lm.contains("sizeW")) layer.size[0] = 1.0f;
        layer.size[1] = lm["sizeH"].toFloat();
        if (layer.size[1] == 0.0f && !lm.contains("sizeH")) layer.size[1] = 1.0f;
        layer.rotation = lm["rotation"].toFloat();
        layer.texturePath = lm["texturePath"].toString();
        layer.visible = lm.contains("visible") ? lm["visible"].toBool() : true;
        sc.layers.append(layer);
    }

    bool ok = paint::PaintSystem::saveSkinConfig(sc, skinPath);
    if (ok) emit liveryModified();
    return ok;
}

bool LiveryEditorBridge::addLayer(const QString& skinPath, const QString& name, const QString& type,
                                   float opacity, float posX, float posY, float sizeW, float sizeH)
{
    paint::PaintSystem::SkinConfig config = paint::PaintSystem::loadSkinConfig(skinPath);

    paint::PaintSystem::PaintLayer layer;
    layer.name = name;
    layer.type = type;
    layer.opacity = opacity;
    layer.position[0] = posX;
    layer.position[1] = posY;
    layer.size[0] = sizeW;
    layer.size[1] = sizeH;

    bool ok = paint::PaintSystem::addLayer(config, layer);
    if (ok) {
        ok = paint::PaintSystem::saveSkinConfig(config, skinPath);
        if (ok) emit liveryModified();
    }
    return ok;
}

bool LiveryEditorBridge::removeLayer(const QString& skinPath, int index)
{
    paint::PaintSystem::SkinConfig config = paint::PaintSystem::loadSkinConfig(skinPath);
    bool ok = paint::PaintSystem::removeLayer(config, index);
    if (ok) {
        ok = paint::PaintSystem::saveSkinConfig(config, skinPath);
        if (ok) emit liveryModified();
    }
    return ok;
}

bool LiveryEditorBridge::moveLayer(const QString& skinPath, int fromIndex, int toIndex)
{
    paint::PaintSystem::SkinConfig config = paint::PaintSystem::loadSkinConfig(skinPath);
    bool ok = paint::PaintSystem::moveLayer(config, fromIndex, toIndex);
    if (ok) {
        ok = paint::PaintSystem::saveSkinConfig(config, skinPath);
        if (ok) emit liveryModified();
    }
    return ok;
}

bool LiveryEditorBridge::generateLicensePlate(const QString& skinPath, const QString& text, const QString& country)
{
    QString outputPath = skinPath + "/license_plate.png";
    bool ok = paint::PaintSystem::generateLicensePlate(text, country, outputPath);
    if (ok) {
        paint::PaintSystem::SkinConfig config = paint::PaintSystem::loadSkinConfig(skinPath);
        paint::PaintSystem::PaintLayer layer;
        layer.name = "license_plate";
        layer.type = "decal";
        layer.opacity = 1.0f;
        layer.position[0] = 0.7f;
        layer.position[1] = 0.3f;
        layer.size[0] = 0.25f;
        layer.size[1] = 0.1f;
        layer.texturePath = outputPath;
        layer.visible = true;
        config.layers.append(layer);
        config.licensePlateText = text;
        config.licensePlateCountry = country;
        ok = paint::PaintSystem::saveSkinConfig(config, skinPath);
        if (ok) emit liveryModified();
    }
    return ok;
}

bool LiveryEditorBridge::generatePreview(const QString& skinPath)
{
    return paint::PaintSystem::generatePreview(skinPath);
}

bool LiveryEditorBridge::validateSkin(const QString& skinPath)
{
    QString error;
    bool valid = paint::PaintSystem::validateSkin(skinPath, &error);
    emit validationResult(valid, error);
    return valid;
}

bool LiveryEditorBridge::exportSkin(const QString& skinPath, const QString& outputPath)
{
    return paint::PaintSystem::exportSkin(skinPath, outputPath);
}

bool LiveryEditorBridge::importSkin(const QString& importPath, const QString& carPath)
{
    bool ok = paint::PaintSystem::importSkin(importPath, carPath);
    if (ok) loadSkins(carPath);
    return ok;
}

} // namespace ks
