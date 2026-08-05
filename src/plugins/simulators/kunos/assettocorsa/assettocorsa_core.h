#ifndef KS_ASSETTOCORSA_CORE_H
#define KS_ASSETTOCORSA_CORE_H

#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QVector>
#include <QSet>
#include <QPair>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDataStream>
#include <QIODevice>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <cmath>
#include <algorithm>
#include <limits>

#include "sdk/SDKBackend.h"
#include "ksAssettoCorsaIni.h"
#include "plugins/simulators/kunos/ks/physics/KsPhysics.h"
#include "plugins/simulators/kunos/ks/track/KsTrack.h"

// ============================================================================
// assettocorsa_core.h — File formats and physics types
// Extracted from assettocorsa.h
// ============================================================================

#ifndef KS_CONSTANTS_H
#define KS_CONSTANTS_H

#include <QString>
#include <QStringList>

namespace ks {

constexpr const char* SDK_VERSION = "1.4";
constexpr const char* SDK_PATH = "";

constexpr float PI = 3.14159265359f;
constexpr float PI2 = PI * 2.0f;
constexpr float DEG2RAD = PI / 180.0f;
constexpr float RAD2DEG = 180.0f / PI;

constexpr float MIN_THROTTLE = 0.0f;
constexpr float MAX_THROTTLE = 1.0f;
constexpr float MIN_BRAKE = 0.0f;
constexpr float MAX_BRAKE = 1.0f;
constexpr float MIN_STEER = -1.0f;
constexpr float MAX_STEER = 1.0f;

constexpr int MAX_TYRES = 4;
constexpr int MAX_DRIVERS = 1;
constexpr int MAX_CARS = 0xFF;
constexpr int MAX_CAMERAS = 10;
constexpr int MAX_PITS = 100;
constexpr int MAX_SECTORS = 3;

constexpr float GRAVITY = 9.81f;
constexpr float AIR_DENSITY = 1.225f;

constexpr float MIN_ENGINE_REDLINE = 1000.0f;
constexpr float MAX_ENGINE_REDLINE = 15000.0f;

constexpr float MIN_FUEL_LAP = 0.0f;
constexpr float MAX_FUEL_LAP = 10.0f;
constexpr float FUEL_TANK_MIN = 0.0f;
constexpr float FUEL_TANK_MAX = 200.0f;

constexpr float BRAKE_BIAS_MIN = 0.3f;
constexpr float BRAKE_BIAS_MAX = 0.75f;
constexpr float TYRE_PRESSURE_MIN = 20.0f;
constexpr float TYRE_PRESSURE_MAX = 50.0f;

constexpr float RIDE_HEIGHT_MIN = 0.05f;
constexpr float RIDE_HEIGHT_MAX = 0.5f;
constexpr float CAMBER_MIN = -10.0f;
constexpr float CAMBER_MAX = 10.0f;
constexpr float TOE_MIN = -5.0f;
constexpr float TOE_MAX = 5.0f;

constexpr float DOWNFORCE_COEFF = 1.0f;
constexpr float DRAG_COEFF = 0.3f;

constexpr float MAX_SPEED = 400.0f;
constexpr float MAX_RPM = 20000.0f;
constexpr float MAX_TURBO_BOOST = 5.0f;

constexpr float BRAKING_FORCE_MAX = 50.0f;
constexpr float DOWNFORCE_MAX = 50.0f;

constexpr float SUSPENSION_STIFFNESS_MIN = 1.0f;
constexpr float SUSPENSION_STIFFNESS_MAX = 100.0f;
constexpr float SUSPENSION_DAMPING_MIN = 0.1f;
constexpr float SUSPENSION_DAMPING_MAX = 10.0f;

constexpr float ABS_EBI_LEVELS = 10;
constexpr float TC_LEVELS = 10;

constexpr float MIN_WHEEL_ANGLE = -50.0f;
constexpr float MAX_WHEEL_ANGLE = 50.0f;

constexpr float TIRE_WIDTH_MIN = 150;
constexpr float TIRE_WIDTH_MAX = 400;
constexpr float TIRE_HEIGHT_MIN = 20;
constexpr float TIRE_HEIGHT_MAX = 80;
constexpr float TIRE_RIM_MIN = 12;
constexpr float TIRE_RIM_MAX = 24;

constexpr int MAX_VERTICES = 100000;
constexpr int MAX_FACES = 100000;
constexpr int MAX_BONES = 256;
constexpr int MAX_ANIMATION_TRACKS = 32;

constexpr float DEFAULT_FOV = 60.0f;
constexpr float DEFAULT_FAR = 2000.0f;
constexpr float DEFAULT_NEAR = 0.1f;
constexpr float DEFAULT_EXPOSURE = 12.0f;

constexpr float AI_LINE_WIDTH = 2.0f;
constexpr float AI_LINE_WIDTH_MIN = 0.5f;
constexpr float AI_LINE_WIDTH_MAX = 8.0f;

constexpr int CAR_VERSION = 2;
constexpr int TRACK_VERSION = 2;

constexpr const char* EXT_KN5 = ".kn5";
constexpr const char* EXT_KNANIM = ".ksanim";
constexpr const char* EXT_KTEX = ".dds";
constexpr const char* EXT_MODEL = ".fbx";

// External URLs
constexpr const char* URL_TRECORSA = "https://trecorsa.com/";
constexpr const char* URL_STEAM_WORKSHOP = "https://steamcommunity.com/workshop/";
constexpr const char* URL_KSEDITOR_API = "https://api.kseditor.io/v1";

constexpr const char* FOLDER_CARS = "content/cars";
constexpr const char* FOLDER_TRACKS = "content/tracks";
constexpr const char* FOLDER_DRIVERS = "content/drivers";
constexpr const char* FOLDER_TEXTURES = "content/texture";
constexpr const char* FOLDER_SKINS = "content/skins";
constexpr const char* FOLDER_SHADERS = "system/shaders";
constexpr const char* FOLDER_SCRIPTS = "extension/lua";
constexpr const char* FOLDER_CONFIG = "extension/config";
constexpr const char* FOLDER_UI = "ui";

constexpr const char* DATA_CAR_INI = "data/car.ini";
constexpr const char* DATA_TYRE_INI = "data/tyres.ini";
constexpr const char* DATA_BRAKE_INI = "data/brakes.ini";
constexpr const char* DATA_AERO_INI = "data/aero.ini";
constexpr const char* DATA_SUSP_INI = "data/suspension.ini";
constexpr const char* DATA_ENGINE_INI = "data/engine.ini";
constexpr const char* DATA_DIFF_INI = "data/differential.ini";

constexpr const char* UI_CAR_JSON = "ui/ui_car.json";
constexpr const char* UI_TRACK_JSON = "ui/ui_track.json";
constexpr const char* UI_PREVIEW = "ui/preview.jpg";
constexpr const char* UI_BADGE = "ui/badge.png";

constexpr const char* SHADER_CAR_PAINT = "ksCarPaint";
constexpr const char* SHADER_SIMPLE = "ksSimple";
constexpr const char* SHADER_SKINNED = "ksSkinned";
constexpr const char* SHADER_PERPIXEL_NM = "ksPerPixelNM";

inline float clamp(float value, float minVal, float maxVal) {
    return value < minVal ? minVal : (value > maxVal ? maxVal : value);
}

inline float lerp(float a, float b, float t) {
    return a + (b - a) * clamp(t, 0.0f, 1.0f);
}

inline float degToRad(float deg) {
    return deg * DEG2RAD;
}

inline float radToDeg(float rad) {
    return rad * RAD2DEG;
}

inline float rpmToAngularVelocity(float rpm) {
    return (rpm * PI2) / 60.0f;
}

inline float kmhToMs(float kmh) {
    return kmh / 3.6f;
}

inline float msToKmh(float ms) {
    return ms * 3.6f;
}

inline float clampAngle(float angle) {
    while (angle > PI) angle -= PI2;
    while (angle < -PI) angle += PI2;
    return angle;
}

inline const char* debugCameraMode(int mode) {
    switch (mode) {
    case 0: return "Cockpit";
    case 1: return "Car";
    case 2: return "Drivable";
    case 3: return "Track";
    case 4: return "Helicopter";
    case 5: return "OnBoardFree";
    case 6: return "Free";
    case 7: return "Deprecated";
    case 8: return "ImageGenerator";
    case 9: return "Start";
    default: return "Unknown";
    }
}

inline QString debugWheel(int index) {
    switch (index) {
    case 0: return "FrontLeft";
    case 1: return "FrontRight";
    case 2: return "RearLeft";
    case 3: return "RearRight";
    case 12: return "Front";
    case 48: return "Rear";
    case 20: return "Left";
    case 40: return "Right";
    case 60: return "All";
    default: return "Invalid";
    }
}

inline QString debugSurface(int type) {
    switch (type) {
    case 0: return "Grass";
    case 1: return "Dirt";
    case 2: return "Snow";
    case 3: return "Gravel";
    case 4: return "Kerb";
    case 5: return "Old";
    case 6: return "Sand";
    case 7: return "Ice";
    case 8: return "Snow";
    case 255: return "Default";
    default: return "Unknown";
    }
}

inline QString debugWeather(int type) {
    switch (type) {
    case 0: return "LightThunderstorm";
    case 1: return "Thunderstorm";
    case 2: return "HeavyThunderstorm";
    case 3: return "LightDrizzle";
    case 4: return "Drizzle";
    case 5: return "HeavyDrizzle";
    case 6: return "LightRain";
    case 7: return "Rain";
    case 8: return "HeavyRain";
    case 9: return "LightSnow";
    case 10: return "Snow";
    case 11: return "HeavySnow";
    case 12: return "LightSleet";
    case 13: return "Sleet";
    case 14: return "HeavySleet";
    case 15: return "Clear";
    case 16: return "FewClouds";
    case 17: return "ScatteredClouds";
    case 18: return "BrokenClouds";
    case 19: return "OvercastClouds";
    case 20: return "Fog";
    case 21: return "Mist";
    case 22: return "Smoke";
    case 23: return "Haze";
    case 24: return "Sand";
    case 25: return "Dust";
    case 26: return "Squalls";
    case 27: return "Tornado";
    case 28: return "Hurricane";
    case 29: return "Cold";
    case 30: return "Hot";
    case 31: return "Windy";
    case 32: return "Hail";
    default: return "Unknown";
    }
}

}

#endif

#pragma once

#include <QString>
#include <QMap>
#include <QVariant>
#include <QFile>
#include <QTextStream>

namespace ks {
namespace plugins {
namespace kunos {
namespace ks {

class KsIniSection {
public:
    KsIniSection() = default;
    explicit KsIniSection(const QString& name) : m_name(name) {}

    QString name() const { return m_name; }

    void setValue(const QString& key, const QVariant& value) {
        m_values[key] = value;
    }

    QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const {
        return m_values.value(key, defaultValue);
    }

    QString string(const QString& key, const QString& defaultValue = QString()) const {
        return m_values.value(key, defaultValue).toString();
    }

    int integer(const QString& key, int defaultValue = 0) const {
        return m_values.value(key, defaultValue).toInt();
    }

    float real(const QString& key, float defaultValue = 0.0f) const {
        return m_values.value(key, defaultValue).toFloat();
    }

    bool boolean(const QString& key, bool defaultValue = false) const {
        return m_values.value(key, defaultValue).toBool();
    }

    QStringList keys() const {
        return m_values.keys();
    }

    bool hasKey(const QString& key) const {
        return m_values.contains(key);
    }

    // Alias methods for compatibility
    float getFloat(const QString& key, float defaultValue = 0.0f) const {
        return real(key, defaultValue);
    }

    QString get(const QString& key, const QString& defaultValue = QString()) const {
        return string(key, defaultValue);
    }

    int getInt(const QString& key, int defaultValue = 0) const {
        return integer(key, defaultValue);
    }

    bool getBool(const QString& key, bool defaultValue = false) const {
        return boolean(key, defaultValue);
    }

    void set(const QString& key, const QVariant& value) {
        setValue(key, value);
    }

    // Convenience setters
    void setFloat(const QString& key, float value) {
        setValue(key, value);
    }

    void setInt(const QString& key, int value) {
        setValue(key, value);
    }

    void setString(const QString& key, const QString& value) {
        setValue(key, value);
    }

private:
    QString m_name;
    QMap<QString, QVariant> m_values;
};

class KsIniDocument {
public:
    KsIniDocument() = default;

    bool load(const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }

        QTextStream in(&file);
        KsIniSection* currentSection = nullptr;

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();

            if (line.isEmpty() || line.startsWith(';') || line.startsWith('#')) {
                continue;
            }

            if (line.startsWith('[') && line.endsWith(']')) {
                QString sectionName = line.mid(1, line.length() - 2).trimmed();
                currentSection = &m_sections[sectionName];
                currentSection = &m_sections[sectionName];
                continue;
            }

            if (currentSection && line.contains('=')) {
                int eqPos = line.indexOf('=');
                QString key = line.left(eqPos).trimmed();
                QString value = line.mid(eqPos + 1).trimmed();
                currentSection->setValue(key, value);
            }
        }

        file.close();
        return true;
    }

    bool save(const QString& filePath) const {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }

        QTextStream out(&file);

        for (auto it = m_sections.constBegin(); it != m_sections.constEnd(); ++it) {
            out << "[" << it.key() << "]\n";
            const KsIniSection& section = it.value();
            for (const QString& key : section.keys()) {
                out << key << "=" << section.value(key).toString() << "\n";
            }
            out << "\n";
        }

        file.close();
        return true;
    }

    KsIniSection* section(const QString& name) {
        if (m_sections.contains(name)) {
            return &m_sections[name];
        }
        return nullptr;
    }

    const KsIniSection* section(const QString& name) const {
        auto it = m_sections.find(name);
        if (it != m_sections.end()) {
            return &(*it);
        }
        return nullptr;
    }

    KsIniSection* createSection(const QString& name) {
        return &m_sections[name];
    }

    void removeSection(const QString& name) {
        m_sections.remove(name);
    }

    QStringList sections() const {
        return m_sections.keys();
    }

    bool hasSection(const QString& name) const {
        return m_sections.contains(name);
    }

private:
    QMap<QString, KsIniSection> m_sections;
};

class KsIniLoader {
public:
    static KsIniDocument load(const QString& filePath) {
        KsIniDocument doc;
        doc.load(filePath);
        return doc;
    }

    static bool save(const KsIniDocument& doc, const QString& filePath) {
        return doc.save(filePath);
    }
};

struct KsCarIniParts {
    QString carName;
    QString brand;
    QString class_;
    QString spec;
    int racePower = 0;
    int raceWeight = 0;
    float maxSpeed = 0.0f;

    KsIniDocument carSpec;
    KsIniDocument engineIni;
    KsIniDocument suspensionIni;
    KsIniDocument brakesIni;
    KsIniDocument tyresIni;
    KsIniDocument aerodynamicsIni;
};

} // namespace ks
} // namespace kunos
} // namespace plugins
} // namespace ks

#ifndef KS_KS_MESH_H
#define KS_KS_MESH_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QList>
#include <QMap>
#include <QPair>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QTextStream>
#include <QDataStream>
#include <QIODevice>
#include <cmath>



namespace ks {

struct KsMeshVertex {
    float position[3];
    float normal[3];
    float texcoord[2];
    float tangent[4];
    float color[4];
    int boneIds[4];
    float boneWeights[4];

    KsMeshVertex() {
        position[0] = position[1] = position[2] = 0;
        normal[0] = 0; normal[1] = 1; normal[2] = 0;
        texcoord[0] = texcoord[1] = 0;
        tangent[0] = tangent[1] = tangent[2] = 0; tangent[3] = 1;
        color[0] = color[1] = color[2] = 1; color[3] = 1;
        for (int i = 0; i < 4; i++) {
            boneIds[i] = -1;
            boneWeights[i] = 0;
        }
    }
};

struct KsMeshFace {
    int indices[3];
    int materialId;
    int smoothingGroup;

    KsMeshFace() : materialId(0), smoothingGroup(0) {
        indices[0] = indices[1] = indices[2] = 0;
    }
};

struct KsMeshMaterial {
    QString name;
    QString shader;
    QString diffuseMap;
    QString normalMap;
    QString specularMap;
    QString emissiveMap;

    float diffuse[4];
    float specular[4];
    float ambient[4];
    float emissive[4];

    float opacity;
    float shininess;
    float bumpStrength;

    KsMeshMaterial() : opacity(1.0f), shininess(32.0f), bumpStrength(1.0f) {
        diffuse[0] = diffuse[1] = diffuse[2] = 0.8f; diffuse[3] = 1.0f;
        specular[0] = specular[1] = specular[2] = 0.5f; specular[3] = 1.0f;
        ambient[0] = ambient[1] = ambient[2] = 0.2f; ambient[3] = 1.0f;
        emissive[0] = emissive[1] = emissive[2] = 0; emissive[3] = 1.0f;
    }
};

class KsMeshData {
public:
    QList<KsMeshVertex> vertices;
    QList<KsMeshFace> faces;
    QList<KsMeshMaterial> materials;

    QString name;
    QString sourceFile;

    float boundingMin[3];
    float boundingMax[3];
    float boundingRadius;

    int vertexCount() const { return vertices.size(); }
    int faceCount() const { return faces.size(); }

    void clear() {
        vertices.clear();
        faces.clear();
        materials.clear();
    }

    void calculateBounds() {
        if (vertices.isEmpty()) return;

        boundingMin[0] = boundingMin[1] = boundingMin[2] = 1e9f;
        boundingMax[0] = boundingMax[1] = boundingMax[2] = -1e9f;

        for (const auto& v : vertices) {
            for (int i = 0; i < 3; i++) {
                if (v.position[i] < boundingMin[i]) boundingMin[i] = v.position[i];
                if (v.position[i] > boundingMax[i]) boundingMax[i] = v.position[i];
            }
        }

        boundingRadius = 0;
        float center[3] = {
            (boundingMin[0] + boundingMax[0]) * 0.5f,
            (boundingMin[1] + boundingMax[1]) * 0.5f,
            (boundingMin[2] + boundingMax[2]) * 0.5f
        };

        for (const auto& v : vertices) {
            float dx = v.position[0] - center[0];
            float dy = v.position[1] - center[1];
            float dz = v.position[2] - center[2];
            float dist = sqrt(dx*dx + dy*dy + dz*dz);
            if (dist > boundingRadius) boundingRadius = dist;
        }
    }

    void getCenter(float center[3]) const {
        calculateBounds();
        center[0] = (boundingMin[0] + boundingMax[0]) * 0.5f;
        center[1] = (boundingMin[1] + boundingMax[1]) * 0.5f;
        center[2] = (boundingMin[2] + boundingMax[2]) * 0.5f;
    }

    void centerToOrigin() {
        float center[3];
        getCenter(center);

        for (auto& v : vertices) {
            v.position[0] -= center[0];
            v.position[1] -= center[1];
            v.position[2] -= center[2];
        }

        calculateBounds();
    }

    void scale(float scaleX, float scaleY, float scaleZ) {
        for (auto& v : vertices) {
            v.position[0] *= scaleX;
            v.position[1] *= scaleY;
            v.position[2] *= scaleZ;
        }
        calculateBounds();
    }

    void normalizeSize(float targetSize) {
        if (boundingRadius <= 0) calculateBounds();
        float scale = targetSize / boundingRadius;
        scale(scale, scale, scale);
    }

    void flipFaces() {
        for (auto& f : faces) {
            int temp = f.indices[0];
            f.indices[0] = f.indices[2];
            f.indices[2] = temp;
        }
    }

    void reverseNormals() {
        for (auto& v : vertices) {
            v.normal[0] = -v.normal[0];
            v.normal[1] = -v.normal[1];
            v.normal[2] = -v.normal[2];
        }
    }

    void generateNormals() {
        for (auto& v : vertices) {
            v.normal[0] = v.normal[1] = v.normal[2] = 0;
        }

        for (const auto& f : faces) {
            const KsMeshVertex& v0 = vertices[f.indices[0]];
            const KsMeshVertex& v1 = vertices[f.indices[1]];
            const KsMeshVertex& v2 = vertices[f.indices[2]];

            float e1[3] = { v1.position[0] - v0.position[0], v1.position[1] - v0.position[1], v1.position[2] - v0.position[2] };
            float e2[3] = { v2.position[0] - v0.position[0], v2.position[1] - v0.position[1], v2.position[2] - v0.position[2] };

            float n[3] = {
                e1[1] * e2[2] - e1[2] * e2[1],
                e1[2] * e2[0] - e1[0] * e2[2],
                e1[0] * e2[1] - e1[1] * e2[0]
            };

            for (int i = 0; i < 3; i++) {
                vertices[f.indices[i]].normal[0] += n[0];
                vertices[f.indices[i]].normal[1] += n[1];
                vertices[f.indices[i]].normal[2] += n[2];
            }
        }

        for (auto& v : vertices) {
            float len = sqrt(v.normal[0]*v.normal[0] + v.normal[1]*v.normal[1] + v.normal[2]*v.normal[2]);
            if (len > 0) {
                v.normal[0] /= len;
                v.normal[1] /= len;
                v.normal[2] /= len;
            }
        }
    }

    void generateTangents() {
        for (auto& v : vertices) {
            v.tangent[0] = v.tangent[1] = 0;
            v.tangent[2] = 1;
            v.tangent[3] = 1;
        }

        for (const auto& f : faces) {
            KsMeshVertex& v0 = vertices[f.indices[0]];
            KsMeshVertex& v1 = vertices[f.indices[1]];
            KsMeshVertex& v2 = vertices[f.indices[2]];

            float dx1 = v1.position[0] - v0.position[0];
            float dx2 = v2.position[0] - v0.position[0];
            float dy1 = v1.position[1] - v0.position[1];
            float dy2 = v2.position[1] - v0.position[1];
            float dz1 = v1.position[2] - v0.position[2];
            float dz2 = v2.position[2] - v0.position[2];

            float du1 = v1.texcoord[0] - v0.texcoord[0];
            float du2 = v2.texcoord[0] - v0.texcoord[0];
            float dv1 = v1.texcoord[1] - v0.texcoord[1];
            float dv2 = v2.texcoord[1] - v0.texcoord[1];

            float r = du1 * dv2 - du2 * dv1;
            if (r != 0) r = 1.0f / r;

            float tx = (dx1 * dv2 - dx2 * dv1) * r;
            float ty = (dy1 * dv2 - dy2 * dv1) * r;
            float tz = (dz1 * dv2 - dz2 * dv1) * r;

            for (int i = 0; i < 3; i++) {
                vertices[f.indices[i]].tangent[0] += tx;
                vertices[f.indices[i]].tangent[1] += ty;
                vertices[f.indices[i]].tangent[2] += tz;
            }
        }

        for (auto& v : vertices) {
            float nx = v.normal[0], ny = v.normal[1], nz = v.normal[2];
            float tx = v.tangent[0], ty = v.tangent[1], tz = v.tangent[2];

            float dot = tx * nx + ty * ny + tz * nz;
            v.tangent[0] -= nx * dot;
            v.tangent[1] -= ny * dot;
            v.tangent[2] -= nz * dot;

            float len = sqrt(v.tangent[0]*v.tangent[0] + v.tangent[1]*v.tangent[1] + v.tangent[2]*v.tangent[2]);
            if (len > 0) {
                v.tangent[0] /= len;
                v.tangent[1] /= len;
                v.tangent[2] /= len;
            }
        }
    }

    void weldVertices(float tolerance) {
        QMap<QPair<int,int>, int> vertexMap;
        QList<KsMeshVertex> newVertices;

        for (int i = 0; i < vertices.size(); i++) {
            const auto& v = vertices[i];
            bool found = false;

            for (int j = 0; j < newVertices.size(); j++) {
                const auto& nv = newVertices[j];
                float dist = sqrt(
                    pow(v.position[0] - nv.position[0], 2) +
                    pow(v.position[1] - nv.position[1], 2) +
                    pow(v.position[2] - nv.position[2], 2)
                );

                if (dist < tolerance) {
                    vertexMap[{i, 0}] = j;
                    found = true;
                    break;
                }
            }

            if (!found) {
                vertexMap[{i, 0}] = newVertices.size();
                newVertices.append(v);
            }
        }

        vertices = newVertices;

        for (auto& f : faces) {
            f.indices[0] = vertexMap.value({f.indices[0], 0}, f.indices[0]);
            f.indices[1] = vertexMap.value({f.indices[1], 0}, f.indices[1]);
            f.indices[2] = vertexMap.value({f.indices[2], 0}, f.indices[2]);
        }
    }

    void removeDuplicates() {
        QSet<QString> seen;
        QList<KsMeshFace> newFaces;

        for (const auto& f : faces) {
            QString key = QString::number(f.indices[0]) + "_" + 
                         QString::number(f.indices[1]) + "_" + 
                         QString::number(f.indices[2]);
            if (!seen.contains(key)) {
                seen.insert(key);
                newFaces.append(f);
            }
        }

        faces = newFaces;
    }

    void removeDegenerate() {
        QList<KsMeshFace> newFaces;

        for (const auto& f : faces) {
            int i0 = f.indices[0];
            int i1 = f.indices[1];
            int i2 = f.indices[2];

            if (i0 == i1 || i1 == i2 || i0 == i2) continue;

            const auto& v0 = vertices[i0];
            const auto& v1 = vertices[i1];
            const auto& v2 = vertices[i2];

            float ax = v1.position[0] - v0.position[0];
            float ay = v1.position[1] - v0.position[1];
            float az = v1.position[2] - v0.position[2];
            float bx = v2.position[0] - v0.position[0];
            float by = v2.position[1] - v0.position[1];
            float bz = v2.position[2] - v0.position[2];

            float cross = ax * (ay * bz - az * by) - ay * (ax * bz - az * bx) + az * (ax * by - ay * bx);
            if (fabs(cross) > 1e-10f) {
                newFaces.append(f);
            }
        }

        faces = newFaces;
    }

    float getSurfaceArea() const {
        float area = 0;
        for (const auto& f : faces) {
            const auto& v0 = vertices[f.indices[0]];
            const auto& v1 = vertices[f.indices[1]];
            const auto& v2 = vertices[f.indices[2]];

            float ax = v1.position[0] - v0.position[0];
            float ay = v1.position[1] - v0.position[1];
            float az = v1.position[2] - v0.position[2];
            float bx = v2.position[0] - v0.position[0];
            float by = v2.position[1] - v0.position[1];
            float bz = v2.position[2] - v0.position[2];

            float cx = ay * bz - az * by;
            float cy = az * bx - ax * bz;
            float cz = ax * by - ay * bx;

            area += sqrt(cx*cx + cy*cy + cz*cz) * 0.5f;
        }
        return area;
    }

    float getVolume() const {
        float volume = 0;
        for (const auto& f : faces) {
            const auto& v0 = vertices[f.indices[0]];
            const auto& v1 = vertices[f.indices[1]];
            const auto& v2 = vertices[f.indices[2]];

            volume += v0.position[0] * (v1.position[1] * v2.position[2] - v2.position[1] * v1.position[2]);
            volume += v1.position[0] * (v2.position[1] * v0.position[2] - v0.position[1] * v2.position[2]);
            volume += v2.position[0] * (v0.position[1] * v1.position[2] - v1.position[1] * v0.position[2]);
        }
        return volume / 6.0f;
    }

    QList<int> getVertexHistogram(int buckets = 10) const {
        QList<int> hist;
        hist.fill(0, buckets);

        if (vertices.isEmpty()) return hist;

        float minX = 1e9f, maxX = -1e9f;
        for (const auto& v : vertices) {
            if (v.position[0] < minX) minX = v.position[0];
            if (v.position[0] > maxX) maxX = v.position[0];
        }

        float range = maxX - minX;
        if (range <= 0) return hist;

        for (const auto& v : vertices) {
            int bucket = int((v.position[0] - minX) / range * buckets);
            bucket = qBound(0, bucket, buckets - 1);
            hist[bucket]++;
        }

        return hist;
    }
};

class KsMeshUtils {
public:
    static KsMeshData* createBox(float width, float height, float depth) {
        KsMeshData* mesh = new KsMeshData();
        mesh->name = "Box";

        float w = width / 2, h = height / 2, d = depth / 2;

        float verts[] = {
            -w, -h,  d,   w, -h,  d,   w,  h,  d,  -w,  h,  d,
            -w, -h, -d,  -w,  h, -d,   w,  h, -d,   w, -h, -d,
            -w,  h, -d,  -w,  h,  d,   w,  h,  d,   w,  h, -d,
            -w, -h, -d,   w, -h, -d,   w, -h,  d,  -w, -h,  d,
             w, -h, -d,   w,  h, -d,   w,  h,  d,   w, -h,  d,
            -w, -h, -d,  -w, -h,  d,  -w,  h,  d,  -w,  h, -d
        };

        float norms[] = {
            0, 0, 1,  0, 0, 1,  0, 0, 1,  0, 0, 1,
            0, 0,-1,  0, 0,-1,  0, 0,-1,  0, 0,-1,
            0, 1, 0,  0, 1, 0,  0, 1, 0,  0, 1, 0,
            0,-1, 0,  0,-1, 0,  0,-1, 0,  0,-1, 0,
            1, 0, 0,  1, 0, 0,  1, 0, 0,  1, 0, 0,
           -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0
        };

        for (int i = 0; i < 24; i++) {
            KsMeshVertex v;
            v.position[0] = verts[i*3];
            v.position[1] = verts[i*3+1];
            v.position[2] = verts[i*3+2];
            v.normal[0] = norms[i*3];
            v.normal[1] = norms[i*3+1];
            v.normal[2] = norms[i*3+2];
            mesh->vertices.append(v);
        }

        for (int i = 0; i < 6; i++) {
            KsMeshFace f;
            f.indices[0] = i * 4;
            f.indices[1] = i * 4 + 1;
            f.indices[2] = i * 4 + 2;
            mesh->faces.append(f);

            f.indices[0] = i * 4;
            f.indices[1] = i * 4 + 2;
            f.indices[2] = i * 4 + 3;
            mesh->faces.append(f);
        }

        mesh->calculateBounds();
        return mesh;
    }

    static KsMeshData* createSphere(float radius, int segments, int rings) {
        KsMeshData* mesh = new KsMeshData();
        mesh->name = "Sphere";

        for (int ring = 0; ring <= rings; ring++) {
            float theta = ring * PI / rings;
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            for (int seg = 0; seg <= segments; seg++) {
                float phi = seg * 2 * PI / segments;
                float sinPhi = sin(phi);
                float cosPhi = cos(phi);

                KsMeshVertex v;
                v.position[0] = radius * cosPhi * sinTheta;
                v.position[1] = radius * cosTheta;
                v.position[2] = radius * sinPhi * sinTheta;

                v.normal[0] = cosPhi * sinTheta;
                v.normal[1] = cosTheta;
                v.normal[2] = sinPhi * sinTheta;

                v.texcoord[0] = float(seg) / segments;
                v.texcoord[1] = float(ring) / rings;

                mesh->vertices.append(v);
            }
        }

        for (int ring = 0; ring < rings; ring++) {
            for (int seg = 0; seg < segments; seg++) {
                int current = ring * (segments + 1) + seg;
                int next = current + segments + 1;

                KsMeshFace f1, f2;
                f1.indices[0] = current;
                f1.indices[1] = next;
                f1.indices[2] = current + 1;

                f2.indices[0] = current + 1;
                f2.indices[1] = next;
                f2.indices[2] = next + 1;

                mesh->faces.append(f1);
                mesh->faces.append(f2);
            }
        }

        mesh->calculateBounds();
        return mesh;
    }

    static KsMeshData* createPlane(float width, float depth, int segW, int segD) {
        KsMeshData* mesh = new KsMeshData();
        mesh->name = "Plane";

        float halfW = width / 2;
        float halfD = depth / 2;

        for (int z = 0; z <= segD; z++) {
            for (int x = 0; x <= segW; x++) {
                float u = float(x) / segW;
                float v = float(z) / segD;

                KsMeshVertex vert;
                vert.position[0] = -halfW + u * width;
                vert.position[1] = 0;
                vert.position[2] = -halfD + v * depth;
                vert.normal[1] = 1;
                vert.texcoord[0] = u;
                vert.texcoord[1] = v;

                mesh->vertices.append(vert);
            }
        }

        for (int z = 0; z < segD; z++) {
            for (int x = 0; x < segW; x++) {
                int current = z * (segW + 1) + x;
                int next = current + segW + 1;

                KsMeshFace f1, f2;
                f1.indices[0] = current;
                f1.indices[1] = next;
                f1.indices[2] = current + 1;

                f2.indices[0] = current + 1;
                f2.indices[1] = next;
                f2.indices[2] = next + 1;

                mesh->faces.append(f1);
                mesh->faces.append(f2);
            }
        }

        mesh->calculateBounds();
        return mesh;
    }

    static KsMeshData* createCylinder(float radius, float height, int segments) {
        KsMeshData* mesh = new KsMeshData();
        mesh->name = "Cylinder";

        KsMeshVertex centerTop, centerBottom;
        centerTop.position[1] = height / 2;
        centerBottom.position[1] = -height / 2;
        centerTop.normal[1] = 1;
        centerBottom.normal[1] = -1;

        mesh->vertices.append(centerBottom);

        for (int i = 0; i <= segments; i++) {
            float angle = 2 * PI * i / segments;
            float x = radius * cos(angle);
            float z = radius * sin(angle);

            KsMeshVertex vt, vb;
            vt.position[0] = vb.position[0] = x;
            vt.position[1] = height / 2;
            vb.position[1] = -height / 2;
            vt.position[2] = vb.position[2] = z;

            vt.normal[0] = vb.normal[0] = cos(angle);
            vt.normal[1] = 0;
            vt.normal[2] = vb.normal[2] = sin(angle);

            mesh->vertices.append(vb);
            mesh->vertices.append(vt);
        }

        mesh->vertices.append(centerTop);

        for (int i = 0; i < segments; i++) {
            KsMeshFace fTop, fBottom, fSide1, fSide2;

            fTop.indices[0] = 0;
            fTop.indices[1] = 1 + i * 2 + 2;
            fTop.indices[2] = 1 + i * 2;

            int topIdx = mesh->vertices.size() - 1;
            fBottom.indices[0] = topIdx;
            fBottom.indices[1] = topIdx - 1 - i * 2;
            fBottom.indices[2] = topIdx - 1 - (i * 2 + 2) % (segments * 2);

            mesh->faces.append(fTop);
            mesh->faces.append(fBottom);
        }

        mesh->calculateBounds();
        return mesh;
    }

    static bool loadFromOBJ(const QString& path, KsMeshData* mesh) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return false;

        QTextStream in(&file);

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith("#")) continue;

            QStringList parts = line.split(" ");
            QString cmd = parts[0];

            if (cmd == "v") {
                KsMeshVertex v;
                v.position[0] = parts[1].toFloat();
                v.position[1] = parts[2].toFloat();
                v.position[2] = parts[3].toFloat();
                mesh->vertices.append(v);
            }
            else if (cmd == "vn") {
                if (mesh->vertices.size() > 0) {
                    KsMeshVertex& v = mesh->vertices.last();
                    v.normal[0] = parts[1].toFloat();
                    v.normal[1] = parts[2].toFloat();
                    v.normal[2] = parts[3].toFloat();
                }
            }
            else if (cmd == "vt") {
                if (mesh->vertices.size() > 0) {
                    KsMeshVertex& v = mesh->vertices.last();
                    v.texcoord[0] = parts[1].toFloat();
                    v.texcoord[1] = parts[2].toFloat();
                }
            }
            else if (cmd == "f") {
                KsMeshFace f;
                QStringList v0 = parts[1].split("/");
                QStringList v1 = parts[2].split("/");
                QStringList v2 = parts[3].split("/");

                f.indices[0] = v0[0].toInt() - 1;
                f.indices[1] = v1[0].toInt() - 1;
                f.indices[2] = v2[0].toInt() - 1;

                mesh->faces.append(f);
            }
        }

        file.close();
        mesh->calculateBounds();
        return true;
    }

    static bool saveToOBJ(const QString& path, const KsMeshData* mesh) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return false;

        QTextStream out(&file);
        out << "# OBJ Export\n";
        out << "# Vertices: " << mesh->vertexCount() << "\n";
        out << "# Faces: " << mesh->faceCount() << "\n\n";

        for (const auto& v : mesh->vertices) {
            out << "v " << v.position[0] << " " << v.position[1] << " " << v.position[2] << "\n";
        }
        out << "\n";

        for (const auto& v : mesh->vertices) {
            out << "vn " << v.normal[0] << " " << v.normal[1] << " " << v.normal[2] << "\n";
        }
        out << "\n";

        for (const auto& v : mesh->vertices) {
            out << "vt " << v.texcoord[0] << " " << v.texcoord[1] << "\n";
        }
        out << "\n";

        for (const auto& f : mesh->faces) {
            out << "f " << f.indices[0] + 1 << " " << f.indices[1] + 1 << " " << f.indices[2] + 1 << "\n";
        }

        file.close();
        return true;
    }
};

inline float calculateDistance(const float a[3], const float b[3]) {
    float dx = b[0] - a[0];
    float dy = b[1] - a[1];
    float dz = b[2] - a[2];
    return sqrt(dx*dx + dy*dy + dz*dz);
}

inline void normalize(float v[3]) {
    float len = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (len > 0) {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
}

inline float dot(const float a[3], const float b[3]) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

inline void cross(float result[3], const float a[3], const float b[3]) {
    result[0] = a[1]*b[2] - a[2]*b[1];
    result[1] = a[2]*b[0] - a[0]*b[2];
    result[2] = a[0]*b[1] - a[1]*b[0];
}
}

#endif

#ifndef KS_KS_CONVERT_H
#define KS_KS_CONVERT_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>



#include "ks_kn5.h"


namespace ks {

enum KsExportFormat {
    Format_OBJ = 0,
    Format_FBX = 1,
    Format_GLTF = 2,
    Format_DAE = 3,
    Format_STL = 4,
    Format_3DS = 5,
    Format_JSON = 6,
    Format_XML = 7
};

enum KsImportFormat {
    Import_OBJ = 0,
    Import_FBX = 1,
    Import_GLTF = 2,
    Import_DAE = 3,
    Import_STL = 4,
    Import_3DS = 5,
    Import_KN5 = 6
};

class KsConverter {
public:
    static bool exportToOBJ(const QString& path, const KsMeshData* mesh) {
        return KsMeshUtils::saveToOBJ(path, mesh);
    }

    static bool importFromOBJ(const QString& path, KsMeshData* mesh) {
        return KsMeshUtils::loadFromOBJ(path, mesh);
    }

    static bool exportToJSON(const QString& path, const KsMeshData* mesh) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return false;

        QJsonObject root;
        root["name"] = mesh->name;
        root["vertexCount"] = mesh->vertices.size();
        root["faceCount"] = mesh->faces.size();

        QJsonArray vertices;
        for (const auto& v : mesh->vertices) {
            QJsonArray vertex;
            vertex.append(v.position[0]);
            vertex.append(v.position[1]);
            vertex.append(v.position[2]);
            vertex.append(v.normal[0]);
            vertex.append(v.normal[1]);
            vertex.append(v.normal[2]);
            vertex.append(v.texcoord[0]);
            vertex.append(v.texcoord[1]);
            vertices.append(vertex);
        }
        root["vertices"] = vertices;

        QJsonArray faces;
        for (const auto& f : mesh->faces) {
            QJsonArray face;
            face.append(f.indices[0]);
            face.append(f.indices[1]);
            face.append(f.indices[2]);
            faces.append(face);
        }
        root["faces"] = faces;

        QJsonDocument doc(root);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        return true;
    }

    static bool importFromJSON(const QString& path, KsMeshData* mesh) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return false;

        QByteArray data = file.readAll();
        file.close();

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) return false;

        QJsonObject root = doc.object();
        mesh->name = root["name"].toString();

        QJsonArray vertices = root["vertices"].toArray();
        for (const QJsonValue& v : vertices) {
            QJsonArray arr = v.toArray();
            KsMeshVertex vert;
            vert.position[0] = arr[0].toDouble();
            vert.position[1] = arr[1].toDouble();
            vert.position[2] = arr[2].toDouble();
            if (arr.size() > 5) {
                vert.normal[0] = arr[3].toDouble();
                vert.normal[1] = arr[4].toDouble();
                vert.normal[2] = arr[5].toDouble();
                if (arr.size() > 7) {
                    vert.texcoord[0] = arr[6].toDouble();
                    vert.texcoord[1] = arr[7].toDouble();
                }
            }
            mesh->vertices.append(vert);
        }

        QJsonArray faces = root["faces"].toArray();
        for (const QJsonValue& f : faces) {
            QJsonArray arr = f.toArray();
            KsMeshFace face;
            face.indices[0] = arr[0].toInt();
            face.indices[1] = arr[1].toInt();
            face.indices[2] = arr[2].toInt();
            mesh->faces.append(face);
        }

        mesh->calculateBounds();
        return true;
    }

    static bool exportToXML(const QString& path, const KsMeshData* mesh) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return false;

        QXmlStreamWriter xml(&file);
        xml.setAutoFormatting(true);
        xml.writeStartDocument();

        xml.writeStartElement("mesh");
        xml.writeAttribute("name", mesh->name);
        xml.writeTextElement("vertexCount", QString::number(mesh->vertices.size()));
        xml.writeTextElement("faceCount", QString::number(mesh->faces.size()));

        xml.writeStartElement("vertices");
        for (const auto& v : mesh->vertices) {
            xml.writeStartElement("vertex");
            xml.writeAttribute("x", QString::number(v.position[0]));
            xml.writeAttribute("y", QString::number(v.position[1]));
            xml.writeAttribute("z", QString::number(v.position[2]));
            xml.writeEndElement();
        }
        xml.writeEndElement();

        xml.writeStartElement("faces");
        for (const auto& f : mesh->faces) {
            xml.writeStartElement("face");
            xml.writeAttribute("v0", QString::number(f.indices[0]));
            xml.writeAttribute("v1", QString::number(f.indices[1]));
            xml.writeAttribute("v2", QString::number(f.indices[2]));
            xml.writeEndElement();
        }
        xml.writeEndElement();

        xml.writeEndElement();
        xml.writeEndDocument();

        file.close();
        return true;
    }

    static bool importFromXML(const QString& path, KsMeshData* mesh) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return false;

        QXmlStreamReader xml(&file);
        if (!xml.readNextStartElement()) return false;

        if (xml.name() != "mesh") return false;
        mesh->name = xml.attributes().value("name").toString();

        while (xml.readNextStartElement()) {
            if (xml.name() == "vertices") {
                while (xml.readNextStartElement()) {
                    if (xml.name() == "vertex") {
                        KsMeshVertex v;
                        v.position[0] = xml.attributes().value("x").toDouble();
                        v.position[1] = xml.attributes().value("y").toDouble();
                        v.position[2] = xml.attributes().value("z").toDouble();
                        mesh->vertices.append(v);
                        xml.skipCurrentElement();
                    }
                }
            } else if (xml.name() == "faces") {
                while (xml.readNextStartElement()) {
                    if (xml.name() == "face") {
                        KsMeshFace f;
                        f.indices[0] = xml.attributes().value("v0").toInt();
                        f.indices[1] = xml.attributes().value("v1").toInt();
                        f.indices[2] = xml.attributes().value("v2").toInt();
                        mesh->faces.append(f);
                        xml.skipCurrentElement();
                    }
                }
            } else {
                xml.skipCurrentElement();
            }
        }

        file.close();
        if (xml.hasError()) {
            qWarning() << "XML parse error in mesh:" << xml.errorString();
            return false;
        }
        mesh->calculateBounds();
        return true;
    }

    static bool exportToSTL(const QString& path, const KsMeshData* mesh, bool ascii = false) {
        if (ascii) {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly)) return false;

            QTextStream out(&file);
            out << "solid default\n";

            for (const auto& f : mesh->faces) {
                const auto& v0 = mesh->vertices[f.indices[0]];
                const auto& v1 = mesh->vertices[f.indices[1]];
                const auto& v2 = mesh->vertices[f.indices[2]];

                float ax = v1.position[0] - v0.position[0];
                float ay = v1.position[1] - v0.position[1];
                float az = v1.position[2] - v0.position[2];
                float bx = v2.position[0] - v0.position[0];
                float by = v2.position[1] - v0.position[1];
                float bz = v2.position[2] - v0.position[2];

                float nx = ay * bz - az * by;
                float ny = az * bx - ax * bz;
                float nz = ax * by - ay * bx;
                float len = sqrt(nx*nx + ny*ny + nz*nz);
                if (len > 0) {
                    nx /= len; ny /= len; nz /= len;
                }

                out << "facet normal " << nx << " " << ny << " " << nz << "\n";
                out << "  outer loop\n";
                out << "    vertex " << v0.position[0] << " " << v0.position[1] << " " << v0.position[2] << "\n";
                out << "    vertex " << v1.position[0] << " " << v1.position[1] << " " << v1.position[2] << "\n";
                out << "    vertex " << v2.position[0] << " " << v2.position[1] << " " << v2.position[2] << "\n";
                out << "  endloop\n";
                out << "endfacet\n";
            }

            out << "endsolid\n";
            file.close();
        } else {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly)) return false;

            QDataStream out(&file);
            out.setByteOrder(QDataStream::LittleEndian);

            out << (quint80)0;
            char header[80] = {0};
            file.write(header, 80);

            quint32 faceCount = mesh->faces.size();
            out << faceCount;

            for (const auto& f : mesh->faces) {
                const auto& v0 = mesh->vertices[f.indices[0]];
                const auto& v1 = mesh->vertices[f.indices[1]];
                const auto& v2 = mesh->vertices[f.indices[2]];

                float ax = v1.position[0] - v0.position[0];
                float ay = v1.position[1] - v0.position[1];
                float az = v1.position[2] - v0.position[2];
                float bx = v2.position[0] - v0.position[0];
                float by = v2.position[1] - v0.position[1];
                float bz = v2.position[2] - v0.position[2];

                float nx = ay * bz - az * by;
                float ny = az * bx - ax * bz;
                float nz = ax * by - ay * bx;
                float len = sqrt(nx*nx + ny*ny + nz*nz);
                if (len > 0) {
                    nx /= len; ny /= len; nz /= len;
                }

                out << (float)nx << (float)ny << (float)nz;
                out << (float)v0.position[0] << (float)v0.position[1] << (float)v0.position[2];
                out << (float)v1.position[0] << (float)v1.position[1] << (float)v1.position[2];
                out << (float)v2.position[0] << (float)v2.position[1] << (float)v2.position[2];
                out << (quint16)0;
            }

            file.close();
        }

        return true;
    }

    static bool importFromSTL(const QString& path, KsMeshData* mesh, bool* isBinary = nullptr) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return false;

        QTextStream in(&file);
        QString firstLine = in.readLine().trimmed();

        bool binary = false;
        if (!firstLine.startsWith("solid")) {
            binary = true;
        }
        file.close();

        if (isBinary) *isBinary = binary;

        if (binary) {
            if (!file.open(QIODevice::ReadOnly)) return false;

            QDataStream in(&file);
            in.setByteOrder(QDataStream::LittleEndian);

            file.seek(80);
            quint32 faceCount;
            in >> faceCount;

            for (quint32 i = 0; i < faceCount; i++) {
                float nx, ny, nz;
                float v0[3], v1[3], v2[3];
                quint16 attr;

                in >> nx >> ny >> nz;
                in >> v0[0] >> v0[1] >> v0[2];
                in >> v1[0] >> v1[1] >> v1[2];
                in >> v2[0] >> v2[1] >> v2[2];
                in >> attr;

                KsMeshVertex vert0, vert1, vert2;
                for (int j = 0; j < 3; j++) {
                    vert0.position[j] = v0[j];
                    vert1.position[j] = v1[j];
                    vert2.position[j] = v2[j];
                    vert0.normal[j] = nx;
                    vert1.normal[j] = ny;
                    vert2.normal[j] = nz;
                }

                int baseIdx = mesh->vertices.size();
                mesh->vertices.append(vert0);
                mesh->vertices.append(vert1);
                mesh->vertices.append(vert2);

                KsMeshFace f;
                f.indices[0] = baseIdx;
                f.indices[1] = baseIdx + 1;
                f.indices[2] = baseIdx + 2;
                mesh->faces.append(f);
            }

            file.close();
        } else {
            if (!file.open(QIODevice::ReadOnly)) return false;

            QTextStream in(&file);
            QString line;

            QList<float> stlVerticesX;
            QList<float> stlVerticesY;
            QList<float> stlVerticesZ;
            float snx, sny, snz;

            while (!(line = in.readLine()).isNull()) {
                line = line.trimmed();
                if (line.startsWith("facet normal")) {
                    QStringList parts = line.split(" ", Qt::SkipEmptyParts);
                    if (parts.size() >= 5) {
                        snx = parts[2].toFloat();
                        sny = parts[3].toFloat();
                        snz = parts[4].toFloat();
                    }
                } else if (line.startsWith("vertex")) {
                    QStringList parts = line.split(" ", Qt::SkipEmptyParts);
                    if (parts.size() >= 4) {
                        stlVerticesX.append(parts[1].toFloat());
                        stlVerticesY.append(parts[2].toFloat());
                        stlVerticesZ.append(parts[3].toFloat());
                        vertices.append(v);
                    }
                } else if (line.startsWith("endfacet")) {
                    if (stlVerticesX.size() >= 3) {
                        int baseIdx = mesh->vertices.size();

                        for (int i = 0; i < 3; i++) {
                            KsMeshVertex vert;
                            vert.position[0] = stlVerticesX[i];
                            vert.position[1] = stlVerticesY[i];
                            vert.position[2] = stlVerticesZ[i];
                            vert.normal[0] = snx;
                            vert.normal[1] = sny;
                            vert.normal[2] = snz;
                            mesh->vertices.append(vert);
                        }

                        KsMeshFace f;
                        f.indices[0] = baseIdx;
                        f.indices[1] = baseIdx + 1;
                        f.indices[2] = baseIdx + 2;
                        mesh->faces.append(f);
                    }
                    stlVerticesX.clear();
                    stlVerticesY.clear();
                    stlVerticesZ.clear();
                }
            }

            file.close();
        }

        mesh->calculateBounds();
        return true;
    }
};

class KsKN5Converter {
public:
    static bool exportToKN5(const QString& path, const KsMeshData* mesh) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return false;

        QDataStream out(&file);
        out.setByteOrder(QDataStream::LittleEndian);

        out << (quint32)0x354E4B;
        out << (quint32)2;
        out << (quint32)mesh->vertexCount();
        out << (quint32)mesh->faceCount();
        out << (quint32)1;

        for (const auto& v : mesh->vertices) {
            out << (float)v.position[0];
            out << (float)v.position[1];
            out << (float)v.position[2];
        }

        for (const auto& v : mesh->vertices) {
            out << (float)v.normal[0];
            out << (float)v.normal[1];
            out << (float)v.normal[2];
        }

        for (const auto& v : mesh->vertices) {
            out << (float)v.texcoord[0];
            out << (float)v.texcoord[1];
        }

        for (const auto& f : mesh->faces) {
            out << (quint32)f.indices[0];
            out << (quint32)f.indices[1];
            out << (quint32)f.indices[2];
        }

        file.close();
        return true;
    }

    static bool importFromKN5(const QString& path, KsMeshData* mesh) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return false;

        QDataStream in(&file);
        in.setByteOrder(QDataStream::LittleEndian);

        quint32 magic, version;
        in >> magic;
        in >> version;

        if (magic != 0x354E4B) {
            file.close();
            return false;
        }

        quint32 vertexCount, faceCount, materialCount;
        in >> vertexCount;
        in >> faceCount;
        in >> materialCount;

        QList<float> positions;
        for (quint32 i = 0; i < vertexCount; i++) {
            float x, y, z;
            in >> x >> y >> z;
            positions.append(x); positions.append(y); positions.append(z);
        }

        QList<float> normals;
        for (quint32 i = 0; i < vertexCount; i++) {
            float nx, ny, nz;
            in >> nx >> ny >> nz;
            normals.append(nx); normals.append(ny); normals.append(nz);
        }

        QList<float> texcoords;
        for (quint32 i = 0; i < vertexCount; i++) {
            float u, v;
            in >> u >> v;
            texcoords.append(u); texcoords.append(v);
        }

        for (quint32 i = 0; i < vertexCount; i++) {
            KsMeshVertex vert;
            vert.position[0] = positions[i*3];
            vert.position[1] = positions[i*3+1];
            vert.position[2] = positions[i*3+2];
            vert.normal[0] = normals[i*3];
            vert.normal[1] = normals[i*3+1];
            vert.normal[2] = normals[i*3+2];
            vert.texcoord[0] = texcoords[i*2];
            vert.texcoord[1] = texcoords[i*2+1];
            mesh->vertices.append(vert);
        }

        for (quint32 i = 0; i < faceCount; i++) {
            KsMeshFace f;
            in >> f.indices[0] >> f.indices[1] >> f.indices[2];
            mesh->faces.append(f);
        }

        file.close();
        mesh->calculateBounds();
        return true;
    }
};

class KsModelConverter {
public:
    static bool convert(const QString& inputPath, const QString& outputPath, KsImportFormat from, KsExportFormat to) {
        KsMeshData* mesh = new KsMeshData();
        bool success = false;

        switch (from) {
            case Import_OBJ:
                success = KsConverter::importFromOBJ(inputPath, mesh);
                break;
            case Import_STL:
                success = KsConverter::importFromSTL(inputPath, mesh);
                break;
            case Import_KN5:
                success = KsKN5Converter::importFromKN5(inputPath, mesh);
                break;
            default:
                delete mesh;
                return false;
        }

        if (!success) {
            delete mesh;
            return false;
        }

        switch (to) {
            case Format_OBJ:
                success = KsConverter::exportToOBJ(outputPath, mesh);
                break;
            case Format_STL:
                success = KsConverter::exportToSTL(outputPath, mesh);
                break;
            case Format_JSON:
                success = KsConverter::exportToJSON(outputPath, mesh);
                break;
            case Format_XML:
                success = KsConverter::exportToXML(outputPath, mesh);
                break;
            default:
                success = false;
        }

        delete mesh;
        return success;
    }

    static QString detectFormat(const QString& path) {
        QFileInfo info(path);
        QString ext = info.suffix().toLower();

        if (ext == "obj") return "OBJ";
        if (ext == "fbx") return "FBX";
        if (ext == "gltf" || ext == "glb") return "GLTF";
        if (ext == "dae") return "DAE";
        if (ext == "stl") return "STL";
        if (ext == "3ds") return "3DS";
        if (ext == "kn5") return "KN5";
        if (ext == "json") return "JSON";
        if (ext == "xml") return "XML";

        return "Unknown";
    }

    static QStringList getSupportedImportFormats() {
        return QStringList() << "OBJ" << "STL" << "KN5" << "GLTF" << "FBX" << "DAE";
    }

    static QStringList getSupportedExportFormats() {
        return QStringList() << "OBJ" << "STL" << "JSON" << "XML" << "GLTF" << "FBX" << "DAE" << "3DS";
    }
};

class KsBatchConverter {
public:
    static int batchConvert(const QString& inputDir, const QString& outputDir, KsImportFormat from, KsExportFormat to, const QString& extFilter = "*.*") {
        QDir inDir(inputDir);
        if (!inDir.exists()) return 0;

        QDir outDir(outputDir);
        if (!outDir.exists()) {
            QDir().mkpath(outputDir);
        }

        QStringList files = inDir.entryList(QStringList() << "*." + extFilter, QDir::Files);

        int converted = 0;
        for (const QString& file : files) {
            QString inputPath = inDir.absoluteFilePath(file);
            QFileInfo info(file);
            QString outputPath = outDir.absoluteFilePath(info.baseName() + "." + KsModelConverter::detectFormat(from).toLower());

            if (KsModelConverter::convert(inputPath, outputPath, from, to)) {
                converted++;
            }
        }

        return converted;
    }

    static int batchConvertAll(const QString& inputDir, const QString& outputDir, KsExportFormat to) {
        QDir inDir(inputDir);
        if (!inDir.exists()) return 0;

        QDir outDir(outputDir);
        if (!outDir.exists()) {
            QDir().mkpath(outputDir);
        }

        QStringList files = inDir.entryList(QDir::Files);

        int converted = 0;
        for (const QString& file : files) {
            QString inputPath = inDir.absoluteFilePath(file);
            QString format = KsModelConverter::detectFormat(file);

            QFileInfo info(file);
            QString outputPath = outDir.absoluteFilePath(info.baseName() + ".obj");

            KsMeshData* mesh = new KsMeshData();
            bool success = false;

            if (format == "OBJ") {
                success = KsConverter::importFromOBJ(inputPath, mesh);
            } else if (format == "STL") {
                success = KsConverter::importFromSTL(inputPath, mesh);
            } else if (format == "KN5") {
                success = KsKN5Converter::importFromKN5(inputPath, mesh);
            }

            if (success) {
                switch (to) {
                    case Format_OBJ:
                        outputPath = outDir.absoluteFilePath(info.baseName() + ".obj");
                        success = KsConverter::exportToOBJ(outputPath, mesh);
                        break;
                    case Format_STL:
                        outputPath = outDir.absoluteFilePath(info.baseName() + ".stl");
                        success = KsConverter::exportToSTL(outputPath, mesh);
                        break;
                    case Format_JSON:
                        outputPath = outDir.absoluteFilePath(info.baseName() + ".json");
                        success = KsConverter::exportToJSON(outputPath, mesh);
                        break;
                    default:
                        success = false;
                }

                if (success) converted++;
            }

            delete mesh;
        }

        return converted;
    }
};
}

#endif

#ifndef KS_KS_PHYSICS_H
#define KS_KS_PHYSICS_H

#include "plugins/simulators/kunos/ks/physics/KsPhysics.h"

namespace ks {

using ks::plugins::kunos::ks::KsWheelState;
using ks::plugins::kunos::ks::KsChassisState;
using ks::plugins::kunos::ks::KsEngineState;
using ks::plugins::kunos::ks::KsAeroState;
using ks::plugins::kunos::ks::KsPhysicsEngine;
using ks::plugins::kunos::ks::KsPhysicsSimulator;
using ks::plugins::kunos::ks::calculateIdealRacingLine;
using ks::plugins::kunos::ks::calculateBrakePoint;
using ks::plugins::kunos::ks::estimateLapTime;
}

#endif

#ifndef KS_KS_SETUP_H
#define KS_KS_SETUP_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QIODevice>

#include "sdk/SDKBackend.h"

namespace ks {
namespace kunos {
using KsIniDocument = ::ks::plugins::kunos::ks::KsIniDocument;
using KsIniSection = ::ks::plugins::kunos::ks::KsIniSection;

struct KsSetupData {
    QString name;
    QString trackId;
    QString trackLayout;
    QString carId;
    QString author;
    QDateTime created;
    QDateTime modified;

    float steerRatio;
    float steerLock;

    float frontCamber[2];
    float rearCamber[2];

    float toeOut[4];
    float toeIn[4];

    float frontCaster;
    float rearCaster;

    float frontSAI;
    float rearSAI;

    float frontPRAO;
    float rearPRAO;

    float rideHeight[2];
    float springRate[4];
    float compression[4];
    float rebound[4];

    float frontARB;
    float rearARB;

    float brakeBias;
    float brakePressure[4];

    float diffPower;
    float diffCoast;
    float diffDrive;

    float engineMapping;
    float launchControl;
    float liftThrottle;

    float frontWing;
    float rearWing;
    float diffusers;

    float frontTyrePressure;
    float rearTyrePressure;

    float fuelLevel;

    KsSetupData() : created(QDateTime::currentDateTime()),
        modified(QDateTime::currentDateTime()),
        steerRatio(13.0f), steerLock(180),
        frontCaster(3.0f), rearCaster(3.0f),
        frontSAI(0), rearSAI(0), frontPRAO(0), rearPRAO(0),
        rideHeight{0.15f, 0.15f},
        frontARB(1), rearARB(1),
        brakeBias(0.5f),
        diffPower(50), diffCoast(50), diffDrive(50),
        frontWing(0), rearWing(0), diffusers(0),
        frontTyrePressure(32), rearTyrePressure(32), fuelLevel(50) {
        for (int i = 0; i < 4; i++) {
            frontCamber[i] = rearCamber[i] = 0;
            toeOut[i] = toeIn[i] = 0;
            springRate[i] = 80;
            compression[i] = 50;
            rebound[i] = 50;
            brakePressure[i] = 0;
        }
    }
};

class KsSetupManager {
public:
    static QString getSetupPath(const QString& carId) {
        return SDKBackend::getFolderPath(KsFolderID::UserSetups) + "/" + carId;
    }

    static bool loadSetup(const QString& carId, const QString& setupName, KsSetupData& setup) {
        QString path = getSetupPath(carId) + "/" + setupName + ".ini";
        return loadSetupFromFile(path, setup);
    }

    static bool saveSetup(const QString& carId, const QString& setupName, const KsSetupData& setup) {
        QString path = getSetupPath(carId) + "/" + setupName + ".ini";
        return saveSetupToFile(path, setup);
    }

    static bool loadSetupFromFile(const QString& path, KsSetupData& setup) {
        KsIniDocument doc;
        if (!doc.load(path)) return false;

        setup.name = QFileInfo(path).baseName();

        KsIniSection* susp = doc.section("SUSPENSION");
        if (susp) {
            setup.steerRatio = susp->getFloat("STEER_RATIO", 13);
            setup.steerLock = susp->getFloat("STEER_LOCK", 180);

            setup.frontCamber[0] = susp->getFloat("CamberFL", 0);
            setup.frontCamber[1] = susp->getFloat("CamberFR", 0);
            setup.rearCamber[0] = susp->getFloat("CamberRL", 0);
            setup.rearCamber[1] = susp->getFloat("CamberRR", 0);

            setup.toeOut[0] = susp->getFloat("ToeOutFL", 0);
            setup.toeOut[1] = susp->getFloat("ToeOutFR", 0);
            setup.toeOut[2] = susp->getFloat("ToeOutRL", 0);
            setup.toeOut[3] = susp->getFloat("ToeOutRR", 0);

            setup.rideHeight[0] = susp->getFloat("RideHeightF", 0.15f);
            setup.rideHeight[1] = susp->getFloat("RideHeightR", 0.15f);

            setup.frontARB = susp->getFloat("ARB_F", 1);
            setup.rearARB = susp->getFloat("ARB_R", 1);
        }

        KsIniSection* brakes = doc.section("BRAKES");
        if (brakes) {
            setup.brakeBias = brakes->getFloat("Bias", 0.5f);
        }

        KsIniSection* diff = doc.section("DIFF");
        if (diff) {
            setup.diffPower = diff->getFloat("Power", 50);
            setup.diffCoast = diff->getFloat("Coast", 50);
            setup.diffDrive = diff->getFloat("Drive", 50);
        }

        KsIniSection* aero = doc.section("AERO");
        if (aero) {
            setup.frontWing = aero->getFloat("Front", 0);
            setup.rearWing = aero->getFloat("Rear", 0);
        }

        KsIniSection* tyres = doc.section("TYRES");
        if (tyres) {
            setup.frontTyrePressure = tyres->getFloat("PressureF", 32);
            setup.rearTyrePressure = tyres->getFloat("PressureR", 32);
        }

        KsIniSection* fuel = doc.section("FUEL");
        if (fuel) {
            setup.fuelLevel = fuel->getFloat("Fuel", 50);
        }

        return true;
    }

    static bool saveSetupToFile(const QString& path, const KsSetupData& setup) {
        QDir dir = QFileInfo(path).absoluteDir();
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        KsIniDocument doc;

        KsIniSection* susp = doc.createSection("SUSPENSION");
        susp->set("STEER_RATIO", setup.steerRatio);
        susp->set("STEER_LOCK", setup.steerLock);
        susp->set("CamberFL", setup.frontCamber[0]);
        susp->set("CamberFR", setup.frontCamber[1]);
        susp->set("CamberRL", setup.rearCamber[0]);
        susp->set("CamberRR", setup.rearCamber[1]);
        susp->set("ToeOutFL", setup.toeOut[0]);
        susp->set("ToeOutFR", setup.toeOut[1]);
        susp->set("ToeOutRL", setup.toeOut[2]);
        susp->set("ToeOutRR", setup.toeOut[3]);
        susp->set("RideHeightF", setup.rideHeight[0]);
        susp->set("RideHeightR", setup.rideHeight[1]);
        susp->set("ARB_F", setup.frontARB);
        susp->set("ARB_R", setup.rearARB);

        KsIniSection* brakes = doc.createSection("BRAKES");
        brakes->set("Bias", setup.brakeBias);

        KsIniSection* diff = doc.createSection("DIFF");
        diff->set("Power", setup.diffPower);
        diff->set("Coast", setup.diffCoast);
        diff->set("Drive", setup.diffDrive);

        KsIniSection* aero = doc.createSection("AERO");
        aero->set("Front", setup.frontWing);
        aero->set("Rear", setup.rearWing);

        KsIniSection* tyres = doc.createSection("TYRES");
        tyres->set("PressureF", setup.frontTyrePressure);
        tyres->set("PressureR", setup.rearTyrePressure);

        KsIniSection* fuel = doc.createSection("FUEL");
        fuel->set("Fuel", setup.fuelLevel);

        return doc.save(path);
    }

    static QStringList getAvailableSetups(const QString& carId) {
        QString path = getSetupPath(carId);
        QDir dir(path);

        if (!dir.exists()) return QStringList();

        return dir.entryList(QStringList() << "*.ini", QDir::Files);
    }

    static bool deleteSetup(const QString& carId, const QString& setupName) {
        QString path = getSetupPath(carId) + "/" + setupName + ".ini";
        return QFile::remove(path);
    }

    static bool duplicateSetup(const QString& carId, const QString& source, const QString& dest) {
        KsSetupData setup;
        if (!loadSetup(carId, source, setup)) return false;

        setup.name = dest;
        return saveSetup(carId, dest, setup);
    }

    static bool exportSetup(const QString& carId, const QString& setupName, const QString& exportPath) {
        KsSetupData setup;
        if (!loadSetup(carId, setupName, setup)) return false;

        QJsonObject obj;
        obj["name"] = setup.name;
        obj["carId"] = carId;
        obj["author"] = setup.author;
        obj["steerRatio"] = setup.steerRatio;
        obj["steerLock"] = setup.steerLock;
        obj["brakeBias"] = setup.brakeBias;
        obj["diffPower"] = setup.diffPower;
        obj["diffCoast"] = setup.diffCoast;
        obj["diffDrive"] = setup.diffDrive;
        obj["frontWing"] = setup.frontWing;
        obj["rearWing"] = setup.rearWing;

        QJsonArray frontCamber;
        frontCamber.append(setup.frontCamber[0]);
        frontCamber.append(setup.frontCamber[1]);
        obj["frontCamber"] = frontCamber;

        QJsonArray rearCamber;
        rearCamber.append(setup.rearCamber[0]);
        rearCamber.append(setup.rearCamber[1]);
        obj["rearCamber"] = rearCamber;

        QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);

        QFile file(exportPath);
        if (!file.open(QIODevice::WriteOnly)) return false;
        file.write(json);
        file.close();

        return true;
    }

    static bool importSetup(const QString& importPath, QString& carId, KsSetupData& setup) {
        QFile file(importPath);
        if (!file.open(QIODevice::ReadOnly)) return false;

        QByteArray json = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(json);
        if (!doc.isObject()) return false;

        QJsonObject obj = doc.object();
        setup.name = obj.value("name").toString();
        carId = obj.value("carId").toString();
        setup.steerRatio = obj.value("steerRatio").toDouble(13);
        setup.steerLock = obj.value("steerLock").toDouble(180);
        setup.brakeBias = obj.value("brakeBias").toDouble(0.5);

        QJsonArray fc = obj.value("frontCamber").toArray();
        if (fc.size() >= 2) {
            setup.frontCamber[0] = fc[0].toDouble();
            setup.frontCamber[1] = fc[1].toDouble();
        }

        QJsonArray rc = obj.value("rearCamber").toArray();
        if (rc.size() >= 2) {
            setup.rearCamber[0] = rc[0].toDouble();
            setup.rearCamber[1] = rc[1].toDouble();
        }

        return true;
    }
};

class KsSetupOptimizer {
public:
    static void optimizeCamber(float& front, float& rear, float trackTemp, float trackWet) {
        float camberComp = 0.001f * trackTemp;
        float wetComp = trackWet * 0.002f;

        front = -3.0f + camberComp + wetComp;
        rear = -2.5f + camberComp + wetComp;
    }

    static void optimizePressure(float& front, float& rear, float trackTemp) {
        float tempComp = (25.0f - trackTemp) * 0.1f;

        front = 32.0f + tempComp;
        rear = 30.0f + tempComp;
    }

    static void optimizeARB(float& front, float& rear, float understeer) {
        if (understeer > 0.1f) {
            front *= 1.2f;
            rear *= 0.9f;
        } else if (understeer < -0.1f) {
            front *= 0.9f;
            rear *= 1.15f;
        }
    }

    static void optimizeBrakeBias(float& bias, float frontBrakeTemp, float rearBrakeTemp) {
        float tempDiff = (frontBrakeTemp - rearBrakeTemp) / 100.0f;

        bias += tempDiff * 0.1f;
        bias = qBound(0.3f, bias, 0.75f);
    }

    static float calculateUndersteerGradient(const KsSetupData& setup, float speed, float radius) {
        float cgHeight = 0.3f;
        float wheelbase = 2.7f;
        float trackWidth = 1.6f;

        float latG = (speed * speed) / (radius * 9.81f);

        float understeer = (setup.frontCamber[0] + setup.frontCamber[1] -
                         setup.rearCamber[0] - setup.rearCamber[1]) / 60.0f;

        understeer += (setup.frontWing - setup.rearWing) / 10.0f;

        understeer += setup.frontARB / setup.rearARB - 1.0f;

        return understeer * latG;
    }

    static void suggestSetupChanges(KsSetupData& current, KsSetupData& target,
                                  float trackTemp, float trackWet,
                                  float currentLap, float targetLap) {
        if (currentLap > targetLap) {
            target.rearWing = qMax(0.0f, current.rearWing - 1);
            target.frontWing = qMax(0.0f, current.frontWing - 1);
            target.frontTyrePressure = current.frontTyrePressure + 0.5f;
            target.rearTyrePressure = current.rearTyrePressure + 0.5f;
        } else if (currentLap < targetLap * 0.95f) {
            target.frontWing = qMin(10.0f, current.frontWing + 1);
            target.rearWing = qMin(10.0f, current.rearWing + 1);
            target.frontTyrePressure = current.frontTyrePressure - 0.5f;
            target.rearTyrePressure = current.rearTyrePressure - 0.5f;
        }

        optimizeCamber(target.frontCamber[0], target.rearCamber[0], trackTemp, trackWet);
        optimizeCamber(target.frontCamber[1], target.rearCamber[1], trackTemp, trackWet);
        optimizePressure(target.frontTyrePressure, target.rearTyrePressure, trackTemp);
    }
};

class KsSetupComparator {
public:
    static float compare(const KsSetupData& a, const KsSetupData& b) {
        float diff = 0;

        diff += qAbs(a.frontCamber[0] - b.frontCamber[0]);
        diff += qAbs(a.frontCamber[1] - b.frontCamber[1]);
        diff += qAbs(a.rearCamber[0] - b.rearCamber[0]);
        diff += qAbs(a.rearCamber[1] - b.rearCamber[1]);

        diff += qAbs(a.rideHeight[0] - b.rideHeight[0]) * 10;
        diff += qAbs(a.rideHeight[1] - b.rideHeight[1]) * 10;

        diff += qAbs(a.brakeBias - b.brakeBias) * 10;

        diff += qAbs(a.diffPower - b.diffPower) / 10;
        diff += qAbs(a.diffCoast - b.diffCoast) / 10;
        diff += qAbs(a.diffDrive - b.diffDrive) / 10;

        diff += qAbs(a.frontWing - b.frontWing);
        diff += qAbs(a.rearWing - b.rearWing);

        return diff;
    }

    static QStringList getDifferences(const KsSetupData& a, const KsSetupData& b) {
        QStringList diffs;
        float threshold = 0.1f;

        for (int i = 0; i < 2; i++) {
            if (qAbs(a.frontCamber[i] - b.frontCamber[i]) > threshold) {
                diffs.append(QString("Front Camber %1: %2 -> %3")
                    .arg(i + 1).arg(a.frontCamber[i]).arg(b.frontCamber[i]));
            }
        }

        for (int i = 0; i < 2; i++) {
            if (qAbs(a.rearCamber[i] - b.rearCamber[i]) > threshold) {
                diffs.append(QString("Rear Camber %1: %2 -> %3")
                    .arg(i + 1).arg(a.rearCamber[i]).arg(b.rearCamber[i]));
            }
        }

        if (qAbs(a.brakeBias - b.brakeBias) > 0.01f) {
            diffs.append(QString("Brake Bias: %1 -> %2")
                .arg(a.brakeBias).arg(b.brakeBias));
        }

        return diffs;
    }
};
}
}

#endif

#ifndef KS_KS_SHADERS_H
#define KS_KS_SHADERS_H

#include <QString>
#include <QStringList>
#include <QMap>

#if QT_VERSION >= 0x060000
#include <QtGui/QRgb>
#else
#include <QRgb>
#endif

namespace ks {

enum class KsShader {
    CarPaint = 0,
    Simple = 1,
    Skinned = 2,
    PerPixelNM = 3,
    PerPixelReflection = 4,
    Tyres = 5,
    SkidMark = 6,
    Sky = 7,
    Clouds = 8,
    Flags = 9,
    Windscreen = 10,
    FakeCarShadows = 11,
    PostProcess = 12,
    Font = 13
};

enum class KsBlendMode {
    Opaque = 0,
    AlphaBlend = 1,
    AlphaTest = 2
};

enum class KsDepthMode {
    Normal = 0,
    NoZWrite = 1,
    Off = 2
};

enum class KsCullMode {
    None = 0,
    Front = 1,
    Back = 2
};

enum class KsTexAddressMode {
    Wrap = 0,
    Clamp = 1,
    Mirror = 2,
    Border = 3
};

enum class KsTexFilterMode {
    Point = 0,
    Linear = 1,
    Anisotropic = 2,
    Cubic = 3
};

struct KsColor3 {
    float r, g, b;
    KsColor3() : r(0), g(0), b(0) {}
    KsColor3(float _r, float _g, float _b) : r(_r), g(_g), b(_b) {}
    KsColor3(QRgb color) {
        r = ((color >> 16) & 0xFF) / 255.0f;
        g = ((color >> 8) & 0xFF) / 255.0f;
        b = (color & 0xFF) / 255.0f;
    }
    QRgb toQRgb() const {
        return qRgb(int(r * 255), int(g * 255), int(b * 255));
    }
};

struct KsColor4 {
    float r, g, b, a;
    KsColor4() : r(0), g(0), b(0), a(1) {}
    KsColor4(float _r, float _g, float _b, float _a) : r(_r), g(_g), b(_b), a(_a) {}
    KsColor4(const KsColor3& c, float _a = 1.0f) : r(c.r), g(c.g), b(c.b), a(_a) {}
    QRgb toQRgb() const {
        return qRgb(int(r * 255), int(g * 255), int(b * 255));
    }
};

struct KsMaterialParam {
    QString name;
    enum Type { Float, Float2, Float3, Float4, Int, Bool, Texture } type;
    union {
        float f;
        float f2[2];
        float f3[3];
        float f4[4];
        int i;
        bool b;
    } value;
    QString texturePath;
};

class KsMaterial {
public:
    QString name;
    QString shader;
    KsBlendMode blendMode;
    KsDepthMode depthMode;
    KsCullMode cullMode;

    QList<KsMaterialParam> params;

    KsMaterial() : blendMode(KsBlendMode::Opaque), depthMode(KsDepthMode::Normal), cullMode(KsCullMode::Back) {}

    void setParamFloat(const QString& name, float value) {
        for (auto& p : params) {
            if (p.name == name && p.type == KsMaterialParam::Float) {
                p.value.f = value;
                return;
            }
        }
        KsMaterialParam p;
        p.name = name;
        p.type = KsMaterialParam::Float;
        p.value.f = value;
        params.append(p);
    }

    void setParamFloat3(const QString& name, float x, float y, float z) {
        for (auto& p : params) {
            if (p.name == name && p.type == KsMaterialParam::Float3) {
                p.value.f3[0] = x;
                p.value.f3[1] = y;
                p.value.f3[2] = z;
                return;
            }
        }
        KsMaterialParam p;
        p.name = name;
        p.type = KsMaterialParam::Float3;
        p.value.f3[0] = x;
        p.value.f3[1] = y;
        p.value.f3[2] = z;
        params.append(p);
    }

    void setParamFloat4(const QString& name, float x, float y, float z, float w) {
        for (auto& p : params) {
            if (p.name == name && p.type == KsMaterialParam::Float4) {
                p.value.f4[0] = x;
                p.value.f4[1] = y;
                p.value.f4[2] = z;
                p.value.f4[3] = w;
                return;
            }
        }
        KsMaterialParam p;
        p.name = name;
        p.type = KsMaterialParam::Float4;
        p.value.f4[0] = x;
        p.value.f4[1] = y;
        p.value.f4[2] = z;
        p.value.f4[3] = w;
        params.append(p);
    }

    void setParamTexture(const QString& name, const QString& path) {
        for (auto& p : params) {
            if (p.name == name && p.type == KsMaterialParam::Texture) {
                p.texturePath = path;
                return;
            }
        }
        KsMaterialParam p;
        p.name = name;
        p.type = KsMaterialParam::Texture;
        p.texturePath = path;
        params.append(p);
    }

    float getParamFloat(const QString& name, float defaultValue = 0.0f) const {
        for (const auto& p : params) {
            if (p.name == name && p.type == KsMaterialParam::Float) {
                return p.value.f;
            }
        }
        return defaultValue;
    }

    void getParamFloat3(const QString& name, float& x, float& y, float& z, float defaultValue = 0.0f) const {
        for (const auto& p : params) {
            if (p.name == name && p.type == KsMaterialParam::Float3) {
                x = p.value.f3[0];
                y = p.value.f3[1];
                z = p.value.f3[2];
                return;
            }
        }
        x = y = z = defaultValue;
    }

    QString getParamTexture(const QString& name) const {
        for (const auto& p : params) {
            if (p.name == name && p.type == KsMaterialParam::Texture) {
                return p.texturePath;
            }
        }
        return QString();
    }
};

const QMap<QString, QString>& getBuiltinShaders() {
    static QMap<QString, QString> shaders;
    if (shaders.isEmpty()) {
        shaders["ksCarPaintSimple"] = "Simple car paint with reflections";
        shaders["ksCarPaint"] = "Advanced car paint with metallic flake";
        shaders["ksSimple"] = "Simple per-pixel lighting";
        shaders["ksPerPixelNM"] = "Per-pixel with normal mapping";
        shaders["ksPerPixelReflection"] = "Per-pixel with reflection";
        shaders["ksSkinnedMesh"] = "Skinned mesh shader";
        shaders["ksSkinnedMesh_NMDetaill"] = "Skinned mesh with detail normal";
        shaders["ksTyres"] = "Tyre shader with tvs";
        shaders["ksSkidMark"] = "Skid mark shader";
        shaders["ksSky"] = "Sky shader";
        shaders["ksSkyCubemap"] = "Sky cubemap shader";
        shaders["ksSkyBox"] = "Skybox shader";
        shaders["ksClouds"] = "Clouds shader";
        shaders["ksFlags"] = "Flags/banner shader";
        shaders["ksWindscreen"] = "Windscreen with transparency";
        shaders["ksFakeCarShadows"] = "Fake car shadows";
        shaders["ksFakeCarShadowsGen"] = "Fake car shadows generation";
        shaders["ksShadowGen"] = "Shadow map generation";
        shaders["ksShadowGenAT"] = "Shadow map for alpha tested";
        shaders["ksShadowGenSKIN"] = "Shadow map for skinned";
        shaders["ksPostCopy"] = "Post-process copy";
        shaders["ksPostCopyLuma"] = "Post-process copy with luma";
        shaders["ksPostBlur"] = "Post-process blur";
        shaders["ksPostBlurH"] = "Post-process blur horizontal";
        shaders["ksPostBlurV"] = "Post-process blur vertical";
        shaders["ksPostBlurRadial"] = "Post-process radial blur";
        shaders["ksPostBlurRadialMS"] = "Post-process radial blur mult-sample";
        shaders["ksPostBlur_MS"] = "Post-process blur mult-sample";
        shaders["ksPostBW"] = "Post-process B&W";
        shaders["ksPostToneMap"] = "Post-process tone mapping";
        shaders["ksPostAdaptLum"] = "Post-process adaptive luminance";
        shaders["ksPostFOG"] = "Post-process fog";
        shaders["ksPostFOG_MS"] = "Post-process fog mult-sample";
        shaders["ksFXAA_0"] = "FXAA 0";
        shaders["ksFXAA_1"] = "FXAA 1";
        shaders["ksFXAA_2"] = "FXAA 2";
        shaders["ksFont"] = "Font shader";
        shaders["ksTree"] = "Tree shader";
        shaders["ksBrokenGlass"] = "Broken glass shader";
        shaders["ksCircularRPM"] = "Circular RPM display";
        shaders["ksCameraDirt"] = "Camera dirt overlay";
        shaders["ksSelectedMesh"] = "Selected mesh highlight";
        shaders["ksColourShader"] = "Solid color shader";
    }
    return shaders;
}

const QMap<QString, QString>& getBuiltinParams() {
    static QMap<QString, QString> params;
    if (params.isEmpty()) {
        params["ksMad"] = "Material ambient diffuse";
        params["ksMss"] = "Material specular specular";
        params["ksMds"] = "Material diffuse specular";
        params["texMap"] = "Diffuse/Albedo texture";
        params["N map"] = "Normal map";
        params["KSmap"] = "Specular map";
        params["Emap"] = "Emissive map";
        params["tfactor"] = "Tint factor";
        params["ksRoughness"] = "Roughness factor";
        params["ksMetalness"] = "Metalness factor";
        params["alphacutoff"] = "Alpha cutoff for alpha test";
        params["ksEmissive"] = "Emissive intensity";
        params["ksEnv"] = "Environment cubemap";
    }
    return params;
}

inline const char* toString(KsShader shader) {
    switch (shader) {
    case KsShader::CarPaint: return "ksCarPaint";
    case KsShader::Simple: return "ksSimple";
    case KsShader::Skinned: return "ksSkinned";
    case KsShader::PerPixelNM: return "ksPerPixelNM";
    case KsShader::PerPixelReflection: return "ksPerPixelReflection";
    default: return "Unknown";
    }
}

inline const char* toString(KsBlendMode mode) {
    switch (mode) {
    case KsBlendMode::Opaque: return "Opaque";
    case KsBlendMode::AlphaBlend: return "AlphaBlend";
    case KsBlendMode::AlphaTest: return "AlphaTest";
    default: return "Unknown";
    }
}

inline const char* toString(KsDepthMode mode) {
    switch (mode) {
    case KsDepthMode::Normal: return "Normal";
    case KsDepthMode::NoZWrite: return "NoZWrite";
    case KsDepthMode::Off: return "Off";
    default: return "Unknown";
    }
}

inline bool isTransparent(KsBlendMode mode) {
    return mode == KsBlendMode::AlphaBlend || mode == KsBlendMode::AlphaTest;
}
}

#endif

#ifndef KS_KS_TRACK_H
#define KS_KS_TRACK_H

#include "plugins/simulators/kunos/ks/track/KsTrack.h"

namespace ks {

using ks::plugins::kunos::ks::KsWaypoint2D;
using ks::plugins::kunos::ks::KsTrackSector;
using ks::plugins::kunos::ks::KsTrackGeometry;
using ks::plugins::kunos::ks::KsCameraSpline;
using ks::plugins::kunos::ks::KsTrackSectorConfig;
using ks::plugins::kunos::ks::KsTrackDatabase;
using ks::plugins::kunos::ks::KsTrackManager;
}

#endif

#ifndef KS_KS_VALIDATE_H
#define KS_KS_VALIDATE_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QPair>
#include <QMap>
#include <QSet>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QTextStream>
#include <cmath>





namespace ks {

enum KsValidationLevel {
    Validate_Warning = 0,
    Validate_Error = 1,
    Validate_Critical = 2
};

struct KsValidationIssue {
    int level;
    QString category;
    QString message;
    QString file;
    int line;
    QString suggestion;

    KsValidationIssue(int lvl = 0, const QString& cat = "", const QString& msg = "", const QString& f = "", int ln = 0, const QString& sug = "")
        : level(lvl), category(cat), message(msg), file(f), line(ln), suggestion(sug) {}
};

struct KsValidationResult {
    int errorCount;
    int warningCount;
    int criticalCount;

    QVector<KsValidationIssue> issues;

    bool passed;

    KsValidationResult()
        : errorCount(0), warningCount(0), criticalCount(0), passed(true) {}

    void addIssue(const KsValidationIssue& issue) {
        issues.append(issue);

        if (issue.level == Validate_Critical) criticalCount++;
        else if (issue.level == Validate_Error) errorCount++;
        else warningCount++;

        if (criticalCount > 0 || errorCount > 0) passed = false;
    }

    bool isEmpty() const { return issues.isEmpty(); }
    bool hasErrors() const { return errorCount > 0 || criticalCount > 0; }

    QString getSummary() const {
        return QString("Validation: %1 critical, %2 errors, %3 warnings")
            .arg(criticalCount).arg(errorCount).arg(warningCount);
    }
};

class KsValidator {
public:
    static KsValidationResult validateCarINI(const KsCarConfig* config, const QString& carPath) {
        KsValidationResult result;

        if (!config) {
            result.addIssue(KsValidationIssue(Validate_Critical, "Config", "Car config is null", carPath));
            return result;
        }

        if (config->name.isEmpty()) {
            result.addIssue(KsValidationIssue(Validate_Error, "Config", "Car name is empty", carPath, 0, "Set a valid car name"));
        }

        if (config->brand.isEmpty()) {
            result.addIssue(KsValidationIssue(Validate_Error, "Config", "Car brand is empty", carPath, 0, "Set a valid car brand"));
        }

        if (config->specs.power <= 0) {
            result.addIssue(KsValidationIssue(Validate_Error, "Specs", "Engine power must be positive", carPath, 0, "Set realistic engine power value"));
        }

        if (config->specs.torque <= 0) {
            result.addIssue(KsValidationIssue(Validate_Error, "Specs", "Engine torque must be positive", carPath, 0, "Set realistic engine torque value"));
        }

        if (config->specs.dryWeight <= 0) {
            result.addIssue(KsValidationIssue(Validate_Critical, "Specs", "Dry weight must be positive", carPath, 0, "Set realistic dry weight"));
        }

        if (config->specs.dryWeight > 2000) {
            result.addIssue(KsValidationIssue(Validate_Warning, "Specs", "Excessive car weight (>2000kg)", carPath, 0, "Consider reducing weight"));
        }

        if (config->specs.maxSpeed <= 0) {
            result.addIssue(KsValidationIssue(Validate_Error, "Specs", "Max speed must be positive", carPath, 0, "Set realistic max speed"));
        }

        if (config->specs.rrDampFract < 0 || config->specs.rrDampFract > 1) {
            result.addIssue(KsValidationIssue(Validate_Error, "Suspension", "Rear ride height out of range", carPath, 0, "Adjust suspension height"));
        }

        for (int i = 0; i < config->aero. wingsCount; i++) {
            if (config->aero.wingsOffset[i] < 0) {
                result.addIssue(KsValidationIssue(Validate_Error, "Aerodynamics", "Wing offset negative", carPath, 0, "Use positive offset value"));
            }
        }

        return result;
    }

    static KsValidationResult validateCarFolder(const QString& carFolder) {
        KsValidationResult result;
        QDir dir(carFolder);

        if (!dir.exists()) {
            result.addIssue(KsValidationIssue(Validate_Critical, "FileSystem", "Car folder does not exist", carFolder));
            return result;
        }

        QStringList requiredDirs;
        requiredDirs << "data" << "skins";

        for (const QString& subDir : requiredDirs) {
            if (!QFileInfo(dir, subDir).exists()) {
                result.addIssue(KsValidationIssue(Validate_Critical, "FileSystem", "Missing required folder: " + subDir, carFolder, 0, "Create " + subDir + " folder"));
            }
        }

        QStringList requiredFiles;
        requiredFiles << "data/car.ini" << "data/suspension.ini" << "data/tyres.ini";

        for (const QString& file : requiredFiles) {
            if (!QFileInfo(dir, file).exists()) {
                result.addIssue(KsValidationIssue(Validate_Critical, "FileSystem", "Missing required file: " + file, carFolder, 0, "Create " + file));
            }
        }

        return result;
    }

    static KsValidationResult validateTrackFolder(const QString& trackFolder) {
        KsValidationResult result;

        if (!QFileInfo(trackFolder).exists()) {
            result.addIssue(KsValidationIssue(Validate_Critical, "FileSystem", "Track folder does not exist", trackFolder));
            return result;
        }

        QStringList required;
        required << "ui/ui_track.json" << "ui/preview.png" << "surfaces.ini";

        for (const QString& file : required) {
            if (!QFileInfo(trackFolder, file).exists()) {
                result.addIssue(KsValidationIssue(Validate_Error, "FileSystem", "Missing required file: " + file, trackFolder, 0, "Create " + file));
            }
        }

        return result;
    }

    static KsValidationResult validateMesh(const KsMeshData* mesh, const QString& name = "") {
        KsValidationResult result;

        if (!mesh) {
            result.addIssue(KsValidationIssue(Validate_Critical, "Mesh", "Mesh is null", name));
            return result;
        }

        if (mesh->vertices.isEmpty()) {
            result.addIssue(KsValidationIssue(Validate_Critical, "Mesh", "Mesh has no vertices", name, 0, "Add vertices to mesh"));
        }

        if (mesh->faces.isEmpty()) {
            result.addIssue(KsValidationIssue(Validate_Critical, "Mesh", "Mesh has no faces", name, 0, "Add faces to mesh"));
        }

        for (int i = 0; i < mesh->vertices.size(); i++) {
            const auto& v = mesh->vertices[i];

            if (isnan(v.position[0]) || isnan(v.position[1]) || isnan(v.position[2])) {
                result.addIssue(KsValidationIssue(Validate_Critical, "Mesh", "Vertex has NaN position", name, i, "Fix vertex position"));
            }

            if (isinf(v.position[0]) || isinf(v.position[1]) || isinf(v.position[2])) {
                result.addIssue(KsValidationIssue(Validate_Critical, "Mesh", "Vertex has inf position", name, i, "Fix vertex position"));
            }

            float len = sqrt(v.normal[0]*v.normal[0] + v.normal[1]*v.normal[1] + v.normal[2]*v.normal[2]);
            if (len < 0.001f) {
                result.addIssue(KsValidationIssue(Validate_Warning, "Mesh", "Vertex has zero normal", name, i, "Recompute normals"));
            }
        }

        for (int i = 0; i < mesh->faces.size(); i++) {
            const auto& f = mesh->faces[i];

            if (f.indices[0] < 0 || f.indices[0] >= mesh->vertices.size() ||
                f.indices[1] < 0 || f.indices[1] >= mesh->vertices.size() ||
                f.indices[2] < 0 || f.indices[2] >= mesh->vertices.size()) {
                result.addIssue(KsValidationIssue(Validate_Critical, "Mesh", "Face has invalid vertex index", name, i, "Fix face indices"));
            }

            if (f.indices[0] == f.indices[1] || f.indices[1] == f.indices[2] || f.indices[0] == f.indices[2]) {
                result.addIssue(KsValidationIssue(Validate_Error, "Mesh", "Face has duplicate indices (degenerate)", name, i, "Remove degenerate face"));
            }
        }

        if (mesh->boundingRadius <= 0) {
            result.addIssue(KsValidationIssue(Validate_Warning, "Mesh", "Bounding radius not computed", name));
        }

        return result;
    }

    static KsValidationResult validateKN5(const QString& kn5Path) {
        KsValidationResult result;

        if (!QFileInfo(kn5Path).exists()) {
            result.addIssue(KsValidationIssue(Validate_Critical, "FileSystem", "KN5 file does not exist", kn5Path));
            return result;
        }

        QFileInfo info(kn5Path);
        if (info.size() == 0) {
            result.addIssue(KsValidationIssue(Validate_Critical, "FileSystem", "KN5 file is empty", kn5Path));
            return result;
        }

        return result;
    }

    static KsValidationResult validateSkin(const QString& skinFolder) {
        KsValidationResult result;

        if (!QFileInfo(skinFolder).exists()) {
            result.addIssue(KsValidationIssue(Validate_Critical, "FileSystem", "Skin folder does not exist", skinFolder));
            return result;
        }

        QString preview = skinFolder + "/preview.png";
        if (!QFileInfo(preview).exists()) {
            result.addIssue(KsValidationIssue(Validate_Warning, "Skin", "Missing preview.png", skinFolder, 0, "Add preview.png"));
        }

        return result;
    }

    static KsValidationResult validateTextures(const QString& texturesFolder) {
        KsValidationResult result;

        if (!QFileInfo(texturesFolder).exists()) {
            result.addIssue(KsValidationIssue(Validate_Critical, "FileSystem", "Textures folder does not exist", texturesFolder));
            return result;
        }

        QStringList supported;
        supported << "*.png" << "*.dds" << "*.jpg" << "*.tga";

        QDir dir(texturesFolder);
        QFileInfoList files = dir.entryInfoList(supported, QDir::Files);

        if (files.isEmpty()) {
            result.addIssue(KsValidationIssue(Validate_Warning, "Textures", "No texture files found", texturesFolder));
        }

        for (const QFileInfo& fi : files) {
            QString ext = fi.suffix().toLower();

            if (fi.size() > 10 * 1024 * 1024) {
                result.addIssue(KsValidationIssue(Validate_Warning, "Textures", "Large texture file: " + fi.fileName(), texturesFolder, 0, "Consider compressing"));
            }

            int w = 0, h = 0;
            if (ext == "png" || ext == "jpg") {
                result.addIssue(KsValidationIssue(Validate_Warning, "Textures", "Texture not POT: " + fi.fileName(), texturesFolder));
            }
        }

        return result;
    }
};

class KsConsistencyChecker {
public:
    static QVector<KsValidationIssue> checkDuplicateModels(const QString& modelsFolder) {
        QVector<KsValidationIssue> issues;
        QMap<QString, QString> hashToPath;

        QDir dir(modelsFolder);
        QStringList files = dir.entryList(QStringList() << "*.kn5", QDir::Files);

        for (const QString& file : files) {
            QString path = dir.absoluteFilePath(file);
            QCryptographicHash hash(QCryptographicHash::Md5);

            QFile f(path);
            if (f.open(QIODevice::ReadOnly)) {
                hash.addData(f.read(1024));
                f.close();
            }

            QString hashStr = hash.result().toHex();
            if (hashToPath.contains(hashStr)) {
                issues.append(KsValidationIssue(Validate_Warning, "Consistency", "Duplicate model: " + file + " vs " + hashToPath[hashStr], path));
            } else {
                hashToPath[hashStr] = path;
            }
        }

        return issues;
    }

    static QVector<KsValidationIssue> checkDuplicateSkins(const QString& carFolder) {
        QVector<KsValidationIssue> issues;

        QDir skinsDir(carFolder + "/skins");
        if (!skinsDir.exists()) return issues;

        QStringList skins = skinsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QString& skin : skins) {
            QDir skinDir(skinsDir.absoluteFilePath(skin));
            if (skinDir.entryList(QStringList() << "*.png", QDir::Files).isEmpty()) {
                issues.append(KsValidationIssue(Validate_Warning, "Consistency", "Skin without preview: " + skin, skinDir.absolutePath()));
            }
        }

        return issues;
    }

    static QVector<KsValidationIssue> checkTrackWaypoints(const KsTrackData* track) {
        QVector<KsValidationIssue> issues;

        if (!track) return issues;

        if (track->waypoints.isEmpty()) {
            issues.append(KsValidationIssue(Validate_Critical, "Track", "Track has no waypoints"));
            return issues;
        }

        for (int i = 0; i < track->waypoints.size() - 1; i++) {
            const auto& w1 = track->waypoints[i];
            const auto& w2 = track->waypoints[i + 1];

            float dx = w2.x - w1.x;
            float dy = w2.y - w1.y;
            float dz = w2.z - w1.z;
            float dist = sqrt(dx*dx + dy*dy + dz*dz);

            if (dist > 50.0f) {
                issues.append(KsValidationIssue(Validate_Warning, "Track", "Large gap between waypoints: " + QString::number(dist), "", i));
            }
        }

        return issues;
    }
};

class KsReporter {
public:
    static QString reportToHTML(const KsValidationResult& result) {
        QString html = "<html><head><title>AC Validation Report</title></head><body>";
        html += "<h1>Validation Report</h1>";
        html += "<p>" + result.getSummary() + "</p>";

        if (!result.issues.isEmpty()) {
            html += "<table border='1'><tr><th>Level</th><th>Category</th><th>Message</th><th>File</th><th>Line</th><th>Suggestion</th></tr>";

            for (const auto& issue : result.issues) {
                QString level;
                if (issue.level == Validate_Critical) level = "Critical";
                else if (issue.level == Validate_Error) level = "Error";
                else level = "Warning";

                html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                    .arg(level).arg(issue.category).arg(issue.message)
                    .arg(issue.file).arg(issue.line).arg(issue.suggestion);
            }

            html += "</table>";
        }

        html += "</body></html>";
        return html;
    }

    static QString reportToText(const KsValidationResult& result) {
        QString text = "=== Validation Report ===\n";
        text += result.getSummary() + "\n\n";

        for (const auto& issue : result.issues) {
            QString level;
            if (issue.level == Validate_Critical) level = "[CRITICAL]";
            else if (issue.level == Validate_Error) level = "[ERROR]";
            else level = "[WARNING]";

            text += QString("%1 %2: %3\n").arg(level).arg(issue.category).arg(issue.message);

            if (!issue.file.isEmpty()) {
                text += QString("  File: %1\n").arg(issue.file);
            }
            if (issue.line > 0) {
                text += QString("  Line: %1\n").arg(issue.line);
            }
            if (!issue.suggestion.isEmpty()) {
                text += QString("  Suggestion: %1\n").arg(issue.suggestion);
            }
            text += "\n";
        }

        return text;
    }

    static QString reportToJSON(const KsValidationResult& result) {
        QString json = "{\n";
        json += "  \"summary\": {\n";
        json += QString("    \"critical\": %1,\n").arg(result.criticalCount);
        json += QString("    \"errors\": %1,\n").arg(result.errorCount);
        json += QString("    \"warnings\": %1,\n").arg(result.warningCount);
        json += QString("    \"passed\": %1\n").arg(result.passed ? "true" : "false");
        json += "  },\n";

        json += "  \"issues\": [\n";
        for (int i = 0; i < result.issues.size(); i++) {
            const auto& issue = result.issues[i];
            json += "    {\n";
            json += QString("      \"level\": %1,\n").arg(issue.level);
            json += QString("      \"category\": \"%1\",\n").arg(issue.category);
            json += QString("      \"message\": \"%1\",\n").arg(issue.message.replace("\"", "\\\""));
            json += QString("      \"file\": \"%1\",\n").arg(issue.file);
            json += QString("      \"line\": %1,\n").arg(issue.line);
            json += QString("      \"suggestion\": \"%1\"\n").arg(issue.suggestion.replace("\"", "\\\""));
            json += "    }";
            if (i < result.issues.size() - 1) json += ",";
            json += "\n";
        }
        json += "  ]\n";
        json += "}\n";

        return json;
    }
};
}

#endif

#endif // KS_ASSETTOCORSA_CORE_H
