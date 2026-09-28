#pragma once
#include <string>
namespace ks { namespace engine { namespace config {
class CspConfigParser {
public:
    static CspConfigParser& instance() { static CspConfigParser s; return s; }
    bool parse(const std::string&) { return false; }
};
}}} // namespace
