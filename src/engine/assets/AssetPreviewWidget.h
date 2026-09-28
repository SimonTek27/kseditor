#pragma once
/** Asset preview is editor-only; runtime uses NativeRenderer. */
#include <string>

namespace ks {
namespace engine {
namespace assets {

class AssetPreview {
public:
    void setPath(const std::string& p) { m_path = p; }
    const std::string& path() const { return m_path; }
private:
    std::string m_path;
};

} // namespace assets
} // namespace engine
} // namespace ks
