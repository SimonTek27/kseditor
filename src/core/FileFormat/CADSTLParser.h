#pragma once

#include "CADTypes.h"

namespace CAD {

/**
 * @brief STL (Stereolithography) format parser
 * 
 * Supports both ASCII and binary STL format parsing.
 */
class STLParser {
public:
    /**
     * Parse STL file (auto-detects ASCII or binary)
     * @param filePath Path to STL file
     * @param outFile Output CAD file model
     * @return True if successful
     */
    static bool parse(const QString& filePath, File& outFile);
    
    /**
     * Parse binary STL format
     * @param filePath Path to STL file
     * @param outFile Output CAD file model
     * @return True if successful
     */
    static bool parseBinary(const QString& filePath, File& outFile);
    
    /**
     * Parse ASCII STL format
     * @param filePath Path to STL file
     * @param outFile Output CAD file model
     * @return True if successful
     */
    static bool parseASCII(const QString& filePath, File& outFile);
    
    /// Get last error message
    static QString getLastError();

private:
    static QString m_lastError;
};

} // namespace CAD
