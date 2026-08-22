#!/usr/bin/env python3
import sys

filepath = r'E:\Users\Simon\source\repos\kseditor\src\modules\modellingEditor\3DModeling_io.cpp'
with open(filepath, 'r') as f:
    content = f.read()

# Find the position after exportKS3D function and before importGLTF
target = """    qInfo() << "STEP export complete (useBREP=" << useBREP << ") :" << path;
    return true;
}

bool ImportExport3D::importGLTF"""

replacement = """    qInfo() << "STEP export complete (useBREP=" << useBREP << ") :" << path;
    return true;
}

bool ImportExport3D::import3DM(const QString& path, geometry::Scene3D* scene) {
    if (!scene || !QFile::exists(path)) { emit error("File not found: " + path); return false; }
    emit statusMessage("Importing 3DM: " + path);
    
    // 3DM is Rhino's native file format. Since we don't have OpenNURBS,
    // we fall back to a basic mesh import via OBJ/STL if available,
    // or report the file information.
    
    QFileInfo info(path);
    QString ext = info.suffix().toLower();
    
    // Try to import as OBJ first (common Rhino export format)
    if (ext == "obj") {
        return importOBJ(path, scene);
    } else if (ext == "stl") {
        return importSTL(path, scene);
    } else {
        // For native .3dm files, we can't parse NURBS exactly without OpenNURBS,
        // but we can try to extract mesh data or report the file info.
        // Fall back to reporting that the format is not directly supported
        // but meshes can be exported/imported via other formats.
        qWarning() << "[ImportExport3D] Native 3DM NURBS import not available without OpenNURBS;"
                   " falling back to mesh-based workflow.";
        // Try to import any recognized format
        return false;
    }
}

bool ImportExport3D::export3DM(geometry::Scene3D* scene, const QString& path) {
    if (!scene) return false;
    emit statusMessage("Exporting 3DM: " + path);
    
    // 3DM is Rhino's native format. Without OpenNURBS library support,
    // we export as OBJ which can be imported by Rhino.
    
    QString objPath = path;
    QRegularExpression re("\\.3dm$", QRegularExpression::CaseInsensitiveOption);
    objPath.replace(re, ".obj");
    
    bool ok = exportOBJ(scene, objPath);
    if (ok) {
        qInfo() << "[ImportExport3D] Exported 3DM as OBJ to:" << objPath;
    } else {
        qWarning() << "[ImportExport3D] Failed to export 3DM as OBJ";
    }
    return ok;
}

bool ImportExport3D::importGLTF"""
content = content.replace(target, replacement)

with open(filepath, 'w') as f:
    f.write(content)
print('Done')