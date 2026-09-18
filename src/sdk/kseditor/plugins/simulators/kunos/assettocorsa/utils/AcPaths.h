#pragma once

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace ks::actools {

// Assetto Corsa path utilities
// These provide AC-specific path handling similar to AcPaths.cs from actuols

// Get AC root directory detection
// Returns true if the directory looks like an AC installation
inline bool IsAcRoot(const std::string& directory) {
    try {
        fs::path p(directory);
        return fs::exists(p / "content" / "cars") &&
               fs::exists(p / "apps") &&
               (fs::exists(p / "acs.exe") || fs::exists(p / "acs_pro.exe"));
    } catch (...) {
        return false;
    }
}

// Get AC root from a known directory (documents, etc.)
// In a real implementation would search for AC installation
inline std::string GetAcRootDirectory(const std::string& startDir = "") {
    // Placeholder - would search upward from startDir for AC installation
    return "";
}

// Documents directory
inline std::string GetDocumentsDirectory() {
    // On Windows: %USERPROFILE%\Documents\Assetto Corsa
    // On Linux: ~/Assetto Corsa
    // On macOS: ~/Library/Application Support/Assetto Corsa
    // For now, return empty or platform-specific
    char* home = nullptr;
#if defined(_WIN32)
    home = getenv("USERPROFILE");
#elif defined(__linux__)
    home = getenv("HOME");
#endif
    if (home) {
        return std::string(home) + "/Assetto Corsa";
    }
    return "";
}

// System cfg directory
inline std::string GetSystemCfgDirectory(const std::string& acRoot) {
    return (fs::path(acRoot) / "system" / "cfg").string();
}

// Documents cfg directory
inline std::string GetDocumentsCfgDirectory() {
    return (fs::path(GetDocumentsDirectory()) / "cfg").string();
}

// Replays directory
inline std::string GetReplaysDirectory() {
    return (fs::path(GetDocumentsDirectory()) / "replay").string();
}

// Documents out directory
inline std::string GetDocumentsOutDirectory() {
    return (fs::path(GetDocumentsDirectory()) / "out").string();
}

// Showroom config filename
inline std::string GetCfgShowroomFilename() {
    return (fs::path(GetDocumentsCfgDirectory()) / "showroom_start.ini").string();
}

// Video config filename
inline std::string GetCfgVideoFilename() {
    return (fs::path(GetDocumentsCfgDirectory()) / "video.ini").string();
}

// Python config filename
inline std::string GetCfgAppsFilename() {
    return (fs::path(GetDocumentsCfgDirectory()) / "python.ini").string();
}

// Controls config filename
inline std::string GetCfgControlsFilename() {
    return (fs::path(GetDocumentsCfgDirectory()) / "controls.ini").string();
}

// Screens directory
inline std::string GetDocumentsScreensDirectory() {
    return (fs::path(GetDocumentsDirectory()) / "screens").string();
}

// Cars directory
inline std::string GetCarsDirectory(const std::string& acRoot) {
    return (fs::path(acRoot) / "content" / "cars").string();
}

// Tracks directory
inline std::string GetTracksDirectory(const std::string& acRoot) {
    return (fs::path(acRoot) / "content" / "tracks").string();
}

// Car directory
inline std::string GetCarDirectory(const std::string& acRoot, const std::string& carName) {
    return (fs::path(GetCarsDirectory(acRoot)) / carName).string();
}

// Car skins directory
inline std::string GetCarSkinsDirectory(const std::string& carDir) {
    return (fs::path(carDir) / "skins").string();
}

// AC root car skins directory
inline std::string GetCarSkinsDirectory(const std::string& acRoot, const std::string& carName) {
    return GetCarSkinsDirectory(GetCarDirectory(acRoot, carName));
}

// Car skin directory
inline std::string GetCarSkinDirectory(const std::string& acRoot, const std::string& carName, const std::string& skinName) {
    return (fs::path(GetCarSkinsDirectory(acRoot, carName)) / skinName).string();
}

// Showrooms directory
inline std::string GetShowroomsDirectory(const std::string& acRoot) {
    return (fs::path(acRoot) / "content" / "showroom").string();
}

// Fonts directory
inline std::string GetFontsDirectory(const std::string& acRoot) {
    return (fs::path(acRoot) / "content" / "fonts").string();
}

// Weather directory
inline std::string GetWeatherDirectory(const std::string& acRoot) {
    return (fs::path(acRoot) / "content" / "weather").string();
}

// PP filters directory
inline std::string GetPpFiltersDirectory(const std::string& acRoot) {
    return (fs::path(acRoot) / "system" / "cfg" / "ppfilters").string();
}

// Driver models directory
inline std::string GetDriverModelsDirectory(const std::string& acRoot) {
    return (fs::path(acRoot) / "content" / "driver").string();
}

// Python apps directory
inline std::string GetPythonAppsDirectory(const std::string& acRoot) {
    return (fs::path(acRoot) / "apps" / "python").string();
}

// Kunos career directory
inline std::string GetKunosCareerDirectory(const std::string& acRoot) {
    return (fs::path(acRoot) / "content" / "career").string();
}

// Career progress filename
inline std::string GetKunosCareerProgressFilename() {
    return (fs::path(GetDocumentsDirectory()) / "launcherdata" / "filestore" / "career.ini").string();
}

// Showroom directory by name
inline std::string GetShowroomDirectory(const std::string& acRoot, const std::string& showroomName) {
    return (fs::path(GetShowroomsDirectory(acRoot)) / showroomName).string();
}

// AC logo filename
inline std::string GetAcLogoFilename(const std::string& acRoot) {
    return (fs::path(acRoot) / "content" / "gui" / "logo_ac_app.png").string();
}

// AC launcher filename
inline std::string GetAcLauncherFilename(const std::string& acRoot) {
    return (fs::path(acRoot) / "AssettoCorsa.exe").string();
}

// Log filename
inline std::string GetLogFilename() {
    return (fs::path(GetDocumentsDirectory()) / "logs" / "log.txt").string();
}

// Log filename with custom name
inline std::string GetLogFilename(const std::string& logFileName) {
    return (fs::path(GetDocumentsDirectory()) / "logs" / logFileName).string();
}

// Race ini filename
inline std::string GetRaceIniFilename() {
    return (fs::path(GetDocumentsCfgDirectory()) / "race.ini").string();
}

// Assists ini filename
inline std::string GetAssistsIniFilename() {
    return (fs::path(GetDocumentsCfgDirectory()) / "assists.ini").string();
}

// Sfx directory
inline std::string GetSfxDirectory(const std::string& acRoot) {
    return (fs::path(acRoot) / "content" / "sfx").string();
}

// Sfx GUIDs filename
inline std::string GetSfxGuidsFilename(const std::string& acRoot) {
    return (fs::path(GetSfxDirectory(acRoot)) / "GUIDs.txt").string();
}

// GUI icons directory
inline std::string GetGuiIconsDirectory(const std::string& acRoot) {
    return (fs::path(acRoot) / "content" / "gui" / "icons").string();
}

// Race output JSON filename
inline std::string GetResultJsonFilename() {
    return (fs::path(GetDocumentsOutDirectory()) / "race_out.json").string();
}

// Get main car filename from car directory
// Looks for LOD files in lods.ini or finds the largest .kn5 file
inline std::string GetMainCarFilename(const std::string& carDir, bool considerHr = false) {
    // Try to read lods.ini
    auto lodsIni = carDir + "/lods.ini";
    if (fs::exists(lodsIni)) {
        // Parse INI file for LOD entries
        // Simplified - would parse the INI file
        // In full implementation would use an INI parser
        auto iniContent = ReadIniFile(lodsIni);
        if (!iniContent.empty()) {
            // Get LOD_0 or LOD_HR file
            auto lod0 = iniContent.find("LOD_0");
            auto lodHr = iniContent.find("LOD_HR");
            
            if (considerHr && lodHr != iniContent.end() && !lodHr->second.empty()) {
                return carDir + "/" + lodHr->second;
            }
            if (lod0 != iniContent.end() && !lod0->second.empty()) {
                return carDir + "/" + lod0->second;
            }
        }
    }
    
    // Fallback: find largest .kn5 file
    std::string bestFile;
    size_t bestSize = 0;
    
    for (const auto& entry : fs::directory_iterator(carDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".kn5") {
            auto size = fs::file_size(entry.path());
            if (size > bestSize) {
                bestSize = size;
                bestFile = entry.path().string();
            }
        }
    }
    
    return bestFile;
}

// Read INI file - simplified parser
// Returns map of section -> {key -> value}
struct IniSection {
    std::string section;
    std::map<std::string, std::string> keys;
};

inline std::map<std::string, IniSection> ReadIniFile(const std::string& filename) {
    std::map<std::string, IniSection> sections;
    // Placeholder - would parse INI file
    return sections;
}

// Get car setups directory
inline std::string GetCarSetupsDirectory() {
    return (fs::path(GetDocumentsDirectory()) / "setups").string();
}

// Get car setups directory by name
inline std::string GetCarSetupsDirectory(const std::string& carName) {
    return (fs::path(GetDocumentsDirectory()) / "setups" / carName).string();
}

// Check if path is AC root
inline bool IsAcRoot(const std::string& directory) {
    return ::ks::actools::IsAcRoot(directory);
}

} // namespace ks::actools