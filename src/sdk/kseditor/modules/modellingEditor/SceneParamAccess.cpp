#include "SceneParamAccess.h"

namespace ks {

namespace {
float component(const QVector3D& v, QChar c)
{
    if (c == 'x') return v.x();
    if (c == 'y') return v.y();
    return v.z();
}
QVector3D setComponent(QVector3D v, QChar c, float value)
{
    if (c == 'x') v.setX(value);
    else if (c == 'y') v.setY(value);
    else v.setZ(value);
    return v;
}
} // namespace

bool sceneParamRead(SceneObject* obj, const QString& channel, float& outValue)
{
    if (!obj) return false;
    if (channel.startsWith(QStringLiteral("position.")))
        outValue = component(obj->position(), channel.at(channel.size() - 1));
    else if (channel.startsWith(QStringLiteral("rotation.")))
        outValue = component(obj->rotationEuler(), channel.at(channel.size() - 1));
    else if (channel.startsWith(QStringLiteral("scale.")))
        outValue = component(obj->scale(), channel.at(channel.size() - 1));
    else if (channel == QLatin1String("visibility"))
        outValue = obj->isVisible() ? 1.0f : 0.0f;
    else if (channel == QLatin1String("opacity"))
        outValue = obj->opacity();
    else if (channel == QLatin1String("metallic"))
        outValue = obj->metallic();
    else if (channel == QLatin1String("roughness"))
        outValue = obj->roughness();
    else
        return false;
    return true;
}

bool sceneParamWrite(SceneObject* obj, const QString& channel, float value)
{
    if (!obj) return false;
    if (channel.startsWith(QStringLiteral("position.")))
        obj->setPosition(setComponent(obj->position(), channel.at(channel.size() - 1), value));
    else if (channel.startsWith(QStringLiteral("rotation.")))
        obj->setRotationEuler(setComponent(obj->rotationEuler(), channel.at(channel.size() - 1), value));
    else if (channel.startsWith(QStringLiteral("scale.")))
        obj->setScale(setComponent(obj->scale(), channel.at(channel.size() - 1), value));
    else if (channel == QLatin1String("visibility"))
        obj->setVisible(value > 0.5f);
    else if (channel == QLatin1String("opacity"))
        obj->setOpacity(qBound(0.0f, value, 1.0f));
    else if (channel == QLatin1String("metallic"))
        obj->setMetallic(qBound(0.0f, value, 1.0f));
    else if (channel == QLatin1String("roughness"))
        obj->setRoughness(qBound(0.0f, value, 1.0f));
    else
        return false;
    return true;
}

} // namespace ks