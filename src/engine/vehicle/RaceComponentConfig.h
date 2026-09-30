#pragma once
/**
 * Race / event component configuration for KN5-style assets.
 *
 * Sidecar INI next to car.kn5:
 *   car_rd1.ini / car_rd1-2.ini / car_rd1-2-rd6.ini
 *
 * Sections:
 *   [Meta] [Nodes] [Livery] [Sound]
 */
#include <string>
#include <vector>
#include <unordered_set>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cctype>
#include <filesystem>

namespace ks {
namespace vehicle {

// Forward — full defs used by bundle; lightweight copies here to avoid cycles
struct LiveryRef {
    std::string id;
    std::string name;
    std::string folder;
    std::string diffuse;
    std::string specular;
    std::string normal;
    std::string preview;
    int number = -1;
    std::string driverName;
    std::string teamName;
};

struct SoundPackRef {
    std::string id;
    std::string name;
    std::string bankPath;
    std::string engineIni;
    std::string soundsIni;
    float engineGain = 1.f;
    float exteriorGain = 1.f;
    float turboGain = 1.f;
    std::map<std::string, std::string> sampleOverrides;
};

struct RaceComponentConfig {
    std::string sourcePath;
    std::string description;
    std::vector<int> rounds;
    std::unordered_set<std::string> active;
    std::unordered_set<std::string> inactive;
    bool hasExplicitList = false;
    LiveryRef livery;
    SoundPackRef sound;

    bool isActive(const std::string& node) const {
        if (inactive.count(node)) return false;
        if (!hasExplicitList) return true;
        if (active.empty()) return !inactive.count(node);
        return active.count(node) > 0;
    }
};

struct VehicleNodeCatalog {
    std::string kn5Path;
    std::vector<std::string> nodes;
    bool empty() const { return nodes.empty(); }
};

class RaceComponentConfigLoader {
public:
    static std::vector<int> parseRoundsFromFilename(const std::string& pathOrStem) {
        std::string stem = pathOrStem;
        auto slash = stem.find_last_of("/\\");
        if (slash != std::string::npos) stem = stem.substr(slash + 1);
        auto dot = stem.find_last_of('.');
        if (dot != std::string::npos) stem = stem.substr(0, dot);

        auto pos = stem.find("_rd");
        if (pos == std::string::npos) return {};
        std::string tag = stem.substr(pos + 1);

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
                if (i + 2 < tag.size() && tag[i + 1] == 'r' && tag[i + 2] == 'd')
                    i += 2;
            } else {
                flush();
            }
        }
        flush();
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
        auto unquote = [&](std::string s) {
            s = trim(s);
            if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') ||
                                  (s.front() == '\'' && s.back() == '\'')))
                return s.substr(1, s.size() - 2);
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
                if (section == "nodes" || section == "active") {
                    cfg.active.insert(line);
                    cfg.hasExplicitList = true;
                }
                continue;
            }

            std::string key = trim(line.substr(0, eq));
            std::string val = unquote(line.substr(eq + 1));

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

            if (section == "livery" || section == "skin") {
                if (key == "Id" || key == "Skin" || key == "Livery") cfg.livery.id = val;
                else if (key == "Name") cfg.livery.name = val;
                else if (key == "Folder" || key == "Path") cfg.livery.folder = val;
                else if (key == "Diffuse" || key == "Albedo" || key == "Texture") cfg.livery.diffuse = val;
                else if (key == "Specular") cfg.livery.specular = val;
                else if (key == "Normal") cfg.livery.normal = val;
                else if (key == "Preview") cfg.livery.preview = val;
                else if (key == "Number") cfg.livery.number = std::atoi(val.c_str());
                else if (key == "Driver") cfg.livery.driverName = val;
                else if (key == "Team") cfg.livery.teamName = val;
                continue;
            }

            if (section == "sound" || section == "audio") {
                if (key == "Id" || key == "Pack") cfg.sound.id = val;
                else if (key == "Name") cfg.sound.name = val;
                else if (key == "Bank" || key == "BankPath" || key == "Path") cfg.sound.bankPath = val;
                else if (key == "EngineIni") cfg.sound.engineIni = val;
                else if (key == "SoundsIni") cfg.sound.soundsIni = val;
                else if (key == "EngineGain") cfg.sound.engineGain = std::strtof(val.c_str(), nullptr);
                else if (key == "ExteriorGain") cfg.sound.exteriorGain = std::strtof(val.c_str(), nullptr);
                else if (key == "TurboGain") cfg.sound.turboGain = std::strtof(val.c_str(), nullptr);
                else if (key.rfind("Sample.", 0) == 0 || key.rfind("Sample_", 0) == 0) {
                    // Sample.EngineInterior=path.wav
                    std::string cat = key.substr(7);
                    cfg.sound.sampleOverrides[cat] = val;
                }
                continue;
            }

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

            // node=0|1 under [Nodes] or bare section
            if (section == "nodes" || section.empty() || section == "active") {
                int on = std::atoi(val.c_str());
                if (val == "true" || val == "on" || val == "yes") on = 1;
                if (val == "false" || val == "off" || val == "no") on = 0;
                if (on) cfg.active.insert(key);
                else cfg.inactive.insert(key);
                cfg.hasExplicitList = true;
            }
        }

        std::fprintf(stderr,
            "RaceComponentConfig: %s nodes+=%zu nodes-=%zu livery=%s sound=%s rounds=%zu\n",
            path.c_str(), cfg.active.size(), cfg.inactive.size(),
            cfg.livery.id.empty() ? cfg.livery.folder.c_str() : cfg.livery.id.c_str(),
            cfg.sound.id.empty() ? cfg.sound.bankPath.c_str() : cfg.sound.id.c_str(),
            cfg.rounds.size());
        return cfg;
    }

    static std::string findConfigPathForRound(const std::string& kn5Path, int round) {
        namespace fs = std::filesystem;
        fs::path kn5(kn5Path);
        fs::path dir = kn5.parent_path();
        if (dir.empty()) dir = ".";
        const std::string stem = kn5.stem().string();

        std::vector<std::string> candidates;
        try {
            for (auto& e : fs::directory_iterator(dir)) {
                if (!e.is_regular_file()) continue;
                if (e.path().extension() != ".ini") continue;
                const std::string name = e.path().filename().string();
                if (name.rfind(stem + "_rd", 0) != 0) continue;
                auto rounds = parseRoundsFromFilename(name);
                if (std::find(rounds.begin(), rounds.end(), round) != rounds.end())
                    candidates.push_back(e.path().string());
            }
        } catch (...) {}

        {
            fs::path p = dir / (stem + "_rd" + std::to_string(round) + ".ini");
            if (fs::exists(p))
                candidates.push_back(p.string());
        }

        if (candidates.empty()) return {};
        std::sort(candidates.begin(), candidates.end(), [](const std::string& a, const std::string& b) {
            return parseRoundsFromFilename(a).size() < parseRoundsFromFilename(b).size();
        });
        candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
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
