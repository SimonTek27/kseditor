#pragma once

#include <QString>
#include <QStringList>

namespace ks {

enum KsExportFormat {
    Format_OBJ = 0,
    Format_FBX = 1,
    Format_GLTF = 2,
    Format_DAE = 3,
    Format_STL = 4,
    Format_3DS = 5,
    Format_JSON = 6,
    Format_XML = 7
};

enum KsImportFormat {
    Import_OBJ = 0,
    Import_FBX = 1,
    Import_GLTF = 2,
    Import_DAE = 3,
    Import_STL = 4,
    Import_3DS = 5,
    Import_KN5 = 6
};

class KsMeshData;
class KsMeshUtils;

class KsConverter {
public:
    static bool exportToOBJ(const QString& path, const KsMeshData* mesh);
    static bool importFromOBJ(const QString& path, KsMeshData* mesh);
    static bool exportToJSON(const QString& path, const KsMeshData* mesh);
    static bool importFromJSON(const QString& path, KsMeshData* mesh);
    static bool exportToXML(const QString& path, const KsMeshData* mesh);
    static bool importFromXML(const QString& path, KsMeshData* mesh);
    static bool exportToSTL(const QString& path, const KsMeshData* mesh, bool ascii = false);
    static bool importFromSTL(const QString& path, KsMeshData* mesh, bool* isBinary = nullptr);
};

class KsKN5Converter {
public:
    static bool exportToKN5(const QString& path, const KsMeshData* mesh);
    static bool importFromKN5(const QString& path, KsMeshData* mesh);
};

class KsModelConverter {
public:
    static bool convert(const QString& inputPath, const QString& outputPath, KsImportFormat from, KsExportFormat to);
    static QString detectFormat(const QString& path);
    static QStringList getSupportedImportFormats();
    static QStringList getSupportedExportFormats();
    static QString exportFormatExtension(KsExportFormat fmt);
};

class KsBatchConverter {
public:
    static int batchConvert(const QString& inputDir, const QString& outputDir, KsImportFormat from, KsExportFormat to, const QString& extFilter = "*.*");
    static int batchConvertAll(const QString& inputDir, const QString& outputDir, KsExportFormat to);
};

} // namespace ks
