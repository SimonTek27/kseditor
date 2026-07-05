#include "CarConfigEditor.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <cmath>

namespace ks {
namespace modeler {

// ============================================================================
// CarConfigEditor Implementation"
// ============================================================================

CarConfigEditor::CarConfigEditor(QObject* parent)
    : QObject(parent), m_loaded(false)
{
}

CarConfigEditor::~CarConfigEditor()
{
}

bool CarConfigEditor::loadINI(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit error("Cannot open file: " + filePath);
        return false;
    }
    
    QTextStream stream(&file);
    QString currentSection;
    m_params.clear();
    
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        
        // Skip empty lines and comments
        if (line.isEmpty() || line.startsWith(';') || line.startsWith('/')) {
            continue;
        }
        
        // Section header
        if (line.startsWith('[') && line.endsWith(']')) {
            currentSection = line.mid(1, line.length() - 2);
            continue;
        }
        
        // Key=value pair
        int eqPos = line.indexOf('=');
        if (eqPos > 0) {
            QString key = line.left(eqPos).trimmed();
            QString value = line.mid(eqPos + 1).trimmed();
            addParameter(currentSection, key, value);
        }
    }
    
    file.close();
    m_filePath = filePath;
    m_loaded = true;
    emit configLoaded(filePath);
    emit configChanged();
    
    qInfo() << "CarConfigEditor: Loaded INI" << filePath << "with" << m_params.size() << "parameters";
    return true;
}

bool CarConfigEditor::saveINI(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit error("Cannot write file: " + filePath);
        return false;
    }
    
    QTextStream stream(&file);
    QString currentSection;
    
    for (const auto& param : m_params) {
        if (param.section != currentSection) {
            currentSection = param.section;
            stream << "\n[" << currentSection << "]\n";
        }
        stream << param.key << "=" << param.value.toString() << "\n";
    }
    
    file.close();
    qInfo() << "CarConfigEditor: Saved INI" << filePath;
    return true;
}

CarConfigParameter CarConfigEditor::getParameter(int index) const
{
    return (index >= 0 && index < m_params.size()) ? m_params[index] : CarConfigParameter();
}

CarConfigParameter CarConfigEditor::findParameter(const QString& section, const QString& key) const
{
    for (const auto& param : m_params) {
        if (param.section == section && param.key == key) {
            return param;
        }
    }
    return CarConfigParameter();
}

void CarConfigEditor::setParameterValue(int index, const QVariant& value)
{
    if (index >= 0 && index < m_params.size()) {
        m_params[index].value = value;
        emit parameterChanged(index, value);
        emit configChanged();
    }
}

void CarConfigEditor::setParameterValue(const QString& section, const QString& key, const QVariant& value)
{
    for (int i = 0; i < m_params.size(); ++i) {
        if (m_params[i].section == section && m_params[i].key == key) {
            m_params[i].value = value;
            emit parameterChanged(i, value);
            emit configChanged();
            return;
        }
    }
}

QStringList CarConfigEditor::getSections() const
{
    QStringList sections;
    for (const auto& param : m_params) {
        if (!sections.contains(param.section)) {
            sections.append(param.section);
        }
    }
    return sections;
}

int CarConfigEditor::getParametersInSection(const QString& section) const
{
    int count = 0;
    for (const auto& param : m_params) {
        if (param.section == section) count++;
    }
    return count;
}

void CarConfigEditor::populateTable(QTableWidget* table)
{
    if (!table) return;
    
    int totalParams = m_params.size();
    
    table->clear();
    table->setRowCount(totalParams);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"Section", "Key", "Value", "Type"});
    
    for (int i = 0; i < totalParams; ++i) {
        const auto& param = m_params[i];
        table->setItem(i, 0, new QTableWidgetItem(param.section));
        table->setItem(i, 1, new QTableWidgetItem(param.key));
        table->setItem(i, 2, new QTableWidgetItem(param.value.toString()));
        table->setItem(i, 3, new QTableWidgetItem(param.type));
    }
}

void CarConfigEditor::updateFromTable(QTableWidget* table)
{
    if (!table) return;
    
    int rows = table->rowCount();
    if (rows != m_params.size()) {
        return; // Mismatch
    }
    
    for (int i = 0; i < rows; ++i) {
        if (table->item(i, 2)) {
            m_params[i].value = table->item(i, 2)->text();
        }
    }
    
    emit configChanged();
}

void CarConfigEditor::applyCarPreset(const QString& presetName)
{
    // Apply common presets for AC cars
    if (presetName == "GT3") {
        setParameterValue("ENGINE", "POWER", "450");
        setParameterValue("ENGINE", "TORQUE", "500");
        setParameterValue("BODY", "WEIGHT", "1300");
    } else if (presetName == "GT4") {
        setParameterValue("ENGINE", "POWER", "300");
        setParameterValue("ENGINE", "TORQUE", "350");
        setParameterValue("BODY", "WEIGHT", "1100");
    }
}

QStringList CarConfigEditor::getAvailablePresets() const
{
    return {"GT3", "GT4", "Touring", "Superbike", "F1"}; // Example presets
}

void CarConfigEditor::parseINISection(const QString& section, const QMap<QString, QVariant>& values)
{
    for (auto it = values.begin(); it != values.end(); ++it) {
        addParameter(section, it.key(), it.value());
    }
}

void CarConfigEditor::addParameter(const QString& section, const QString& key, const QVariant& value)
{
    CarConfigParameter param;
    param.section = section;
    param.key = key;
    param.value = value;
    param.displayName = key;
    param.type = "string"; // Default
    
    // Try to detect type
    bool ok;
    value.toInt(&ok);
    if (ok) param.type = "int";
    else {
        value.toFloat(&ok);
        if (ok) param.type = "float";
    }
    
    m_params.append(param);
}

// ============================================================================
// KsAssetBrowser Implementation (simple file-scanner)
// ============================================================================

#include <QDebug>

KsAssetBrowser::KsAssetBrowser(QObject* parent)
    : QObject(parent)
{
}

KsAssetBrowser::~KsAssetBrowser()
{
}

void KsAssetBrowser::setContentPath(const QString& path)
{
    m_contentPath = path;
    refresh();
}

void KsAssetBrowser::refresh()
{
    m_assets.clear();
    scanCars();
    scanTracks();
    scanTextures();
    scanSounds();
    scanPhysics();
    emit assetsRefreshed();
    qInfo() << "KsAssetBrowser: Found" << m_assets.size() << "assets";
}

void KsAssetBrowser::scanCars()
{
    QDir carsDir(m_contentPath + "/cars");
    if (!carsDir.exists()) return;

    QStringList carDirs = carsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto& carDir : carDirs) {
        KsAssetBrowser::AssetInfo asset;
        asset.name = carDir;
        asset.type = "car";
        asset.filePath = carsDir.absoluteFilePath(carDir);

        QString iniPath = asset.filePath + "/car.ini";
        if (QFile::exists(iniPath)) {
            asset.metadata["hasINI"] = true;
        }

        QString kn5Path = asset.filePath + "/" + carDir + ".kn5";
        if (QFile::exists(kn5Path)) {
            asset.metadata["hasKN5"] = true;
            asset.previewPath = kn5Path;
        }

        m_assets.append(asset);
    }
}

void KsAssetBrowser::scanTracks()
{
    QDir tracksDir(m_contentPath + "/tracks");
    if (!tracksDir.exists()) return;

    QStringList trackDirs = tracksDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto& trackDir : trackDirs) {
        KsAssetBrowser::AssetInfo asset;
        asset.name = trackDir;
        asset.type = "track";
        asset.filePath = tracksDir.absoluteFilePath(trackDir);
        
        QString surfDir = asset.filePath + "/surfaces";
        if (QDir(surfDir).exists()) {
            asset.metadata["hasSurfaces"] = true;
        }
        
        m_assets.append(asset);
    }
}

void KsAssetBrowser::scanTextures()
{
    QDir texDir(m_contentPath + "/texture");
    if (!texDir.exists()) return;

    QStringList filters; filters << "*.dds" << "*.png" << "*.jpg" << "*.tga";
    QFileInfoList files = texDir.entryInfoList(filters, QDir::Files);
    
    for (const auto& file : files) {
        KsAssetBrowser::AssetInfo asset;
        asset.name = file.baseName();
        asset.type = "texture";
        asset.filePath = file.absoluteFilePath();
        asset.previewPath = file.absoluteFilePath();
        m_assets.append(asset);
    }
}

void KsAssetBrowser::scanSounds()
{
    QDir sndDir(m_contentPath + "/sounds");
    if (!sndDir.exists()) return;

    QStringList filters; filters << "*.bnk" << "*.wav" << "*.ogg";
    QFileInfoList files = sndDir.entryInfoList(filters, QDir::Files);
    
    for (const auto& file : files) {
        KsAssetBrowser::AssetInfo asset;
        asset.name = file.baseName();
        asset.type = "sound";
        asset.filePath = file.absoluteFilePath();
        m_assets.append(asset);
    }
}

void KsAssetBrowser::scanPhysics()
{
    QDir physDir(m_contentPath + "/physics");
    if (!physDir.exists()) return;

    QStringList filters; filters << "*.ini";
    QFileInfoList files = physDir.entryInfoList(filters, QDir::Files);
    
    for (const auto& file : files) {
        KsAssetBrowser::AssetInfo asset;
        asset.name = file.baseName();
        asset.type = "physics";
        asset.filePath = file.absoluteFilePath();
        m_assets.append(asset);
    }
}

QVector<KsAssetBrowser::AssetInfo> KsAssetBrowser::getAssets(const QString& type) const
{
    if (type.isEmpty()) return m_assets;
    
    QVector<KsAssetBrowser::AssetInfo> result;
    for (const auto& asset : m_assets) {
        if (asset.type == type) result.append(asset);
    }
    return result;
}

KsAssetBrowser::AssetInfo KsAssetBrowser::findAsset(const QString& name) const
{
    for (const auto& asset : m_assets) {
        if (asset.name == name) return asset;
    }
    return AssetInfo();
}

QVector<KsAssetBrowser::AssetInfo> KsAssetBrowser::search(const QString& query) const
{
    QVector<KsAssetBrowser::AssetInfo> result;
    QString lowerQuery = query.toLower();
    
    for (const auto& asset : m_assets) {
        if (asset.name.toLower().contains(lowerQuery) ||
            asset.type.toLower().contains(lowerQuery)) {
            result.append(asset);
        }
    }
    return result;
}

void KsAssetBrowser::setTypeFilter(const QString& type)
{
    m_typeFilter = type;
}

bool KsAssetBrowser::generatePreview(const QString& assetPath, const QString& outputPath)
{
    if (!QFile::exists(assetPath)) return false;

    // Generate a preview thumbnail from the asset file
    QImage preview(256, 256, QImage::Format_ARGB32);
    preview.fill(QColor(40, 40, 45));

    QPainter painter(&preview);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw a preview icon based on file type
    if (assetPath.endsWith(".kn5", Qt::CaseInsensitive) || assetPath.endsWith(".fbx", Qt::CaseInsensitive)) {
        // Draw a 3D model icon
        painter.setBrush(QColor(70, 120, 180));
        painter.setPen(Qt::NoPen);
        QPointF verts[] = {{128,30}, {210,80}, {210,180}, {128,220}, {46,180}, {46,80}};
        painter.drawPolygon(verts, 6);
    } else if (assetPath.endsWith(".png", Qt::CaseInsensitive) || assetPath.endsWith(".jpg", Qt::CaseInsensitive)) {
        QImage img(assetPath);
        if (!img.isNull()) {
            QPixmap scaled = QPixmap::fromImage(img).scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            painter.drawPixmap(0, 0, scaled);
        }
    } else {
        painter.setPen(QColor(180, 180, 180));
        painter.setFont(QFont("Arial", 12));
        painter.drawText(preview.rect(), Qt::AlignCenter, QFileInfo(assetPath).suffix().toUpper());
    }
    painter.end();

    if (preview.save(outputPath)) {
        return true;
    }
    return false;
}

QString KsAssetBrowser::getPreviewPath(const QString& assetPath) const
{
    for (const auto& asset : m_assets) {
        if (asset.filePath == assetPath) return asset.previewPath;
    }
    return QString();
}

// ============================================================================
// ReferenceImageSystem Implementation
// ============================================================================

ReferenceImageSystem::ReferenceImageSystem(QObject* parent)
    : QObject(parent)
{
}

ReferenceImageSystem::~ReferenceImageSystem()
{
}

int ReferenceImageSystem::addImage(const QString& imagePath, const QString& name)
{
    ReferenceImage img;
    img.name = name.isEmpty() ? QFileInfo(imagePath).baseName() : name;
    img.imagePath = imagePath;
    img.position = QVector3D(0, 0, 0);
    img.rotation = QVector3D(0, 0, 0);
    img.scale = QVector3D(1, 1, 1);
    img.opacity = 1.0f;
    img.visible = true;
    img.drawLayer = 0;
    
    int index = m_images.size();
    m_images.append(img);
    emit imageAdded(index, img.name);
    return index;
}

bool ReferenceImageSystem::removeImage(int index)
{
    if (index < 0 || index >= m_images.size()) return false;
    m_images.removeAt(index);
    emit imageRemoved(index);
    return true;
}

void ReferenceImageSystem::clearImages()
{
    m_images.clear();
    emit imageRemoved(-1);
}

ReferenceImageSystem::ReferenceImage ReferenceImageSystem::getImage(int index) const
{
    if (index >= 0 && index < m_images.size())
        return m_images[index];
    return ReferenceImage();
}

void ReferenceImageSystem::setImagePosition(int index, const QVector3D& pos)
{
    if (index >= 0 && index < m_images.size()) {
        m_images[index].position = pos;
        emit imageChanged(index);
    }
}

void ReferenceImageSystem::setImageRotation(int index, const QVector3D& rot)
{
    if (index >= 0 && index < m_images.size()) {
        m_images[index].rotation = rot;
        emit imageChanged(index);
    }
}

void ReferenceImageSystem::setImageScale(int index, const QVector3D& scale)
{
    if (index >= 0 && index < m_images.size()) {
        m_images[index].scale = scale;
        emit imageChanged(index);
    }
}

void ReferenceImageSystem::setImageOpacity(int index, float opacity)
{
    if (index >= 0 && index < m_images.size()) {
        m_images[index].opacity = opacity;
        emit imageChanged(index);
    }
}

void ReferenceImageSystem::setImageVisibility(int index, bool visible)
{
    if (index >= 0 && index < m_images.size()) {
        m_images[index].visible = visible;
        emit imageChanged(index);
    }
}

void ReferenceImageSystem::setDrawLayer(int index, int layer)
{
    if (index >= 0 && index < m_images.size()) {
        m_images[index].drawLayer = layer;
        emit imageChanged(index);
    }
}

} // namespace modeler
} // namespace ks
