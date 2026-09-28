#pragma once
/** Real INI-style config loader (Qt-free). */
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>

namespace ks {
namespace engine {
namespace config {

class ConfigLoader {
public:
    static ConfigLoader& instance() {
        static ConfigLoader s;
        return s;
    }

    bool load(const std::string& path) {
        std::ifstream in(path);
        if (!in) return false;
        m_path = path;
        m_vals.clear();
        std::string line, section;
        while (std::getline(in, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
                line.pop_back();
            if (line.empty() || line[0] == ';' || line[0] == '#' || line[0] == '/')
                continue;
            if (line.front() == '[' && line.back() == ']') {
                section = line.substr(1, line.size() - 2);
                continue;
            }
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            auto trim = [](std::string& s) {
                while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(s.begin());
                while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
            };
            trim(key);
            trim(val);
            auto sc = val.find(';');
            if (sc != std::string::npos) val = val.substr(0, sc);
            trim(val);
            if (!section.empty())
                m_vals[section + "/" + key] = val;
            m_vals[key] = val;
        }
        return true;
    }

    std::string get(const std::string& key, const std::string& def = "") const {
        auto it = m_vals.find(key);
        return it == m_vals.end() ? def : it->second;
    }

    float getFloat(const std::string& key, float def = 0.f) const {
        auto it = m_vals.find(key);
        if (it == m_vals.end()) return def;
        try { return std::stof(it->second); } catch (...) { return def; }
    }

    int getInt(const std::string& key, int def = 0) const {
        auto it = m_vals.find(key);
        if (it == m_vals.end()) return def;
        try { return std::stoi(it->second); } catch (...) { return def; }
    }

    void set(const std::string& key, const std::string& val) { m_vals[key] = val; }

    bool save(const std::string& path = {}) const {
        const std::string outPath = path.empty() ? m_path : path;
        if (outPath.empty()) return false;
        std::ofstream out(outPath);
        if (!out) return false;
        for (const auto& kv : m_vals) {
            if (kv.first.find('/') != std::string::npos) continue; // skip section/key dups
            out << kv.first << '=' << kv.second << '\n';
        }
        return true;
    }

    const std::string& path() const { return m_path; }
    size_t size() const { return m_vals.size(); }

private:
    std::string m_path;
    std::unordered_map<std::string, std::string> m_vals;
};

} // namespace config
} // namespace engine
} // namespace ks
