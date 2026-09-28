#pragma once
#include <string>
#include <vector>
namespace ks { namespace engine { namespace fileformat {
struct ValidationIssue {
    std::string path;
    std::string message;
};
class FormatValidator {
public:
    bool validate(const std::string& /*path*/) { return true; }
    const std::vector<ValidationIssue>& issues() const { return m_issues; }
private:
    std::vector<ValidationIssue> m_issues;
};
}}} // namespace
