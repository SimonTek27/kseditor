#pragma once
#include <string>
#include <vector>

namespace ks {
namespace engine {
namespace assets {

class AssetSearchEngine {
public:
    static AssetSearchEngine& instance() { static AssetSearchEngine s; return s; }
    void setRoot(const std::string& root) { m_root = root; }
    std::vector<std::string> search(const std::string& /*query*/) const { return {}; }

private:
    std::string m_root;
};

} // namespace assets
} // namespace engine
} // namespace ks
