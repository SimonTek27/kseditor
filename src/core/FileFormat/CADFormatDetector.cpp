#include "CADFormatDetector.h"
#include <QFileInfo>
#include <QFile>
#include <QRegularExpression>

namespace CAD {

QString FormatDetector::detectFormat(const QString& filePath)
{
    // Try extension-based detection first
    QString format = detectByExtension(filePath);
    if (format != "UNKNOWN") {
        return format;
    }
    
    // Fall back to content-based detection
    return detectByContent(filePath);
}

QString FormatDetector::detectByExtension(const QString& filePath)
{
    QString ext = QFileInfo(filePath).suffix().toLower();
    
    if (ext == "stp" || ext == "step") return "STEP";
    if (ext == "igs" || ext == "iges") return "IGES";
    if (ext == "stl") return "STL";
    if (ext == "obj") return "OBJ";
    if (ext == "dxf") return "DXF";
    if (ext == "brep") return "BREP";
    
    return "UNKNOWN";
}

QString FormatDetector::detectByContent(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return "UNKNOWN";
    }
    
    QByteArray header = file.peek(512);
    file.close();
    
    QString headerStr = QString::fromLatin1(header);
    
    // STEP format signature
    if (headerStr.contains("ISO-10303-21")) {
        return "STEP";
    }
    
    // IGES format signature
    if (headerStr.contains("IGES") || headerStr.contains("S ")) {
        return "IGES";
    }
    
    // STL format (ASCII or binary)
    if (headerStr.contains("solid") || headerStr.contains("facet")) {
        return "STL";
    }
    
    // OBJ format
    if (headerStr.contains("v ") && (headerStr.contains("vt ") || headerStr.contains("vn "))) {
        return "OBJ";
    }
    
    // DXF format
    if (headerStr.contains("SECTION") && headerStr.contains("HEADER")) {
        return "DXF";
    }
    
    // BREP format (OpenCascade)
    if (headerStr.contains("DBRep_DrawableShape") || headerStr.contains("TShape")) {
        return "BREP";
    }
    
    return "UNKNOWN";
}

bool FormatDetector::isValid(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray header = file.peek(200);
    file.close();
    
    QString headerStr = QString::fromLatin1(header);
    
    if (headerStr.contains("ISO-10303-21")) return true;
    if (headerStr.contains("IGES")) return true;
    if (headerStr.contains("solid") || headerStr.contains("facet")) return true;
    if (headerStr.contains("SECTION") && headerStr.contains("HEADER")) return true;
    if (headerStr.contains("v ")) return true;
    if (headerStr.contains("DBRep")) return true;
    
    return false;
}

} // namespace CAD
