#include "TrackLoader.h"
#include "engine/AI/AiFileReader.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <cstdio>

namespace fs = std::filesystem;

namespace ks::sim {

namespace {

constexpr uint32_t kKn5Magic = 0x346E6B73;
constexpr uint32_t kKn5MinVersion = 4;
constexpr uint32_t kKn5MaxVersion = 5;
constexpr std::size_t kKn5HeaderBytes = 13 * sizeof(uint32_t);

bool hasKn5Extension(const fs::path& path)
{
    std::string extension = path.extension().string();
    for (char& c : extension)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return extension == ".kn5";
}

} // namespace
    std::vector<fs::path> candidateFiles;
TrackLoader::TrackLoader() = default;
TrackLoader::~TrackLoader() = default;

TrackData TrackLoader::loadTrackFolder(const std::string& trackDirectory)
{
    m_lastError.clear();
    TrackData track;
    track.directory = trackDirectory;
    track.name = trackNameFromDirectory(trackDirectory);

    if (!fs::is_directory(trackDirectory)) {
        m_lastError = "Not a directory: " + trackDirectory;
        return track;
    }

    track.kn5Path = findKn5File(trackDirectory);
    if (track.kn5Path.empty()) {
        m_lastError = "No KN5 file found in track: " + trackDirectory;
        return track;
    }
    const TrackData kn5 = loadKn5File(track.kn5Path);
    if (!kn5.kn5HeaderValidated) return track;
    track.kn5HeaderValidated = true;

    std::string splinePath = findAiSpline(trackDirectory);
    if (!splinePath.empty())
        track.aiSplineLoaded = loadAiSpline(track, splinePath);

    loadTrackIni(track, trackDirectory);
    return track;
}

TrackData TrackLoader::loadKn5File(const std::string& kn5Path)
{
    m_lastError.clear();
    TrackData track;
    track.kn5Path = kn5Path;
    track.name = fs::path(kn5Path).stem().string();
    track.directory = fs::path(kn5Path).parent_path().string();

    std::error_code error;
    const uint64_t fileSize = fs::file_size(kn5Path, error);
    if (error || fileSize < kKn5HeaderBytes) {
        m_lastError = "KN5 header is missing or truncated: " + kn5Path;
        return track;
    }

    std::ifstream input(kn5Path, std::ios::binary);
    std::array<uint32_t, 13> header{};
    for (uint32_t& field : header) {
        unsigned char bytes[4];
        if (!input.read(reinterpret_cast<char*>(bytes), sizeof(bytes))) {
            m_lastError = "Failed to read KN5 header: " + kn5Path;
            return track;
        }
        field = static_cast<uint32_t>(bytes[0]) |
                (static_cast<uint32_t>(bytes[1]) << 8) |
                (static_cast<uint32_t>(bytes[2]) << 16) |
                (static_cast<uint32_t>(bytes[3]) << 24);
    }

    if (header[0] != kKn5Magic) {
        m_lastError = "Invalid KN5 magic: " + kn5Path;
        return track;
    }
    if (header[1] < kKn5MinVersion || header[1] > kKn5MaxVersion) {
        m_lastError = "Unsupported KN5 version " + std::to_string(header[1]) +
                      ": " + kn5Path;
        return track;
    }
    if (header[5] == 0) {
        m_lastError = "KN5 contains no nodes: " + kn5Path;
        return track;
    }

    if ((header[6] != 0 && header[6] > fileSize) ||
        header[7] > fileSize || header[8] > fileSize ||
        header[9] > fileSize || header[10] > fileSize ||
        static_cast<uint64_t>(header[9]) + header[11] > fileSize ||
        static_cast<uint64_t>(header[10]) + header[12] > fileSize) {
        m_lastError = "KN5 header points outside the file: " + kn5Path;
        return track;
    }

    track.kn5HeaderValidated = true;
    return track;
}

std::string TrackLoader::findKn5File(const std::string& trackDirectory)
{
    std::error_code error;
    std::vector<fs::path> candidates;
    for (fs::directory_iterator it(trackDirectory, error), end;
         !error && it != end; it.increment(error)) {
        if (it->is_regular_file(error) && hasKn5Extension(it->path()))
            candidates.push_back(it->path());
    }
    if (!candidates.empty()) {
        std::sort(candidates.begin(), candidates.end());
        return candidates.front().string();
    }

    error.clear();
    for (fs::recursive_directory_iterator it(
             trackDirectory, fs::directory_options::skip_permission_denied, error), end;
         !error && it != end; it.increment(error)) {
        if (it->is_regular_file(error) && hasKn5Extension(it->path()))
            candidates.push_back(it->path());
    }
    if (!candidates.empty()) {
        std::sort(candidates.begin(), candidates.end());
        return candidates.front().string();
    }
    return {};
}

std::string TrackLoader::findAiSpline(const std::string& trackDirectory)
{
    const fs::path preferredPaths[] = {
        fs::path("ai") / "fast_lane.ai",
        fs::path("ai") / "fast_lane.ai.txt",
        fs::path("data") / "ai" / "fast_lane.ai",
    };
    const fs::path root(trackDirectory);
    for (const fs::path& relativePath : preferredPaths) {
        const fs::path p = root / relativePath;
        if (fs::is_regular_file(p))
            return p.string();
    }
    fs::path aiDir = root / "ai";
    std::error_code error;
    std::vector<fs::path> candidateFiles;
    if (!fs::is_directory(aiDir, error)) return {};
    for (fs::directory_iterator it(aiDir, error), end; !error && it != end;
         it.increment(error)) {
        if (!it->is_regular_file(error)) continue;
        std::string extension = it->path().extension().string();
        for (char& c : extension)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (extension == ".ai" || extension == ".txt")
            candidateFiles.push_back(it->path());
    }
    std::sort(candidateFiles.begin(), candidateFiles.end(), [](const fs::path& a, const fs::path& b) {
        const auto extensionPriority = [](const fs::path& path) {
            std::string extension = path.extension().string();
            for (char& c : extension)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return extension == ".ai" ? 0 : 1;
        };
        const int priorityA = extensionPriority(a);
        const int priorityB = extensionPriority(b);
        return priorityA != priorityB ? priorityA < priorityB : a < b;
    });
    return candidateFiles.empty() ? std::string() : candidateFiles.front().string();
}

std::string TrackLoader::trackNameFromDirectory(const std::string& trackDirectory)
{
    fs::path path(trackDirectory);
    if (path.filename().empty()) path = path.parent_path();
    return path.filename().string();
}

bool TrackLoader::loadAiSpline(TrackData& track, const std::string& splinePath)
{
    track.aiSpline = std::make_shared<ks::ai::AiSpline>(
        ks::ai::AiFileReader::readSpline(splinePath));
    if (!track.aiSpline || !track.aiSpline->isValid()) {
        m_lastError = "Failed to parse AI spline: " + splinePath;
        std::fprintf(stderr, "TrackLoader: %s\n", m_lastError.c_str());
        return false;
    }
    track.trackLength = track.aiSpline->totalDistance;
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
