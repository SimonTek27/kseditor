#pragma once
/** Qt-free stub — original in git history. */
#include <string>
namespace ks { namespace engine { namespace tools {
class BackupSystem {
public:
    static BackupSystem& instance() { static BackupSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
