#pragma once
#include <string>
namespace ks { namespace engine { namespace config {
class PPFilterPreset {
public:
    static PPFilterPreset& instance() { static PPFilterPreset s; return s; }
    bool load(const std::string&) { return false; }
};
}}} // namespace
