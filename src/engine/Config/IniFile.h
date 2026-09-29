#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace ks {
namespace config {

// Minimal INI reader/writer replacing QSettings(IniFormat). Groups and keys are
// kept in file order and matched case-insensitively, so loading and saving a
// file keeps it readable and keeps comments/unknown lines structure intact.
class IniFile {
public:
    struct Entry {
        std::string key;
        std::string value;
    };
    struct Group {
        std::string name;
        std::vector<Entry> entries;
    };

    IniFile() = default;

    bool load(const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;
        m_path = path;
        m_groups.clear();

        std::string line;
        std::string currentGroup;
        bool haveGroup = false;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const std::string trimmed = trim(line);
            if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') continue;
            if (trimmed.front() == '[' && trimmed.back() == ']') {
                currentGroup = trim(trimmed.substr(1, trimmed.size() - 2));
                haveGroup = true;
                groupFor(currentGroup);
                continue;
            }
            const std::size_t sep = trimmed.find_first_of("=:");
            if (sep == std::string::npos) continue;
            const std::string key = trim(trimmed.substr(0, sep));
            const std::string value = unquote(trim(trimmed.substr(sep + 1)));
            if (key.empty()) continue;
            set(haveGroup ? currentGroup : std::string(), key, value);
        }
        return true;
    }

    bool save(const std::string& path) const {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        bool first = true;
        for (const Group& group : m_groups) {
            if (!first) out << "\n";
            first = false;
            if (!group.name.empty()) out << "[" << group.name << "]\n";
            for (const Entry& entry : group.entries) {
                out << entry.key << "=" << entry.value << "\n";
            }
        }
        return static_cast<bool>(out);
    }

    const std::string& path() const { return m_path; }
    const std::vector<Group>& groups() const { return m_groups; }

    bool contains(const std::string& groupName, const std::string& key) const {
        const Group* group = findGroup(groupName);
        return group && findEntry(*group, key);
    }

    std::string getString(const std::string& groupName, const std::string& key,
                          const std::string& defaultValue = std::string()) const {
        const Group* group = findGroup(groupName);
        if (!group) return defaultValue;
        const Entry* entry = findEntry(*group, key);
        return entry ? entry->value : defaultValue;
    }

    double getDouble(const std::string& groupName, const std::string& key,
                     double defaultValue = 0.0) const {
        const std::string text = rawString(groupName, key);
        if (text.empty()) return defaultValue;
        char* end = nullptr;
        const double value = std::strtod(text.c_str(), &end);
        return (end && *end == '\0') ? value : defaultValue;
    }

    int getInt(const std::string& groupName, const std::string& key, int defaultValue = 0) const {
        const std::string text = rawString(groupName, key);
        if (text.empty()) return defaultValue;
        char* end = nullptr;
        const long value = std::strtol(text.c_str(), &end, 10);
        if (!end || *end != '\0') return defaultValue;
        return static_cast<int>(value);
    }

    bool getBool(const std::string& groupName, const std::string& key,
                 bool defaultValue = false) const {
        const std::string text = toLower(rawString(groupName, key));
        if (text.empty()) return defaultValue;
        if (text == "true" || text == "yes" || text == "on") return true;
        if (text == "false" || text == "no" || text == "off") return false;
        char* end = nullptr;
        const double value = std::strtod(text.c_str(), &end);
        if (end && *end == '\0') return value != 0.0;
        return defaultValue;
    }

    void set(const std::string& groupName, const std::string& key, const std::string& value) {
        Group& group = groupFor(groupName);
        if (Entry* entry = findEntry(group, key)) {
            entry->value = value;
            return;
        }
        group.entries.push_back(Entry{key, value});
    }

    void setNumber(const std::string& groupName, const std::string& key, double value) {
        set(groupName, key, numberToString(value));
    }

    void setBool(const std::string& groupName, const std::string& key, bool value) {
        set(groupName, key, value ? "1" : "0");
    }

    void remove(const std::string& groupName, const std::string& key) {
        Group* group = findGroup(groupName);
        if (!group) return;
        for (auto it = group->entries.begin(); it != group->entries.end(); ++it) {
            if (equals(it->key, key)) {
                group->entries.erase(it);
                return;
            }
        }
    }

    static std::string numberToString(double value) {
        if (value == static_cast<long long>(value) &&
            value >= -9.0e15 && value <= 9.0e15) {
            return std::to_string(static_cast<long long>(value));
        }
        std::ostringstream out;
        out << std::defaultfloat << std::setprecision(7) << static_cast<float>(value);
        return out.str();
    }

    static std::string trim(const std::string& text) {
        const std::size_t first = text.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return std::string();
        const std::size_t last = text.find_last_not_of(" \t\r\n");
        return text.substr(first, last - first + 1);
    }

private:
    static bool equals(const std::string& a, const std::string& b) {
        if (a.size() != b.size()) return false;
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(a[i])) !=
                std::tolower(static_cast<unsigned char>(b[i]))) {
                return false;
            }
        }
        return true;
    }

    static std::string toLower(const std::string& text) {
        std::string lowered = text;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return lowered;
    }

    static std::string unquote(const std::string& value) {
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            return value.substr(1, value.size() - 2);
        }
        return value;
    }

    std::string rawString(const std::string& groupName, const std::string& key) const {
        const Group* group = findGroup(groupName);
        if (!group) return std::string();
        const Entry* entry = findEntry(*group, key);
        return entry ? entry->value : std::string();
    }

    Group* findGroup(const std::string& name) {
        for (Group& group : m_groups) {
            if (equals(group.name, name)) return &group;
        }
        return nullptr;
    }

    const Group* findGroup(const std::string& name) const {
        for (const Group& group : m_groups) {
            if (equals(group.name, name)) return &group;
        }
        return nullptr;
    }

    static const Entry* findEntry(const Group& group, const std::string& key) {
        for (const Entry& entry : group.entries) {
            if (equals(entry.key, key)) return &entry;
        }
        return nullptr;
    }

    static Entry* findEntry(Group& group, const std::string& key) {
        for (Entry& entry : group.entries) {
            if (equals(entry.key, key)) return &entry;
        }
        return nullptr;
    }

    Group& groupFor(const std::string& name) {
        if (Group* group = findGroup(name)) return *group;
        m_groups.push_back(Group{name, {}});
        return m_groups.back();
    }

    std::string m_path;
    std::vector<Group> m_groups;
};

}  // namespace config
}  // namespace ks
