#pragma once
#include <string>
namespace ks { namespace engine { namespace fileformat {
class BankParserInterface {
public:
    virtual ~BankParserInterface() = default;
    virtual bool open(const std::string& path) = 0;
};
}}} // namespace
