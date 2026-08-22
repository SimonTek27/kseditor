#include "AssetListModel.h"
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QSettings>
#include <QSet>

AssetListModel::AssetListModel(QObject* parent) : QAbstractListModel(parent) {
    scanAssets();
}

int AssetListModel::rowCount(const QModelIndex&) const { return m_assets.size(); }

QVariant AssetListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_assets.size()) return {};
    const auto& a = m_assets[index.row()];
    switch (role) {
        case NameRole: return a.name;
        case TypeRole: return a.type;
        case IconRole: return a.icon;
        case PathRole: return a.path;
        case SizeRole: return a.size;
        default: return {};
    }
}

QHash<int, QByteArray> AssetListModel::roleNames() const {
    return {
        {NameRole, "assetName"},
        {TypeRole, "assetType"},
        {IconRole, "assetIcon"},
        {PathRole, "assetPath"},
        {SizeRole, "assetSize"},
    };
}

void AssetListModel::refresh() {
    beginResetModel();
    scanAssets();
    endResetModel();
}

QString AssetListModel::assetPath(int row) const {
    if (row >= 0 && row < m_assets.size()) return m_assets[row].path;
    return {};
}

void AssetListModel::scanAssets() {
    m_assets.clear();
    QStringList dirs = {
        QCoreApplication::applicationDirPath() + "/resources/assets",
        QCoreApplication::applicationDirPath() + "/../resources/assets",
    };

    QSettings s;
    QString acContent = s.value(QGuiApplication::organizationName() + "/contentPath", "").toString();
    if (!acContent.isEmpty() && QDir(acContent).exists()) {
        dirs << acContent + "/cars";
        dirs << acContent + "/tracks";
        dirs << acContent + "/texture";
        dirs << acContent + "/shared";
    }

    for (const QString& baseDir : dirs) {
        QDir dir(baseDir);
        if (!dir.exists()) continue;
        scanDir(dir, "");
        if (m_assets.size() > 500) break;
    }
    if (m_assets.isEmpty()) {
        m_assets.append({"body_paint", "Texture", "#0070ff", "", "-"});
        m_assets.append({"plastic_black", "Material", "#222222", "", "-"});
        m_assets.append({"glass", "Material", "#88ccff", "", "-"});
        m_assets.append({"chrome", "Material", "#cccccc", "", "-"});
        m_assets.append({"rubber", "Material", "#333333", "", "-"});
        m_assets.append({"carbon_fiber", "Texture", "#555555", "", "-"});
        m_assets.append({"metal", "Material", "#aaaaaa", "", "-"});
        m_assets.append({"leather", "Material", "#8B4513", "", "-"});
    }
}

void AssetListModel::scanDir(const QDir& dir, const QString& relPath) {
    static const QSet<QString> skipDirs = {"__pycache__", ".git", "node_modules", ".svn"};
    for (const QFileInfo& fi : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        if (skipDirs.contains(fi.fileName())) continue;
        scanDir(QDir(fi.absoluteFilePath()), relPath + fi.fileName() + "/");
        if (m_assets.size() > 500) return;
    }
    for (const QFileInfo& fi : dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name)) {
        QString ext = fi.suffix().toLower();
        QString type = "File";
        QString icon = "#888888";
        if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "tga" || ext == "dds") { type = "Texture"; icon = "#0070ff"; }
        else if (ext == "wav" || ext == "ogg" || ext == "mp3" || ext == "flac") { type = "Sound"; icon = "#10b981"; }
        else if (ext == "kn5" || ext == "fbx" || ext == "obj" || ext == "dae") { type = "Model"; icon = "#f59e0b"; }
        else if (ext == "acf") { type = "Font"; icon = "#a78bfa"; }
        else if (ext == "ini" || ext == "json" || ext == "xml") { type = "Config"; icon = "#6b7280"; }
        else if (ext == "ksproj") { type = "Project"; icon = "#FF0000"; }
        else if (ext == "ksaudio") { type = "Audio"; icon = "#FF0000"; }
        else if (ext == "ks") { type = "Package"; icon = "#8b5cf6"; }
        else if (ext == "preset") { type = "Preset"; icon = "#FF0000"; }
        else if (ext == "replay") { type = "Replay"; icon = "#FF0000"; }
        else continue;
        m_assets.append({fi.baseName(), type, icon, fi.absoluteFilePath(), formatSize(fi.size())});
    }
}

QString AssetListModel::formatSize(qint64 bytes) {
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
}
