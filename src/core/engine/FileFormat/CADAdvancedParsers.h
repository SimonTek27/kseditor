#pragma once

#include "CADTypes.h"

namespace CAD {

using File = ks::fileformat::File;
using Component = ks::fileformat::Component;
using Assembly = ks::fileformat::Assembly;
using Solid = ks::fileformat::Solid;
using Point3D = ks::fileformat::Point3D;
using Vector3D = ks::fileformat::Vector3D;
using Face = ks::fileformat::Face;
using Edge = ks::fileformat::Edge;
using Curve = ks::fileformat::Curve;
using Surface = ks::fileformat::Surface;

class STEPParser {
public:
    static bool parse(const QString& filePath, File& outFile);
    static QString getLastError();
private:
    static QString m_lastError;
};

class IGESParser {
public:
    static bool parse(const QString& filePath, File& outFile);
    static QString getLastError();
private:
    static QString m_lastError;
};

class DXFParser {
public:
    static bool parse(const QString& filePath, File& outFile);
    static QString getLastError();
private:
    static QString m_lastError;
};

class BREPParser {
public:
    static bool parse(const QString& filePath, File& outFile);
    static QString getLastError();
private:
    static QString m_lastError;
};

} // namespace CAD
