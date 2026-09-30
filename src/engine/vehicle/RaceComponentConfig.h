#pragma once
/**
 * Race / event component configuration for KN5-style assets.
 *
 * Concept (content pipeline, independent product identity):
 *   - The KN5 (or node catalog) holds the FULL vehicle mesh set (like a complete node list).
 *   - A sidecar INI selects which nodes are active for a given race / weekend.
 *
 * Naming convention (next to car.kn5):
 *   car_rd1.ini           — race / round 1 layout
 *   car_rd1-2.ini         — same layout used for rounds 1 and 2
 *   car_rd1-2-rd6.ini     — rounds 1,2 and 6
 *   car_rd3.ini           — round 3 only
 *
 * INI format (simple):
 *   [Nodes]
 *   GEO_WHEEL_LF=1
 *   GEO_WING_RACE=1
 *   GEO_WING_STOCK=0
 *   ; or list form:
 *   Active=GEO_WHEEL_LF,GEO_WING_RACE,COCKPIT
 *   Inactive=GEO_WING_STOCK
 *
 *   [Meta]
 *   Rounds=1,2,6
 *   Description=High downforce package
 */
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cctype>

namespace ks {
namespace vehicle {

struct RaceComponentConfig {
    std::string sourcePath;
    std::string description;
    std::vector<int> rounds;                 // parsed from filename and/or Meta
    std::unordered_set<std::string> active;  // nodes ON
    std::unordered_set<std::string> inactive; // nodes forced OFF
    bool hasExplicitList = false;

    bool isActive(const std::string& node) const {
        if (inactive.count(node)) return false;
        if (!hasExplicitList) return true; // no filter → all on
        if (active.empty()) return !inactive.count(node);
        return active.count(node) > 0;
    }
};

/** Full catalog = every node known for the car (ProView-like complete list). */
struct VehicleNodeCatalog {
    std::string kn5Path;
    std::vector<std::string> nodes; // ordered

    bool empty() const { return nodes.empty(); }
};

class RaceComponentConfigLoader {
public:
    /**
 * Parse rounds from filename stem suffix after last '_rd'.
 * Examples:
 *   car_rd1          → {1}
 *   car_rd1-2        → {1,2}
 *   car_rd1-2-rd6    → {1,2,6}
 *   car_rd3          → {3}
 */
    static std::vector<int> parseRoundsFromFilename(const std::string& pathOrStem) {
        std::string stem = pathOrStem;
        auto slash = stem.find_last_of("/\\"
                                       );
        if (slash != std::string::npos) stem = stem.substr(slash + 1);
        auto dot = stem.find_last_of('.');
        if (dot != std::string::npos) stem = stem.substr(0, dot);

        // find "_rd"
        auto pos = stem.find("_rd");
        if (pos == std::string::npos) return {};
        std::string tag = stem.substr(pos + 1); // rd1-2-rd6

        std::vector<int> rounds;
        std::string num;
        auto flush = [&]() {
            if (!num.empty()) {
                rounds.push_back(std::atoi(num.c_str()));
                num.clear();
            }
        };
        for (size_t i = 0; i < tag.size(); ++i) {
            char c = tag[i];
            if (std::isdigit(static_cast<unsigned char>(c))) {
                num.push_back(c);
            } else if (c == '-' || c == '_') {
                flush();
                // skip "rd" letters after separator
                if (i + 2 < tag.size() && tag[i + 1] == 'r' && tag[i + 2] == 'd')
                    i += 2;
            } else {
                flush();
            }
        }
        flush();
        // unique sorted
        std::sort(rounds.begin(), rounds.end());
        rounds.erase(std::unique(rounds.begin(), rounds.end()), rounds.end());
        return rounds;
    }

    static RaceComponentConfig loadIni(const std::string& path) {
        RaceComponentConfig cfg;
        cfg.sourcePath = path;
        cfg.rounds = parseRoundsFromFilename(path);

        std::ifstream in(path);
        if (!in) {
            std::fprintf(stderr, "RaceComponentConfig: cannot open %s\n", path.c_str());
            return cfg;
        }

        auto trim = [](std::string s) {
            while (!s.empty() && (unsigned char)s.front() <= ' ') s.erase(s.begin());
            while (!s.empty() && (unsigned char)s.back() <= ' ') s.pop_back();
            return s;
        };

        std::string line, section;
        while (std::getline(in, line)) {
            auto sc = line.find(';');
            if (sc != std::string::npos) line = line.substr(0, sc);
            auto hash = line.find('#');
            if (hash != std::string::npos) line = line.substr(0, hash);
            line = trim(line);
            if (line.empty()) continue;

            if (line.front() == '[' && line.back() == ']') {
                section = line.substr(1, line.size() - 2);
                for (char& c : section) c = static_cast<char>(std::tolower((unsigned char)c));
                continue;
            }

            auto eq = line.find('=');
            if (eq == std::string::npos) {
                // bare node name in [Nodes] → active
                if (section == "nodes" || section == "active") {
                    cfg.active.insert(line);
                    cfg.hasExplicitList = true;
                }
                continue;
            }

            std::string key = trim(line.substr(0, eq));
            std::string val = trim(line.substr(eq + 1));

            if (section == "meta") {
                if (key == "Description") cfg.description = val;
                else if (key == "Rounds") {
                    std::stringstream ss(val);
                    std::string part;
                    while (std::getline(ss, part, ',')) {
                        part = trim(part);
                        if (!part.empty()) cfg.rounds.push_back(std::atoi(part.c_str()));
                    }
                    std::sort(cfg.rounds.begin(), cfg.rounds.end());
                    cfg.rounds.erase(std::unique(cfg.rounds.begin(), cfg.rounds.end()), cfg.rounds.end());
                }
                continue;
            }

            // [Nodes] KEY=0|1  or Active=a,b,c
            if (key == "Active" || key == "Nodes") {
                std::stringstream ss(val);
                std::string part;
                while (std::getline(ss, part, ',')) {
                    part = trim(part);
                    if (!part.empty()) cfg.active.insert(part);
                }
                cfg.hasExplicitList = true;
                continue;
            }
            if (key == "Inactive" || key == "Disabled") {
                std::stringstream ss(val);
                std::string part;
                while (std::getline(ss, part, ',')) {
                    part = trim(part);
                    if (!part.empty()) cfg.inactive.insert(part);
                }
                cfg.hasExplicitList = true;
                continue;
            }

            // GEO_FOO=1 / 0
            int on = std::atoi(val.c_str());
            if (val == "true" || val == "on" || val == "yes") on = 1;
            if (val == "false" || val == "off" || val == "no") on = 0;
            if (on)
                cfg.active.insert(key);
            else
                cfg.inactive.insert(key);
            cfg.hasExplicitList = true;
        }

        std::fprintf(stderr, "RaceComponentConfig: %s active=%zu inactive=%zu rounds=%zu\n",
                     path.c_str(), cfg.active.size(), cfg.inactive.size(), cfg.rounds.size());
        return cfg;
    }

    /** Find best config file next to kn5 for a given round number. */
    static std::string findConfigPathForRound(const std::string& kn5Path, int round) {
        namespace fs = std::__fs::filesystem; // may not exist — use string ops
        std::string dir, stem;
        auto slash = kn5Path.find_last_of("/\\"
                                         );
        if (slash != std::string::npos) {
            dir = kn5Path.substr(0, slash + 1);
            stem = kn5Path.substr(slash + 1);
        } else {
            stem = kn5Path;
        }
        auto dot = stem.find_last_of('.');
        if (dot != std::string::npos) stem = stem.substr(0, dot);

        // Candidate patterns: stem_rd*.ini in same folder — scan via sequential try
        // Prefer exact match containing this round; longest match wins (most specific).
        std::vector<std::string> candidates;
        // We cannot list dir portably without filesystem — try common names + read dir if available
#ifdef __cpp_lib_filesystem
        try {
            for (auto& e : std::filesystem::directory_iterator(dir.empty() ? "." : dir)) {
                if (!e.is_regular_file()) continue;
                auto p = e.path().string();
                auto name = e.path().filename().string();
                if (name.find(stem + "_rd") != 0) continue;
                if (e.path().extension() != ".ini") continue;
                auto rounds = parseRoundsFromFilename(name);
                if (std::find(rounds.begin(), rounds.end(), round) != rounds.end())
                    candidates.push_back(p);
            }
        } catch (...) {}
#else
        (void)dir;
#endif
        // Fallback explicit tries
        auto tryPush = [&](const std::string& name) {
            std::string p = dir + name;
            std::ifstream t(p);
            if (t) candidates.push_back(p);
        };
        tryPush(stem + "_rd" + std::to_string(round) + ".ini");

        if (candidates.empty()) return {};
        // Prefer file whose rounds list is smallest (most specific) among those containing round
        std::sort(candidates.begin(), candidates.end(), [](const std::string& a, const std::string& b) {
            return parseRoundsFromFilename(a).size() < parseRoundsFromFilename(b).size();
        });
        return candidates.front();
    }

    static RaceComponentConfig loadForRound(const std::string& kn5Path, int round) {
        auto path = findConfigPathForRound(kn5Path, round);
        if (path.empty()) {
            RaceComponentConfig empty;
            empty.description = "default (all nodes)";
            return empty;
        }
        return loadIni(path);
    }

    /** Filter catalog → list of nodes to draw / collide. */
    static std::vector<std::string> filterNodes(const VehicleNodeCatalog& catalog,
                                                 const RaceComponentConfig& cfg) {
        std::vector<std::string> out;
        out.reserve(catalog.nodes.size());
        for (const auto& n : catalog.nodes) {
            if (cfg.isActive(n))
                out.push_back(n);
        }
        return out;
    }
};

} // namespace vehicle
} // namespace ks

// filesystem include for directory scan
#include <filesystem>
