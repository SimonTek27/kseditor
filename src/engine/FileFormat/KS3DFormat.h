#pragma once
#include <string>
#include <cstdint>
namespace ks { namespace engine { namespace fileformat {
struct KS3DHeader {
    char magic[4] = {'K','S','3','D'};
    uint32_t version = 1;
};
}}} // namespace
