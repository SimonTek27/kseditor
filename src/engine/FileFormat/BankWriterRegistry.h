#pragma once
#include <string>
#include <memory>
#include "BankWriterInterface.h"
namespace ks { namespace engine { namespace fileformat {
class BankWriterRegistry {
public:
    static BankWriterRegistry& instance() { static BankWriterRegistry s; return s; }
    void registerAll() {}
    std::unique_ptr<BankWriterInterface> create(const std::string& /*hint*/) { return nullptr; }
};
}}} // namespace
