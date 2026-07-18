#include "FontCreatorQmlBridge.h"
#include <QFontDatabase>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QRandomGenerator>

namespace ks {

FontCreatorQmlBridge* FontCreatorQmlBridge::s_instance = nullptr;

FontCreatorQmlBridge::FontCreatorQmlBridge(QObject* parent)
    : QObject(parent)
{
}

FontCreatorQmlBridge::~FontCreatorQmlBridge()
{
}

FontCreatorQmlBridge* FontCreatorQmlBridge::instance()
{
    if (!s_instance) {
        s_instance = new FontCreatorQmlBridge();
    }
    return s_instance;
}

QStringList FontCreatorQmlBridge::getSystemFonts()
{
    return QFontDatabase::families();
}

bool FontCreatorQmlBridge::generateAtlas(const QString& outputPngPath)
{
    m_isGenerating = true;
    emit generatingChanged();
    m_statusMessage = "Generating atlas...";
    emit statusMessageChanged();

    // Placeholder: create a simple atlas
    QImage atlas(m_atlasWidth, m_atlasHeight, QImage::Format_RGBA8888);
    atlas.fill(Qt::transparent);
    m_lastAtlas = atlas;
    m_isGenerating = false;
    emit generatingChanged();
    emit atlasGenerated(outputPngPath, m_atlasWidth, m_atlasHeight);
    return atlas.save(outputPngPath);
}

bool FontCreatorQmlBridge::generateSDFAtlas(const QString& outputPngPath, int spread)
{
    Q_UNUSED(spread);
    m_isGenerating = true;
    emit generatingChanged();
    m_statusMessage = "Generating SDF atlas...";
    emit statusMessageChanged();

    QImage atlas(m_atlasWidth, m_atlasHeight, QImage::Format_RGBA8888);
    atlas.fill(Qt::transparent);
    m_lastAtlas = atlas;
    m_isGenerating = false;
    emit generatingChanged();
    emit atlasGenerated(outputPngPath, m_atlasWidth, m_atlasHeight);
    return atlas.save(outputPngPath);
}

bool FontCreatorQmlBridge::generateMSDFAtlas(const QString& outputPngPath, int spread)
{
    Q_UNUSED(spread);
    m_isGenerating = true;
    emit generatingChanged();
    m_statusMessage = "Generating MSDF atlas...";
    emit statusMessageChanged();

    QImage atlas(m_atlasWidth, m_atlasHeight, QImage::Format_RGBA8888);
    atlas.fill(Qt::transparent);
    m_lastAtlas = atlas;
    m_isGenerating = false;
    emit generatingChanged();
    emit atlasGenerated(outputPngPath, m_atlasWidth, m_atlasHeight);
    return atlas.save(outputPngPath);
}

bool FontCreatorQmlBridge::saveAtlas(const QString& pngPath, const QString& acfPath)
{
    Q_UNUSED(acfPath);
    if (!m_lastAtlas.isNull()) {
        return m_lastAtlas.save(pngPath);
    }
    return false;
}

bool FontCreatorQmlBridge::loadPreset(const QString& acfPath)
{
    QFile file(acfPath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    
    QByteArray data = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) return false;
    
    QJsonObject obj = doc.object();
    setCurrentConfig(obj.toVariantMap());
    m_currentFont = obj["fontFamily"].toString();
    emit presetLoaded(acfPath);
    return true;
}

bool FontCreatorQmlBridge::savePreset(const QString& acfPath)
{
    QFile file(acfPath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    
    QJsonObject obj = QJsonObject::fromVariantMap(getCurrentConfig());
    obj["fontFamily"] = m_currentFont;
    QJsonDocument doc(obj);
    file.write(doc.toJson(QJsonDocument::Indented));
    emit presetSaved(acfPath);
    return true;
}

QVariantMap FontCreatorQmlBridge::getCurrentConfig()
{
    QVariantMap config;
    config["fontFamily"] = m_fontFamily;
    config["fontWeight"] = m_fontWeight;
    config["italic"] = m_italic;
    config["charset"] = m_charset;
    config["globalHeight"] = m_globalHeight;
    config["globalVPad"] = m_globalVPad;
    config["atlasWidth"] = m_atlasWidth;
    config["atlasHeight"] = m_atlasHeight;
    config["glyphs"] = m_glyphs;
    config["kerningPairs"] = m_kerningPairs;
    config["enabledRanges"] = m_enabledRanges;
    config["hintingEnabled"] = m_hintingEnabled;
    config["hintingLevel"] = m_hintingLevel;
    config["gridFitting"] = m_gridFitting;
    config["subpixelHinting"] = m_subpixelHinting;
    config["antiAliasMode"] = m_antiAliasMode;
    return config;
}

void FontCreatorQmlBridge::setCurrentConfig(const QVariantMap& config)
{
    m_fontFamily = config.value("fontFamily").toString();
    m_fontWeight = config.value("fontWeight").toInt();
    m_italic = config.value("italic").toBool();
    m_charset = config.value("charset").toString();
    m_globalHeight = config.value("globalHeight").toInt();
    m_globalVPad = config.value("globalVPad").toInt();
    m_atlasWidth = config.value("atlasWidth").toInt();
    m_atlasHeight = config.value("atlasHeight").toInt();
    m_glyphs = config.value("glyphs").toList();
    m_kerningPairs = config.value("kerningPairs").toList();
    m_enabledRanges = config.value("enabledRanges").toStringList();
    m_hintingEnabled = config.value("hintingEnabled").toBool();
    m_hintingLevel = config.value("hintingLevel").toInt();
    m_gridFitting = config.value("gridFitting").toBool();
    m_subpixelHinting = config.value("subpixelHinting").toBool();
    m_antiAliasMode = config.value("antiAliasMode").toInt();
}

void FontCreatorQmlBridge::setFontFamily(const QString& family)
{
    m_fontFamily = family;
}

void FontCreatorQmlBridge::setFontWeight(int weight)
{
    m_fontWeight = weight;
}

void FontCreatorQmlBridge::setItalic(bool italic)
{
    m_italic = italic;
}

void FontCreatorQmlBridge::setCharset(const QString& charset)
{
    m_charset = charset;
}

void FontCreatorQmlBridge::setGlobalHeight(int height)
{
    m_globalHeight = height;
}

void FontCreatorQmlBridge::setGlobalVPad(int vpad)
{
    m_globalVPad = vpad;
}

void FontCreatorQmlBridge::addGlyph(uint codepoint, int cellWidth, int hPad, int vPad)
{
    QVariantMap glyph;
    glyph["codepoint"] = static_cast<int>(codepoint);
    glyph["cellWidth"] = cellWidth;
    glyph["hPad"] = hPad;
    glyph["vPad"] = vPad;
    m_glyphs.append(glyph);
}

void FontCreatorQmlBridge::removeGlyph(uint codepoint)
{
    for (int i = 0; i < m_glyphs.size(); ++i) {
        if (m_glyphs[i].toMap().value("codepoint").toInt() == static_cast<int>(codepoint)) {
            m_glyphs.removeAt(i);
            return;
        }
    }
}

QVariantList FontCreatorQmlBridge::getGlyphs()
{
    return m_glyphs;
}

QString FontCreatorQmlBridge::getPreviewText()
{
    return m_previewText;
}

void FontCreatorQmlBridge::setPreviewText(const QString& text)
{
    m_previewText = text;
}

QString FontCreatorQmlBridge::exportToACF(const QString& acfPath)
{
    Q_UNUSED(acfPath);
    return "";
}

QString FontCreatorQmlBridge::exportToJSON(const QString& jsonPath)
{
    QFile file(jsonPath);
    if (!file.open(QIODevice::WriteOnly)) return "";
    
    QJsonObject obj = QJsonObject::fromVariantMap(getCurrentConfig());
    QJsonDocument doc(obj);
    file.write(doc.toJson(QJsonDocument::Indented));
    return jsonPath;
}

bool FontCreatorQmlBridge::importFromJSON(const QString& jsonPath)
{
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    
    QByteArray data = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) return false;
    
    setCurrentConfig(doc.object().toVariantMap());
    return true;
}

QVariantMap FontCreatorQmlBridge::getDefaultConfig()
{
    QVariantMap config;
    config["fontFamily"] = "";
    config["fontWeight"] = 400;
    config["italic"] = false;
    config["charset"] = "";
    config["globalHeight"] = 85;
    config["globalVPad"] = 13;
    config["atlasWidth"] = 512;
    config["atlasHeight"] = 512;
    config["glyphs"] = QVariantList();
    config["kerningPairs"] = QVariantList();
    config["enabledRanges"] = QStringList();
    config["hintingEnabled"] = true;
    config["hintingLevel"] = 0;
    config["gridFitting"] = true;
    config["subpixelHinting"] = false;
    config["antiAliasMode"] = 1;
    return config;
}

QStringList FontCreatorQmlBridge::getCommonCharsets()
{
    return {"ASCII", "Latin-1", "Latin Extended", "Cyrillic", "Greek", "Custom"};
}

QStringList FontCreatorQmlBridge::getAvailableRanges()
{
    return {"Basic Latin", "Latin-1 Supplement", "Latin Extended-A", "Latin Extended-B"};
}

void FontCreatorQmlBridge::enableRange(const QString& rangeName, bool enable)
{
    if (enable && !m_enabledRanges.contains(rangeName)) {
        m_enabledRanges.append(rangeName);
    } else if (!enable) {
        m_enabledRanges.removeAll(rangeName);
    }
}

QStringList FontCreatorQmlBridge::getEnabledRanges()
{
    return m_enabledRanges;
}

bool FontCreatorQmlBridge::applyCombinedCharset()
{
    return true;
}

void FontCreatorQmlBridge::clearRanges()
{
    m_enabledRanges.clear();
}

QVariantMap FontCreatorQmlBridge::validateCoverage()
{
    QVariantMap result;
    result["covered"] = m_glyphs.size();
    result["total"] = 256;
    result["percentage"] = m_glyphs.size() > 0 ? 100.0 * m_glyphs.size() / 256.0 : 0.0;
    return result;
}

QVariantList FontCreatorQmlBridge::getKerningPairs()
{
    return m_kerningPairs;
}

void FontCreatorQmlBridge::setKerningPair(uint left, uint right, int kerning)
{
    uint key = (left << 16) | right;
    QVariantMap pair;
    pair["left"] = static_cast<int>(left);
    pair["right"] = static_cast<int>(right);
    pair["kerning"] = kerning;
    
    for (int i = 0; i < m_kerningPairs.size(); ++i) {
        if (m_kerningPairs[i].toMap().value("left").toInt() == static_cast<int>(left) &&
            m_kerningPairs[i].toMap().value("right").toInt() == static_cast<int>(right)) {
            m_kerningPairs[i] = pair;
            return;
        }
    }
    m_kerningPairs.append(pair);
}

void FontCreatorQmlBridge::removeKerningPair(uint left, uint right)
{
    for (int i = 0; i < m_kerningPairs.size(); ++i) {
        if (m_kerningPairs[i].toMap().value("left").toInt() == static_cast<int>(left) &&
            m_kerningPairs[i].toMap().value("right").toInt() == static_cast<int>(right)) {
            m_kerningPairs.removeAt(i);
            return;
        }
    }
}

void FontCreatorQmlBridge::extractKerning()
{
    // Placeholder
}

void FontCreatorQmlBridge::clearKerningPairs()
{
    m_kerningPairs.clear();
}

int FontCreatorQmlBridge::getKerningOffset(uint left, uint right)
{
    uint key = (left << 16) | right;
    for (const auto& pair : m_kerningPairs) {
        if (pair.toMap().value("left").toInt() == static_cast<int>(left) &&
            pair.toMap().value("right").toInt() == static_cast<int>(right)) {
            return pair.toMap().value("kerning").toInt();
        }
    }
    return 0;
}

void FontCreatorQmlBridge::setHintingEnabled(bool enabled)
{
    m_hintingEnabled = enabled;
}

bool FontCreatorQmlBridge::isHintingEnabled() const
{
    return m_hintingEnabled;
}

void FontCreatorQmlBridge::setHintingLevel(int level)
{
    m_hintingLevel = level;
}

int FontCreatorQmlBridge::hintingLevel() const
{
    return m_hintingLevel;
}

void FontCreatorQmlBridge::setGridFitting(bool enabled)
{
    m_gridFitting = enabled;
}

bool FontCreatorQmlBridge::isGridFitting() const
{
    return m_gridFitting;
}

void FontCreatorQmlBridge::setSubpixelHinting(bool enabled)
{
    m_subpixelHinting = enabled;
}

bool FontCreatorQmlBridge::isSubpixelHinting() const
{
    return m_subpixelHinting;
}

void FontCreatorQmlBridge::setAntiAliasMode(int mode)
{
    m_antiAliasMode = mode;
}

int FontCreatorQmlBridge::antiAliasMode() const
{
    return m_antiAliasMode;
}

QStringList FontCreatorQmlBridge::antiAliasModeNames() const
{
    return {"None", "Standard", "Subpixel"};
}

QVariantMap FontCreatorQmlBridge::analyzeMetrics()
{
    QVariantMap result;
    result["avgWidth"] = 0;
    result["avgHeight"] = 0;
    return result;
}

void FontCreatorQmlBridge::applyOptimizedMetrics(const QVariantMap& suggestion)
{
    Q_UNUSED(suggestion);
}

} // namespace ks