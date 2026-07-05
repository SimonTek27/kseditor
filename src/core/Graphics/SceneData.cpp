#include "SceneData.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QtMath>

namespace Ks {

QVector3D SceneNode::translation() const {
    return localTransform.column(3).toVector3D();
}

QVector3D SceneNode::scale() const {
    return {
        localTransform.column(0).toVector3D().length(),
        localTransform.column(1).toVector3D().length(),
        localTransform.column(2).toVector3D().length()
    };
}

QVector3D SceneNode::rotation() const {
    QVector3D s = scale();
    if (s.x() == 0 || s.y() == 0 || s.z() == 0) return {};

    // Normalize the rotation matrix columns to extract pure rotation
    QMatrix4x4 rotMat = localTransform;
    rotMat.setColumn(0, QVector4D(localTransform.column(0).toVector3D() / s.x(), 0));
    rotMat.setColumn(1, QVector4D(localTransform.column(1).toVector3D() / s.y(), 0));
    rotMat.setColumn(2, QVector4D(localTransform.column(2).toVector3D() / s.z(), 0));
    rotMat.setColumn(3, QVector4D(0, 0, 0, 1));

    float r00 = rotMat(0,0), r10 = rotMat(1,0), r20 = rotMat(2,0);
    float r01 = rotMat(0,1), r11 = rotMat(1,1), r21 = rotMat(2,1);
    float r02 = rotMat(0,2), r12 = rotMat(1,2), r22 = rotMat(2,2);

    float pitch, yaw, roll;

    // Handle gimbal lock (pitch near +/-90 degrees)
    const float eps = 0.9999999f;
    if (qAbs(r20) < eps) {
        pitch = qAtan2(-r20, qSqrt(r00 * r00 + r10 * r10));
        yaw   = qAtan2(r10, r00);
        roll  = qAtan2(r21, r22);
    } else {
        pitch = r20 > 0 ? -static_cast<float>(M_PI) / 2.0f : static_cast<float>(M_PI) / 2.0f;
        yaw   = qAtan2(-r01, r11);
        roll  = 0;
    }

    return { qRadiansToDegrees(pitch), qRadiansToDegrees(yaw), qRadiansToDegrees(roll) };
}

void SceneNode::setTranslation(const QVector3D& t) {
    localTransform.setColumn(3, QVector4D(t, 1.0f));
}

void SceneNode::setRotation(const QVector3D& euler) {
    QVector3D s = scale();
    QMatrix4x4 rot;
    rot.rotate(euler.x(), {1,0,0});
    rot.rotate(euler.y(), {0,1,0});
    rot.rotate(euler.z(), {0,0,1});
    QVector3D tr = translation();

    localTransform = QMatrix4x4();
    localTransform.translate(tr);
    localTransform = localTransform * rot;
    localTransform.scale(s);
}

void SceneNode::setScale(const QVector3D& s) {
    // Preserve rotation by rebuilding the full transform
    QVector3D tr = translation();
    QVector3D euler = rotation();
    QMatrix4x4 rot;
    rot.rotate(euler.x(), {1,0,0});
    rot.rotate(euler.y(), {0,1,0});
    rot.rotate(euler.z(), {0,0,1});

    localTransform = QMatrix4x4();
    localTransform.translate(tr);
    localTransform = localTransform * rot;
    localTransform.scale(s);
}

QMatrix4x4 SceneNode::worldTransform() const {
    if (!parent) return localTransform;
    return parent->worldTransform() * localTransform;
}

QMap<QString, ShaderDef> ShaderDef::loadAllFromDirectory(const QString& htmlDir)
{
    QMap<QString, ShaderDef> result;
    QDir dir(htmlDir);
    if (!dir.exists()) return result;

    for (const QString& shaderName : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString htmlPath = dir.filePath(shaderName + "/" + shaderName + ".html");
        QFile f(htmlPath);
        if (!f.open(QFile::ReadOnly)) continue;

        QString html = QString::fromLatin1(f.readAll());
        f.close();

        ShaderDef def;
        def.name = shaderName;

        QRegularExpression rAlpha(R"(Is Alpha Tested.*?<td>(\d))");
        auto m = rAlpha.match(html);
        if (m.hasMatch()) def.isAlphaTested = m.captured(1) == "1";

        QRegularExpression rSkin(R"(Is Skinned.*?<td>(\d))");
        m = rSkin.match(html);
        if (m.hasMatch()) def.isSkinned = m.captured(1) == "1";

        QRegularExpression rPart(R"(Is Particle.*?<td>(\d))");
        m = rPart.match(html);
        if (m.hasMatch()) def.isParticle = m.captured(1) == "1";

        QRegularExpression rPS(R"(PS Model.*?<td>(ps_\d+_\d+))");
        m = rPS.match(html);
        if (m.hasMatch()) def.psModel = m.captured(1);

        QRegularExpression rVS(R"(VS Model.*?<td>(vs_\d+_\d+))");
        m = rVS.match(html);
        if (m.hasMatch()) def.vsModel = m.captured(1);

        QRegularExpression rParam(R"(-> (ks\w+|angle\w*|colour\w*|boh\w*|frequency\w*|distortion\w*|bloomLevel\w*|baseLevel\w*|threshold\w*|speed\w*|quality\w*|amount\w*|gain\w*|refHeight\w*|heightGain\w*|emissiveBlend\w*|minDistance\w*|deltaT\w*|LensCenter\w*|ScreenCenter\w*|Scale\w*|bones\w*)\s*</p>\s*(.*?)<p)");
        rParam.setPatternOptions(QRegularExpression::DotMatchesEverythingOption);
        auto it = rParam.globalMatch(html);
        while (it.hasNext()) {
            auto match = it.next();
            ShaderParam p;
            p.name = match.captured(1);
            QString desc = match.captured(2);
            desc.remove(QRegularExpression("<[^>]+>"));
            p.description = desc.simplified();

            QRegularExpression rRange(R"(\[([0-9\-\.]+),\s*([0-9\+inf\.]+)\))");
            auto rm = rRange.match(desc);
            if (rm.hasMatch()) {
                p.rangeMin = rm.captured(1).toFloat();
                QString maxStr = rm.captured(2);
                p.rangeMax = maxStr == "+inf" ? 10.0f : maxStr.toFloat();
            }
            def.params.append(p);
        }

        QRegularExpression rTex(R"(TEXTURE\s+name:\s+(\w+)\s*Notes\s*:\s*(.*?)</p>)");
        rTex.setPatternOptions(QRegularExpression::DotMatchesEverythingOption);
        auto ti = rTex.globalMatch(html);
        while (ti.hasNext()) {
            auto match = ti.next();
            ShaderTexture t;
            t.name  = match.captured(1);
            t.notes = match.captured(2).simplified();
            t.notes.remove(QRegularExpression("<[^>]+>"));
            def.textures.append(t);
        }

        result.insert(shaderName, def);
    }

    return result;
}

void Scene::clear() {
    name.clear();
    filePath.clear();
    root.reset();
    materials.clear();
    shaderDefs.clear();
    physicsBodies.clear();
    soundEmitters.clear();
    isDirty = false;
}

} // namespace Ks