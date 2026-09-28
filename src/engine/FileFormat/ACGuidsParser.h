#pragma once
/** Redirect: AC GUIDs are content adapters, not core FileFormat. */
#include "../../adapters/assetto_corsa/ACGuidsParser.h"
namespace ks { namespace engine { namespace fileformat {
using ACGuidsParser = ks::adapters::assetto_corsa::ACGuidsParser;
}}} // namespace
