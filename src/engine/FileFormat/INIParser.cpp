#include "INIParser.h"
#include <fstream>
namespace ks { namespace engine { namespace fileformat {
bool INIParser::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;
    std::string line, section;
    while (std::getline(in, line)) {
        if (line.empty() || line[0]==';' || line[0]==';') continue;
        if (line.front()=='[' && line.back()==']') { section = line.substr(1, line.size()-2); continue; }
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        auto key = line.substr(0, eq);
        auto val = line.substr(eq+1);
        while (!key.empty() && (key.back()==' '||key.back()=='\t')) key.pop_back();
        while (!val.empty() && (val.front()==' '||val.front()=='\t')) val.erase(val.begin());
        m_data[section][key] = val;
    }
    return true;
}
std::string INIParser::get(const std::string& section, const std::string& key, const std::string& def) const {
    auto sit = m_data.find(section);
    if (sit == m_data.end()) return def;
    auto kit = sit->second.find(key);
    return kit == sit->second.end() ? def : kit->second;
}
void INIParser::set(const std::string& section, const std::string& key, const std::string& value) {
    m_data[section][key] = value;
}
}}} // namespace
