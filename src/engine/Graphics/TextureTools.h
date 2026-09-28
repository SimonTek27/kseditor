#pragma once
/**
 * Texture tools facade (std only).
 * Commercial-title livery/DDS tooling belongs in adapters or editor tools.
 */
#include <string>
#include <vector>
#include <cstdint>

namespace ks {

enum class KsTextureFormat {
    DXT1,
    DXT5,
    DXT5_RG,
    BC7,
    Uncompressed
};

struct KsNormalMapSettings {
    bool convertToRG = true;
    bool swapGreen = false;
    bool invertRed = false;
    bool invertGreen = true;
    float strength = 1.0f;
    bool generateMipmaps = true;
};

struct TextureBuffer {
    int width = 0;
    int height = 0;
    int channels = 4;
    std::vector<uint8_t> pixels;
};

class KsTextureTools {
public:
    static bool saveRaw(const TextureBuffer& /*buf*/, const std::string& /*path*/) { return false; }
    static TextureBuffer loadRaw(const std::string& /*path*/) { return {}; }
    static bool saveAsDDS(const TextureBuffer& /*buf*/, const std::string& /*path*/, KsTextureFormat) {
        return false;
    }
};

} // namespace ks
