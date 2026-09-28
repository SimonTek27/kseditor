#pragma once
#include <string>
#include "BankParserInterface.h"
namespace ks { namespace engine { namespace fileformat {
class BankParser : public BankParserInterface {
public:
    bool open(const std::string& /*path*/) override { return false; }
};
}}} // namespace
