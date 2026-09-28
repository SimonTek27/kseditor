#pragma once
#include <string>
namespace ks { namespace engine { namespace fileformat {
class PublishValidatorBridge {
public:
    bool validate(const std::string& /*path*/) { return true; }
};
}}} // namespace
