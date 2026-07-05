#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QSet>
#include <QImage>
#include <QMap>
#include <QVector>

struct GlyphMetrics {
    uint32_t codepoint = 0;
    int cellWidth = 0;
    int hPad = 0;
    int vPad = 0;
    int lsbDelta = 0;
    int rsbDelta = 0;
};

struct KerningPair {
    uint32_t left = 0;
    uint32_t right = 0;
    int kerning = 0;
};

enum class AntiAliasMode : int {
    None = 0,
    Standard,
    SubpixelRGB,
    LCD
};

struct HintingConfig {
    bool enableAutoHinting = true;
    bool enableGridFitting = true;
    int hintingLevel = 0; // 0=light, 1=normal, 2=full
    bool enableSubpixel = false;
    bool optimizeForLCD = false;
    AntiAliasMode antiAlias = AntiAliasMode::Standard;
};

struct AtlasConfig {
    QString fontFamily;
    QString fontPath;
    int fontSize = 48;
    int fontWeight = 400;
    bool italic = false;
    int atlasWidth = 512;
    int atlasHeight = 512;
    int globalHeight = 85;
    int globalVPad = 13;
    QString charset;
    QVector<GlyphMetrics> glyphs;
    QMap<uint32_t, float> uvMap;
    QVector<KerningPair> kerningPairs;
    HintingConfig hinting;
};

struct AtlasResult {
    bool success = false;
    QString errorMsg;
    AtlasConfig config;
    QImage image;
};

class FontAtlasGenerator : public QObject {
    Q_OBJECT
public:
    explicit FontAtlasGenerator();
    ~FontAtlasGenerator();

    AtlasResult generate(const AtlasConfig& config);
    bool save(const AtlasResult& result, const QString& pngPath, QString* errorOut = nullptr);
    
    AtlasConfig loadPreset(const QString& acfPath, bool* ok = nullptr);
    bool savePreset(const AtlasConfig& cfg, const QString& acfPath);
    
    static QStringList systemFonts();
    static QString fontFileForFamily(const QString& family, int weight, bool italic);

    // ── Auto-hinting ─────────────────────────────────────────────────────
    QVector<GlyphMetrics> applyHinting(const AtlasConfig& config);
    void setHintingConfig(const HintingConfig& config) { m_hintingConfig = config; }
    HintingConfig hintingConfig() const { return m_hintingConfig; }

    // ── Kerning ──────────────────────────────────────────────────────────
    QVector<KerningPair> extractKerningPairs(const QString& fontFamily, int fontSize, const QString& charset);
    void setKerningEnabled(bool enabled) { m_kerningEnabled = enabled; }
    bool kerningEnabled() const { return m_kerningEnabled; }
    int getKerning(uint32_t left, uint32_t right) const;

    // ── Unicode support ──────────────────────────────────────────────────
    static QStringList availableUnicodeRanges();
    static QString generateCharsetForRange(const QString& rangeName);
    bool setUnicodeRange(const QString& rangeName);
    void enableUnicodeRange(const QString& rangeName, bool enable = true);
    QStringList enabledUnicodeRanges() const;
    QString generateCombinedCharset() const;

    // ── SDF (Signed Distance Field) ──────────────────────────────────────
    struct SDFConfig {
        int spread = 8;           // distance field spread in output pixels
        int onedgeValue = 128;    // value at the glyph edge (0-255)
        int padding = 4;          // padding around each glyph in the atlas
        double scale = 2.0;       // supersampling scale factor for quality
    };

    AtlasResult generateSDF(const AtlasConfig& config, const SDFConfig& sdfConfig = SDFConfig());
    AtlasResult generateMSDF(const AtlasConfig& config, const SDFConfig& sdfConfig = SDFConfig());

    // ── Metrics Optimizer ────────────────────────────────────────────────
    struct MetricsSuggestion {
        int suggestedGlobalHeight = 0;
        int suggestedGlobalVPad = 0;
        int suggestedHPad = 0;
        int suggestedVPad = 0;
        int optimalAtlasWidth = 512;
        int optimalAtlasHeight = 512;
        double averageAdvance = 0.0;
        double maxAdvance = 0.0;
        double minAdvance = 0.0;
        int glyphOverflowCount = 0;
        QStringList overflowGlyphs;
    };
    MetricsSuggestion analyzeMetrics(const AtlasConfig& config) const;
    AtlasConfig applyOptimizedMetrics(const AtlasConfig& config, const MetricsSuggestion& suggestion);
    static QStringList suggestOptimalAtlasSize(const QVector<GlyphMetrics>& glyphs, int cellHeight);

    // Glyph coverage validation
    struct CoverageReport {
        int totalRequested = 0;
        int available = 0;
        int missing = 0;
        double coveragePercent = 0.0;
        QStringList missingChars;
    };
    CoverageReport validateGlyphCoverage(const QString& fontFamily, int fontSize, const QString& charset) const;

signals:
    void progress(int percent);

private:
    struct FTContext;
    FTContext* m_ft = nullptr;
    
    bool initFreeType();
    void shutdownFreeType();

    HintingConfig m_hintingConfig;
    bool m_kerningEnabled = true;
    QMap<QPair<uint32_t, uint32_t>, int> m_kerningCache;
    QString m_charset;
    QSet<QString> m_enabledRanges;
};