#pragma once
#include <memory>
#include <string>
#include "BankWriterInterface.h"
namespace ks { namespace engine { namespace fileformat {
class BankWriterFactory {
public:
    static std::unique_ptr<BankWriterInterface> create(const std::string&) { return nullptr; }
};
}}} // namespace
