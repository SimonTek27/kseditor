#include "FontCreatorQmlBridge.h"
#include "FontGenerator.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include "../../core/sys/LogManager.h"

namespace ks {

FontCreatorQmlBridge* FontCreatorQmlBridge::s_instance = nullptr;

FontCreatorQmlBridge* FontCreatorQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new FontCreatorQmlBridge();
    }
    return s_instance;
}

FontCreatorQmlBridge::FontCreatorQmlBridge(QObject* parent)
    : QObject(parent), m_currentFont("Arial"), m_charset("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 .,!?;:'\"()-+=@#$%&*/\\<>[]{}|~^`")
{
}

QStringList FontCreatorQmlBridge::getSystemFonts() {
    return FontAtlasGenerator::systemFonts();
}

bool FontCreatorQmlBridge::generateAtlas(const QString& outputPngPath) {
    m_isGenerating = true;
    emit generatingChanged();
    m_statusMessage = "Generating atlas...";
    emit statusMessageChanged();

    FontAtlasGenerator generator;

    AtlasConfig config;
    config.fontFamily = m_fontFamily.isEmpty() ? m_currentFont : m_fontFamily;
    config.fontSize = m_fontSize;
    config.fontWeight = m_fontWeight;
    config.italic = m_italic;
    config.atlasWidth = m_atlasWidth;
    config.atlasHeight = m_atlasHeight;
    config.globalHeight = m_globalHeight;
    config.globalVPad = m_globalVPad;
    config.charset = m_charset.isEmpty() ? m_previewText : m_charset;

    config.hinting.enableAutoHinting = m_hintingEnabled;
    config.hinting.hintingLevel = m_hintingLevel;
    config.hinting.enableGridFitting = m_gridFitting;
    config.hinting.enableSubpixel = m_subpixelHinting;
    config.hinting.antiAlias = static_cast<AntiAliasMode>(m_antiAliasMode);

    if (!m_glyphs.isEmpty()) {
        for (const auto& gVar : m_glyphs) {
            QVariantMap g = gVar.toMap();
            GlyphMetrics gm;
            gm.codepoint = g["codepoint"].toUInt();
            gm.cellWidth = g["cellWidth"].toInt();
            gm.hPad = g["hPad"].toInt();
            gm.vPad = g["vPad"].toInt();
            config.glyphs.append(gm);
        }
    }

    connect(&generator, &FontAtlasGenerator::progress, this, [this](int percent) {
        emit generationProgress(percent);
    });

    AtlasResult result = generator.generate(config);

    if (result.success) {
        QString error;
        bool saved = generator.save(result, outputPngPath, &error);
        if (saved) {
            m_statusMessage = "Atlas generated: " + outputPngPath;
            emit statusMessageChanged();
            emit atlasGenerated(outputPngPath, result.image.width(), result.image.height());
            m_isGenerating = false;
            emit generatingChanged();
            return true;
        } else {
            m_statusMessage = "Save failed: " + error;
            emit statusMessageChanged();
        }
    } else {
        m_statusMessage = "Generation failed: " + result.errorMsg;
        emit statusMessageChanged();
    }

    m_isGenerating = false;
    emit generatingChanged();
    return false;
}

bool FontCreatorQmlBridge::generateSDFAtlas(const QString& outputPngPath, int spread) {
    m_isGenerating = true;
    emit generatingChanged();
    m_statusMessage = "Generating SDF atlas...";
    emit statusMessageChanged();

    FontAtlasGenerator generator;

    AtlasConfig config;
    config.fontFamily = m_fontFamily.isEmpty() ? m_currentFont : m_fontFamily;
    config.fontSize = m_fontSize;
    config.fontWeight = m_fontWeight;
    config.italic = m_italic;
    config.atlasWidth = m_atlasWidth;
    config.atlasHeight = m_atlasHeight;
    config.globalHeight = m_globalHeight;
    config.globalVPad = m_globalVPad;
    config.charset = m_charset.isEmpty() ? m_previewText : m_charset;

    FontAtlasGenerator::SDFConfig sdfConfig;
    sdfConfig.spread = spread;
    sdfConfig.onedgeValue = 128;
    sdfConfig.padding = 4;
    sdfConfig.scale = 2.0;

    connect(&generator, &FontAtlasGenerator::progress, this, [this](int percent) {
        emit generationProgress(percent);
    });

    AtlasResult result = generator.generateSDF(config, sdfConfig);

    if (result.success) {
        QString error;
        bool saved = generator.save(result, outputPngPath, &error);
        if (saved) {
            QString acfPath = outputPngPath;
            acfPath.replace(".png", ".acf");
            generator.savePreset(result, acfPath);
            m_statusMessage = "SDF atlas generated: " + outputPngPath;
            emit statusMessageChanged();
            emit atlasGenerated(outputPngPath, result.image.width(), result.image.height());
            m_isGenerating = false;
            emit generatingChanged();
            return true;
        } else {
            m_statusMessage = "SDF save failed: " + error;
            emit statusMessageChanged();
        }
    } else {
        m_statusMessage = "SDF generation failed: " + result.errorMsg;
        emit statusMessageChanged();
    }

    m_isGenerating = false;
    emit generatingChanged();
    return false;
}

bool FontCreatorQmlBridge::generateMSDFAtlas(const QString& outputPngPath, int spread) {
    m_isGenerating = true;
    emit generatingChanged();
    m_statusMessage = "Generating MSDF atlas...";
    emit statusMessageChanged();

    FontAtlasGenerator generator;

    AtlasConfig config;
    config.fontFamily = m_fontFamily.isEmpty() ? m_currentFont : m_fontFamily;
    config.fontSize = m_fontSize;
    config.fontWeight = m_fontWeight;
    config.italic = m_italic;
    config.atlasWidth = m_atlasWidth;
    config.atlasHeight = m_atlasHeight;
    config.globalHeight = m_globalHeight;
    config.globalVPad = m_globalVPad;
    config.charset = m_charset.isEmpty() ? m_previewText : m_charset;

    FontAtlasGenerator::SDFConfig sdfConfig;
    sdfConfig.spread = spread;
    sdfConfig.onedgeValue = 128;
    sdfConfig.padding = 4;
    sdfConfig.scale = 2.5;

    connect(&generator, &FontAtlasGenerator::progress, this, [this](int percent) {
        emit generationProgress(percent);
    });

    AtlasResult result = generator.generateMSDF(config, sdfConfig);

    if (result.success) {
        QString error;
        // MSDF saves as RGB PNG
        bool saved = result.image.save(outputPngPath, "PNG");
        if (!saved)
            error = "Failed to save MSDF PNG";
        if (saved) {
            QString acfPath = outputPngPath;
            acfPath.replace(".png", ".acf");
            generator.savePreset(result, acfPath);
            m_statusMessage = "MSDF atlas generated: " + outputPngPath;
            emit statusMessageChanged();
            emit atlasGenerated(outputPngPath, result.image.width(), result.image.height());
            m_isGenerating = false;
            emit generatingChanged();
            return true;
        } else {
            m_statusMessage = "MSDF save failed: " + error;
            emit statusMessageChanged();
        }
    } else {
        m_statusMessage = "MSDF generation failed: " + result.errorMsg;
        emit statusMessageChanged();
    }

    m_isGenerating = false;
    emit generatingChanged();
    return false;
}

bool FontCreatorQmlBridge::saveAtlas(const QString& pngPath, const QString& acfPath) {
    FontAtlasGenerator generator;

    AtlasConfig config;
    config.fontFamily = m_fontFamily.isEmpty() ? m_currentFont : m_fontFamily;
    config.fontSize = m_fontSize;
    config.fontWeight = m_fontWeight;
    config.italic = m_italic;
    config.atlasWidth = m_atlasWidth;
    config.atlasHeight = m_atlasHeight;
    config.globalHeight = m_globalHeight;
    config.globalVPad = m_globalVPad;
    config.charset = m_charset.isEmpty() ? m_previewText : m_charset;

    config.hinting.enableAutoHinting = m_hintingEnabled;
    config.hinting.hintingLevel = m_hintingLevel;
    config.hinting.enableGridFitting = m_gridFitting;
    config.hinting.enableSubpixel = m_subpixelHinting;
    config.hinting.antiAlias = static_cast<AntiAliasMode>(m_antiAliasMode);

    AtlasResult result = generator.generate(config);

    if (result.success) {
        QString error;
        bool savedPng = generator.save(result, pngPath, &error);
        bool savedAcf = generator.savePreset(config, acfPath);
        return savedPng && savedAcf;
    }
    return false;
}

bool FontCreatorQmlBridge::loadPreset(const QString& acfPath) {
    FontAtlasGenerator generator;
    bool ok = false;
    AtlasConfig config = generator.loadPreset(acfPath, &ok);

    if (ok) {
        m_fontFamily = config.fontFamily;
        m_fontSize = config.fontSize;
        m_fontWeight = config.fontWeight;
        m_italic = config.italic;
        m_atlasWidth = config.atlasWidth;
        m_atlasHeight = config.atlasHeight;
        m_globalHeight = config.globalHeight;
        m_globalVPad = config.globalVPad;
        m_charset = config.charset;
        m_currentFont = config.fontFamily;

        m_glyphs.clear();
        for (const auto& gm : config.glyphs) {
            QVariantMap g;
            g["codepoint"] = gm.codepoint;
            g["cellWidth"] = gm.cellWidth;
            g["hPad"] = gm.hPad;
            g["vPad"] = gm.vPad;
            m_glyphs.append(g);
        }

        m_kerningPairs.clear();
        for (const auto& kp : config.kerningPairs) {
            QVariantMap p;
            p["left"] = kp.left;
            p["right"] = kp.right;
            p["kerning"] = kp.kerning;
            m_kerningPairs.append(p);
        }

        m_hintingEnabled = config.hinting.enableAutoHinting;
        m_hintingLevel = config.hinting.hintingLevel;
        m_gridFitting = config.hinting.enableGridFitting;
        m_subpixelHinting = config.hinting.enableSubpixel;
        m_antiAliasMode = static_cast<int>(config.hinting.antiAlias);

        emit currentFontChanged();
        emit fontSizeChanged();
        emit atlasSizeChanged();
        emit presetLoaded(acfPath);
        return true;
    }
    return false;
}

bool FontCreatorQmlBridge::savePreset(const QString& acfPath) {
    FontAtlasGenerator generator;

    AtlasConfig config;
    config.fontFamily = m_fontFamily.isEmpty() ? m_currentFont : m_fontFamily;
    config.fontSize = m_fontSize;
    config.fontWeight = m_fontWeight;
    config.italic = m_italic;
    config.atlasWidth = m_atlasWidth;
    config.atlasHeight = m_atlasHeight;
    config.globalHeight = m_globalHeight;
    config.globalVPad = m_globalVPad;
    config.charset = m_charset;

    for (const auto& gVar : m_glyphs) {
        QVariantMap g = gVar.toMap();
        GlyphMetrics gm;
        gm.codepoint = g["codepoint"].toUInt();
        gm.cellWidth = g["cellWidth"].toInt();
        gm.hPad = g["hPad"].toInt();
        gm.vPad = g["vPad"].toInt();
        config.glyphs.append(gm);
    }

    for (const auto& pVar : m_kerningPairs) {
        QVariantMap p = pVar.toMap();
        KerningPair kp;
        kp.left    = p["left"].toUInt();
        kp.right   = p["right"].toUInt();
        kp.kerning = p["kerning"].toInt();
        config.kerningPairs.append(kp);
    }

    bool saved = generator.savePreset(config, acfPath);
    if (saved) {
        emit presetSaved(acfPath);
    }
    return saved;
}

QVariantMap FontCreatorQmlBridge::getCurrentConfig() {
    QVariantMap config;
    config["fontFamily"] = m_fontFamily.isEmpty() ? m_currentFont : m_fontFamily;
    config["fontSize"] = m_fontSize;
    config["fontWeight"] = m_fontWeight;
    config["italic"] = m_italic;
    config["atlasWidth"] = m_atlasWidth;
    config["atlasHeight"] = m_atlasHeight;
    config["globalHeight"] = m_globalHeight;
    config["globalVPad"] = m_globalVPad;
    config["charset"] = m_charset;
    config["glyphs"] = m_glyphs;
    config["kerningPairs"] = m_kerningPairs;
    return config;
}

void FontCreatorQmlBridge::setCurrentConfig(const QVariantMap& config) {
    if (config.contains("fontFamily")) m_fontFamily = config["fontFamily"].toString();
    if (config.contains("fontSize")) m_fontSize = config["fontSize"].toInt();
    if (config.contains("fontWeight")) m_fontWeight = config["fontWeight"].toInt();
    if (config.contains("italic")) m_italic = config["italic"].toBool();
    if (config.contains("atlasWidth")) m_atlasWidth = config["atlasWidth"].toInt();
    if (config.contains("atlasHeight")) m_atlasHeight = config["atlasHeight"].toInt();
    if (config.contains("globalHeight")) m_globalHeight = config["globalHeight"].toInt();
    if (config.contains("globalVPad")) m_globalVPad = config["globalVPad"].toInt();
    if (config.contains("charset")) m_charset = config["charset"].toString();
    if (config.contains("glyphs")) m_glyphs = config["glyphs"].toList();
    if (config.contains("kerningPairs")) m_kerningPairs = config["kerningPairs"].toList();
    if (config.contains("currentFont")) m_currentFont = config["currentFont"].toString();
}

void FontCreatorQmlBridge::setFontFamily(const QString& family) {
    m_fontFamily = family;
    m_currentFont = family;
    emit currentFontChanged();
}

void FontCreatorQmlBridge::setFontWeight(int weight) {
    m_fontWeight = weight;
}

void FontCreatorQmlBridge::setItalic(bool italic) {
    m_italic = italic;
}

static QString resolveCharsetName(const QString& name) {
    if (name == "ASCII" || name == "Basic Latin (ASCII)")
        return QChar(0x20) + QString(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~");
    if (name == "Latin-1" || name == "Latin-1 Supplement") {
        QString s;
        for (int c = 0xA0; c <= 0xFF; ++c) s += QChar(c);
        return s;
    }
    if (name == "Numbers") return "0123456789";
    if (name == "Alphanumeric") return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    if (name == "Full ASCII + Symbols")
        return QChar(0x20) + QString(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~") +
               QString("\u00A0\u00A1\u00A2\u00A3\u00A4\u00A5\u00A6\u00A7\u00A8\u00A9\u00AA\u00AB\u00AC\u00AD\u00AE\u00AF\u00B0\u00B1\u00B2\u00B3\u00B4\u00B5\u00B6\u00B7\u00B8\u00B9\u00BA\u00BB\u00BC\u00BD\u00BE\u00BF");
    if (name == "Cyrillic") {
        QString s;
        for (int c = 0x400; c <= 0x4FF; ++c) s += QChar(c);
        return s;
    }
    if (name == "Japanese Hiragana") {
        QString s;
        for (int c = 0x3040; c <= 0x309F; ++c) s += QChar(c);
        return s;
    }
    return name;
}

void FontCreatorQmlBridge::setCharset(const QString& charset) {
    m_charset = resolveCharsetName(charset);
}

void FontCreatorQmlBridge::setGlobalHeight(int height) {
    m_globalHeight = height;
}

void FontCreatorQmlBridge::setGlobalVPad(int vpad) {
    m_globalVPad = vpad;
}

void FontCreatorQmlBridge::addGlyph(uint codepoint, int cellWidth, int hPad, int vPad) {
    QVariantMap g;
    g["codepoint"] = codepoint;
    g["cellWidth"] = cellWidth;
    g["hPad"] = hPad;
    g["vPad"] = vPad;
    m_glyphs.append(g);
}

void FontCreatorQmlBridge::removeGlyph(uint codepoint) {
    for (int i = 0; i < m_glyphs.size(); ++i) {
        if (m_glyphs[i].toMap()["codepoint"].toUInt() == codepoint) {
            m_glyphs.removeAt(i);
            return;
        }
    }
}

QVariantList FontCreatorQmlBridge::getGlyphs() {
    return m_glyphs;
}

QString FontCreatorQmlBridge::getPreviewText() {
    return m_previewText;
}

void FontCreatorQmlBridge::setPreviewText(const QString& text) {
    m_previewText = text;
}

QString FontCreatorQmlBridge::exportToACF(const QString& acfPath) {
    if (savePreset(acfPath)) {
        return acfPath;
    }
    return "";
}

QString FontCreatorQmlBridge::exportToJSON(const QString& jsonPath) {
    QJsonObject obj;
    obj["fontFamily"] = m_fontFamily.isEmpty() ? m_currentFont : m_fontFamily;
    obj["fontSize"] = m_fontSize;
    obj["fontWeight"] = m_fontWeight;
    obj["italic"] = m_italic;
    obj["atlasWidth"] = m_atlasWidth;
    obj["atlasHeight"] = m_atlasHeight;
    obj["globalHeight"] = m_globalHeight;
    obj["globalVPad"] = m_globalVPad;
    obj["charset"] = m_charset;

    QJsonArray glyphsArr;
    for (const auto& gVar : m_glyphs) {
        glyphsArr.append(gVar.toJsonObject());
    }
    obj["glyphs"] = glyphsArr;

    QJsonArray kerningArr;
    for (const auto& pVar : m_kerningPairs) {
        kerningArr.append(pVar.toJsonObject());
    }
    obj["kerningPairs"] = kerningArr;

    QJsonDocument doc(obj);
    QFile file(jsonPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        return jsonPath;
    }
    return "";
}

bool FontCreatorQmlBridge::importFromJSON(const QString& jsonPath) {
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull() || !doc.isObject()) return false;

    QJsonObject obj = doc.object();
    setCurrentConfig(obj.toVariantMap());
    return true;
}

QVariantMap FontCreatorQmlBridge::getDefaultConfig() {
    QVariantMap config;
    config["fontFamily"] = "Arial";
    config["fontSize"] = 48;
    config["fontWeight"] = 400;
    config["italic"] = false;
    config["atlasWidth"] = 512;
    config["atlasHeight"] = 512;
    config["globalHeight"] = 85;
    config["globalVPad"] = 13;
    config["charset"] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 .,!?;:'\"()-+=@#$%&*/\\<>[]{}|~^`";
    return config;
}

QStringList FontCreatorQmlBridge::getCommonCharsets() {
    return {
        "ASCII",
        "Latin-1",
        "Numbers",
        "Alphanumeric",
        "Full ASCII + Symbols",
        "Cyrillic",
        "Japanese Hiragana",
        "Custom"
    };
}

// ── Unicode Range Selection ──────────────────────────────────────────

QStringList FontCreatorQmlBridge::getAvailableRanges() {
    return FontAtlasGenerator::availableUnicodeRanges();
}

void FontCreatorQmlBridge::enableRange(const QString& rangeName, bool enable) {
    if (enable) {
        if (!m_enabledRanges.contains(rangeName))
            m_enabledRanges.append(rangeName);
    } else {
        m_enabledRanges.removeAll(rangeName);
    }
}

QStringList FontCreatorQmlBridge::getEnabledRanges() {
    return m_enabledRanges;
}

bool FontCreatorQmlBridge::applyCombinedCharset() {
    QSet<QChar> chars;
    for (const QString& range : m_enabledRanges) {
        QString cs = FontAtlasGenerator::generateCharsetForRange(range);
        for (const QChar& c : cs)
            chars.insert(c);
    }
    if (chars.isEmpty())
        return false;

    QString result;
    for (const QChar& c : chars)
        result += c;
    m_charset = result;
    return true;
}

void FontCreatorQmlBridge::clearRanges() {
    m_enabledRanges.clear();
}

QVariantMap FontCreatorQmlBridge::validateCoverage() {
    FontAtlasGenerator gen;
    auto report = gen.validateGlyphCoverage(
        m_fontFamily.isEmpty() ? m_currentFont : m_fontFamily,
        m_fontSize,
        m_charset
    );
    QVariantMap map;
    map["totalRequested"] = report.totalRequested;
    map["available"] = report.available;
    map["missing"] = report.missing;
    map["coveragePercent"] = report.coveragePercent;
    QVariantList missingList;
    for (const QString& c : report.missingChars)
        missingList.append(c);
    map["missingChars"] = missingList;
    return map;
}

// ── Kerning ──────────────────────────────────────────────────────────

QVariantList FontCreatorQmlBridge::getKerningPairs() {
    return m_kerningPairs;
}

void FontCreatorQmlBridge::setKerningPair(uint left, uint right, int kerning) {
    for (int i = 0; i < m_kerningPairs.size(); ++i) {
        QVariantMap p = m_kerningPairs[i].toMap();
        if (p["left"].toUInt() == left && p["right"].toUInt() == right) {
            if (kerning == 0) {
                m_kerningPairs.removeAt(i);
            } else {
                p["kerning"] = kerning;
                m_kerningPairs[i] = p;
            }
            emit statusMessageChanged();
            return;
        }
    }
    if (kerning != 0) {
        QVariantMap p;
        p["left"] = left;
        p["right"] = right;
        p["kerning"] = kerning;
        m_kerningPairs.append(p);
    }
}

void FontCreatorQmlBridge::removeKerningPair(uint left, uint right) {
    for (int i = 0; i < m_kerningPairs.size(); ++i) {
        QVariantMap p = m_kerningPairs[i].toMap();
        if (p["left"].toUInt() == left && p["right"].toUInt() == right) {
            m_kerningPairs.removeAt(i);
            return;
        }
    }
}

void FontCreatorQmlBridge::extractKerning() {
    FontAtlasGenerator gen;
    QVector<KerningPair> pairs = gen.extractKerningPairs(
        m_fontFamily.isEmpty() ? m_currentFont : m_fontFamily,
        m_fontSize,
        m_charset
    );
    m_kerningPairs.clear();
    for (const auto& kp : pairs) {
        QVariantMap p;
        p["left"] = kp.left;
        p["right"] = kp.right;
        p["kerning"] = kp.kerning;
        m_kerningPairs.append(p);
    }
    m_statusMessage = QString("Extracted %1 kerning pairs").arg(m_kerningPairs.size());
    emit statusMessageChanged();
}

void FontCreatorQmlBridge::clearKerningPairs() {
    m_kerningPairs.clear();
    m_statusMessage = "Kerning pairs cleared";
    emit statusMessageChanged();
}

int FontCreatorQmlBridge::getKerningOffset(uint left, uint right) {
    FontAtlasGenerator gen;
    return gen.getKerning(left, right);
}

// ── Hinting ──────────────────────────────────────────────────────────

void FontCreatorQmlBridge::setHintingEnabled(bool enabled) {
    m_hintingEnabled = enabled;
}

bool FontCreatorQmlBridge::isHintingEnabled() const {
    return m_hintingEnabled;
}

void FontCreatorQmlBridge::setHintingLevel(int level) {
    m_hintingLevel = qBound(0, level, 2);
}

int FontCreatorQmlBridge::hintingLevel() const {
    return m_hintingLevel;
}

void FontCreatorQmlBridge::setGridFitting(bool enabled) {
    m_gridFitting = enabled;
}

bool FontCreatorQmlBridge::isGridFitting() const {
    return m_gridFitting;
}

void FontCreatorQmlBridge::setSubpixelHinting(bool enabled) {
    m_subpixelHinting = enabled;
}

bool FontCreatorQmlBridge::isSubpixelHinting() const {
    return m_subpixelHinting;
}

// ── Anti-aliasing ────────────────────────────────────────────────────

void FontCreatorQmlBridge::setAntiAliasMode(int mode) {
    m_antiAliasMode = qBound(0, mode, 3);
}

int FontCreatorQmlBridge::antiAliasMode() const {
    return m_antiAliasMode;
}

QStringList FontCreatorQmlBridge::antiAliasModeNames() const {
    return {"None", "Standard", "Subpixel RGB", "LCD"};
}

// ── Metrics Optimizer ────────────────────────────────────────────────

QVariantMap FontCreatorQmlBridge::analyzeMetrics() {
    FontAtlasGenerator gen;

    AtlasConfig config;
    config.fontFamily = m_fontFamily.isEmpty() ? m_currentFont : m_fontFamily;
    config.fontSize = m_fontSize;
    config.fontWeight = m_fontWeight;
    config.italic = m_italic;
    config.charset = m_charset;

    auto result = gen.analyzeMetrics(config);

    QVariantMap map;
    map["suggestedGlobalHeight"] = result.suggestedGlobalHeight;
    map["suggestedGlobalVPad"] = result.suggestedGlobalVPad;
    map["suggestedHPad"] = result.suggestedHPad;
    map["suggestedVPad"] = result.suggestedVPad;
    map["optimalAtlasWidth"] = result.optimalAtlasWidth;
    map["optimalAtlasHeight"] = result.optimalAtlasHeight;
    map["averageAdvance"] = result.averageAdvance;
    map["maxAdvance"] = result.maxAdvance;
    map["minAdvance"] = result.minAdvance;
    map["glyphOverflowCount"] = result.glyphOverflowCount;
    map["overflowGlyphs"] = result.overflowGlyphs;
    return map;
}

void FontCreatorQmlBridge::applyOptimizedMetrics(const QVariantMap& suggestion) {
    if (suggestion.contains("suggestedGlobalHeight"))
        m_globalHeight = suggestion["suggestedGlobalHeight"].toInt();
    if (suggestion.contains("suggestedGlobalVPad"))
        m_globalVPad = suggestion["suggestedGlobalVPad"].toInt();
    if (suggestion.contains("optimalAtlasWidth"))
        m_atlasWidth = suggestion["optimalAtlasWidth"].toInt();
    if (suggestion.contains("optimalAtlasHeight"))
        m_atlasHeight = suggestion["optimalAtlasHeight"].toInt();
}

// ============================================================================
// FontCreatorEditorModule
// ============================================================================

FontCreatorEditorModule::FontCreatorEditorModule(QWidget* parent)
    : EditorModule(parent)
{}

bool FontCreatorEditorModule::initialize()
{
    LOG_INFO("FontCreatorEditorModule", "Font Creator module initialized");
    return true;
}

void FontCreatorEditorModule::shutdown()
{
    LOG_INFO("FontCreatorEditorModule", "Font Creator module shutdown");
}

void FontCreatorEditorModule::importFile(const QString& filePath)
{
    if (auto* bridge = FontCreatorQmlBridge::instance()) {
        bridge->loadPreset(filePath);
    }
}

void FontCreatorEditorModule::exportFile(const QString& filePath)
{
    if (auto* bridge = FontCreatorQmlBridge::instance()) {
        bridge->savePreset(filePath);
    }
}

QJsonObject FontCreatorEditorModule::serializeProject() const
{
    QJsonObject data;
    auto* bridge = FontCreatorQmlBridge::instance();
    if (bridge) {
        data["currentFont"] = bridge->currentFont();
        data["fontSize"] = bridge->fontSize();
    }
    return data;
}

void FontCreatorEditorModule::deserializeProject(const QJsonObject& data)
{
    auto* bridge = FontCreatorQmlBridge::instance();
    if (!bridge) return;
    if (data.contains("currentFont")) {
        bridge->setFontFamily(data["currentFont"].toString());
    }
    if (data.contains("fontSize")) {
        bridge->setFontSize(data["fontSize"].toInt());
    }
}

} // namespace ks
