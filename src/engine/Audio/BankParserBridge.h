#pragma once
#include <string>
namespace ks { namespace engine { namespace audio {
class BankParserBridge {
public:
    static BankParserBridge& instance() { static BankParserBridge s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
