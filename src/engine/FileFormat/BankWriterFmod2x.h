#pragma once
#include "BankWriterInterface.h"
namespace ks { namespace engine { namespace fileformat {
class BankWriterFmod2x : public BankWriterInterface {
public:
    bool write(const std::string& /*path*/) override { return false; }
};
}}} // namespace
