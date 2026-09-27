#include "TrackLoader.h"
#include "engine/FileFormat/AiSpline.h"
#include <filesystem>
#include <fstream>
#include <cstdio>

namespace fs = std::filesystem;

namespace ks::sim {

TrackData TrackLoader::loadTrackFolder(const std::string& trackDirectory)
{
    TrackData track;
    track.directory = trackDirectory;
    track.name = trackNameFromDirectory(trackDirectory);

    if (!fs::is_directory(trackDirectory)) {
        m_lastError = "Not a directory: " + trackDirectory;
        return track;
    }

    track.kn5Path = findKn5File(trackDirectory);
    std::string splinePath = findAiSpline(trackDirectory);
    if (!splinePath.empty())
        loadAiSpline(track, splinePath);

    loadTrackIni(track, trackDirectory);
    return track;
}

std::string TrackLoader::findKn5File(const std::string& trackDirectory)
{
    for (auto& e : fs::directory_iterator(trackDirectory)) {
        if (e.path().extension() == ".kn5")
            return e.path().string();
    }
    try {
        for (auto& e : fs::recursive_directory_iterator(trackDirectory)) {
            if (e.path().extension() == ".kn5")
                return e.path().string();
        }
    } catch (...) {}
    return {};
}

std::string TrackLoader::findAiSpline(const std::string& trackDirectory)
{
    const char* candidates[] = {
        "/ai/fast_lane.ai",
        "/ai/fast_lane.ai.txt",
        "/data/ai/fast_lane.ai",
    };
    for (const char* c : candidates) {
        fs::path p = fs::path(trackDirectory) / c;
        // path join: candidates start with /
        p = fs::path(trackDirectory + c);
        if (fs::exists(p))
            return p.string();
    }
    fs::path aiDir = fs::path(trackDirectory) / "ai";
    if (fs::is_directory(aiDir)) {
        for (auto& e : fs::directory_iterator(aiDir)) {
            if (e.path().extension() == ".ai" || e.path().extension() == ".txt")
                return e.path().string();
        }
    }
    return {};
}

std::string TrackLoader::trackNameFromDirectory(const std::string& trackDirectory)
{
    return fs::path(trackDirectory).filename().string();
}

bool TrackLoader::loadAiSpline(TrackData& track, const std::string& splinePath)
{
    track.aiSpline = ks::ai::AiFileReader::readSpline(splinePath);
    if (!track.aiSpline.isValid()) {
        m_lastError = "Failed to parse AI spline: " + splinePath;
        std::fprintf(stderr, "TrackLoader: %s\n", m_lastError.c_str());
        return false;
    }
    return true;
}

bool TrackLoader::loadTrackIni(TrackData& track, const std::string& trackDir)
{
    fs::path ini = fs::path(trackDir) / "data" / "surfaces.ini";
    if (!fs::exists(ini))
        ini = fs::path(trackDir) / "surfaces.ini";
    if (!fs::exists(ini))
        return false;
    (void)track;
    return true;
}

} // namespace ks::sim
