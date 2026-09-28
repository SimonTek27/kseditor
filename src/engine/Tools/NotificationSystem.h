#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class NotificationSystem {
public:
    static NotificationSystem& instance() { static NotificationSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
