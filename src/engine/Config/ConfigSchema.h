#pragma once
#include <string>
namespace ks { namespace engine { namespace config {
class ConfigSchema {
public:
    static ConfigSchema& instance() { static ConfigSchema s; return s; }
    bool validate(const std::string&) { return true; }
};
}}} // namespace
