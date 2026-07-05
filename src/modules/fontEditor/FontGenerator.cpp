#include "FontGenerator.h"

#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QFontDatabase>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>
#include <QtMath>

struct FontAtlasGenerator::FTContext {
    // No FreeType - using Qt's font rendering instead
    bool available = false;
};

static QString defaultCharset()
{
    QString cs;
    for (int c = 0x20; c <= 0x7E; ++c)
        cs += QChar(c);
    return cs;
}

FontAtlasGenerator::FontAtlasGenerator()
    : m_ft(new FTContext)
{
    m_ft->available = true; // Using Qt's built-in font system
}

FontAtlasGenerator::~FontAtlasGenerator()
{
    delete m_ft;
}

bool FontAtlasGenerator::initFreeType()
{
    return m_ft->available;
}

void FontAtlasGenerator::shutdownFreeType()
{
    // Nothing to do with Qt's built-in font system
}

AtlasResult FontAtlasGenerator::generate(const AtlasConfig &cfg)
{
    AtlasResult result;
    result.config = cfg;

    QString fontFile = fontFileForFamily(cfg.fontFamily, cfg.fontWeight, cfg.italic);
    if (fontFile.isEmpty() && !cfg.fontFamily.isEmpty()) {
        result.errorMsg = QString("Cannot find font for \"%1\".").arg(cfg.fontFamily);
        return result;
    }

    QString chars = cfg.charset.isEmpty() ? defaultCharset() : cfg.charset;

    QVector<GlyphMetrics> glyphs;
    QMap<uint32_t, float> uvMap;
    
    QFont font(cfg.fontFamily.isEmpty() ? "Arial" : cfg.fontFamily);
    font.setPixelSize(cfg.fontSize);
    font.setBold(cfg.fontWeight >= 700);
    font.setItalic(cfg.italic);
    
    QImage atlas(cfg.atlasWidth, cfg.atlasHeight, QImage::Format_ARGB32);
    atlas.fill(Qt::transparent);
    
    QPainter painter(&atlas);
    painter.setFont(font);
    painter.setPen(Qt::white);

    // Apply anti-aliasing mode
    switch (cfg.hinting.antiAlias) {
    case AntiAliasMode::None:
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setRenderHint(QPainter::TextAntialiasing, false);
        break;
    case AntiAliasMode::Standard:
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        break;
    case AntiAliasMode::SubpixelRGB:
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        font.setStyleStrategy(QFont::PreferAntialias);
        painter.setFont(font);
        break;
    case AntiAliasMode::LCD:
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        font.setStyleStrategy(QFont::PreferAntialias);
        painter.setFont(font);
        break;
    }
    
    int cellH = cfg.globalHeight;
    int x = 0;
    int y = 0;
    
    QFontMetrics fm(font);
    int total = chars.length();
    
    for (int i = 0; i < total; ++i) {
        if (i % 16 == 0)
            emit progress(i * 100 / total);
        QChar qc = chars[i];
        uint32_t cp = qc.unicode();
        
        GlyphMetrics gm;
        gm.codepoint = cp;
        gm.cellWidth = fm.horizontalAdvance(qc) + 4;
        gm.hPad = 2;
        gm.vPad = cfg.globalVPad;
        
        // Wrap to next row
        if (x + gm.cellWidth > cfg.atlasWidth) {
            x = 0;
            y += cellH;
        }
        if (y + cellH > cfg.atlasHeight) {
            qWarning() << "Atlas too small - truncating";
            break;
        }
        
        uvMap[cp] = static_cast<float>(x) / cfg.atlasWidth;
        
        // Draw the character
        painter.drawText(x + gm.hPad, y + cfg.globalVPad + fm.ascent(), QString(qc));
        
        x += gm.cellWidth;
        glyphs.append(gm);
    }
    
    painter.end();

    // Apply hinting if configured
    if (cfg.hinting.enableAutoHinting) {
        m_hintingConfig = cfg.hinting;
        AtlasConfig hintedCfg = cfg;
        hintedCfg.glyphs = glyphs;
        hintedCfg.uvMap = uvMap;
        glyphs = applyHinting(hintedCfg);

        // Recompute layout with hinted cell widths
        x = 0;
        y = 0;
        uvMap.clear();
        for (int i = 0; i < glyphs.size(); ++i) {
            if (x + glyphs[i].cellWidth > cfg.atlasWidth) {
                x = 0;
                y += cellH;
            }
            if (y + cellH > cfg.atlasHeight) {
                qWarning() << "Atlas too small after hinting - truncating";
                break;
            }
            uvMap[glyphs[i].codepoint] = static_cast<float>(x) / cfg.atlasWidth;
            x += glyphs[i].cellWidth;
        }
    }

    emit progress(100);

    result.success = true;
    result.image = atlas;
    result.config.glyphs = glyphs;
    result.config.uvMap = uvMap;
    result.config.charset = chars;
    
    return result;
}

bool FontAtlasGenerator::save(const AtlasResult &result,
                               const QString    &pngPath,
                               QString          *errorOut)
{
    auto fail = [&](const QString &msg) {
        if (errorOut) *errorOut = msg;
        return false;
    };

    if (!result.success)
        return fail("Result is not valid.");

    QByteArray pngBytes = pngPath.toLocal8Bit();
    const QImage &img   = result.image;
    
    img.save(pngPath, "PNG");

    QString acfPath = pngPath;
    acfPath.replace(QRegularExpression("\\.png$", QRegularExpression::CaseInsensitiveOption), ".acf");
    if (!savePreset(result.config, acfPath))
        return fail(QString("Could not write preset to \"%1\".").arg(acfPath));

    return true;
}

AtlasConfig FontAtlasGenerator::loadPreset(const QString &acfPath, bool *ok)
{
    AtlasConfig cfg;
    if (ok) *ok = false;

    QFile f(acfPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return cfg;

    QMap<QString, QString> vals;
    QTextStream ts(&f);
    while (!ts.atEnd()) {
        QString line = ts.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('[') || line.startsWith(';'))
            continue;
        int eq = line.indexOf('=');
        if (eq < 0) continue;
        QString key = line.left(eq).trimmed().toUpper();
        QString val = line.mid(eq + 1).trimmed();
        vals[key] = val;
    }

    cfg.fontFamily   = vals.value("FONT");
    cfg.fontSize     = vals.value("SIZE", "48").toInt();
    cfg.globalHeight = vals.value("HEIGHT", "85").toInt();
    cfg.hinting.enableAutoHinting = vals.value("HINTING", "1").toInt() != 0;
    cfg.hinting.hintingLevel = vals.value("HINT_LEVEL", "0").toInt();
    cfg.hinting.enableGridFitting = vals.value("GRID_FIT", "1").toInt() != 0;
    cfg.hinting.enableSubpixel = vals.value("SUBPIXEL", "0").toInt() != 0;
    cfg.hinting.antiAlias = static_cast<AntiAliasMode>(vals.value("ANTIALIAS", "1").toInt());

    QString charset;
    int idx = 0;
    while (vals.contains(QString("WIDTH_%1").arg(idx))) {
        GlyphMetrics gm;
        gm.codepoint = static_cast<uint32_t>(0x20 + idx);
        gm.hPad      = vals.value(QString("HPAD_%1").arg(idx), "0").toInt();
        gm.vPad      = vals.value(QString("VPAD_%1").arg(idx), "13").toInt();
        gm.cellWidth = vals.value(QString("WIDTH_%1").arg(idx), "42").toInt();
        cfg.glyphs.append(gm);
        charset += QChar(gm.codepoint);
        ++idx;
    }
    cfg.charset = charset;

    // Load kerning pairs
    int ki = 0;
    while (vals.contains(QString("KERN_%1_LEFT").arg(ki))) {
        KerningPair kp;
        kp.left    = vals.value(QString("KERN_%1_LEFT").arg(ki)).toUInt();
        kp.right   = vals.value(QString("KERN_%1_RIGHT").arg(ki)).toUInt();
        kp.kerning = vals.value(QString("KERN_%1_VALUE").arg(ki)).toInt();
        cfg.kerningPairs.append(kp);
        m_kerningCache[qMakePair(kp.left, kp.right)] = kp.kerning;
        ++ki;
    }

    if (ok) *ok = true;
    return cfg;
}

bool FontAtlasGenerator::savePreset(const AtlasConfig &cfg, const QString &acfPath)
{
    QFile f(acfPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream ts(&f);
    ts << "[CONFIG]\r\n";
    ts << "FONT = "   << cfg.fontFamily   << "\r\n";
    ts << "SIZE = "   << cfg.fontSize     << "\r\n";
    ts << "FAMILY = " << cfg.fontFamily   << "\r\n";
    ts << "WEIGHT = " << (cfg.fontWeight >= 700 ? "B" : "R") << "\r\n";
    ts << "STYLE = "  << (cfg.italic ? "I" : "N") << "\r\n";
    ts << "HEIGHT = " << cfg.globalHeight << "\r\n";
    ts << "HINTING = " << (cfg.hinting.enableAutoHinting ? 1 : 0) << "\r\n";
    ts << "HINT_LEVEL = " << cfg.hinting.hintingLevel << "\r\n";
    ts << "GRID_FIT = " << (cfg.hinting.enableGridFitting ? 1 : 0) << "\r\n";
    ts << "SUBPIXEL = " << (cfg.hinting.enableSubpixel ? 1 : 0) << "\r\n";
    ts << "ANTIALIAS = " << static_cast<int>(cfg.hinting.antiAlias) << "\r\n";

    for (int i = 0; i < cfg.glyphs.size(); ++i) {
        const GlyphMetrics &gm = cfg.glyphs[i];
        ts << "HPAD_"  << i << " = " << gm.hPad      << "\r\n";
        ts << "VPAD_"  << i << " = " << gm.vPad      << "\r\n";
        ts << "WIDTH_" << i << " = " << gm.cellWidth  << "\r\n";
    }

    // Kerning pairs
    for (int i = 0; i < cfg.kerningPairs.size(); ++i) {
        const KerningPair &kp = cfg.kerningPairs[i];
        ts << "KERN_" << i << "_LEFT = "  << kp.left     << "\r\n";
        ts << "KERN_" << i << "_RIGHT = " << kp.right    << "\r\n";
        ts << "KERN_" << i << "_VALUE = " << kp.kerning  << "\r\n";
    }

    f.close();
    return true;
}

QStringList FontAtlasGenerator::systemFonts()
{
    QFontDatabase db;
    return db.families();
}

QString FontAtlasGenerator::fontFileForFamily(const QString &family, int weight, bool italic)
{
    QFontDatabase db;
    QFont::Weight qtWeight = static_cast<QFont::Weight>(weight);
    QString styleName;

    if (italic) styleName += "Italic ";
    if (weight == QFont::Bold) styleName += "Bold";
    else if (weight == QFont::Light) styleName += "Light";

    QStringList styles = db.styles(family);
    for (const QString& style : styles) {
        if (style.contains(styleName, Qt::CaseInsensitive)) {
            return family + " " + style.trimmed();
        }
    }

    return family;
}

// ============================================================================
// Auto-hinting
// ============================================================================

QVector<GlyphMetrics> FontAtlasGenerator::applyHinting(const AtlasConfig& config) {
    QVector<GlyphMetrics> hinted = config.glyphs;

    if (!m_hintingConfig.enableAutoHinting) return hinted;

    QFont font(config.fontFamily);
    font.setPixelSize(config.fontSize);
    font.setBold(config.fontWeight >= 700);
    font.setItalic(config.italic);

    if (m_hintingConfig.enableGridFitting) {
        font.setHintingPreference(QFont::PreferFullHinting);
    } else {
        font.setHintingPreference(QFont::PreferNoHinting);
    }

    if (m_hintingConfig.enableSubpixel) {
        font.setStyleStrategy(QFont::PreferAntialias);
    }

    QFontMetrics fm(font);

    for (auto& glyph : hinted) {
        QChar qc(glyph.codepoint);

        // Calculate side bearing deltas for hinting alignment
        int advance = fm.horizontalAdvance(qc);
        QRect bbox = fm.boundingRect(qc);

        glyph.lsbDelta = bbox.x();
        glyph.rsbDelta = advance - (bbox.x() + bbox.width());

        // Apply hinting to cell width
        switch (m_hintingConfig.hintingLevel) {
            case 1: // Normal
                glyph.cellWidth = ((glyph.cellWidth + 2) / 4) * 4;
                break;
            case 2: // Full
                glyph.cellWidth = ((glyph.cellWidth + 1) / 2) * 2;
                break;
            default: // Light
                break;
        }
    }

    return hinted;
}

// ============================================================================
// Kerning
// ============================================================================

QVector<KerningPair> FontAtlasGenerator::extractKerningPairs(const QString& fontFamily, int fontSize, const QString& charset) {
    m_kerningCache.clear();
    QVector<KerningPair> pairs;

    if (!m_kerningEnabled) return pairs;
    if (charset.length() < 2 || charset.length() > 2000) return pairs;

    QFont font(fontFamily);
    font.setPixelSize(fontSize);
    QFontMetrics fm(font);

    // Extract kerning for all character pairs in the charset
    int len = charset.length();
    for (int i = 0; i < len; ++i) {
        for (int j = 0; j < len; ++j) {
            QChar left = charset[i];
            QChar right = charset[j];

            int advanceWithKerning = fm.horizontalAdvance(QString(left) + QString(right));
            int advanceLeft = fm.horizontalAdvance(left);
            int advanceRight = fm.horizontalAdvance(right);

            int kerningValue = advanceWithKerning - advanceLeft - advanceRight;
            if (kerningValue != 0) {
                KerningPair pair;
                pair.left = left.unicode();
                pair.right = right.unicode();
                pair.kerning = kerningValue;
                pairs.append(pair);
                m_kerningCache[qMakePair(pair.left, pair.right)] = kerningValue;
            }
        }
    }

    return pairs;
}

int FontAtlasGenerator::getKerning(uint32_t left, uint32_t right) const {
    return m_kerningCache.value(qMakePair(left, right), 0);
}

// ============================================================================
// Unicode Support
// ============================================================================

QStringList FontAtlasGenerator::availableUnicodeRanges() {
    return {
        "Basic Latin (ASCII)",
        "Latin-1 Supplement",
        "Latin Extended-A",
        "Latin Extended-B",
        "Cyrillic",
        "Greek",
        "Arabic",
        "Hebrew",
        "CJK Unified Ideographs",
        "Hiragana",
        "Katakana",
        "Hangul Syllables"
    };
}

QString FontAtlasGenerator::generateCharsetForRange(const QString& rangeName) {
    QString charset;

    if (rangeName == "Basic Latin (ASCII)") {
        for (int c = 0x20; c <= 0x7E; ++c) charset += QChar(c);
    } else if (rangeName == "Latin-1 Supplement") {
        for (int c = 0xA0; c <= 0xFF; ++c) charset += QChar(c);
    } else if (rangeName == "Latin Extended-A") {
        for (int c = 0x100; c <= 0x17F; ++c) charset += QChar(c);
    } else if (rangeName == "Latin Extended-B") {
        for (int c = 0x180; c <= 0x24F; ++c) charset += QChar(c);
    } else if (rangeName == "Cyrillic") {
        for (int c = 0x400; c <= 0x4FF; ++c) charset += QChar(c);
    } else if (rangeName == "Greek") {
        for (int c = 0x370; c <= 0x3FF; ++c) charset += QChar(c);
    } else if (rangeName == "Arabic") {
        for (int c = 0x600; c <= 0x6FF; ++c) charset += QChar(c);
        for (int c = 0x750; c <= 0x77F; ++c) charset += QChar(c);
        for (int c = 0x8A0; c <= 0x8FF; ++c) charset += QChar(c);
        for (int c = 0xFB50; c <= 0xFDFF; ++c) charset += QChar(c);
        for (int c = 0xFE70; c <= 0xFEFF; ++c) charset += QChar(c);
    } else if (rangeName == "Hebrew") {
        for (int c = 0x591; c <= 0x5C7; ++c) charset += QChar(c);
        for (int c = 0x5D0; c <= 0x5EA; ++c) charset += QChar(c);
        for (int c = 0x5EF; c <= 0x5F4; ++c) charset += QChar(c);
    } else if (rangeName == "CJK Unified Ideographs") {
        for (int c = 0x4E00; c <= 0x9FFF; ++c) charset += QChar(c);
        if (charset.length() > 5000) charset = charset.left(5000);
    } else if (rangeName == "Hiragana") {
        for (int c = 0x3041; c <= 0x3096; ++c) charset += QChar(c);
        charset += QChar(0x3099); charset += QChar(0x309A);
        charset += QChar(0x309B); charset += QChar(0x309C);
        charset += QChar(0x309D); charset += QChar(0x309E);
        charset += QChar(0x309F);
    } else if (rangeName == "Katakana") {
        for (int c = 0x30A0; c <= 0x30FF; ++c) charset += QChar(c);
    } else if (rangeName == "Hangul Syllables") {
        for (int c = 0xAC00; c <= 0xD7A3; ++c) charset += QChar(c);
        if (charset.length() > 5000) charset = charset.left(5000);
    } else {
        charset = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    }

    return charset;
}

bool FontAtlasGenerator::setUnicodeRange(const QString& rangeName) {
    QString charset = generateCharsetForRange(rangeName);
    if (!charset.isEmpty()) m_charset = charset;
    return !charset.isEmpty();
}

void FontAtlasGenerator::enableUnicodeRange(const QString& rangeName, bool enable) {
    if (enable)
        m_enabledRanges.insert(rangeName);
    else
        m_enabledRanges.remove(rangeName);
}

QStringList FontAtlasGenerator::enabledUnicodeRanges() const {
    return QStringList(m_enabledRanges.begin(), m_enabledRanges.end());
}

QString FontAtlasGenerator::generateCombinedCharset() const {
    if (m_enabledRanges.isEmpty())
        return m_charset;

    QSet<QChar> chars;
    for (const QString& range : m_enabledRanges) {
        QString cs = generateCharsetForRange(range);
        for (const QChar& c : cs)
            chars.insert(c);
    }

    QString result;
    for (const QChar& c : chars)
        result += c;
    return result;
}

// ============================================================================
// SDF Atlas Generation
// ============================================================================

// Compute signed distance from each pixel to the nearest edge pixel.
// Uses a simple two-pass approximation (Chamfer distance) for speed.
static QImage computeSDF(const QImage& binary, int spread)
{
    int w = binary.width();
    int h = binary.height();
    QImage sdf(w, h, QImage::Format_Grayscale8);
    sdf.fill(0);

    // Convert to float distance map: inside=large positive, outside=large negative
    QVector<float> dist(w * h);
    const float INF = 1e9f;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            bool inside = qGray(binary.pixel(x, y)) > 128;
            dist[y * w + x] = inside ? INF : -INF;
        }
    }

    // First pass (top-left to bottom-right)
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;
            if (dist[idx] > 0) {
                // Inside: propagate from top/left neighbors
                if (x > 0) dist[idx] = qMin(dist[idx], dist[idx - 1] + 1.0f);
                if (y > 0) dist[idx] = qMin(dist[idx], dist[idx - w] + 1.0f);
                if (x > 0 && y > 0) dist[idx] = qMin(dist[idx], dist[idx - w - 1] + 1.414f);
                if (x < w - 1 && y > 0) dist[idx] = qMin(dist[idx], dist[idx - w + 1] + 1.414f);
            } else {
                // Outside: propagate from top/left neighbors
                if (x > 0) dist[idx] = qMax(dist[idx], dist[idx - 1] - 1.0f);
                if (y > 0) dist[idx] = qMax(dist[idx], dist[idx - w] - 1.0f);
                if (x > 0 && y > 0) dist[idx] = qMax(dist[idx], dist[idx - w - 1] - 1.414f);
                if (x < w - 1 && y > 0) dist[idx] = qMax(dist[idx], dist[idx - w + 1] - 1.414f);
            }
        }
    }

    // Second pass (bottom-right to top-left)
    for (int y = h - 1; y >= 0; --y) {
        for (int x = w - 1; x >= 0; --x) {
            int idx = y * w + x;
            if (dist[idx] > 0) {
                if (x < w - 1) dist[idx] = qMin(dist[idx], dist[idx + 1] + 1.0f);
                if (y < h - 1) dist[idx] = qMin(dist[idx], dist[idx + w] + 1.0f);
                if (x < w - 1 && y < h - 1) dist[idx] = qMin(dist[idx], dist[idx + w + 1] + 1.414f);
                if (x > 0 && y < h - 1) dist[idx] = qMin(dist[idx], dist[idx + w - 1] + 1.414f);
            } else {
                if (x < w - 1) dist[idx] = qMax(dist[idx], dist[idx + 1] - 1.0f);
                if (y < h - 1) dist[idx] = qMax(dist[idx], dist[idx + w] - 1.0f);
                if (x < w - 1 && y < h - 1) dist[idx] = qMax(dist[idx], dist[idx + w + 1] - 1.414f);
                if (x > 0 && y < h - 1) dist[idx] = qMax(dist[idx], dist[idx + w - 1] - 1.414f);
            }
        }
    }

    // Normalize to 0-255 range centered at onedgeValue
    float scale = 128.0f / qMax(spread, 1);
    for (int i = 0; i < w * h; ++i) {
        float d = dist[i];
        // Clamp to spread range
        if (d > spread) d = spread;
        if (d < -spread) d = -spread;
        // Map: -spread → 0, 0 → 128, +spread → 255
        int v = qBound(0, static_cast<int>(d * scale + 128.0f), 255);
        sdf.bits()[i] = static_cast<uchar>(v);
    }

    return sdf;
}

AtlasResult FontAtlasGenerator::generateSDF(const AtlasConfig& config, const SDFConfig& sdfConfig)
{
    AtlasResult result;
    result.config = config;

    QString fontFile = fontFileForFamily(config.fontFamily, config.fontWeight, config.italic);
    if (fontFile.isEmpty() && !config.fontFamily.isEmpty()) {
        result.errorMsg = QString("Cannot find font for \"%1\".").arg(config.fontFamily);
        return result;
    }

    QString chars = config.charset.isEmpty() ? defaultCharset() : config.charset;

    // Render each glyph at higher resolution for SDF quality
    QFont font(config.fontFamily.isEmpty() ? "Arial" : config.fontFamily);
    int sdfFontSize = static_cast<int>(config.fontSize * sdfConfig.scale);
    font.setPixelSize(sdfFontSize);
    font.setBold(config.fontWeight >= 700);
    font.setItalic(config.italic);

    QFontMetricsF fm(font);

    // Output atlas (single channel grayscale)
    int outW = config.atlasWidth;
    int outH = config.atlasHeight;
    QImage atlas(outW, outH, QImage::Format_Grayscale8);
    atlas.fill(0);

    QVector<GlyphMetrics> glyphs;
    QMap<uint32_t, float> uvMap;
    int cellH = config.globalHeight;
    int x = sdfConfig.padding;
    int y = sdfConfig.padding;
    int total = chars.length();

    for (int i = 0; i < total; ++i) {
        if (i % 8 == 0)
            emit progress(i * 100 / total);

        QChar qc = chars[i];
        uint32_t cp = qc.unicode();

        // Render glyph as binary high-res
        QImage glyphImg(static_cast<int>(fm.horizontalAdvance(qc) * sdfConfig.scale) + sdfConfig.padding * 2,
                        static_cast<int>(fm.height() * sdfConfig.scale) + sdfConfig.padding * 2,
                        QImage::Format_ARGB32);
        glyphImg.fill(Qt::black);

        QPainter gp(&glyphImg);
        QFont gf = font;
        gp.setFont(gf);
        gp.setPen(Qt::white);
        gp.drawText(QPointF(sdfConfig.padding, glyphImg.height() - sdfConfig.padding - fm.descent()),
                    QString(qc));
        gp.end();

        // Compute SDF from binary
        QImage sdfGlyph = computeSDF(glyphImg, sdfConfig.spread);

        // Place in atlas
        int cellW = sdfGlyph.width() + sdfConfig.padding;
        if (x + cellW > outW - sdfConfig.padding) {
            x = sdfConfig.padding;
            y += cellH;
        }
        if (y + cellH > outH - sdfConfig.padding) {
            qWarning() << "SDF atlas too small - truncating at" << i;
            break;
        }

        // Copy SDF glyph into atlas
        for (int sy = 0; sy < sdfGlyph.height() && sy + y < outH; ++sy) {
            const uchar* srcLine = sdfGlyph.constScanLine(sy);
            uchar* dstLine = atlas.scanLine(y + sy);
            int copyW = qMin(sdfGlyph.width(), outW - x);
            memcpy(dstLine + x, srcLine, static_cast<size_t>(copyW));
        }

        uvMap[cp] = static_cast<float>(x) / outW;

        GlyphMetrics gm;
        gm.codepoint = cp;
        gm.cellWidth = cellW;
        gm.hPad = sdfConfig.padding;
        gm.vPad = sdfConfig.padding;
        glyphs.append(gm);

        x += cellW;
    }

    emit progress(100);

    result.success = true;
    result.image = atlas;
    result.config.glyphs = glyphs;
    result.config.uvMap = uvMap;
    result.config.charset = chars;

    return result;
}

AtlasResult FontAtlasGenerator::generateMSDF(const AtlasConfig& config, const SDFConfig& sdfConfig)
{
    // MSDF uses 3 overlapping SDF channels to preserve corners.
    // Generate three SDF fields with slight angular offsets and store as RGB.
    AtlasResult result;
    result.config = config;

    QString fontFile = fontFileForFamily(config.fontFamily, config.fontWeight, config.italic);
    if (fontFile.isEmpty() && !config.fontFamily.isEmpty()) {
        result.errorMsg = QString("Cannot find font for \"%1\". (MSDF)").arg(config.fontFamily);
        return result;
    }

    QString chars = config.charset.isEmpty() ? defaultCharset() : config.charset;

    QFont font(config.fontFamily.isEmpty() ? "Arial" : config.fontFamily);
    int sdfFontSize = static_cast<int>(config.fontSize * sdfConfig.scale);
    font.setPixelSize(sdfFontSize);
    font.setBold(config.fontWeight >= 700);
    font.setItalic(config.italic);

    QFontMetricsF fm(font);
    int outW = config.atlasWidth;
    int outH = config.atlasHeight;
    QImage atlas(outW, outH, QImage::Format_RGB888);
    atlas.fill(Qt::black);

    QVector<GlyphMetrics> glyphs;
    QMap<uint32_t, float> uvMap;
    int cellH = config.globalHeight;
    int x = sdfConfig.padding;
    int y = sdfConfig.padding;
    int total = chars.length();

    for (int i = 0; i < total; ++i) {
        if (i % 8 == 0)
            emit progress(i * 100 / total);

        QChar qc = chars[i];
        uint32_t cp = qc.unicode();

        int gW = static_cast<int>(fm.horizontalAdvance(qc) * sdfConfig.scale) + sdfConfig.padding * 2;
        int gH = static_cast<int>(fm.height() * sdfConfig.scale) + sdfConfig.padding * 2;

        // Generate three offset binary renders for R, G, B channels
        // Offsets simulate slight angular shifts for corner detection
        const QPointF offsets[] = {
            QPointF(0.5, 0.0),   // R channel - horizontal shift
            QPointF(0.0, 0.5),   // G channel - vertical shift
            QPointF(0.35, 0.35)  // B channel - diagonal shift
        };

        QRgb channels[3];
        QImage channelSDFs[3];

        for (int ch = 0; ch < 3; ++ch) {
            QImage glyphImg(gW, gH, QImage::Format_ARGB32);
            glyphImg.fill(Qt::black);
            QPainter gp(&glyphImg);
            gp.setFont(font);
            gp.setPen(Qt::white);
            gp.drawText(QPointF(sdfConfig.padding + offsets[ch].x(),
                                gH - sdfConfig.padding - fm.descent() + offsets[ch].y()),
                        QString(qc));
            gp.end();
            channelSDFs[ch] = computeSDF(glyphImg, sdfConfig.spread);
        }

        // Place in atlas
        int cellW = gW + sdfConfig.padding;
        if (x + cellW > outW - sdfConfig.padding) {
            x = sdfConfig.padding;
            y += cellH;
        }
        if (y + cellH > outH - sdfConfig.padding) {
            qWarning() << "MSDF atlas too small - truncating at" << i;
            break;
        }

        // Interleave three channels into RGB
        for (int sy = 0; sy < gH && sy + y < outH; ++sy) {
            uchar* dstLine = atlas.scanLine(y + sy);
            int copyW = qMin(gW, outW - x);
            for (int sx = 0; sx < copyW; ++sx) {
                dstLine[(x + sx) * 3 + 0] = channelSDFs[0].constScanLine(sy)[sx];
                dstLine[(x + sx) * 3 + 1] = channelSDFs[1].constScanLine(sy)[sx];
                dstLine[(x + sx) * 3 + 2] = channelSDFs[2].constScanLine(sy)[sx];
            }
        }

        uvMap[cp] = static_cast<float>(x) / outW;

        GlyphMetrics gm;
        gm.codepoint = cp;
        gm.cellWidth = cellW;
        gm.hPad = sdfConfig.padding;
        gm.vPad = sdfConfig.padding;
        glyphs.append(gm);

        x += cellW;
    }

    emit progress(100);

    result.success = true;
    result.image = atlas;
    result.config.glyphs = glyphs;
    result.config.uvMap = uvMap;
    result.config.charset = chars;

    return result;
}

// ============================================================================
// Metrics Optimizer
// ============================================================================

FontAtlasGenerator::MetricsSuggestion FontAtlasGenerator::analyzeMetrics(const AtlasConfig& config) const
{
    MetricsSuggestion result;

    if (config.charset.isEmpty() || config.fontFamily.isEmpty())
        return result;

    QFont font(config.fontFamily);
    font.setPixelSize(config.fontSize);
    font.setBold(config.fontWeight >= 700);
    font.setItalic(config.italic);
    QFontMetricsF fm(font);

    double totalAdvance = 0.0;
    double maxAdv = 0.0;
    double minAdv = 1e9;
    int count = 0;

    for (int i = 0; i < config.charset.size(); ++i) {
        QChar c = config.charset[i];
        double adv = fm.horizontalAdvance(c);
        totalAdvance += adv;
        maxAdv = qMax(maxAdv, adv);
        minAdv = qMin(minAdv, adv);
        ++count;

        QRect bbox = fm.boundingRect(c);
        if (bbox.x() < 0 || bbox.right() > qCeil(adv)) {
            result.overflowGlyphs << QString(c);
            ++result.glyphOverflowCount;
        }
    }

    if (count == 0) return result;

    result.averageAdvance = totalAdvance / count;
    result.maxAdvance = maxAdv;
    result.minAdvance = minAdv;

    // Suggested vPad based on font metrics: use cap height ratio
    double capHeight = fm.boundingRect(QChar('H')).height();
    result.suggestedGlobalHeight = qMax(static_cast<int>(capHeight * 1.35), config.fontSize);
    result.suggestedGlobalVPad = qMax(static_cast<int>(capHeight * 0.12), 4);

    // Suggested hPad: small fraction of average advance
    result.suggestedHPad = qMax(static_cast<int>(result.averageAdvance * 0.06), 1);
    result.suggestedVPad = result.suggestedGlobalVPad;

    // Optimal atlas size
    QVector<GlyphMetrics> tempGlyphs;
    for (int i = 0; i < config.charset.size(); ++i) {
        GlyphMetrics gm;
        gm.codepoint = config.charset[i].unicode();
        gm.cellWidth = qCeil(fm.horizontalAdvance(config.charset[i])) + result.suggestedHPad * 2;
        tempGlyphs.append(gm);
    }
    auto sizes = suggestOptimalAtlasSize(tempGlyphs, result.suggestedGlobalHeight);
    result.optimalAtlasWidth = sizes.size() > 0 ? sizes[0].toInt() : result.optimalAtlasWidth;
    result.optimalAtlasHeight = sizes.size() > 1 ? sizes[1].toInt() : result.optimalAtlasHeight;

    return result;
}

AtlasConfig FontAtlasGenerator::applyOptimizedMetrics(const AtlasConfig& config, const MetricsSuggestion& suggestion)
{
    AtlasConfig optimized = config;
    optimized.globalHeight = suggestion.suggestedGlobalHeight;
    optimized.globalVPad = suggestion.suggestedGlobalVPad;

    // Recompute glyph metrics with optimized spacing
    QFont font(optimized.fontFamily);
    font.setPixelSize(optimized.fontSize);
    font.setBold(optimized.fontWeight >= 700);
    font.setItalic(optimized.italic);
    QFontMetricsF fm(font);

    QVector<GlyphMetrics> newGlyphs;
    for (int i = 0; i < config.charset.size(); ++i) {
        QChar c = config.charset[i];
        GlyphMetrics gm;
        gm.codepoint = c.unicode();
        gm.cellWidth = qCeil(fm.horizontalAdvance(c)) + suggestion.suggestedHPad * 2;
        gm.hPad = suggestion.suggestedHPad;
        gm.vPad = suggestion.suggestedVPad;
        newGlyphs.append(gm);
    }
    optimized.glyphs = newGlyphs;
    optimized.atlasWidth = suggestion.optimalAtlasWidth;
    optimized.atlasHeight = suggestion.optimalAtlasHeight;

    return optimized;
}

QStringList FontAtlasGenerator::suggestOptimalAtlasSize(const QVector<GlyphMetrics>& glyphs, int cellHeight)
{
    if (glyphs.isEmpty() || cellHeight <= 0)
        return {"512", "512"};

    // Power-of-two sizes to try
    const int sizes[] = {128, 256, 512, 1024, 2048};
    qint64 totalArea = 0;
    int maxRowWidth = 0;
    int currentRow = 0;
    int currentX = 0;

    for (const auto& g : glyphs) {
        if (currentX + g.cellWidth > 4096) {
            currentRow += cellHeight;
            currentX = 0;
        }
        currentX += g.cellWidth;
        maxRowWidth = qMax(maxRowWidth, currentX);
    }
    currentRow += cellHeight;

    // Find smallest power-of-two that fits
    for (int w : sizes) {
        for (int h : sizes) {
            if (w >= maxRowWidth && h >= currentRow) {
                return {QString::number(w), QString::number(h)};
            }
        }
    }

    return {"2048", "2048"};
}

FontAtlasGenerator::CoverageReport FontAtlasGenerator::validateGlyphCoverage(
    const QString& fontFamily, int fontSize, const QString& charset) const
{
    CoverageReport report;
    report.totalRequested = charset.length();
    if (charset.isEmpty() || fontFamily.isEmpty())
        return report;

    QFont font(fontFamily);
    font.setPixelSize(fontSize);
    QFontMetricsF fm(font);

    int available = 0;
    QStringList missing;
    for (int i = 0; i < charset.length(); ++i) {
        QChar c = charset[i];
        if (fm.inFont(c)) {
            ++available;
        } else {
            missing << QString(c);
        }
    }

    report.available = available;
    report.missing = charset.length() - available;
    report.coveragePercent = charset.length() > 0
        ? (100.0 * available) / charset.length() : 0.0;
    report.missingChars = missing;
    return report;
}