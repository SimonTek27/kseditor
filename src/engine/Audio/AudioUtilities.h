#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class AudioUtilities {
public:
    static float dbToLin(float db);
    static float linToDb(float lin);
};
}}} // namespace
