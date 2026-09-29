#include "Paths.h"
#include "SimInstallDetector.h"

#include <algorithm>
#include <filesystem>

namespace ks {

// ─── KsPathDetector ──────────────────────────────────────────────────────────
// Split out of Paths.cpp so SimInstallDetector methods are not duplicated
// (Paths.cpp is excluded from the build; SimInstallDetector.cpp owns those).

KsPathDetector::~KsPathDetector() = default;

std::string KsPathDetector::detect()
{
    const std::vector<std::string> candidates = SimInstallDetector::findAllInstallations();
    if (candidates.empty()) {
        error(std::string("Simulator installation not found"));
        return {};
    }
    m_simPath = candidates.front();
    m_contentPath = m_simPath + "/content";
    m_cars = SimInstallDetector::getCarList(m_simPath);
    m_tracks = SimInstallDetector::getTrackList(m_simPath);
    detected(m_simPath);
    return m_simPath;
}

} // namespace ks
