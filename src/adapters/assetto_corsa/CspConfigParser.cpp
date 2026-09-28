#include "CspConfigParser.h"
#include <fstream>

namespace ks {
namespace adapters {
namespace assetto_corsa {

bool CspConfigParser::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;
    m_vals.clear();
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        auto key = line.substr(0, eq);
        auto val = line.substr(eq + 1);
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
        while (!val.empty() && (val.front() == ' ' || val.front() == '\t')) val.erase(val.begin());
        m_vals[key] = val;
    }
    return true;
}

std::string CspConfigParser::get(const std::string& key, const std::string& def) const {
    auto it = m_vals.find(key);
    return it == m_vals.end() ? def : it->second;
}

void CspConfigParser::set(const std::string& key, const std::string& value) {
    m_vals[key] = value;
}

} // namespace assetto_corsa
} // namespace adapters
} // namespace ks
