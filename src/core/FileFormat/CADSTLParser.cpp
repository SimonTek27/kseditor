#include "CADSTLParser.h"
#include "../sys/LogManager.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

namespace CAD {

QString STLParser::m_lastError;

bool STLParser::parse(const QString& filePath, File& outFile)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Failed to open STL file: " + filePath;
        return false;
    }
    
    outFile.format = "STL";
    outFile.sourceFile = filePath;
    outFile.assembly.name = QFileInfo(filePath).baseName();
    
    QByteArray data = file.readAll();
    file.close();
    
    // Check if binary STL (based on size and header)
    if (data.size() > 84 && data.left(5) != "solid") {
        return parseBinary(filePath, outFile);
    }
    
    return parseASCII(filePath, outFile);
}

bool STLParser::parseBinary(const QString& filePath, File& outFile)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Failed to open binary STL file";
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    if (data.size() < 84) {
        m_lastError = "Invalid binary STL: file too small";
        return false;
    }
    
    outFile.format = "STL";
    outFile.sourceFile = filePath;
    outFile.assembly.name = QFileInfo(filePath).baseName();
    
    // Read triangle count from bytes 80-83
    quint32 numTriangles = *reinterpret_cast<const quint32*>(data.data() + 80);
    
    Solid solid;
    solid.name = outFile.assembly.name;
    solid.type = SolidType::BrepSolid;
    
    const char* ptr = data.data() + 84;
    int maxTriangles = (data.size() - 84) / 50;  // Each triangle is 50 bytes
    numTriangles = qMin(numTriangles, static_cast<quint32>(maxTriangles));
    
     for (quint32 i = 0; i < numTriangles; ++i) {
         const float* normal = reinterpret_cast<const float*>(ptr);
         const float* v0_data = reinterpret_cast<const float*>(ptr + 12);
         const float* v1_data = reinterpret_cast<const float*>(ptr + 24);
         const float* v2_data = reinterpret_cast<const float*>(ptr + 36);
         
         ptr += 50;
         
         // Parse three vertices
         Vector3 v0(v0_data[0], v0_data[1], v0_data[2]);
         Vector3 v1(v1_data[0], v1_data[1], v1_data[2]);
         Vector3 v2(v2_data[0], v2_data[1], v2_data[2]);
         
         int baseIdx = solid.tessellatedVertices.size();
         solid.tessellatedVertices.append(v0);
         solid.tessellatedVertices.append(v1);
         solid.tessellatedVertices.append(v2);
         
         QVector<int> indices = {baseIdx, baseIdx + 1, baseIdx + 2};
         solid.tessellatedPolygons.append(indices);
     }
    
    outFile.assembly.rootComponent.solids.append(solid);
    outFile.faceCount = numTriangles;
    outFile.vertexCount = numTriangles * 3;
    outFile.solidCount = 1;
    
    LOG_INFO("CADSTLParser", QString("Loaded binary STL: %1 faces, %2 vertices")
        .arg(outFile.faceCount).arg(outFile.vertexCount));
    
    return true;
}

bool STLParser::parseASCII(const QString& filePath, File& outFile)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = "Failed to open ASCII STL file";
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    outFile.format = "STL";
    outFile.sourceFile = filePath;
    outFile.assembly.name = QFileInfo(filePath).baseName();
    
    QString content = QString::fromLatin1(data);
    
    // Parse ASCII STL format
    QRegularExpression facetRegex(
        R"(facet\s+normal\s+([-\d.eE+]+)\s+([-\d.eE+]+)\s+([-\d.eE+]+)\s*)"
        R"(outer\s+loop\s*)"
        R"(vertex\s+([-\d.eE+]+)\s+([-\d.eE+]+)\s+([-\d.eE+]+)\s*)"
        R"(vertex\s+([-\d.eE+]+)\s+([-\d.eE+]+)\s+([-\d.eE+]+)\s*)"
        R"(vertex\s+([-\d.eE+]+)\s+([-\d.eE+]+)\s+([-\d.eE+]+)\s*)",
        QRegularExpression::CaseInsensitiveOption
    );
    
    Solid solid;
    solid.name = outFile.assembly.name;
    solid.type = SolidType::BrepSolid;
    
    QRegularExpressionMatchIterator it = facetRegex.globalMatch(content);
    int solidIdx = 0;
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        
        Vector3 normal(match.captured(1).toFloat(),
                       match.captured(2).toFloat(),
                       match.captured(3).toFloat());
        
        Vector3 v0(match.captured(4).toFloat(),
                   match.captured(5).toFloat(),
                   match.captured(6).toFloat());
        Vector3 v1(match.captured(7).toFloat(),
                   match.captured(8).toFloat(),
                   match.captured(9).toFloat());
        Vector3 v2(match.captured(10).toFloat(),
                   match.captured(11).toFloat(),
                   match.captured(12).toFloat());
        
        int baseIdx = solid.tessellatedVertices.size();
        solid.tessellatedVertices.append(v0);
        solid.tessellatedVertices.append(v1);
        solid.tessellatedVertices.append(v2);
        
        QVector<int> indices = {baseIdx, baseIdx + 1, baseIdx + 2};
        solid.tessellatedPolygons.append(indices);
        
        solidIdx++;
    }
    
    outFile.assembly.rootComponent.solids.append(solid);
    outFile.faceCount = solidIdx;
    outFile.vertexCount = solidIdx * 3;
    outFile.solidCount = 1;
    
    LOG_INFO("CADSTLParser", QString("Loaded ASCII STL: %1 faces, %2 vertices")
        .arg(outFile.faceCount).arg(outFile.vertexCount));
    
    return true;
}

QString STLParser::getLastError()
{
    return m_lastError;
}

} // namespace CAD
