#pragma once
#include <string>
namespace ks { namespace engine { namespace sys {
class DatabaseManager {
public:
    static DatabaseManager& instance() { static DatabaseManager s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
