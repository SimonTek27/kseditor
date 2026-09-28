#pragma once
#include <string>
namespace ks { namespace engine { namespace sys {
class StateMachine {
public:
    static StateMachine& instance() { static StateMachine s; return s; }
    void setState(const std::string& s) { m_state = s; }
    std::string state() const { return m_state; }
private:
    std::string m_state;
};
}}} // namespace
