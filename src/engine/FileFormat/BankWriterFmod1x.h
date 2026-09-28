#pragma once
#include "BankWriterInterface.h"
namespace ks { namespace engine { namespace fileformat {
class BankWriterFmod1x : public BankWriterInterface {
public:
    bool write(const std::string& /*path*/) override { return false; }
};
}}} // namespace
