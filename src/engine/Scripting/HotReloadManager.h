#pragma once
#include <string>
#include <functional>
#include <vector>

namespace ks {
namespace scripting {

class HotReloadManager {
public:
    static HotReloadManager& instance() { static HotReloadManager s; return s; }
    void watch(const std::string& /*path*/) {}
    void update() {}
    void onFileChanged(std::function<void(const std::string&)> cb) { m_cbs.push_back(std::move(cb)); }
private:
    std::vector<std::function<void(const std::string&)>> m_cbs;
};

} // namespace scripting
} // namespace ks
