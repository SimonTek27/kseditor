#pragma once
#include <string>
namespace ks { namespace engine { namespace fileformat {
class BankWriterInterface {
public:
    virtual ~BankWriterInterface() = default;
    virtual bool write(const std::string& path) = 0;
};
}}} // namespace
