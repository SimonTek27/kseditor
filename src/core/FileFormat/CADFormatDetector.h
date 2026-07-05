#pragma once

#include <QString>

namespace CAD {

/**
 * @brief Format detection utilities for CAD files
 */
class FormatDetector {
public:
    /**
     * Detect CAD file format from file extension and content
     * @param filePath Path to the CAD file
     * @return Format string: "STEP", "IGES", "STL", "OBJ", "DXF", "BREP", or "UNKNOWN"
     */
    static QString detectFormat(const QString& filePath);
    
    /**
     * Validate if file is a recognized CAD format
     * @param filePath Path to the CAD file
     * @return True if valid CAD file
     */
    static bool isValid(const QString& filePath);

private:
    static QString detectByExtension(const QString& filePath);
    static QString detectByContent(const QString& filePath);
};

} // namespace CAD
