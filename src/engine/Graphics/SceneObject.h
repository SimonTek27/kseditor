#pragma once
#include "GfxTypes.h"
#include <string>

namespace ks {
namespace engine {
namespace graphics {

class SceneObject {
public:
    SceneObject() = default;
    explicit SceneObject(std::string name) : m_name(std::move(name)) {}

    void setName(const std::string& n) { m_name = n; }
    const std::string& name() const { return m_name; }

    void setTransform(const Mat4& t) { m_transform = t; }
    const Mat4& transform() const { return m_transform; }

    void setVisible(bool v) { m_visible = v; }
    bool visible() const { return m_visible; }

private:
    std::string m_name;
    Mat4 m_transform;
    bool m_visible = true;
};

} // namespace graphics
} // namespace engine
} // namespace ks
