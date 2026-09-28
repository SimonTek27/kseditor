#pragma once
#include <string>
namespace ks { namespace engine { namespace fileformat {
class JSONParser {
public:
    bool load(const std::string& /*path*/) { return false; }
    std::string raw() const { return m_raw; }
private:
    std::string m_raw;
};
}}} // namespace
