#pragma once
#include <cstdint>
namespace ks { namespace engine { namespace fileformat {
enum class BankVersion : uint32_t { Unknown = 0, Fmod1x = 1, Fmod2x = 2 };
}}} // namespace
