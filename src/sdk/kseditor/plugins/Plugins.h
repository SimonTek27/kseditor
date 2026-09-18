#pragma once

#include "base/PluginBase.h"
#include "simulators/kunos/KsPlugin.h"

namespace ks {
namespace plugins {

inline PluginManagerBase* manager() {
    return PluginManagerBase::instance();
}

inline kunos::KsPlugin* ks() {
    return kunos::KsPlugin::instance();
}

}
}