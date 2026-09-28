#pragma once
#include <string>
#include <unordered_map>
namespace ks { namespace engine { namespace sys {
class SettingsManager {
public:
    static SettingsManager& instance() { static SettingsManager s; return s; }
    void set(const std::string& k, const std::string& v) { m_[k]=v; }
    std::string get(const std::string& k, const std::string& def="") const {
        auto it=m_.find(k); return it==m_.end()?def:it->second;
    }
private:
    std::unordered_map<std::string,std::string> m_;
};
}}} // namespace
