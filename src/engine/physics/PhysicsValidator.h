#pragma once
#include <string>
#include <vector>
namespace ks { namespace physics {
struct PhysicsValidationIssue { std::string message; };
class PhysicsValidator {
public:
    bool validate() { return true; }
    const std::vector<PhysicsValidationIssue>& issues() const { return m_issues; }
private:
    std::vector<PhysicsValidationIssue> m_issues;
};
}} // namespace
