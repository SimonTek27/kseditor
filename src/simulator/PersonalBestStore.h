#pragma once
#include <string>
#include <fstream>
#include <cstdio>
#include <map>

namespace ks {
namespace sim {

struct PersonalBestRecord {
    std::string trackId;
    std::string carId;
    std::string driver;
    float bestLap = 1e9f;
    float sectors[3] = {1e9f, 1e9f, 1e9f};
    int lapsCompleted = 0;
};

class PersonalBestStore {
public:
    void setDirectory(const std::string& dir) { m_dir = dir; }
    static std::string key(const std::string& track, const std::string& car) {
        return track + "__" + car;
    }
    PersonalBestRecord get(const std::string& track, const std::string& car) const {
        auto k = key(track, car);
        auto it = m_cache.find(k);
        if (it != m_cache.end()) return it->second;
        PersonalBestRecord r; r.trackId = track; r.carId = car; return r;
    }
    bool submitLap(const std::string& track, const std::string& car,
                   const std::string& driver, float lapTime, const float sectors[3]) {
        if (!(lapTime > 0.f) || lapTime > 1e8f) return false;
        auto& r = m_cache[key(track, car)];
        r.trackId = track; r.carId = car; r.driver = driver; r.lapsCompleted++;
        bool improved = false;
        if (lapTime < r.bestLap) { r.bestLap = lapTime; improved = true; }
        if (sectors) {
            for (int i = 0; i < 3; ++i)
                if (sectors[i] > 0.f && sectors[i] < r.sectors[i]) {
                    r.sectors[i] = sectors[i]; improved = true;
                }
        }
        if (improved) saveOne(r);
        return improved;
    }
    bool loadOne(const std::string& track, const std::string& car) {
        std::ifstream in(filePath(track, car));
        if (!in) return false;
        PersonalBestRecord r; r.trackId = track; r.carId = car;
        std::string line;
        while (std::getline(in, line)) {
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string k = line.substr(0, eq), val = line.substr(eq + 1);
            if (k == "BestLap") r.bestLap = std::strtof(val.c_str(), nullptr);
            else if (k == "S0") r.sectors[0] = std::strtof(val.c_str(), nullptr);
            else if (k == "S1") r.sectors[1] = std::strtof(val.c_str(), nullptr);
            else if (k == "S2") r.sectors[2] = std::strtof(val.c_str(), nullptr);
            else if (k == "Laps") r.lapsCompleted = std::atoi(val.c_str());
            else if (k == "Driver") r.driver = val;
        }
        m_cache[key(track, car)] = r;
        return true;
    }
    bool saveOne(const PersonalBestRecord& r) const {
        std::ofstream out(filePath(r.trackId, r.carId));
        if (!out) { out.open(r.trackId + "__" + r.carId + ".pb"); if (!out) return false; }
        out << "Track=" << r.trackId << "\nCar=" << r.carId << "\nDriver=" << r.driver
            << "\nBestLap=" << r.bestLap << "\nS0=" << r.sectors[0] << "\nS1=" << r.sectors[1]
            << "\nS2=" << r.sectors[2] << "\nLaps=" << r.lapsCompleted << "\n";
        return true;
    }
private:
    std::string filePath(const std::string& track, const std::string& car) const {
        if (m_dir.empty()) return key(track, car) + ".pb";
        return m_dir + "/" + key(track, car) + ".pb";
    }
    std::string m_dir = "user/pb";
    std::map<std::string, PersonalBestRecord> m_cache;
};

} // namespace sim
} // namespace ks
