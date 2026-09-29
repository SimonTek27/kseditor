#include "AI/AiFileReader.h"

#include <cmath>
#include <cstdlib>
#include <fstream>

namespace ks {
namespace ai {
namespace {

constexpr unsigned int kAiMagic = 0x00414900u;
constexpr unsigned int kMaxPoints = 100000u;

bool readExact(std::ifstream& in, void* dst, std::size_t bytes)
{
    in.read(static_cast<char*>(dst), static_cast<std::streamsize>(bytes));
    return static_cast<std::size_t>(in.gcount()) == bytes;
}

void finishSpline(AiSpline& spline)
{
    float cum = 0.0f;
    for (std::size_t i = 0; i < spline.points.size(); ++i) {
        if (i > 0) {
            const AiSplinePoint& a = spline.points[i - 1];
            const AiSplinePoint& b = spline.points[i];
            const float dx = b.position.x - a.position.x;
            const float dy = b.position.y - a.position.y;
            const float dz = b.position.z - a.position.z;
            cum += std::sqrt(dx * dx + dy * dy + dz * dz);
        }
        spline.points[i].distance = cum;
    }
    spline.totalDistance = cum;

    if (spline.points.size() >= 2) {
        const AiSplinePoint& a = spline.points.front();
        const AiSplinePoint& b = spline.points.back();
        const float dx = b.position.x - a.position.x;
        const float dy = b.position.y - a.position.y;
        const float dz = b.position.z - a.position.z;
        const float gap = std::sqrt(dx * dx + dy * dy + dz * dz);
        spline.closed = spline.totalDistance > 0.0f
                        && gap < spline.totalDistance * 0.05f;
    }
}

AiSpline readTextSpline(const std::string& path)
{
    AiSpline spline;
    std::ifstream in(path);
    if (!in) return spline;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;

        float f[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        int fields = 0;
        const char* p = line.c_str();
        while (fields < 5 && *p) {
            char* end = nullptr;
            const float v = std::strtof(p, &end);
            if (end == p) break;
            f[fields++] = v;
            p = end;
            while (*p == ' ' || *p == '\t') ++p;
            if (*p == ',' || *p == ';' || *p == '\t') ++p;
        }
        if (fields < 3) continue;

        AiSplinePoint pt;
        pt.position = {f[0], f[1], f[2]};
        pt.curvature = f[3];
        pt.speed = f[4];
        spline.points.push_back(pt);
        if (spline.points.size() > kMaxPoints) {
            spline.points.clear();
            return spline;
        }
    }

    finishSpline(spline);
    return spline;
}

} // namespace

AiSpline AiFileReader::readSpline(const std::string& path)
{
    AiSpline spline;
    std::ifstream in(path, std::ios::binary);
    if (!in) return spline;

    unsigned int magic = 0;
    if (!readExact(in, &magic, 4) || magic != kAiMagic) {
        in.close();
        return readTextSpline(path);
    }

    unsigned int version = 0;
    unsigned int count = 0;
    if (!readExact(in, &version, 4) || !readExact(in, &count, 4)
        || count == 0 || count > kMaxPoints) {
        return spline;
    }

    spline.points.reserve(count);
    for (unsigned int i = 0; i < count; ++i) {
        float f[5];
        if (!readExact(in, f, sizeof(f))) {
            spline.points.clear();
            return spline;
        }
        AiSplinePoint pt;
        pt.position = {f[0], f[1], f[2]};
        pt.curvature = f[3];
        pt.speed = f[4];
        spline.points.push_back(pt);
    }

    finishSpline(spline);
    return spline;
}

} // namespace ai
} // namespace ks
