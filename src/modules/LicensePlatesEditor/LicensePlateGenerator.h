#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QColor>
#include <QVariant>
#include <QDateTime>
#include <QJsonObject>
#include <QRandomGenerator>

namespace ks {
namespace license {

// ─── License Plate Generator for 22 Countries ─────────────────────────────────

enum class CountryCode {
    // EU Countries
    Germany = 0,
    France,
    Italy,
    Spain,
    Netherlands,
    Belgium,
    Austria,
    Poland,
    Sweden,
    Finland,
    Denmark,
    Ireland,
    Portugal,
    Greece,
    CzechRepublic,
    Hungary,
    Romania,
    Slovakia,
    Slovenia,
    Croatia,
    Bulgaria,
    // Non-EU
    UK,
    Switzerland,
    Norway,
    USA,
    Canada,
    Japan,
    Australia,
    Custom
};

struct LicensePlateTemplate {
    CountryCode country;
    QString name;
    QString pattern;           // Regex pattern for validation
    QString formatTemplate;    // Template with placeholders like {numbers}, {letters}, {region}
    QString regionCode;        // ISO region code
    QVector<QString> regionCodes;  // Multiple region codes for countries with regions
    QString fontFamily;        // Font to use
    int fontSize = 24;
    QColor backgroundColor = Qt::white;
    QColor textColor = Qt::black;
    QColor borderColor = Qt::black;
    bool useEUStrip = false;   // EU blue strip with stars
    QString euCountryCode;     // Country code for EU strip (e.g., "D", "F", "I", "E", "NL", "B", "A", "PL", "S", "FIN", "DK", "IRL", "P", "GR", "CZ", "H", "RO", "SK", "SLO", "HR", "BG")
    bool isVertical = false;   // For vertical plates (some countries)
    int plateWidth = 520;      // mm
    int plateHeight = 110;     // mm
    QMap<QString, QString> characterMap;  // Character replacements (e.g., for special characters)
    QString description;
};

struct LicensePlate {
    QString plateText;         // The actual plate text (e.g., "AB-123-CD")
    CountryCode country;
    QString region;            // Region/state code
    QString formattedText;     // Display text with formatting
    QDateTime generatedAt;
    QString seed;              // Seed for reproducible generation
    QVariantMap metadata;      // Additional data
};

struct BatchGenerationOptions {
    int count = 1;
    CountryCode country = CountryCode::Germany;
    QString region;            // Specific region, or empty for random
    QString prefix;            // Prefix for all plates
    QString suffix;            // Suffix for all plates
    bool sequential = false;   // Sequential numbering
    int startNumber = 1;
    int seed = 0;
    QString outputDir;
    QString outputFormat = "png";  // png, jpg, svg, pdf
    int dpi = 300;
    bool addWeathering = false;  // Add dirt/scratches
    float weatheringIntensity = 0.3f;
    bool addReflection = false;
    bool transparentBackground = false;
    QVariantMap customMetadata;
};

class LicensePlateGenerator : public QObject {
    Q_OBJECT

public:
    explicit LicensePlateGenerator(QObject* parent = nullptr);
    ~LicensePlateGenerator() override;

    // Template management
    void registerTemplate(const LicensePlateTemplate& template_);
    void unregisterTemplate(CountryCode country);
    LicensePlateTemplate getTemplate(CountryCode country) const;
    QVector<CountryCode> availableCountries() const;
    
    // Single plate generation
    LicensePlate generatePlate(CountryCode country, const QString& region = QString(), const QString& seed = QString());
    LicensePlate generatePlate(const LicensePlateTemplate& template_, const QString& region = QString(), const QString& seed = QString());
    
    // Batch generation
    QVector<LicensePlate> generateBatch(const BatchGenerationOptions& options);
    bool generateBatchToFiles(const BatchGenerationOptions& options);
    
    // Validation
    bool validatePlate(const QString& plateText, CountryCode country, QString* error = nullptr) const;
    bool validateRegion(const QString& region, CountryCode country) const;
    QString normalizePlate(const QString& plateText, CountryCode country) const;
    
    // Rendering
    QImage renderPlate(const LicensePlate& plate, int width = 520, int height = 110, int dpi = 300) const;
    bool savePlateImage(const LicensePlate& plate, const QString& filePath, int dpi = 300) const;
    
    // Weathering effects
    QImage applyWeathering(const QImage& plate, float intensity = 0.3f) const;
    QImage applyReflection(const QImage& plate) const;
    
    // EU strip rendering
    QImage renderEUStrip(const QString& countryCode, int width, int height) const;
    
    // Utility
    QStringList getRegionsForCountry(CountryCode country) const;
    QString getCountryName(CountryCode country) const;
    static QString countryCodeToString(CountryCode code);
    static CountryCode stringToCountryCode(const QString& code);
    
    // Presets
    void loadPresets(const QString& presetName);
    void savePresets(const QString& presetName) const;
    QStringList availablePresets() const;

signals:
    void plateGenerated(const LicensePlate& plate);
    void batchProgress(int current, int total);
    void batchCompleted(const QVector<LicensePlate>& plates);
    void generationError(const QString& error);
    void plateSaved(const QString& filePath);

private:
    void initializeTemplates();
    void initializeCountryData();
    LicensePlate generateFromTemplate(const LicensePlateTemplate& tmpl, const QString& region, const QString& seed);
    QString generateRandomPlate(const LicensePlateTemplate& tmpl, const QString& region) const;
    QString generateSequentialPlate(const LicensePlateTemplate& tmpl, const QString& region, int number) const;
    QString applyPattern(const QString& pattern, const QMap<QString, QString>& replacements) const;
    
    QImage renderText(const QString& text, const LicensePlateTemplate& tmpl, int width, int height) const;
    QImage applyDirt(const QImage& image, float intensity) const;
    QImage applyScratches(const QImage& image, float intensity) const;
    QImage addNoise(const QImage& image, float intensity) const;
    QImage addReflection(const QImage& image) const;
    QImage applyPerspective(const QImage& image, float angle) const;
    
    QString genRandomLetters(int count, QRandomGenerator& gen) const;
    QString genRandomNumbers(int count, QRandomGenerator& gen) const;
    QString genRandomPrefecture(QRandomGenerator& gen) const;
    QString genRandomClass(QRandomGenerator& gen) const;
    QString genRandomHiragana(QRandomGenerator& gen) const;
    
    static QString formatWithSeparators(const QString& text, const QString& separator, int groupSize);
    static bool matchesPattern(const QString& text, const QString& pattern);
    static QString replacePlaceholders(const QString& template_, const QMap<QString, QString>& values);
    
    struct CountryData {
        QString name;
        QString isoCode;
        QString euCode;
        QStringList regions;
        QString defaultFont;
        QColor defaultBgColor;
        QColor defaultTextColor;
        bool hasEUStrip = true;
    };
    
    QMap<CountryCode, LicensePlateTemplate> m_templates;
    QMap<CountryCode, CountryData> m_countryData;
    QMap<QString, LicensePlateTemplate> m_presets;
};

} // namespace license
} // namespace ks

Q_DECLARE_METATYPE(ks::license::LicensePlate)
Q_DECLARE_METATYPE(ks::license::CountryCode)
Q_DECLARE_METATYPE(ks::license::BatchGenerationOptions)