#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <QImage>
#include <QList>
#include "../../core/editor/EditorModule.h"

namespace ks {

class FontCreatorQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentFont READ currentFont NOTIFY currentFontChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(int atlasWidth READ atlasWidth WRITE setAtlasWidth NOTIFY atlasSizeChanged)
    Q_PROPERTY(int atlasHeight READ atlasHeight WRITE setAtlasHeight NOTIFY atlasSizeChanged)
    Q_PROPERTY(bool isGenerating READ isGenerating NOTIFY generatingChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    ~FontCreatorQmlBridge();

    static FontCreatorQmlBridge* instance();

    QString currentFont() const { return m_currentFont; }
    int fontSize() const { return m_fontSize; }
    void setFontSize(int size) { m_fontSize = size; emit fontSizeChanged(); }
    int atlasWidth() const { return m_atlasWidth; }
    void setAtlasWidth(int w) { m_atlasWidth = w; emit atlasSizeChanged(); }
    int atlasHeight() const { return m_atlasHeight; }
    void setAtlasHeight(int h) { m_atlasHeight = h; emit atlasSizeChanged(); }
    bool isGenerating() const { return m_isGenerating; }
    QString statusMessage() const { return m_statusMessage; }

    Q_INVOKABLE QStringList getSystemFonts();
    Q_INVOKABLE bool generateAtlas(const QString& outputPngPath);
    Q_INVOKABLE bool generateSDFAtlas(const QString& outputPngPath, int spread);
    Q_INVOKABLE bool generateMSDFAtlas(const QString& outputPngPath, int spread);
    Q_INVOKABLE bool saveAtlas(const QString& pngPath, const QString& acfPath);
    Q_INVOKABLE bool loadPreset(const QString& acfPath);
    Q_INVOKABLE bool savePreset(const QString& acfPath);
    Q_INVOKABLE QVariantMap getCurrentConfig();
    Q_INVOKABLE void setCurrentConfig(const QVariantMap& config);
    Q_INVOKABLE void setFontFamily(const QString& family);
    Q_INVOKABLE void setFontWeight(int weight);
    Q_INVOKABLE void setItalic(bool italic);
    Q_INVOKABLE void setCharset(const QString& charset);
    Q_INVOKABLE void setGlobalHeight(int height);
    Q_INVOKABLE void setGlobalVPad(int vpad);
    Q_INVOKABLE void addGlyph(uint codepoint, int cellWidth, int hPad, int vPad);
    Q_INVOKABLE void removeGlyph(uint codepoint);
    Q_INVOKABLE QVariantList getGlyphs();
    Q_INVOKABLE QString getPreviewText();
    Q_INVOKABLE void setPreviewText(const QString& text);
    Q_INVOKABLE QString exportToACF(const QString& acfPath);
    Q_INVOKABLE QString exportToJSON(const QString& jsonPath);
    Q_INVOKABLE bool importFromJSON(const QString& jsonPath);
    Q_INVOKABLE QVariantMap getDefaultConfig();
    Q_INVOKABLE QStringList getCommonCharsets();
    Q_INVOKABLE QStringList getAvailableRanges();
    Q_INVOKABLE void enableRange(const QString& rangeName, bool enable);
    Q_INVOKABLE QStringList getEnabledRanges();
    Q_INVOKABLE bool applyCombinedCharset();
    Q_INVOKABLE void clearRanges();
    Q_INVOKABLE QVariantMap validateCoverage();
    Q_INVOKABLE QVariantList getKerningPairs();
    Q_INVOKABLE void setKerningPair(uint left, uint right, int kerning);
    Q_INVOKABLE void removeKerningPair(uint left, uint right);
    Q_INVOKABLE void extractKerning();
    Q_INVOKABLE void clearKerningPairs();
    Q_INVOKABLE int getKerningOffset(uint left, uint right);
    Q_INVOKABLE void setHintingEnabled(bool enabled);
    Q_INVOKABLE bool isHintingEnabled() const;
    Q_INVOKABLE void setHintingLevel(int level);
    Q_INVOKABLE int hintingLevel() const;
    Q_INVOKABLE void setGridFitting(bool enabled);
    Q_INVOKABLE bool isGridFitting() const;
    Q_INVOKABLE void setSubpixelHinting(bool enabled);
    Q_INVOKABLE bool isSubpixelHinting() const;
    Q_INVOKABLE void setAntiAliasMode(int mode);
    Q_INVOKABLE int antiAliasMode() const;
    Q_INVOKABLE QStringList antiAliasModeNames() const;
    Q_INVOKABLE QVariantMap analyzeMetrics();
    Q_INVOKABLE void applyOptimizedMetrics(const QVariantMap& suggestion);

signals:
    void currentFontChanged();
    void fontSizeChanged();
    void atlasSizeChanged();
    void generatingChanged();
    void statusMessageChanged();
    void atlasGenerated(const QString& pngPath, int width, int height);
    void presetLoaded(const QString& acfPath);
    void presetSaved(const QString& acfPath);
    void generationProgress(int percent);
    void fontLoaded(const QString& fontPath);

private:
    static FontCreatorQmlBridge* s_instance;
    FontCreatorQmlBridge(QObject* parent = nullptr);

    QString m_currentFont;
    int m_fontSize = 48;
    int m_atlasWidth = 512;
    int m_atlasHeight = 512;
    bool m_isGenerating = false;
    QString m_statusMessage;
    QString m_fontFamily;
    int m_fontWeight = 400;
    bool m_italic = false;
    QString m_charset;
    int m_globalHeight = 85;
    int m_globalVPad = 13;
    QString m_previewText;
    QVariantList m_glyphs;
    QVariantList m_kerningPairs;
    QStringList m_enabledRanges;
    bool m_hintingEnabled = true;
    int m_hintingLevel = 0;
    bool m_gridFitting = true;
    bool m_subpixelHinting = false;
    int m_antiAliasMode = 1; // Standard
    QList<int> m_loadedFonts;
    QImage m_lastAtlas;
};

class FontCreatorEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit FontCreatorEditorModule(QWidget* parent = nullptr);
    ~FontCreatorEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Font Creator"; }
    QString moduleId() const override { return "fontCreator"; }
    int getModulePriority() const override { return 45; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;
};

} // namespace ks
