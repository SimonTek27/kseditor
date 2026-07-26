#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QFont>
#include <QMap>
#include <QVector>

class AcfFile;

namespace ks {

struct KerningPair {
    int left = 0;
    int right = 0;
    int kerning = 0;
};

class FontCreatorQmlBridge : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString currentFont READ currentFont NOTIFY currentFontChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(bool isGenerating READ isGenerating NOTIFY isGeneratingChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(int atlasWidth READ atlasWidth NOTIFY atlasSizeChanged)
    Q_PROPERTY(int atlasHeight READ atlasHeight NOTIFY atlasSizeChanged)

public:
    static FontCreatorQmlBridge* instance();

    QString currentFont() const { return m_currentFont; }
    int fontSize() const { return m_fontSize; }
    void setFontSize(int size);
    bool isGenerating() const { return m_isGenerating; }
    QString statusMessage() const { return m_statusMessage; }
    int atlasWidth() const { return m_atlasWidth; }
    int atlasHeight() const { return m_atlasHeight; }

    // QML-callable methods
    Q_INVOKABLE QStringList getSystemFonts();
    Q_INVOKABLE void setFontFamily(const QString& family);

    Q_INVOKABLE QVariantList getGlyphs();
    Q_INVOKABLE QStringList getCommonCharsets();

    Q_INVOKABLE bool loadPreset(const QString& path);
    Q_INVOKABLE bool savePreset(const QString& path);
    Q_INVOKABLE bool importFromJSON(const QString& path);
    Q_INVOKABLE bool exportToJSON(const QString& path);
    Q_INVOKABLE bool generateAtlas(const QString& path);

    Q_INVOKABLE void extractKerning();
    Q_INVOKABLE void clearKerningPairs();
    Q_INVOKABLE void setKerningPair(int left, int right, int value);
    Q_INVOKABLE void removeKerningPair(int left, int right);
    Q_INVOKABLE QVariantList getKerningPairs();
    Q_INVOKABLE int getKerningOffset(int left, int right) const;

    Q_INVOKABLE void setCharset(const QString& charset);
    Q_INVOKABLE QStringList getAvailableRanges();
    Q_INVOKABLE QStringList getEnabledRanges();
    Q_INVOKABLE void enableRange(const QString& range, bool enabled);
    Q_INVOKABLE void applyCombinedCharset();
    Q_INVOKABLE void clearRanges();

    Q_INVOKABLE QString getPreviewText() const;
    Q_INVOKABLE void setPreviewText(const QString& text);

signals:
    void currentFontChanged();
    void fontSizeChanged();
    void isGeneratingChanged();
    void statusMessageChanged();
    void atlasSizeChanged();
    void atlasGenerated(const QString& pngPath);
    void presetLoaded(const QString& acfPath);
    void presetSaved(const QString& acfPath);

private:
    explicit FontCreatorQmlBridge(QObject* parent = nullptr);
    ~FontCreatorQmlBridge() override;

    void setStatusMessage(const QString& msg);
    void setIsGenerating(bool generating);

    static FontCreatorQmlBridge* s_instance;

    QString m_currentFont;
    int m_fontSize = 32;
    bool m_isGenerating = false;
    QString m_statusMessage;
    QString m_previewText;
    int m_atlasWidth = 4096;
    int m_atlasHeight = 64;

    QFont m_font;
    bool m_bold = false;
    bool m_italic = false;

    // Glyph data: codepoint -> {width, hPad, vPad}
    struct GlyphData {
        int width = 0;
        int hPad = 0;
        int vPad = 0;
    };
    QMap<int, GlyphData> m_glyphs;

    // Kerning
    QMap<qint64, int> m_kerningPairs; // key = (left << 16) | right

    // Charset ranges
    QStringList m_availableRanges;
    QStringList m_enabledRanges;
};

} // namespace ks
