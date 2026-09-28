#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class ImportExportFilters {
public:
    static ImportExportFilters& instance() { static ImportExportFilters s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
