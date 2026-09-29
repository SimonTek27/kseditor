#pragma once
/**
 * Physics golden-harness (P0 1.1) — compare sim samples to reference CSV.
 * Format (header optional):
 *   t,speed_ms,rpm,x,y,z
 * Qt-free.
 */
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace ks {
namespace physics {

struct GoldenSample {
    double t = 0;
    float speed_ms = 0;
    float rpm = 0;
    float x = 0, y = 0, z = 0;
};

struct GoldenReport {
    int samples = 0;
    int compared = 0;
    double maeSpeed = 0;
    double maeRpm = 0;
    double maePos = 0;
    double corrSpeed = 0; // Pearson-ish on speed if enough points
    bool ok = false;
};

class PhysicsGolden {
public:
    bool loadReferenceCsv(const std::string& path) {
        m_ref.clear();
        std::ifstream in(path);
        if (!in) return false;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#' || line.find("t,") == 0 || line.find("time") == 0)
                continue;
            for (char& c : line) if (c == ';') c = ',';
            std::stringstream ss(line);
            GoldenSample s;
            char comma;
            if (!(ss >> s.t)) continue;
            if (ss.peek() == ',') ss >> comma;
            ss >> s.speed_ms;
            if (ss.peek() == ',') ss >> comma;
            ss >> s.rpm;
            if (ss.peek() == ',') ss >> comma;
            ss >> s.x;
            if (ss.peek() == ',') ss >> comma;
            ss >> s.y;
            if (ss.peek() == ',') ss >> comma;
            ss >> s.z;
            m_ref.push_back(s);
        }
        return !m_ref.empty();
    }

    void clearSim() { m_sim.clear(); }

    void addSim(const GoldenSample& s) { m_sim.push_back(s); }

    GoldenReport evaluate(double speedTol = 2.0, double posTol = 5.0) const {
        GoldenReport r;
        r.samples = static_cast<int>(m_ref.size());
        if (m_ref.empty() || m_sim.empty()) return r;

        double sumAbsSpeed = 0, sumAbsRpm = 0, sumAbsPos = 0;
        int n = 0;

        // nearest-time match
        size_t j = 0;
        for (const auto& ref : m_ref) {
            while (j + 1 < m_sim.size() && m_sim[j + 1].t <= ref.t)
                ++j;
            const auto& s = m_sim[j];
            const double ds = std::fabs(s.speed_ms - ref.speed_ms);
            const double dr = std::fabs(s.rpm - ref.rpm);
            const double dx = s.x - ref.x, dy = s.y - ref.y, dz = s.z - ref.z;
            const double dp = std::sqrt(dx * dx + dy * dy + dz * dz);
            sumAbsSpeed += ds;
            sumAbsRpm += dr;
            sumAbsPos += dp;
            ++n;
        }
        r.compared = n;
        if (n == 0) return r;
        r.maeSpeed = sumAbsSpeed / n;
        r.maeRpm = sumAbsRpm / n;
        r.maePos = sumAbsPos / n;

        // correlation speed
        double meanR = 0, meanS = 0;
        for (int i = 0; i < n; ++i) {
            meanR += m_ref[static_cast<size_t>(i)].speed_ms;
            size_t jj = std::min(static_cast<size_t>(i), m_sim.size() - 1);
            meanS += m_sim[jj].speed_ms;
        }
        meanR /= n; meanS /= n;
        double num = 0, denR = 0, denS = 0;
        for (int i = 0; i < n; ++i) {
            size_t jj = std::min(static_cast<size_t>(i), m_sim.size() - 1);
            double a = m_ref[static_cast<size_t>(i)].speed_ms - meanR;
            double b = m_sim[jj].speed_ms - meanS;
            num += a * b; denR += a * a; denS += b * b;
        }
        if (denR > 1e-9 && denS > 1e-9)
            r.corrSpeed = num / std::sqrt(denR * denS);

        r.ok = (r.maeSpeed <= speedTol) && (r.maePos <= posTol);
        return r;
    }

    const std::vector<GoldenSample>& reference() const { return m_ref; }

private:
    std::vector<GoldenSample> m_ref;
    std::vector<GoldenSample> m_sim;
};

} // namespace physics
} // namespace ks
