#include "ModelerContext.h"

namespace ks {

ModelerContext* ModelerContext::s_instance = nullptr;

ModelerContext::ModelerContext(QObject* parent)
    : QObject(parent)
{
    m_toolsByType[TypeCar] = {"Select", "Move", "Rotate", "Scale", "Paint", "Animate"};
    m_toolsByType[TypeTrack] = {"Select", "Move", "Terrain", "Road", "Vegetation"};
    m_toolsByType[TypeCharacter] = {"Select", "Pose", "Cloth", "Skin"};

    m_toolsByMode[ModeSelect] = {"Select", "BoxSelect", "Lasso"};
    m_toolsByMode[ModeEdit] = {"Move", "Rotate", "Scale", "Extrude", "LoopCut"};
    m_toolsByMode[ModePaint] = {"Paint", "Erase", "Smudge", "Clone"};
    m_toolsByMode[ModeAnimate] = {"Keyframe", "Timeline", "Graph"};
}

ModelerContext* ModelerContext::instance()
{
    if (!s_instance) {
        s_instance = new ModelerContext();
    }
    return s_instance;
}

QStringList ModelerContext::getToolsForType(EditorType type) const
{
    return m_toolsByType.value(type);
}

QStringList ModelerContext::getToolsForMode(EditMode mode) const
{
    return m_toolsByMode.value(mode);
}

bool ModelerContext::isToolValid(const QString& tool) const
{
    for (const auto& list : m_toolsByType.values()) {
        if (list.contains(tool)) return true;
    }
    for (const auto& list : m_toolsByMode.values()) {
        if (list.contains(tool)) return true;
    }
    return false;
}

} // namespace ks
