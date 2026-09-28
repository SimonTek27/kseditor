#pragma once
#include <string>
#include <memory>
#include "BankParserInterface.h"
namespace ks { namespace engine { namespace fileformat {
class BankParserRegistry {
public:
    static BankParserRegistry& instance() { static BankParserRegistry s; return s; }
    void registerAll() {}
    std::unique_ptr<BankParserInterface> create(const std::string& /*path*/) { return nullptr; }
};
}}} // namespace
