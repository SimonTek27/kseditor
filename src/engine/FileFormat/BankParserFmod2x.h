#pragma once
#include "BankParserInterface.h"
namespace ks { namespace engine { namespace fileformat {
class BankParserFmod2x : public BankParserInterface {
public:
    bool open(const std::string& /*path*/) override { return false; }
};
}}} // namespace
