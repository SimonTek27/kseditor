#pragma once

#include <cctype>
#include <string>
#include <vector>

#include "LogManager.h"

namespace ks::str {

namespace detail {

inline void collectValues(std::vector<std::string>&) {}

template <typename T, typename... Rest>
void collectValues(std::vector<std::string>& out, const T& value, const Rest&... rest) {
    out.push_back(ks::log::toText(value));
    collectValues(out, rest...);
}

}  // namespace detail

// Qt-style "%1"/"%2" placeholder substitution. Replaces QString::arg() in the
// log and validation helpers: every placeholder is substituted with the
// matching argument (1-based), a placeholder followed by more digits ("%10")
// is never matched by "%1".
template <typename... Args>
std::string format(const std::string& pattern, const Args&... args) {
    std::vector<std::string> values;
    detail::collectValues(values, args...);

    std::string out = pattern;
    for (size_t i = 0; i < values.size(); ++i) {
        const std::string marker = "%" + std::to_string(i + 1);
        size_t pos = 0;
        while ((pos = out.find(marker, pos)) != std::string::npos) {
            const size_t after = pos + marker.size();
            if (after < out.size() && std::isdigit(static_cast<unsigned char>(out[after]))) {
                pos = after;
                continue;
            }
            out.replace(pos, marker.size(), values[i]);
            pos += values[i].size();
        }
    }
    return out;
}

template <typename... Args>
std::string format(const char* pattern, const Args&... args) {
    return format(std::string(pattern), args...);
}

}  // namespace ks::str
