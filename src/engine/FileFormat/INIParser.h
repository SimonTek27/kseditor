#pragma once
#include <string>
#include <unordered_map>
namespace ks { namespace engine { namespace fileformat {
class INIParser {
public:
    bool load(const std::string& path);
    std::string get(const std::string& section, const std::string& key, const std::string& def = "") const;
    void set(const std::string& section, const std::string& key, const std::string& value);
private:
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_data;
};
}}} // namespace
