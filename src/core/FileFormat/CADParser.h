#pragma once

#include <QString>
#include "CADTypes.h"

/**
 * @brief CAD file parser - reads and writes various CAD formats (STL, OBJ, STEP, IGES, DXF, BREP)
 */

namespace CAD {

class Parser
{
public:
    // Read a CAD file, auto-detecting format from extension/content
    static bool read(const QString& filePath, File& outFile);

    // Write a CAD file (default output: STEP)
    static bool write(const QString& filePath, const File& file);

    // Validate a CAD file without fully parsing it
    static bool isValid(const QString& filePath);

    // Return the last error message
    static QString getLastError();

private:
    static QString m_lastError;
};

} // namespace CAD
