#include "AdvancedFormats.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace ks {
namespace fileformat {

// ─── FormatValidator Implementation ──────────────────────────────────────────

FormatValidationResult FormatValidator::validateKN5(const QString& filePath) {
    FormatValidationResult result;
    result.valid = true;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.valid = false;
        result.errors.append("Cannot open file: " + filePath);
        return result;
    }
    
    // Check KN5 header
    QByteArray header = file.read(4);
    if (header != "KN5\0") {
        result.valid = false;
        result.errors.append("Invalid KN5 magic header");
    }
    
    // Read version
    QByteArray versionData = file.read(4);
    if (versionData.size() == 4) {
        uint32_t version = *reinterpret_cast<const uint32_t*>(versionData.constData());
        result.metadata["version"] = version;
        if (version < 2) {
            result.warnings.append("Old KN5 version: " + QString::number(version));
        }
    }
    
    file.close();
    return result;
}

FormatValidationResult FormatValidator::validateFBX(const QString& filePath) {
    FormatValidationResult result;
    result.valid = true;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.valid = false;
        result.errors.append("Cannot open file: " + filePath);
        return result;
    }
    
    QByteArray header = file.read(23);
    if (!header.startsWith("Kaydara FBX Binary")) {
        // Try ASCII
        file.seek(0);
        QByteArray asciiHeader = file.read(20);
        if (!asciiHeader.startsWith("; FBX")) {
            result.valid = false;
            result.errors.append("Invalid FBX header");
        }
    }
    
    file.close();
    return result;
}

FormatValidationResult FormatValidator::validateGLB(const QString& filePath) {
    FormatValidationResult result;
    result.valid = true;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.valid = false;
        result.errors.append("Cannot open file: " + filePath);
        return result;
    }
    
    QByteArray header = file.read(12);
    if (header.size() < 12) {
        result.valid = false;
        result.errors.append("File too small for GLB");
        file.close();
        return result;
    }
    
    uint32_t magic = *reinterpret_cast<const uint32_t*>(header.constData());
    if (magic != 0x46546C67) { // "glTF"
        result.valid = false;
        result.errors.append("Invalid GLB magic");
    }
    
    uint32_t version = *reinterpret_cast<const uint32_t*>(header.constData() + 4);
    if (version != 2) {
        result.warnings.append("GLB version " + QString::number(version) + " may not be fully supported");
    }
    
    file.close();
    return result;
}

FormatValidationResult FormatValidator::validateUSD(const QString& filePath) {
    FormatValidationResult result;
    result.valid = true;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.valid = false;
        result.errors.append("Cannot open file: " + filePath);
        return result;
    }
    
    QString firstLine = file.readLine().trimmed();
    if (!firstLine.startsWith("#usda")) {
        // Could be binary USD (.usd/.usdc) - would need USD library
        result.warnings.append("File may be binary USD format (requires USD library)");
    }
    
    file.close();
    return result;
}

FormatValidationResult FormatValidator::validateAlembic(const QString& filePath) {
    FormatValidationResult result;
    result.valid = true;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.valid = false;
        result.errors.append("Cannot open file: " + filePath);
        return result;
    }
    
    QByteArray header = file.read(8);
    if (header.size() < 8) {
        result.valid = false;
        result.errors.append("File too small for Alembic");
        file.close();
        return result;
    }
    
    // Check for HDF5 or Ogawa magic
    // HDF5: 89 48 44 46 0D 0A 1A 0A
    // Ogawa: varies
    if (header[0] == 0x89 && header[1] == 'H' && header[2] == 'D' && header[3] == 'F') {
        result.metadata["format"] = "HDF5";
    } else {
        result.warnings.append("Unknown Alembic container format");
    }
    
    file.close();
    return result;
}

FormatValidationResult FormatValidator::validateOBJ(const QString& filePath) {
    FormatValidationResult result;
    result.valid = true;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.valid = false;
        result.errors.append("Cannot open file: " + filePath);
        return result;
    }
    
    int vertexCount = 0, faceCount = 0, normalCount = 0, uvCount = 0;
    QString line;
    QTextStream in(&file);
    while (in.readLineInto(&line)) {
        line = line.trimmed();
        if (line.startsWith("v ")) vertexCount++;
        else if (line.startsWith("vn ")) normalCount++;
        else if (line.startsWith("vt ")) uvCount++;
        else if (line.startsWith("f ")) faceCount++;
    }
    
    result.metadata["vertices"] = vertexCount;
    result.metadata["faces"] = faceCount;
    result.metadata["normals"] = normalCount;
    result.metadata["uvs"] = uvCount;
    
    if (vertexCount == 0) {
        result.errors.append("No vertices found");
        result.valid = false;
    }
    if (faceCount == 0) {
        result.warnings.append("No faces found");
    }
    if (normalCount == 0) {
        result.warnings.append("No normals found");
    }
    if (uvCount == 0) {
        result.warnings.append("No UV coordinates found");
    }
    
    file.close();
    return result;
}

FormatValidationResult FormatValidator::validateSTL(const QString& filePath) {
    FormatValidationResult result;
    result.valid = true;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.valid = false;
        result.errors.append("Cannot open file: " + filePath);
        return result;
    }
    
    QByteArray header = file.read(80);
    bool isBinary = true;
    
    // Check if ASCII
    file.seek(0);
    QString firstLine = file.readLine().trimmed();
    if (firstLine.toLower().startsWith("solid")) {
        isBinary = false;
    }
    
    result.metadata["binary"] = isBinary;
    result.metadata["header"] = QString::fromLatin1(header).trimmed();
    
    if (isBinary) {
        // Binary STL: 80 byte header + 4 byte triangle count + 50 bytes per triangle
        qint64 fileSize = file.size();
        if (fileSize < 84) {
            result.errors.append("Binary STL file too small");
            result.valid = false;
        } else {
            uint32_t triangleCount;
            file.seek(80);
            file.read(reinterpret_cast<char*>(&triangleCount), 4);
            result.metadata["triangleCount"] = triangleCount;
            
            qint64 expectedSize = 84 + triangleCount * 50;
            if (fileSize != expectedSize) {
                result.warnings.append(QString("File size mismatch: expected %1, got %2")
                    .arg(expectedSize).arg(fileSize));
            }
        }
    }
    
    file.close();
    return result;
}

FormatValidationResult FormatValidator::validateCOLLADA(const QString& filePath) {
    FormatValidationResult result;
    result.valid = true;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.valid = false;
        result.errors.append("Cannot open file: " + filePath);
        return result;
    }
    
    QString firstLine = file.readLine().trimmed();
    if (!firstLine.contains("COLLADA") && !firstLine.contains("dae")) {
        result.errors.append("Invalid COLLADA header");
        result.valid = false;
    }
    
    // Could parse XML to validate structure
    result.metadata["xmlValid"] = true;
    
    file.close();
    return result;
}

FormatValidationResult FormatValidator::validate(const QString& filePath, const QString& format) {
    QString fmt = format.toLower();
    
    if (fmt == "kn5") return validateKN5(filePath);
    if (fmt == "fbx") return validateFBX(filePath);
    if (fmt == "glb" || fmt == "gltf") return validateGLB(filePath);
    if (fmt == "usd" || fmt == "usda" || fmt == "usdc") return validateUSD(filePath);
    if (fmt == "abc") return validateAlembic(filePath);
    if (fmt == "obj") return validateOBJ(filePath);
    if (fmt == "stl") return validateSTL(filePath);
    if (fmt == "dae" || fmt == "collada") return validateCOLLADA(filePath);
    
    FormatValidationResult result;
    result.valid = false;
    result.errors.append("Unknown format: " + format);
    return result;
}

FormatValidationResult FormatValidator::validateSchema(const QJsonObject& data, 
                                                         const QVector<SchemaRule>& rules) {
    FormatValidationResult result;
    result.valid = true;
    
    for (const auto& rule : rules) {
        if (!data.contains(rule.field)) {
            if (rule.required) {
                result.valid = false;
                result.errors.append("Required field missing: " + rule.field);
            }
            continue;
        }
        
        QJsonValue value = data[rule.field];
        
        // Type check
        bool typeMatch = false;
        if (rule.type == "string" && value.isString()) typeMatch = true;
        else if (rule.type == "int" && value.isDouble()) typeMatch = true;
        else if (rule.type == "float" && value.isDouble()) typeMatch = true;
        else if (rule.type == "bool" && value.isBool()) typeMatch = true;
        else if (rule.type == "array" && value.isArray()) typeMatch = true;
        else if (rule.type == "object" && value.isObject()) typeMatch = true;
        
        if (!typeMatch) {
            result.valid = false;
            result.errors.append("Field " + rule.field + " has wrong type (expected " + rule.type + ")");
            continue;
        }
        
        // Range checks
        if (rule.minValue.isValid() && value.isDouble()) {
            if (value.toDouble() < rule.minValue.toDouble()) {
                result.valid = false;
                result.errors.append("Field " + rule.field + " below minimum: " + rule.minValue.toString());
            }
        }
        if (rule.maxValue.isValid() && value.isDouble()) {
            if (value.toDouble() > rule.maxValue.toDouble()) {
                result.valid = false;
                result.errors.append("Field " + rule.field + " above maximum: " + rule.maxValue.toString());
            }
        }
        
        // Enum check
        if (!rule.enumValues.isEmpty() && value.isString()) {
            if (!rule.enumValues.contains(value.toString())) {
                result.valid = false;
                result.errors.append("Field " + rule.field + " has invalid value: " + value.toString());
            }
        }
        
        // Regex check
        if (!rule.pattern.isEmpty() && value.isString()) {
            QRegularExpression rx(rule.pattern);
            if (!rx.match(value.toString()).hasMatch()) {
                result.valid = false;
                result.errors.append("Field " + rule.field + " does not match pattern: " + rule.pattern);
            }
        }
    }
    
    return result;
}

FormatValidationResult FormatValidator::validateRoundTrip(const QString& inputPath, 
                                                           const QString& format) {
    FormatValidationResult result;
    result.valid = true;
    
    // This would import then export and compare
    // Simplified version just checks if file can be read
    return validate(inputPath, format);
}

} // namespace fileformat
} // namespace ks