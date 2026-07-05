#include "PPFiltersQmlBridge.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QSettings>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "../sys/LogManager.h"

namespace ks {

PPFiltersQmlBridge* PPFiltersQmlBridge::s_instance = nullptr;

PPFiltersQmlBridge* PPFiltersQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new PPFiltersQmlBridge();
    }
    return s_instance;
}

void PPFiltersQmlBridge::setCurrentFilter(const QString& filter) {
    if (m_currentFilter != filter) {
        m_currentFilter = filter;
        emit currentFilterChanged();
    }
}

void PPFiltersQmlBridge::loadFiltersFromDirectory(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) return;

    m_filters.clear();
    QStringList iniFiles = dir.entryList(QStringList() << "*.ini", QDir::Files);

    for (const auto& file : iniFiles) {
        QString filePath = dir.absoluteFilePath(file);
        QVariantMap filter = parseFilterINI(filePath);
        QString baseName = QFileInfo(file).baseName();
        filter["_path"] = filePath;
        filter["_name"] = baseName;
        filter["id"] = baseName;
        filter["name"] = baseName;
        filter["author"] = filter.value("DESCRIPTION.author", "").toString();
        filter["preview"] = QColor::fromHsv(qHash(baseName) % 360, 180, 80).name();
        m_filters.append(filter);
    }

    m_filterCount = m_filters.size();
    emit filterCountChanged();
}

void PPFiltersQmlBridge::loadFilter(const QString& path) {
    m_currentFilter = path;
    m_currentParams = parseFilterINI(path);
    emit currentFilterChanged();
    emit filterLoaded();
}

void PPFiltersQmlBridge::saveFilter(const QString& path) {
    m_currentFilter = path;
    writeFilterINI(path, m_currentParams);
    emit currentFilterChanged();
    emit filterSaved();
}

void PPFiltersQmlBridge::exportFilter(const QString& path) {
    saveFilter(path);
}

QVariantList PPFiltersQmlBridge::getFilters() {
    if (m_filters.isEmpty() && !m_currentFilter.isEmpty()) {
        loadFiltersFromDirectory(QFileInfo(m_currentFilter).absolutePath());
    }
    return m_filters;
}

QVariantMap PPFiltersQmlBridge::getFilter(int index) {
    if (index < 0 || index >= m_filters.size()) return QVariantMap();
    return m_filters[index].toMap();
}

QVariantList PPFiltersQmlBridge::getParameters() {
    QVariantList params;
    for (auto it = m_currentParams.constBegin(); it != m_currentParams.constEnd(); ++it) {
        QVariantMap p;
        p["name"] = it.key();
        p["value"] = it.value();
        params.append(p);
    }
    return params;
}

QVariantMap PPFiltersQmlBridge::getParameterMap(const QString& name) {
    QVariantMap p;
    p["name"] = name;
    p["value"] = m_currentParams.value(name);
    return p;
}

void PPFiltersQmlBridge::setParameter(const QString& name, float value) {
    m_currentParams[name] = value;
    emit parameterChanged(name, value);
}

float PPFiltersQmlBridge::getParameterValue(const QString& name) {
    return m_currentParams.value(name).toFloat();
}

void PPFiltersQmlBridge::resetParameters() {
    if (!m_currentFilter.isEmpty()) {
        m_currentParams = parseFilterINI(m_currentFilter);
        emit filterLoaded();
    }
}

void PPFiltersQmlBridge::applyPreset(const QString& presetName) {
    QVariantMap preset = getPresetData(presetName);
    if (!preset.isEmpty()) {
        m_currentParams = preset;
        emit filterLoaded();
    }
}

QStringList PPFiltersQmlBridge::getPresets() {
    return {"Natural", "Vivid", "Cinematic", "Desaturated", "High Contrast", "Night", "Overcast", "Golden Hour"};
}

void PPFiltersQmlBridge::startPreview() {
    m_isPreviewActive = true;
    emit previewActiveChanged();
}

void PPFiltersQmlBridge::stopPreview() {
    m_isPreviewActive = false;
    emit previewActiveChanged();
}

void PPFiltersQmlBridge::reloadFilter() {
    if (!m_currentFilter.isEmpty()) {
        loadFilter(m_currentFilter);
    }
}

QVariantMap PPFiltersQmlBridge::parseFilterINI(const QString& path) {
    QVariantMap result;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return result;

    QTextStream in(&file);
    QString currentSection;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(";") || line.startsWith("#")) continue;

        if (line.startsWith("[") && line.endsWith("]")) {
            currentSection = line.mid(1, line.length() - 2);
            continue;
        }

        int eqIdx = line.indexOf("=");
        if (eqIdx > 0) {
            QString key = line.left(eqIdx).trimmed();
            QString value = line.mid(eqIdx + 1).trimmed();

            if (!currentSection.isEmpty()) {
                key = currentSection + "." + key;
            }

            bool ok = false;
            float fVal = value.toFloat(&ok);
            if (ok) {
                result[key] = fVal;
            } else {
                result[key] = value;
            }
        }
    }

    file.close();
    return result;
}

void PPFiltersQmlBridge::writeFilterINI(const QString& path, const QVariantMap& params) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);
    out << "; ksEditor PP Filter Configuration\n";
    out << "; Generated by ksEditor v1.16\n\n";

    QString currentSection;
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        QString key = it.key();
        QVariant value = it.value();

        if (key.contains(".")) {
            QString section = key.section(".", 0, 0);
            QString subKey = key.section(".", 1);
            if (section != currentSection) {
                if (!currentSection.isEmpty()) out << "\n";
                out << "[" << section << "]\n";
                currentSection = section;
                key = subKey;
            } else {
                key = subKey;
            }
        }

        out << key << " = " << value.toString() << "\n";
    }

    file.close();
}

QVariantMap PPFiltersQmlBridge::getPresetData(const QString& presetName) {
    QVariantMap preset;

    if (presetName == "Natural") {
        preset["EXPOSURE.exposure"] = 0.0f;
        preset["EXPOSURE.highlights"] = -0.1f;
        preset["EXPOSURE.shadows"] = 0.1f;
        preset["COLOR.saturation"] = 1.0f;
        preset["BLOOM.amount"] = 0.3f;
        preset["LENS.vignette"] = 0.15f;
    } else if (presetName == "Vivid") {
        preset["EXPOSURE.exposure"] = 0.1f;
        preset["EXPOSURE.highlights"] = -0.2f;
        preset["EXPOSURE.shadows"] = 0.2f;
        preset["COLOR.saturation"] = 1.3f;
        preset["COLOR.vibrance"] = 0.2f;
        preset["BLOOM.amount"] = 0.5f;
        preset["LENS.vignette"] = 0.1f;
    } else if (presetName == "Cinematic") {
        preset["EXPOSURE.exposure"] = -0.1f;
        preset["EXPOSURE.highlights"] = -0.3f;
        preset["EXPOSURE.shadows"] = 0.3f;
        preset["COLOR.saturation"] = 0.8f;
        preset["COLOR.temperature"] = -10.0f;
        preset["BLOOM.amount"] = 0.4f;
        preset["LENS.vignette"] = 0.3f;
        preset["LENS.grain"] = 0.1f;
    } else if (presetName == "Desaturated") {
        preset["COLOR.saturation"] = 0.5f;
        preset["EXPOSURE.exposure"] = 0.0f;
        preset["BLOOM.amount"] = 0.2f;
        preset["LENS.vignette"] = 0.2f;
    } else if (presetName == "High Contrast") {
        preset["EXPOSURE.exposure"] = 0.0f;
        preset["EXPOSURE.highlights"] = 0.2f;
        preset["EXPOSURE.shadows"] = -0.2f;
        preset["COLOR.saturation"] = 1.1f;
        preset["BLOOM.amount"] = 0.2f;
        preset["LENS.vignette"] = 0.25f;
    } else if (presetName == "Night") {
        preset["EXPOSURE.exposure"] = -0.5f;
        preset["EXPOSURE.highlights"] = -0.5f;
        preset["EXPOSURE.shadows"] = 0.5f;
        preset["COLOR.saturation"] = 0.7f;
        preset["COLOR.temperature"] = -20.0f;
        preset["BLOOM.amount"] = 0.8f;
        preset["LENS.vignette"] = 0.4f;
    } else if (presetName == "Overcast") {
        preset["EXPOSURE.exposure"] = 0.2f;
        preset["EXPOSURE.highlights"] = -0.1f;
        preset["EXPOSURE.shadows"] = 0.3f;
        preset["COLOR.saturation"] = 0.9f;
        preset["COLOR.temperature"] = 5.0f;
        preset["BLOOM.amount"] = 0.2f;
        preset["LENS.vignette"] = 0.1f;
    } else if (presetName == "Golden Hour") {
        preset["EXPOSURE.exposure"] = 0.1f;
        preset["EXPOSURE.highlights"] = -0.15f;
        preset["EXPOSURE.shadows"] = 0.15f;
        preset["COLOR.saturation"] = 1.2f;
        preset["COLOR.temperature"] = 25.0f;
        preset["BLOOM.amount"] = 0.6f;
        preset["LENS.vignette"] = 0.2f;
    }

    return preset;
}

// --- Color Grading ---

void PPFiltersQmlBridge::setColorGradingParam(const QString& name, float value)
{
    auto p = m_colorGrading.params();
    if (name == "exposure") p.exposure = value;
    else if (name == "gamma") p.gamma = value;
    else if (name == "contrast") p.contrast = value;
    else if (name == "brightness") p.brightness = value;
    else if (name == "saturation") p.saturation = value;
    else if (name == "vibrance") p.vibrance = value;
    else if (name == "temperature") p.temperature = value;
    else if (name == "tint") p.tint = value;
    else if (name == "liftR") p.liftR = value;
    else if (name == "liftG") p.liftG = value;
    else if (name == "liftB") p.liftB = value;
    else if (name == "gammaR") p.gammaR = value;
    else if (name == "gammaG") p.gammaG = value;
    else if (name == "gammaB") p.gammaB = value;
    else if (name == "gainR") p.gainR = value;
    else if (name == "gainG") p.gainG = value;
    else if (name == "gainB") p.gainB = value;
    else if (name == "shadowsSat") p.shadowsSat = value;
    else if (name == "highlightsSat") p.highlightsSat = value;
    else if (name == "balance") p.balance = value;
    else if (name == "lutIntensity") p.lutIntensity = value;
    m_colorGrading.setParams(p);
    emit colorGradingChanged();
}

QVariantMap PPFiltersQmlBridge::getColorGradingParams() const
{
    auto p = m_colorGrading.params();
    QVariantMap map;
    map["exposure"] = p.exposure;
    map["gamma"] = p.gamma;
    map["contrast"] = p.contrast;
    map["brightness"] = p.brightness;
    map["saturation"] = p.saturation;
    map["vibrance"] = p.vibrance;
    map["temperature"] = p.temperature;
    map["tint"] = p.tint;
    map["liftR"] = p.liftR; map["liftG"] = p.liftG; map["liftB"] = p.liftB;
    map["gammaR"] = p.gammaR; map["gammaG"] = p.gammaG; map["gammaB"] = p.gammaB;
    map["gainR"] = p.gainR; map["gainG"] = p.gainG; map["gainB"] = p.gainB;
    map["shadowsSat"] = p.shadowsSat;
    map["highlightsSat"] = p.highlightsSat;
    map["balance"] = p.balance;
    map["lutIntensity"] = p.lutIntensity;
    return map;
}

void PPFiltersQmlBridge::applyColorGradingPreset(const QString& name)
{
    ColorGradingParams p;
    if (name == "Cinematic") p = PPFilterColorGrading::cinematicParams();
    else if (name == "Vivid") p = PPFilterColorGrading::vividParams();
    else if (name == "Vintage") p = PPFilterColorGrading::vintageParams();
    else if (name == "Moody") p = PPFilterColorGrading::moodyParams();
    else p = PPFilterColorGrading::defaultParams();
    m_colorGrading.setParams(p);
    emit colorGradingChanged();
}

QVariantList PPFiltersQmlBridge::getColorGradingPresets() const
{
    return {"Default", "Cinematic", "Vivid", "Vintage", "Moody"};
}

bool PPFiltersQmlBridge::exportCubeLUT(const QString& path)
{
    return m_colorGrading.exportCubeLUT(path);
}

QString PPFiltersQmlBridge::getColorGradingJson() const
{
    return QString::fromUtf8(QJsonDocument(m_colorGrading.toJson()).toJson());
}

void PPFiltersQmlBridge::loadColorGradingJson(const QString& json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isObject()) {
        m_colorGrading.fromJson(doc.object());
        emit colorGradingChanged();
    }
}

void PPFiltersQmlBridge::syncColorGradingToUI()
{
    emit colorGradingChanged();
}

bool PPFiltersQmlBridge::exportToAC(const QString& acPath, const QString& filterName)
{
    if (acPath.isEmpty() || filterName.isEmpty()) return false;

    QString ppfiltersDir = acPath + "/system/cfg/ppfilters";
    QDir().mkpath(ppfiltersDir);

    QString filePath = ppfiltersDir + "/" + filterName;
    if (!filePath.endsWith(".ini")) filePath += ".ini";

    writeFilterINI(filePath, m_currentParams);

    auto cg = m_colorGrading.params();
    m_currentParams["COLOR_GRADING.exposure"] = cg.exposure;
    m_currentParams["COLOR_GRADING.gamma"] = cg.gamma;
    m_currentParams["COLOR_GRADING.contrast"] = cg.contrast;
    m_currentParams["COLOR_GRADING.brightness"] = cg.brightness;
    m_currentParams["COLOR_GRADING.saturation"] = cg.saturation;
    m_currentParams["COLOR_GRADING.vibrance"] = cg.vibrance;
    m_currentParams["COLOR_GRADING.temperature"] = cg.temperature;
    m_currentParams["COLOR_GRADING.tint"] = cg.tint;
    writeFilterINI(filePath, m_currentParams);

    m_currentFilter = filePath;
    emit currentFilterChanged();
    emit filterSaved();
    return true;
}

void PPFiltersQmlBridge::exportToACDialog()
{
    QString path = QFileDialog::getSaveFileName(
        nullptr,
        "Export Filter to AC",
        QDir::homePath() + "/.local/share/Steam/steamapps/common/assettocorsa/system/cfg/ppfilters/my_filter.ini",
        "PP Filter (*.ini)"
    );
    if (!path.isEmpty()) {
        QString name = QFileInfo(path).baseName();
        QString acPath = path;
        // Walk up to find assettocorsa root
        while (!acPath.isEmpty() && !QDir(acPath + "/system/cfg").exists()) {
            acPath = QFileInfo(acPath).path();
        }
        if (!acPath.isEmpty()) {
            exportToAC(acPath, name);
        } else {
            // Fallback: just save to the selected path
            writeFilterINI(path, m_currentParams);
            m_currentFilter = path;
            emit currentFilterChanged();
            emit filterSaved();
        }
    }
}

// ============================================================================
// PPFiltersEditorModule
// ============================================================================

PPFiltersEditorModule::PPFiltersEditorModule(QWidget* parent)
    : EditorModule(parent)
{}

bool PPFiltersEditorModule::initialize()
{
    LOG_INFO("PPFiltersEditorModule", "PP Filters Editor module initialized");
    return true;
}

void PPFiltersEditorModule::shutdown()
{
    LOG_INFO("PPFiltersEditorModule", "PP Filters Editor module shutdown");
}

void PPFiltersEditorModule::importFile(const QString& filePath)
{
    if (auto* bridge = PPFiltersQmlBridge::instance()) {
        bridge->loadFilter(filePath);
    }
}

void PPFiltersEditorModule::exportFile(const QString& filePath)
{
    if (auto* bridge = PPFiltersQmlBridge::instance()) {
        bridge->saveFilter(filePath);
    }
}

QJsonObject PPFiltersEditorModule::serializeProject() const
{
    QJsonObject data;
    auto* bridge = PPFiltersQmlBridge::instance();
    if (bridge) {
        data["currentFilter"] = bridge->currentFilter();
    }
    return data;
}

void PPFiltersEditorModule::deserializeProject(const QJsonObject& data)
{
    if (data.contains("currentFilter")) {
        QString filterPath = data["currentFilter"].toString();
        if (!filterPath.isEmpty()) {
            importFile(filterPath);
        }
    }
}

} // namespace ks
