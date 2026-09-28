#include "AudioUtilities.h"
#include <cmath>
namespace ks { namespace engine { namespace audio {
float AudioUtilities::dbToLin(float db) { return std::pow(10.f, db / 20.f); }
float AudioUtilities::linToDb(float lin) { return lin <= 1e-8f ? -160.f : 20.f * std::log10(lin); }
}}} // namespace
