#pragma once
#include <memory>
#include <string>
#include "BankParserInterface.h"
namespace ks { namespace engine { namespace fileformat {
class BankParserFactory {
public:
    static std::unique_ptr<BankParserInterface> create(const std::string& /*path*/) { return nullptr; }
};
}}} // namespace
