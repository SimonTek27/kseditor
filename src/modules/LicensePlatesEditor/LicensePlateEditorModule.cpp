#include "LicensePlateEditorModule.h"

#include <QApplication>
#include <QClipboard>
#include <QPainter>
#include <QPainterPath>
#include <QDebug>
#include <QtMath>
#include <cmath>
#include <QCryptographicHash>
#include <cstdlib>
#include <random>
#include <QStandardPaths>
#include <QSettings>

namespace ks {

// ═══════════════════════════════════════════════════════════════════════
// LicensePlatesManager Implementation
// ═══════════════════════════════════════════════════════════════════════

LicensePlatesManager::LicensePlatesManager() {
    initDefaultCountries();
}

LicensePlatesManager::~LicensePlatesManager() {
}

void LicensePlatesManager::initDefaultCountries() {
    CountryFormat it;
    it.code = "IT";
    it.name = "Italy";
    it.plateExample = "AB-123-CD";
    it.pattern = QRegularExpression("^[A-Z]{2}[ -]?\\d{3}[ -]?[A-Z]{2}$");
    it.maxLength = 7;
    it.backgroundColor = QColor(255, 255, 255);
    it.textColor = QColor(0, 0, 0);
    it.borderColor = QColor(0, 0, 0);
    it.hasCountryBand = true;
    it.countryBandText = "I";
    it.hasEUStars = true;
    it.fontFamily = "Arial";
    m_countryFormats["IT"] = it;

    CountryFormat de;
    de.code = "DE";
    de.name = "Germany";
    de.plateExample = "AB CD 1234";
    de.pattern = QRegularExpression("^[A-Z]{1,3}[ -]?[A-Z]{1,2}[ -]?\\d{1,4}$");
    de.maxLength = 8;
    de.backgroundColor = QColor(255, 255, 255);
    de.textColor = QColor(0, 0, 0);
    de.borderColor = QColor(0, 0, 0);
    de.hasCountryBand = true;
    de.countryBandText = "D";
    de.hasEUStars = true;
    de.fontFamily = "FE-Schrift";
    m_countryFormats["DE"] = de;

    CountryFormat uk;
    uk.code = "UK";
    uk.name = "United Kingdom";
    uk.plateExample = "AB12 CDE";
    uk.pattern = QRegularExpression("^[A-Z]{2}\\d{2}[ -]?[A-Z]{3}$");
    uk.maxLength = 7;
    uk.backgroundColor = QColor(255, 255, 255);
    uk.textColor = QColor(0, 0, 0);
    uk.borderColor = QColor(0, 0, 0);
    uk.hasCountryBand = true;
    uk.countryBandText = "GB";
    uk.hasEUStars = false;
    uk.fontFamily = "Charles Wright";
    m_countryFormats["UK"] = uk;

    CountryFormat fr;
    fr.code = "FR";
    fr.name = "France";
    fr.plateExample = "AB-123-CD";
    fr.pattern = QRegularExpression("^[A-Z]{2}[ -]?\\d{3}[ -]?[A-Z]{2}$");
    fr.maxLength = 7;
    fr.backgroundColor = QColor(255, 255, 255);
    fr.textColor = QColor(0, 0, 0);
    fr.borderColor = QColor(0, 0, 0);
    fr.hasCountryBand = true;
    fr.countryBandText = "F";
    fr.hasEUStars = true;
    fr.fontFamily = "Arial";
    m_countryFormats["FR"] = fr;

    CountryFormat es;
    es.code = "ES";
    es.name = "Spain";
    es.plateExample = "1234 ABC";
    es.pattern = QRegularExpression("^\\d{4}[ -]?[A-Z]{3}$");
    es.maxLength = 7;
    es.backgroundColor = QColor(255, 255, 255);
    es.textColor = QColor(0, 0, 0);
    es.borderColor = QColor(0, 0, 0);
    es.hasCountryBand = true;
    es.countryBandText = "E";
    es.hasEUStars = true;
    m_countryFormats["ES"] = es;

    CountryFormat jp;
    jp.code = "JP";
    jp.name = "Japan";
    jp.plateExample = "12-34";
    jp.pattern = QRegularExpression("^\\d{2,3}[ -]?\\d{2,4}$");
    jp.maxLength = 6;
    jp.backgroundColor = QColor(255, 255, 255);
    jp.textColor = QColor(0, 0, 0);
    jp.borderColor = QColor(0, 0, 200);
    jp.hasCountryBand = false;
    jp.fontFamily = "Arial";
    m_countryFormats["JP"] = jp;

    CountryFormat us;
    us.code = "US";
    us.name = "United States";
    us.plateExample = "ABC-1234";
    us.pattern = QRegularExpression("^[A-Z0-9]{1,7}$");
    us.maxLength = 7;
    us.backgroundColor = QColor(255, 255, 255);
    us.textColor = QColor(0, 0, 150);
    us.borderColor = QColor(0, 0, 0);
    us.hasCountryBand = false;
    us.fontFamily = "Arial";
    m_countryFormats["US"] = us;

    CountryFormat au;
    au.code = "AU";
    au.name = "Australia";
    au.plateExample = "ABC-123";
    au.pattern = QRegularExpression("^[A-Z]{3}[ -]?\\d{3}$");
    au.maxLength = 6;
    au.backgroundColor = QColor(255, 255, 255);
    au.textColor = QColor(0, 0, 0);
    au.borderColor = QColor(0, 0, 0);
    au.hasCountryBand = false;
    au.fontFamily = "Arial";
    m_countryFormats["AU"] = au;

    CountryFormat br;
    br.code = "BR";
    br.name = "Brazil";
    br.plateExample = "ABC-1234";
    br.pattern = QRegularExpression("^[A-Z]{3}[ -]?\\d{4}$");
    br.maxLength = 7;
    br.backgroundColor = QColor(255, 255, 0);
    br.textColor = QColor(0, 0, 0);
    br.borderColor = QColor(0, 0, 0);
    br.hasCountryBand = false;
    br.fontFamily = "Arial";
    m_countryFormats["BR"] = br;

    CountryFormat cn;
    cn.code = "CN";
    cn.name = "China";
    cn.plateExample = "京A-12345";
    cn.pattern = QRegularExpression("^[\\x4e00-\\x9fff][A-Z][ -]?[A-Z0-9]{5}$");
    cn.maxLength = 8;
    cn.backgroundColor = QColor(255, 255, 255);
    cn.textColor = QColor(0, 0, 0);
    cn.borderColor = QColor(0, 0, 0);
    cn.hasCountryBand = false;
    cn.fontFamily = "Arial";
    m_countryFormats["CN"] = cn;

    CountryFormat nl;
    nl.code = "NL";
    nl.name = "Netherlands";
    nl.plateExample = "AB-123-CD";
    nl.pattern = QRegularExpression("^[A-Z]{2}[ -]?\\d{3}[ -]?[A-Z]{2}$");
    nl.maxLength = 7;
    nl.backgroundColor = QColor(255, 255, 0);
    nl.textColor = QColor(0, 0, 0);
    nl.borderColor = QColor(0, 0, 0);
    nl.hasCountryBand = true;
    nl.countryBandText = "NL";
    nl.hasEUStars = true;
    nl.fontFamily = "Arial";
    m_countryFormats["NL"] = nl;

    CountryFormat se;
    se.code = "SE";
    se.name = "Sweden";
    se.plateExample = "ABC 123";
    se.pattern = QRegularExpression("^[A-Z]{3}[ -]?\\d{3}$");
    se.maxLength = 6;
    se.backgroundColor = QColor(255, 255, 255);
    se.textColor = QColor(0, 0, 0);
    se.borderColor = QColor(0, 0, 0);
    se.hasCountryBand = true;
    se.countryBandText = "S";
    se.hasEUStars = true;
    se.fontFamily = "Arial";
    m_countryFormats["SE"] = se;

    CountryFormat ch;
    ch.code = "CH";
    ch.name = "Switzerland";
    ch.plateExample = "AB 123456";
    ch.pattern = QRegularExpression("^[A-Z]{2}[ -]?\\d{1,6}$");
    ch.maxLength = 8;
    ch.backgroundColor = QColor(255, 255, 255);
    ch.textColor = QColor(0, 0, 0);
    ch.borderColor = QColor(0, 0, 0);
    ch.hasCountryBand = true;
    ch.countryBandText = "CH";
    ch.hasEUStars = false;
    ch.fontFamily = "Arial";
    m_countryFormats["CH"] = ch;

    CountryFormat ru;
    ru.code = "RU";
    ru.name = "Russia";
    ru.plateExample = "A123BC";
    ru.pattern = QRegularExpression("^[A-Z]\\d{3}[A-Z]{2}$");
    ru.maxLength = 6;
    ru.backgroundColor = QColor(255, 255, 255);
    ru.textColor = QColor(0, 0, 0);
    ru.borderColor = QColor(0, 0, 0);
    ru.hasCountryBand = false;
    ru.fontFamily = "Arial";
    m_countryFormats["RU"] = ru;

    CountryFormat be;
    be.code = "BE"; be.name = "Belgium"; be.plateExample = "ABC-123";
    be.pattern = QRegularExpression("^[A-Z]{3}[ -]?\\d{3}$");
    be.maxLength = 6; be.backgroundColor = QColor(255, 255, 255);
    be.textColor = QColor(200, 0, 0); be.borderColor = QColor(0, 0, 0);
    be.hasCountryBand = true; be.countryBandText = "B"; be.hasEUStars = true;
    be.fontFamily = "Arial"; m_countryFormats["BE"] = be;

    CountryFormat at;
    at.code = "AT"; at.name = "Austria"; at.plateExample = "AB-123CD";
    at.pattern = QRegularExpression("^[A-Z]{2}[ -]?\\d{3,6}$");
    at.maxLength = 7; at.backgroundColor = QColor(255, 255, 255);
    at.textColor = QColor(0, 0, 0); at.borderColor = QColor(0, 0, 0);
    at.hasCountryBand = true; at.countryBandText = "A"; at.hasEUStars = true;
    at.fontFamily = "Arial"; m_countryFormats["AT"] = at;

    CountryFormat no;
    no.code = "NO"; no.name = "Norway"; no.plateExample = "AB 12345";
    no.pattern = QRegularExpression("^[A-Z]{2}[ -]?\\d{5}$");
    no.maxLength = 7; no.backgroundColor = QColor(255, 255, 255);
    no.textColor = QColor(0, 0, 0); no.borderColor = QColor(0, 0, 0);
    no.hasCountryBand = true; no.countryBandText = "N"; no.hasEUStars = false;
    no.fontFamily = "Arial"; m_countryFormats["NO"] = no;

    CountryFormat dk;
    dk.code = "DK"; dk.name = "Denmark"; dk.plateExample = "AB 12 345";
    dk.pattern = QRegularExpression("^[A-Z]{2}[ -]?\\d{2,5}$");
    dk.maxLength = 7; dk.backgroundColor = QColor(255, 255, 255);
    dk.textColor = QColor(0, 0, 0); dk.borderColor = QColor(0, 0, 0);
    dk.hasCountryBand = true; dk.countryBandText = "DK"; dk.hasEUStars = true;
    dk.fontFamily = "Arial"; m_countryFormats["DK"] = dk;

    CountryFormat fi;
    fi.code = "FI"; fi.name = "Finland"; fi.plateExample = "ABC-123";
    fi.pattern = QRegularExpression("^[A-Z]{3}[ -]?\\d{3}$");
    fi.maxLength = 6; fi.backgroundColor = QColor(255, 255, 255);
    fi.textColor = QColor(0, 0, 0); fi.borderColor = QColor(0, 0, 0);
    fi.hasCountryBand = true; fi.countryBandText = "FIN"; fi.hasEUStars = true;
    fi.fontFamily = "Arial"; m_countryFormats["FI"] = fi;

    CountryFormat pl;
    pl.code = "PL"; pl.name = "Poland"; pl.plateExample = "ABC 12345";
    pl.pattern = QRegularExpression("^[A-Z]{2,3}[ -]?[A-Z0-9]{3,5}$");
    pl.maxLength = 8; pl.backgroundColor = QColor(255, 255, 255);
    pl.textColor = QColor(0, 0, 0); pl.borderColor = QColor(0, 0, 200);
    pl.hasCountryBand = true; pl.countryBandText = "PL"; pl.hasEUStars = true;
    pl.fontFamily = "Arial"; m_countryFormats["PL"] = pl;

    CountryFormat cz;
    cz.code = "CZ"; cz.name = "Czech Republic"; cz.plateExample = "1AB 1234";
    cz.pattern = QRegularExpression("^\\d{1,2}[A-Z]{2}[ -]?\\d{3,4}$");
    cz.maxLength = 8; cz.backgroundColor = QColor(255, 255, 255);
    cz.textColor = QColor(0, 0, 0); cz.borderColor = QColor(0, 0, 0);
    cz.hasCountryBand = true; cz.countryBandText = "CZ"; cz.hasEUStars = true;
    cz.fontFamily = "Arial"; m_countryFormats["CZ"] = cz;

    CountryFormat pt;
    pt.code = "PT"; pt.name = "Portugal"; pt.plateExample = "AB-12-CD";
    pt.pattern = QRegularExpression("^[A-Z]{2}[ -]?\\d{2}[ -]?[A-Z]{2}$");
    pt.maxLength = 6; pt.backgroundColor = QColor(255, 255, 255);
    pt.textColor = QColor(0, 0, 0); pt.borderColor = QColor(0, 0, 0);
    pt.hasCountryBand = true; pt.countryBandText = "P"; pt.hasEUStars = true;
    pt.fontFamily = "Arial"; m_countryFormats["PT"] = pt;
}

QStringList LicensePlatesManager::availableCountries() const {
    return m_countryFormats.keys();
}

CountryFormat LicensePlatesManager::getCountryFormat(const QString& countryCode) const {
    return m_countryFormats.value(countryCode);
}

bool LicensePlatesManager::validatePlateText(const QString& text, const QString& countryCode) {
    if (!m_countryFormats.contains(countryCode)) return false;
    const CountryFormat& fmt = m_countryFormats[countryCode];
    return fmt.pattern.match(text).hasMatch();
}

void LicensePlatesManager::addCountryFormat(const CountryFormat& format) {
    m_countryFormats[format.code] = format;
}

void LicensePlatesManager::setQRCodeConfig(const QRCodeConfig& config) {
    m_qrConfig = config;
}

QImage LicensePlatesManager::generateQRCode(const QString& text, int moduleSize,
                                             const QColor& fg, const QColor& bg) {
    return QRCodeWriter::encode(text, 0, QRCodeWriter::M, moduleSize, fg, bg);
}

void LicensePlatesManager::setHolographicEffect(const HolographicEffect& effect) {
    m_holoEffect = effect;
}

void LicensePlatesManager::applyHolographicEffect(QImage& image) {
    if (!m_holoEffect.enabled) return;

    QPainter painter(&image);
    painter.setCompositionMode(QPainter::CompositionMode_Overlay);

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            double angle = m_holoEffect.angle * M_PI / 180.0;
            double wave = qSin((x * qCos(angle) + y * qSin(angle)) * 0.05) * 0.5 + 0.5;
            double wave2 = qSin((x * qSin(angle) - y * qCos(angle)) * 0.03) * 0.5 + 0.5;

            QColor base = image.pixelColor(x, y);
            int r = qBound(0, static_cast<int>(base.red() + (m_holoEffect.primaryColor.red() - base.red()) * wave * m_holoEffect.intensity), 255);
            int g = qBound(0, static_cast<int>(base.green() + (m_holoEffect.secondaryColor.green() - base.green()) * wave2 * m_holoEffect.intensity), 255);
            int b = qBound(0, static_cast<int>(base.blue() + (m_holoEffect.primaryColor.blue() - base.blue()) * wave * m_holoEffect.intensity), 255);

            image.setPixelColor(x, y, QColor(r, g, b, base.alpha()));
        }
    }

    painter.end();
}

bool LicensePlatesManager::loadStyle(const QString& stylePath, PlateStyle& outStyle) {
    if (m_styleCache.contains(stylePath)) {
        outStyle = m_styleCache[stylePath];
        return true;
    }

    QFile file(stylePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open style file:" << stylePath;
        return false;
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    QRegularExpression sizeRe("size\\s*=\\s*\\{(\\d+),\\s*(\\d+)\\}");
    QRegularExpression bgRectRe("backgroundRect\\s*=\\s*\\{(\\d+),\\s*(\\d+),\\s*(\\d+),\\s*(\\d+)\\}");
    QRegularExpression textRectRe("textRect\\s*=\\s*\\{(\\d+),\\s*(\\d+),\\s*(\\d+),\\s*(\\d+)\\}");
    QRegularExpression fontFamilyRe("fontFamily\\s*=\\s*\"([^\"]+)\"");
    QRegularExpression fontSizeRe("fontSize\\s*=\\s*(\\d+)");
    QRegularExpression textColorRe("textColor\\s*=\\s*\\{(\\d+),\\s*(\\d+),\\s*(\\d+)\\}");
    QRegularExpression bgColorRe("backgroundColor\\s*=\\s*\\{(\\d+),\\s*(\\d+),\\s*(\\d+)\\}");
    QRegularExpression borderColorRe("borderColor\\s*=\\s*\\{(\\d+),\\s*(\\d+),\\s*(\\d+)\\}");
    QRegularExpression borderWidthRe("borderWidth\\s*=\\s*(\\d+)");
    QRegularExpression reflectiveRe("reflective\\s*=\\s*(true|false)");
    QRegularExpression bgImageRe("backgroundImage\\s*=\\s*\"([^\"]+)\"");

    auto match = sizeRe.match(content);
    if (match.hasMatch()) {
        outStyle.size = QSize(match.captured(1).toInt(), match.captured(2).toInt());
    } else {
        outStyle.size = QSize(512, 128);
    }

    match = bgRectRe.match(content);
    if (match.hasMatch()) {
        outStyle.backgroundRect = QRect(match.captured(1).toInt(), match.captured(2).toInt(),
                                        match.captured(3).toInt(), match.captured(4).toInt());
    } else {
        outStyle.backgroundRect = QRect(0, 0, outStyle.size.width(), outStyle.size.height());
    }

    match = textRectRe.match(content);
    if (match.hasMatch()) {
        outStyle.textRect = QRect(match.captured(1).toInt(), match.captured(2).toInt(),
                                  match.captured(3).toInt(), match.captured(4).toInt());
    } else {
        outStyle.textRect = QRect(50, 20, outStyle.size.width() - 100, outStyle.size.height() - 40);
    }

    match = fontFamilyRe.match(content);
    if (match.hasMatch()) {
        outStyle.textFont.setFamily(match.captured(1));
    } else {
        outStyle.textFont.setFamily("Arial");
    }

    match = fontSizeRe.match(content);
    if (match.hasMatch()) {
        outStyle.textFont.setPointSize(match.captured(1).toInt());
    } else {
        outStyle.textFont.setPointSize(48);
    }

    match = textColorRe.match(content);
    if (match.hasMatch()) {
        outStyle.textColor = QColor(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt());
    } else {
        outStyle.textColor = Qt::black;
    }

    match = bgColorRe.match(content);
    if (match.hasMatch()) {
        outStyle.backgroundColor = QColor(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt());
    } else {
        outStyle.backgroundColor = Qt::white;
    }

    match = borderColorRe.match(content);
    if (match.hasMatch()) {
        outStyle.borderColor = QColor(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt());
    } else {
        outStyle.borderColor = Qt::black;
    }

    match = borderWidthRe.match(content);
    if (match.hasMatch()) {
        outStyle.borderWidth = match.captured(1).toFloat();
    } else {
        outStyle.borderWidth = 2.0f;
    }

    match = reflectiveRe.match(content);
    if (match.hasMatch()) {
        outStyle.isReflective = (match.captured(1) == "true");
    } else {
        outStyle.isReflective = false;
    }

    match = bgImageRe.match(content);
    if (match.hasMatch()) {
        outStyle.backgroundImagePath = match.captured(1);
    } else {
        outStyle.backgroundImagePath.clear();
    }

    m_styleCache[stylePath] = outStyle;
    return true;
}

LicensePlateResult LicensePlatesManager::generatePlate(const PlateStyle& style, const QString& text) {
    LicensePlateResult result;

    QString formattedText = formatTextLua(text, style);
    QColor finalTextColor = getTextColorLua(formattedText, style);

    PlateGenerationParams params;
    params.text = formattedText;
    params.textColor = finalTextColor;
    params.backgroundColor = style.backgroundColor;
    params.borderColor = style.borderColor;
    params.borderWidth = style.borderWidth;
    params.width = style.size.width();
    params.height = style.size.height();
    params.fontFamily = style.textFont.family();
    params.fontSize = style.textFont.pointSize();
    params.isReflective = style.isReflective;

    result.texture = renderPlateImage(params);
    result.width = params.width;
    result.height = params.height;

    if (!style.backgroundImagePath.isEmpty()) {
        QImage bgImage(style.backgroundImagePath);
        if (!bgImage.isNull()) {
            QPainter painter(&result.texture);
            painter.drawImage(style.backgroundRect, bgImage.scaled(style.backgroundRect.size()));
            painter.end();
        }
    }

    QPainter painter(&result.texture);
    painter.setFont(style.textFont);
    painter.setPen(finalTextColor);
    painter.drawText(style.textRect, Qt::AlignCenter, formattedText);
    painter.end();

    applyPostProcessing(result.texture, style.isReflective);

    result.success = true;
    return result;
}

LicensePlateResult LicensePlatesManager::generatePlateSimple(const PlateGenerationParams& params) {
    LicensePlateResult result;
    result.texture = renderPlateImage(params);
    result.width = params.width;
    result.height = params.height;
    result.success = true;
    return result;
}

std::vector<LicensePlateResult> LicensePlatesManager::generatePlatesBatch(
    const std::vector<QString>& texts,
    const PlateGenerationParams& baseParams)
{
    std::vector<LicensePlateResult> results;
    results.reserve(texts.size());

    for (const QString& text : texts) {
        PlateGenerationParams params = baseParams;
        params.text = text;
        results.push_back(generatePlateSimple(params));
    }

    return results;
}

QImage LicensePlatesManager::createAtlas(const std::vector<LicensePlateResult>& plates, int maxWidth) {
    if (plates.empty()) return QImage();

    int cols = std::ceil(std::sqrt(plates.size()));
    int rows = std::ceil((float)plates.size() / cols);

    int maxPlateWidth = 0, maxPlateHeight = 0;
    for (const auto& plate : plates) {
        maxPlateWidth = std::max(maxPlateWidth, plate.width);
        maxPlateHeight = std::max(maxPlateHeight, plate.height);
    }

    int atlasWidth = std::min(cols * maxPlateWidth, maxWidth);
    int atlasHeight = rows * maxPlateHeight;

    QImage atlas(atlasWidth, atlasHeight, QImage::Format_RGBA8888);
    atlas.fill(Qt::black);

    QPainter painter(&atlas);
    int x = 0, y = 0;
    for (const auto& plate : plates) {
        painter.drawImage(x, y, plate.texture);
        x += maxPlateWidth;
        if (x + maxPlateWidth > atlasWidth) {
            x = 0;
            y += maxPlateHeight;
        }
    }
    painter.end();

    return atlas;
}

bool LicensePlatesManager::savePlateTexture(const LicensePlateResult& plate, const QString& outputPath) {
    if (plate.texture.isNull()) return false;
    QString lower = outputPath.toLower();
    if (lower.endsWith(".dds")) return saveAsDDS(plate, outputPath);
    if (lower.endsWith(".tga")) return plate.texture.save(outputPath, "TGA");
    return plate.texture.save(outputPath);
}

bool LicensePlatesManager::saveAsDDS(const LicensePlateResult& plate, const QString& outputPath) {
    QImage img = plate.texture.convertToFormat(QImage::Format_RGBA8888);
    if (img.isNull()) return false;

    int w = img.width(), h = img.height();
    int pitch = ((w * 32 + 31) / 32) * 4;
    int size = pitch * h;

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly)) return false;

    // DDS header (DXT1-style RGBA)
    struct {
        char magic[4] = {'D', 'D', 'S', ' '};
        quint32 size = 124;
        quint32 flags = 0x00021007;
        quint32 height;
        quint32 width;
        quint32 pitchOrSize;
        quint32 depth = 0;
        quint32 mipMapCount = 0;
        quint32 reserved[11] = {};
        struct {
            quint32 size = 32;
            quint32 flags = 0x00000001 | 0x00000002 | 0x00000004 | 0x00000040;
            char fourCC[4] = {};
            quint32 rgbBitCount = 32;
            quint32 rMask = 0x00FF0000;
            quint32 gMask = 0x0000FF00;
            quint32 bMask = 0x000000FF;
            quint32 aMask = 0xFF000000;
        } pixelFormat;
        struct {
            quint32 caps1 = 0x00001000;
            quint32 caps2 = 0;
            quint32 caps3 = 0;
            quint32 caps4 = 0;
        } caps;
        quint32 reserved2 = 0;
    } ddsHeader;

    ddsHeader.width = static_cast<quint32>(w);
    ddsHeader.height = static_cast<quint32>(h);
    ddsHeader.pitchOrSize = static_cast<quint32>(pitch);

    file.write(reinterpret_cast<const char*>(&ddsHeader), sizeof(ddsHeader));

    // Write pixel data (BGRA byte order for DDS)
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            QRgb px = img.pixel(x, y);
            quint8 bgra[4] = {
                static_cast<quint8>(qBlue(px)),
                static_cast<quint8>(qGreen(px)),
                static_cast<quint8>(qRed(px)),
                static_cast<quint8>(qAlpha(px))
            };
            file.write(reinterpret_cast<const char*>(bgra), 4);
        }
    }

    file.close();
    return true;
}

QImage LicensePlatesManager::renderPlateImage(const PlateGenerationParams& params) {
    QImage image(params.width, params.height, QImage::Format_RGBA8888);

    // Background type
    if (params.backgroundType == 0) {
        image.fill(params.backgroundColor);
    } else if (params.backgroundType == 1) {
        // Gradient
        image.fill(params.backgroundColor);
        QPainter gradPainter(&image);
        QLinearGradient gradient(0, 0, params.width, params.height);
        gradient.setColorAt(0.0, params.backgroundColor);
        gradient.setColorAt(1.0, params.gradientColor);
        gradPainter.fillRect(0, 0, params.width, params.height, gradient);
        gradPainter.end();
    } else if (params.backgroundType == 2 && !params.backgroundTexturePath.isEmpty()) {
        QImage texture(params.backgroundTexturePath);
        if (!texture.isNull()) {
            image = texture.scaled(params.width, params.height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        } else {
            image.fill(params.backgroundColor);
        }
    }

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    // Border with rounded corners
    if (params.borderWidth > 0) {
        QPen pen(params.borderColor, params.borderWidth);
        painter.setPen(pen);
        if (params.cornerRadius > 0) {
            painter.drawRoundedRect(QRectF(0, 0, params.width, params.height),
                                    params.cornerRadius, params.cornerRadius);
        } else {
            painter.drawRect(0, 0, params.width, params.height);
        }
    }

    // Clip to rounded rect for text
    if (params.cornerRadius > 0) {
        QPainterPath clipPath;
        clipPath.addRoundedRect(QRectF(0, 0, params.width, params.height),
                                params.cornerRadius, params.cornerRadius);
        painter.setClipPath(clipPath);
    }

    // Draw EU band / country band (simplified)
    QFont bandFont("Arial", 10);
    painter.setFont(bandFont);
    painter.setPen(QColor(0, 0, 200));
    painter.drawText(4, 4, params.width, 14, Qt::AlignLeft | Qt::AlignTop,
                     params.countryCode);

    QFont textFont(params.fontFamily, params.fontSize);
    if (params.fontBold) textFont.setBold(true);
    painter.setFont(textFont);
    painter.setPen(params.textColor);

    int flags = Qt::AlignVCenter;
    if (params.textAlignment == 0) flags |= Qt::AlignHCenter;
    else if (params.textAlignment == 1) flags |= Qt::AlignLeft;
    else flags |= Qt::AlignRight;

    QRect textRect(20, 16, params.width - 40, params.height - 24);
    painter.drawText(textRect, flags, params.text);

    painter.end();

    // Holographic effect
    if (params.holographicEnabled) {
        m_holoEffect.enabled = true;
        applyHolographicEffect(image);
        m_holoEffect.enabled = false;
    }

    return image;
}

void LicensePlatesManager::applyPostProcessing(QImage& image, bool isReflective) {
    if (isReflective) {
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                QRgb pixel = image.pixel(x, y);
                int r = qRed(pixel);
                int g = qGreen(pixel);
                int b = qBlue(pixel);
                r = std::min(255, (int)(r * 1.2f));
                g = std::min(255, (int)(g * 1.2f));
                b = std::min(255, (int)(b * 1.3f));
                image.setPixel(x, y, qRgb(r, g, b));
            }
        }
    }

    int noiseAmount = 10;
    for (int x = 0; x < image.width(); x++) {
        for (int y = 0; y < image.height(); y++) {
            if ((x * 7 + y * 31) % 100 < noiseAmount) {
                QColor pixel = image.pixelColor(x, y);
                int noise = (rand() % 21) - 10;
                pixel.setRed(qBound(0, pixel.red() + noise, 255));
                pixel.setGreen(qBound(0, pixel.green() + noise, 255));
                pixel.setBlue(qBound(0, pixel.blue() + noise, 255));
                image.setPixelColor(x, y, pixel);
            }
        }
    }
}

QString LicensePlatesManager::formatTextLua(const QString& text, const PlateStyle& style) {
    QString formatted = text.toUpper();

    static const QMap<QString, std::function<QString(QString)>> formatters = {
        {"IT", [](QString t) { return t.left(2) + " " + t.mid(2, 3) + " " + t.mid(5); }},
        {"DE", [](QString t) { return t.left(1) + "-" + t.mid(1, 2) + "-" + t.mid(3); }},
        {"UK", [](QString t) { return t.left(2) + " " + t.mid(2); }},
    };

    auto it = formatters.find(style.countryCode);
    if (it != formatters.end()) {
        formatted = it.value()(formatted);
    }

    return formatted;
}

QColor LicensePlatesManager::getTextColorLua(const QString& text, const PlateStyle& style) {
    QColor base = style.textColor;

    if (text.length() > 7) {
        base = base.darker(100 + (text.length() - 7) * 5);
    }

    if (style.countryCode == "DE") {
        base = base.lighter(110);
    }

    return base;
}

// ═══════════════════════════════════════════════════════════════════════
// PlatePreviewWidget Implementation
// ═══════════════════════════════════════════════════════════════════════

PlatePreviewWidget::PlatePreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(200, 80);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setBackgroundRole(QPalette::Dark);
}

void PlatePreviewWidget::setImage(const QImage& image) {
    m_image = image;
    update();
}

void PlatePreviewWidget::setPlateText(const QString& text) {
    m_plateText = text;
    update();
}

void PlatePreviewWidget::setPlateStyle(const QString& styleName) {
    m_plateStyle = styleName;
    update();
}

void PlatePreviewWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setClipRect(event->region().boundingRect());

    painter.fillRect(rect(), QColor(30, 30, 35));

    if (!m_image.isNull()) {
        int pw = width();
        int ph = height();
        int iw = m_image.width();
        int ih = m_image.height();

        float scale = qMin((float)pw / iw, (float)ph / ih);
        int dw = qRound(iw * scale);
        int dh = qRound(ih * scale);
        int dx = (pw - dw) / 2;
        int dy = (ph - dh) / 2;

        QImage scaled = m_image.scaled(dw, dh, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter.drawImage(dx, dy, scaled);

        if (!m_plateText.isEmpty()) {
            painter.setPen(QColor(255, 220, 100));
            painter.setFont(QFont("Segoe UI", 10));
            painter.drawText(5, height() - 8, m_plateText);
        }
        if (!m_plateStyle.isEmpty()) {
            painter.setPen(QColor(150, 150, 150));
            painter.setFont(QFont("Segoe UI", 9));
            painter.drawText(width() - painter.fontMetrics().horizontalAdvance(m_plateStyle) - 5, height() - 8, m_plateStyle);
        }
    } else {
        painter.setPen(QColor(80, 80, 80));
        painter.drawRect(rect().adjusted(2, 2, -2, -2));
        painter.setPen(QColor(100, 100, 100));
        painter.setFont(QFont("Segoe UI", 14));
        painter.drawText(rect(), Qt::AlignCenter, "Preview");
    }
}

// ═══════════════════════════════════════════════════════════════════════
// LicensePlateEditorModule Implementation
// ═══════════════════════════════════════════════════════════════════════

LicensePlateEditorModule::LicensePlateEditorModule(QWidget* parent)
    : EditorModule(parent)
    , m_centralWidget(nullptr)
    , m_dockWidget(nullptr)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_previewPanel(nullptr)
    , m_previewWidget(nullptr)
    , m_paramsPanel(nullptr)
    , m_paramsScrollArea(nullptr)
    , m_textEdit(nullptr)
    , m_countryCombo(nullptr)
    , m_styleCombo(nullptr)
    , m_textColorBtn(nullptr)
    , m_bgColorBtn(nullptr)
    , m_borderColorBtn(nullptr)
    , m_textColorLabel(nullptr)
    , m_bgColorLabel(nullptr)
    , m_borderColorLabel(nullptr)
    , m_widthSpin(nullptr)
    , m_heightSpin(nullptr)
    , m_fontSizeSpin(nullptr)
    , m_borderWidthSpin(nullptr)
    , m_borderRadiusSpin(nullptr)
    , m_reflectiveCheck(nullptr)
    , m_uppercaseCheck(nullptr)
    , m_autoFormatCheck(nullptr)
    , m_fontCombo(nullptr)
    , m_textAlignCombo(nullptr)
    , m_backgroundTypeCombo(nullptr)
    , m_presetList(nullptr)
    , m_savePresetBtn(nullptr)
    , m_loadPresetBtn(nullptr)
    , m_deletePresetBtn(nullptr)
    , m_generateBtn(nullptr)
    , m_exportBtn(nullptr)
    , m_exportBatchBtn(nullptr)
    , m_copyBtn(nullptr)
    , m_statusLabel(nullptr)
    , m_infoLabel(nullptr)
    , m_manager(new LicensePlatesManager())
{
    m_currentParams.text = "AB-123-CD";
    m_currentParams.textColor = Qt::black;
    m_currentParams.backgroundColor = Qt::white;
    m_currentParams.borderColor = Qt::black;
    m_currentParams.borderWidth = 2.0f;
    m_currentParams.width = 512;
    m_currentParams.height = 128;
    m_currentParams.fontFamily = "Arial";
    m_currentParams.fontSize = 48;
    m_currentParams.isReflective = false;

    setupUi();
    setupConnections();
    populateCountryCodes();
    populateStyleList();
    populatePresetList();
    updatePreview();
}

LicensePlateEditorModule::~LicensePlateEditorModule() {
}

QDockWidget* LicensePlateEditorModule::getOrCreateDockWidget(QMainWindow* mainWindow) {
    if (!m_dockWidget) {
        m_dockWidget = new QDockWidget("License Plate Editor", mainWindow);
        m_dockWidget->setWidget(m_centralWidget);
        m_dockWidget->setAllowedAreas(Qt::AllDockWidgetAreas);
    }
    return m_dockWidget;
}

void LicensePlateEditorModule::setupUi() {
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    m_centralWidget = new QWidget(this);
    outerLayout->addWidget(m_centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, m_centralWidget);
    mainLayout->addWidget(splitter);

    splitter->addWidget(createLeftPanel());
    splitter->addWidget(createRightPanel());
    splitter->setSizes({300, 600});
}

QWidget* LicensePlateEditorModule::createLeftPanel() {
    QWidget* panel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);

    QGroupBox* textGroup = new QGroupBox("Plate Text");
    QFormLayout* textForm = new QFormLayout();

    m_textEdit = new QLineEdit(m_currentParams.text);
    m_textEdit->setPlaceholderText("e.g., AB-123-CD");
    textForm->addRow("Text:", m_textEdit);

    m_uppercaseCheck = new QCheckBox("Uppercase");
    m_uppercaseCheck->setChecked(true);
    textForm->addRow("", m_uppercaseCheck);

    m_autoFormatCheck = new QCheckBox("Auto-format");
    m_autoFormatCheck->setChecked(true);
    textForm->addRow("", m_autoFormatCheck);

    textGroup->setLayout(textForm);
    layout->addWidget(textGroup);

    QGroupBox* styleGroup = new QGroupBox("Style");
    QFormLayout* styleForm = new QFormLayout();

    m_countryCombo = new QComboBox();
    m_countryCombo->setEditable(true);
    styleForm->addRow("Country:", m_countryCombo);

    m_styleCombo = new QComboBox();
    m_styleCombo->setEditable(true);
    styleForm->addRow("Template:", m_styleCombo);

    styleGroup->setLayout(styleForm);
    layout->addWidget(styleGroup);

    QGroupBox* colorGroup = new QGroupBox("Colors");
    QFormLayout* colorForm = new QFormLayout();

    QHBoxLayout* textColorLayout = new QHBoxLayout();
    m_textColorBtn = new QPushButton();
    m_textColorBtn->setFixedSize(50, 24);
    m_textColorBtn->setStyleSheet("background-color: black; border: 1px solid #555;");
    m_textColorLabel = new QLabel("#000000");
    textColorLayout->addWidget(m_textColorBtn);
    textColorLayout->addWidget(m_textColorLabel);
    textColorLayout->addStretch();
    colorForm->addRow("Text:", textColorLayout);

    QHBoxLayout* bgColorLayout = new QHBoxLayout();
    m_bgColorBtn = new QPushButton();
    m_bgColorBtn->setFixedSize(50, 24);
    m_bgColorBtn->setStyleSheet("background-color: white; border: 1px solid #555;");
    m_bgColorLabel = new QLabel("#FFFFFF");
    bgColorLayout->addWidget(m_bgColorBtn);
    bgColorLayout->addWidget(m_bgColorLabel);
    bgColorLayout->addStretch();
    colorForm->addRow("Background:", bgColorLayout);

    QHBoxLayout* borderColorLayout = new QHBoxLayout();
    m_borderColorBtn = new QPushButton();
    m_borderColorBtn->setFixedSize(50, 24);
    m_borderColorBtn->setStyleSheet("background-color: black; border: 1px solid #555;");
    m_borderColorLabel = new QLabel("#000000");
    borderColorLayout->addWidget(m_borderColorBtn);
    borderColorLayout->addWidget(m_borderColorLabel);
    borderColorLayout->addStretch();
    colorForm->addRow("Border:", borderColorLayout);

    colorGroup->setLayout(colorForm);
    layout->addWidget(colorGroup);

    QGroupBox* dimGroup = new QGroupBox("Dimensions");
    QFormLayout* dimForm = new QFormLayout();

    m_widthSpin = new QSpinBox();
    m_widthSpin->setRange(64, 2048);
    m_widthSpin->setSuffix(" px");
    m_widthSpin->setValue(m_currentParams.width);
    dimForm->addRow("Width:", m_widthSpin);

    m_heightSpin = new QSpinBox();
    m_heightSpin->setRange(32, 512);
    m_heightSpin->setSuffix(" px");
    m_heightSpin->setValue(m_currentParams.height);
    dimForm->addRow("Height:", m_heightSpin);

    m_borderWidthSpin = new QSpinBox();
    m_borderWidthSpin->setRange(0, 20);
    m_borderWidthSpin->setSuffix(" px");
    m_borderWidthSpin->setValue(static_cast<int>(m_currentParams.borderWidth));
    dimForm->addRow("Border Width:", m_borderWidthSpin);

    m_borderRadiusSpin = new QDoubleSpinBox();
    m_borderRadiusSpin->setRange(0, 50);
    m_borderRadiusSpin->setSuffix(" px");
    m_borderRadiusSpin->setDecimals(1);
    m_borderRadiusSpin->setValue(0);
    dimForm->addRow("Corner Radius:", m_borderRadiusSpin);

    dimGroup->setLayout(dimForm);
    layout->addWidget(dimGroup);

    QGroupBox* fontGroup = new QGroupBox("Typography");
    QFormLayout* fontForm = new QFormLayout();

    m_fontCombo = new QComboBox();
    m_fontCombo->addItems({"Arial", "Helvetica", "Times New Roman", "Courier New", "Verdana", "Tahoma", "Segoe UI", "Roboto"});
    m_fontCombo->setCurrentText(m_currentParams.fontFamily);
    fontForm->addRow("Font:", m_fontCombo);

    m_fontSizeSpin = new QSpinBox();
    m_fontSizeSpin->setRange(8, 200);
    m_fontSizeSpin->setSuffix(" pt");
    m_fontSizeSpin->setValue(m_currentParams.fontSize);
    fontForm->addRow("Size:", m_fontSizeSpin);

    m_textAlignCombo = new QComboBox();
    m_textAlignCombo->addItems({"Center", "Left", "Right"});
    fontForm->addRow("Align:", m_textAlignCombo);

    fontGroup->setLayout(fontForm);
    layout->addWidget(fontGroup);

    QGroupBox* effectGroup = new QGroupBox("Effects");
    QFormLayout* effectForm = new QFormLayout();

    m_reflectiveCheck = new QCheckBox("Reflective coating");
    effectForm->addRow("", m_reflectiveCheck);

    m_backgroundTypeCombo = new QComboBox();
    m_backgroundTypeCombo->addItems({"Solid", "Gradient", "Texture"});
    effectForm->addRow("Background:", m_backgroundTypeCombo);

    effectGroup->setLayout(effectForm);
    layout->addWidget(effectGroup);

    QGroupBox* presetGroup = new QGroupBox("Presets");
    QVBoxLayout* presetLayout = new QVBoxLayout();

    m_presetList = new QListWidget();
    m_presetList->setAlternatingRowColors(true);
    m_presetList->setMaximumHeight(120);
    presetLayout->addWidget(m_presetList);

    QHBoxLayout* presetBtnLayout = new QHBoxLayout();
    m_savePresetBtn = new QPushButton("Save");
    m_deletePresetBtn = new QPushButton("Delete");
    presetBtnLayout->addWidget(m_savePresetBtn);
    presetBtnLayout->addWidget(m_deletePresetBtn);
    presetLayout->addLayout(presetBtnLayout);

    presetGroup->setLayout(presetLayout);
    layout->addWidget(presetGroup);

    layout->addStretch();
    return panel;
}

QWidget* LicensePlateEditorModule::createRightPanel() {
    QWidget* panel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);

    m_previewWidget = new PlatePreviewWidget();
    m_previewWidget->setMinimumHeight(150);
    layout->addWidget(m_previewWidget, 1);

    QHBoxLayout* scaleLayout = new QHBoxLayout();
    scaleLayout->addWidget(new QLabel("Scale:"));
    m_previewScaleSlider = new QSlider(Qt::Horizontal);
    m_previewScaleSlider->setRange(25, 400);
    m_previewScaleSlider->setValue(100);
    m_previewScaleSlider->setTickPosition(QSlider::TicksBelow);
    m_previewScaleSlider->setTickInterval(25);
    m_previewScaleLabel = new QLabel("100%");
    scaleLayout->addWidget(m_previewScaleSlider);
    scaleLayout->addWidget(m_previewScaleLabel);
    layout->addLayout(scaleLayout);

    QFrame* sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #444;");
    layout->addWidget(sep);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_generateBtn = new QPushButton("Generate");
    m_generateBtn->setDefault(true);
    m_copyBtn = new QPushButton("Copy");
    m_exportBtn = new QPushButton("Export PNG");
    m_exportBatchBtn = new QPushButton("Batch Export");
    btnLayout->addWidget(m_generateBtn);
    btnLayout->addWidget(m_copyBtn);
    btnLayout->addWidget(m_exportBtn);
    btnLayout->addWidget(m_exportBatchBtn);
    layout->addLayout(btnLayout);

    m_infoLabel = new QLabel();
    m_infoLabel->setStyleSheet("color: #888; font-size: 11px;");
    m_infoLabel->setWordWrap(true);
    layout->addWidget(m_infoLabel);

    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setStyleSheet("color: #aaa; padding: 4px;");
    layout->addWidget(m_statusLabel);

    return panel;
}

void LicensePlateEditorModule::setupConnections() {
    connect(m_textEdit, &QLineEdit::textChanged, this, &LicensePlateEditorModule::onTextChanged);
    connect(m_countryCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, &LicensePlateEditorModule::onCountryChanged);
    connect(m_styleCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, &LicensePlateEditorModule::onStyleChanged);

    connect(m_textColorBtn, &QPushButton::clicked, this, &LicensePlateEditorModule::onTextColorClicked);
    connect(m_bgColorBtn, &QPushButton::clicked, this, &LicensePlateEditorModule::onBgColorClicked);
    connect(m_borderColorBtn, &QPushButton::clicked, this, &LicensePlateEditorModule::onBorderColorClicked);

    connect(m_widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &LicensePlateEditorModule::onWidthChanged);
    connect(m_heightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &LicensePlateEditorModule::onHeightChanged);
    connect(m_fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &LicensePlateEditorModule::onFontSizeChanged);
    connect(m_borderWidthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &LicensePlateEditorModule::onWidthChanged);

    connect(m_reflectiveCheck, &QCheckBox::toggled, this, &LicensePlateEditorModule::onReflectiveToggled);
    connect(m_uppercaseCheck, &QCheckBox::toggled, this, &LicensePlateEditorModule::onTextChanged);

    connect(m_previewScaleSlider, &QSlider::valueChanged, this, [this](int val) {
        m_previewScaleLabel->setText(QString::number(val) + "%");
        updatePreview();
    });

    connect(m_presetList, &QListWidget::itemClicked, this, &LicensePlateEditorModule::onPresetSelected);
    connect(m_savePresetBtn, &QPushButton::clicked, this, &LicensePlateEditorModule::onSavePreset);
    connect(m_deletePresetBtn, &QPushButton::clicked, this, &LicensePlateEditorModule::onDeletePreset);

    connect(m_generateBtn, &QPushButton::clicked, this, &LicensePlateEditorModule::onGenerate);
    connect(m_exportBtn, &QPushButton::clicked, this, &LicensePlateEditorModule::onExport);
    connect(m_exportBatchBtn, &QPushButton::clicked, this, &LicensePlateEditorModule::onExportBatch);
    connect(m_copyBtn, &QPushButton::clicked, this, &LicensePlateEditorModule::onCopyToClipboard);
}

void LicensePlateEditorModule::populateCountryCodes() {
    m_countryCombo->addItem("Germany (D)");
    m_countryCombo->addItem("Italy (I)");
    m_countryCombo->addItem("France (F)");
    m_countryCombo->addItem("UK (GB)");
    m_countryCombo->addItem("Spain (E)");
    m_countryCombo->addItem("Netherlands (NL)");
    m_countryCombo->addItem("Belgium (B)");
    m_countryCombo->addItem("Austria (A)");
    m_countryCombo->addItem("Switzerland (CH)");
    m_countryCombo->addItem("USA (USA)");
    m_countryCombo->addItem("Japan (J)");
    m_countryCombo->addItem("Australia (AUS)");
    m_countryCombo->addItem("Brazil (BR)");
    m_countryCombo->addItem("Custom");
    m_countryCombo->setCurrentIndex(0);
}

void LicensePlateEditorModule::populateStyleList() {
    m_styleCombo->addItem("Standard EU");
    m_styleCombo->addItem("Standard US");
    m_styleCombo->addItem("UK Rectangle");
    m_styleCombo->addItem("US Motorcycle");
    m_styleCombo->addItem("Vintage");
    m_styleCombo->addItem("Racing");
    m_styleCombo->addItem("Police");
    m_styleCombo->addItem("Taxi");
    m_styleCombo->addItem("Custom");
    m_styleCombo->setCurrentIndex(0);
}

void LicensePlateEditorModule::populatePresetList() {
    m_presetList->clear();
    m_presetList->addItem("DE - Standard White");
    m_presetList->addItem("DE - Yellow Taxi");
    m_presetList->addItem("IT - Standard Blue");
    m_presetList->addItem("UK - Yellow Background");
    m_presetList->addItem("USA - California");
    m_presetList->addItem("Racing - Carbon Fiber");
    m_presetList->addItem("Police - Black/White");
    m_presetList->addItem("Vintage - Cream");
}

void LicensePlateEditorModule::onTextChanged() {
    QString text = m_textEdit->text();
    if (m_uppercaseCheck->isChecked()) {
        text = text.toUpper();
        if (m_textEdit->text() != text) {
            m_textEdit->blockSignals(true);
            m_textEdit->setText(text);
            m_textEdit->blockSignals(false);
        }
    }
    m_currentParams.text = text;
    updatePreview();
    updateInfoLabel();
}

void LicensePlateEditorModule::onCountryChanged(const QString& country) {
    applyCountryStyle(country);
    updatePreview();
    updateInfoLabel();
}

void LicensePlateEditorModule::onStyleChanged(const QString& style) {
    if (style == "Standard") {
        m_currentParams.fontSize = 42;
        m_currentParams.fontBold = true;
    } else if (style == "Vintage") {
        m_currentParams.fontSize = 38;
        m_currentParams.fontBold = false;
    } else if (style == "Racing") {
        m_currentParams.fontSize = 48;
        m_currentParams.fontBold = true;
    } else if (style == "Compact") {
        m_currentParams.fontSize = 36;
        m_currentParams.fontBold = true;
    }
    updatePreview();
}

void LicensePlateEditorModule::onTextColorClicked() {
    QColor color = QColorDialog::getColor(m_currentParams.textColor, this, "Select Text Color");
    if (color.isValid()) {
        m_currentParams.textColor = color;
        m_textColorBtn->setStyleSheet("background-color: " + color.name() + "; border: 1px solid #555;");
        m_textColorLabel->setText(color.name().toUpper());
        updatePreview();
    }
}

void LicensePlateEditorModule::onBgColorClicked() {
    QColor color = QColorDialog::getColor(m_currentParams.backgroundColor, this, "Select Background Color");
    if (color.isValid()) {
        m_currentParams.backgroundColor = color;
        m_bgColorBtn->setStyleSheet("background-color: " + color.name() + "; border: 1px solid #555;");
        m_bgColorLabel->setText(color.name().toUpper());
        updatePreview();
    }
}

void LicensePlateEditorModule::onBorderColorClicked() {
    QColor color = QColorDialog::getColor(m_currentParams.borderColor, this, "Select Border Color");
    if (color.isValid()) {
        m_currentParams.borderColor = color;
        m_borderColorBtn->setStyleSheet("background-color: " + color.name() + "; border: 1px solid #555;");
        m_borderColorLabel->setText(color.name().toUpper());
        updatePreview();
    }
}

void LicensePlateEditorModule::onWidthChanged() {
    m_currentParams.width = m_widthSpin->value();
    m_currentParams.borderWidth = static_cast<float>(m_borderWidthSpin->value());
    updatePreview();
    updateInfoLabel();
}

void LicensePlateEditorModule::onHeightChanged() {
    m_currentParams.height = m_heightSpin->value();
    updatePreview();
    updateInfoLabel();
}

void LicensePlateEditorModule::onFontSizeChanged() {
    m_currentParams.fontSize = m_fontSizeSpin->value();
    updatePreview();
}

void LicensePlateEditorModule::onReflectiveToggled(bool checked) {
    m_currentParams.isReflective = checked;
    updatePreview();
}

void LicensePlateEditorModule::onGenerate() {
    m_currentParams.text = m_textEdit->text();
    m_currentParams.width = m_widthSpin->value();
    m_currentParams.height = m_heightSpin->value();
    m_currentParams.borderWidth = static_cast<float>(m_borderWidthSpin->value());
    m_currentParams.fontFamily = m_fontCombo->currentText();
    m_currentParams.fontSize = m_fontSizeSpin->value();
    m_currentParams.isReflective = m_reflectiveCheck->isChecked();

    LicensePlateResult result = m_manager->generatePlateSimple(m_currentParams);
    if (result.success) {
        m_generatedImage = result.texture;
        m_previewWidget->setImage(m_generatedImage);
        m_previewWidget->setPlateText(m_currentParams.text);
        m_statusLabel->setText("Generated: " + QString::number(result.width) + "x" + QString::number(result.height) + " px");
    } else {
        m_statusLabel->setText("Error: " + result.errorMessage);
    }
}

void LicensePlateEditorModule::onExport() {
    if (m_generatedImage.isNull()) {
        onGenerate();
    }
    if (m_generatedImage.isNull()) return;

    if (m_lastExportDir.isEmpty()) {
        m_lastExportDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }
    QString path = QFileDialog::getSaveFileName(this, "Export License Plate",
        m_lastExportDir + "/plate_" + m_textEdit->text().simplified().replace(" ", "_") + ".png",
        "PNG Image (*.png);;JPEG Image (*.jpg)");
    if (path.isEmpty()) return;

    m_lastExportDir = QFileInfo(path).absolutePath();
    if (m_generatedImage.save(path)) {
        m_statusLabel->setText("Exported: " + QFileInfo(path).fileName());
    } else {
        QMessageBox::critical(this, "Export Error", "Failed to save image.");
    }
}

void LicensePlateEditorModule::onExportBatch() {
    bool ok;
    int count = QInputDialog::getInt(this, "Batch Export", "Number of plates:", 10, 1, 999, 1, &ok);
    if (!ok) return;

    QString prefix = QInputDialog::getText(this, "Batch Export", "Plate prefix:", QLineEdit::Normal, "PLATE", &ok);
    if (!ok) return;

    QString dir = QFileDialog::getExistingDirectory(this, "Select Output Directory",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    if (dir.isEmpty()) return;

    m_statusLabel->setText("Generating " + QString::number(count) + " plates...");
    QApplication::processEvents();

    for (int i = 0; i < count; i++) {
        PlateGenerationParams params = m_currentParams;
        params.text = QString("%1-%2").arg(prefix).arg(i + 1, 3, 10, QChar('0'));
        LicensePlateResult result = m_manager->generatePlateSimple(params);
        if (result.success) {
            QString path = dir + QString("/plate_%1_%2.png").arg(prefix).arg(i + 1, 3, 10, QChar('0'));
            result.texture.save(path);
        }
    }

    m_statusLabel->setText(QString("Batch export complete: %1 plates to %2").arg(count).arg(dir));
}

void LicensePlateEditorModule::onSavePreset() {
    bool ok;
    QString name = QInputDialog::getText(this, "Save Preset", "Preset name:", QLineEdit::Normal, "My Preset", &ok);
    if (!ok || name.isEmpty()) return;

    PlatePreset preset;
    preset.name = name;
    preset.countryCode = m_countryCombo->currentText();
    preset.params = saveParameters();
    m_presets.append(preset);

    m_presetList->addItem(name);
    m_statusLabel->setText("Preset saved: " + name);
}

void LicensePlateEditorModule::onLoadPreset() {
    QString path = QFileDialog::getOpenFileName(this, "Load Preset", QString(),
        "Preset Files (*.ksplate *.json);;All Files (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) return;

    QJsonObject obj = doc.object();
    PlatePreset preset;
    preset.name = obj["name"].toString();
    preset.countryCode = obj["countryCode"].toString();
    preset.params.text = obj["text"].toString();
    preset.params.width = obj["width"].toInt(520);
    preset.params.height = obj["height"].toInt(110);
    preset.params.fontSize = obj["fontSize"].toInt(72);
    preset.params.borderWidth = static_cast<float>(obj["borderWidth"].toDouble(3.0));
    preset.params.isReflective = obj["isReflective"].toBool(false);
    if (obj.contains("textColor")) {
        preset.params.textColor = QColor(obj["textColor"].toString());
    }
    if (obj.contains("backgroundColor")) {
        preset.params.backgroundColor = QColor(obj["backgroundColor"].toString());
    }
    if (obj.contains("borderColor")) {
        preset.params.borderColor = QColor(obj["borderColor"].toString());
    }
    preset.params.fontFamily = obj["fontFamily"].toString();
    loadParameters(preset.params);
    m_currentParams = preset.params;
    m_presets.append(preset);
    m_presetList->addItem(preset.name);
    m_statusLabel->setText("Preset loaded: " + preset.name);
}

void LicensePlateEditorModule::onDeletePreset() {
    QListWidgetItem* item = m_presetList->currentItem();
    if (!item) return;

    int row = m_presetList->row(item);
    if (row >= 0 && row < m_presets.size()) {
        m_presets.removeAt(row);
    }
    delete item;
    m_statusLabel->setText("Preset deleted");
}

void LicensePlateEditorModule::onPresetSelected(QListWidgetItem* item) {
    if (!item) return;
    int row = m_presetList->row(item);
    if (row >= 0 && row < m_presets.size()) {
        PlatePreset& preset = m_presets[row];
        loadParameters(preset.params);
        updatePreview();
        m_statusLabel->setText("Loaded preset: " + preset.name);
    }
}

void LicensePlateEditorModule::updatePreview() {
    PlateGenerationParams params = saveParameters();
    LicensePlateResult result = m_manager->generatePlateSimple(params);
    if (result.success) {
        m_previewWidget->setImage(result.texture);
        m_previewWidget->setPlateText(params.text);
        m_previewWidget->setPlateStyle(m_styleCombo->currentText());
    }
}

void LicensePlateEditorModule::updateInfoLabel() {
    QString info = QString("%1x%2 | %3 | Font: %4 %5pt")
        .arg(m_widthSpin->value())
        .arg(m_heightSpin->value())
        .arg(m_countryCombo->currentText())
        .arg(m_fontCombo->currentText())
        .arg(m_fontSizeSpin->value());
    m_infoLabel->setText(info);
}

void LicensePlateEditorModule::loadParameters(const PlateGenerationParams& params) {
    m_textEdit->setText(params.text);
    m_widthSpin->setValue(params.width);
    m_heightSpin->setValue(params.height);
    m_fontSizeSpin->setValue(params.fontSize);
    m_borderWidthSpin->setValue(static_cast<int>(params.borderWidth));
    m_reflectiveCheck->setChecked(params.isReflective);
    m_textColorBtn->setStyleSheet("background-color: " + params.textColor.name() + "; border: 1px solid #555;");
    m_textColorLabel->setText(params.textColor.name().toUpper());
    m_bgColorBtn->setStyleSheet("background-color: " + params.backgroundColor.name() + "; border: 1px solid #555;");
    m_bgColorLabel->setText(params.backgroundColor.name().toUpper());
    m_borderColorBtn->setStyleSheet("background-color: " + params.borderColor.name() + "; border: 1px solid #555;");
    m_borderColorLabel->setText(params.borderColor.name().toUpper());
}

PlateGenerationParams LicensePlateEditorModule::saveParameters() {
    PlateGenerationParams params;
    params.text = m_textEdit->text();
    params.textColor = m_currentParams.textColor;
    params.backgroundColor = m_currentParams.backgroundColor;
    params.borderColor = m_currentParams.borderColor;
    params.borderWidth = static_cast<float>(m_borderWidthSpin->value());
    params.width = m_widthSpin->value();
    params.height = m_heightSpin->value();
    params.fontFamily = m_fontCombo->currentText();
    params.fontSize = m_fontSizeSpin->value();
    params.isReflective = m_reflectiveCheck->isChecked();
    params.countryCode = m_countryCombo->currentText();
    return params;
}

void LicensePlateEditorModule::applyCountryStyle(const QString& country) {
    if (country.contains("Germany") || country.contains("D)")) {
        m_widthSpin->setValue(520);
        m_heightSpin->setValue(110);
        m_currentParams.backgroundColor = Qt::white;
        m_currentParams.textColor = Qt::black;
        m_bgColorBtn->setStyleSheet("background-color: #FFFFFF; border: 1px solid #555;");
        m_bgColorLabel->setText("#FFFFFF");
        m_textColorBtn->setStyleSheet("background-color: #000000; border: 1px solid #555;");
        m_textColorLabel->setText("#000000");
        m_currentParams.borderWidth = 2.0f;
        m_borderWidthSpin->setValue(2);
    } else if (country.contains("Italy") || country.contains("I)")) {
        m_widthSpin->setValue(480);
        m_heightSpin->setValue(100);
        m_currentParams.backgroundColor = QColor(0, 100, 180);
        m_currentParams.textColor = Qt::white;
        m_bgColorBtn->setStyleSheet("background-color: #0064B4; border: 1px solid #555;");
        m_bgColorLabel->setText("#0064B4");
        m_textColorBtn->setStyleSheet("background-color: #FFFFFF; border: 1px solid #555;");
        m_textColorLabel->setText("#FFFFFF");
    } else if (country.contains("UK") || country.contains("GB)")) {
        m_widthSpin->setValue(560);
        m_heightSpin->setValue(110);
        m_currentParams.backgroundColor = QColor(255, 215, 0);
        m_currentParams.textColor = QColor(0, 0, 0);
        m_bgColorBtn->setStyleSheet("background-color: #FFD700; border: 1px solid #555;");
        m_bgColorLabel->setText("#FFD700");
        m_textColorBtn->setStyleSheet("background-color: #000000; border: 1px solid #555;");
        m_textColorLabel->setText("#000000");
    } else if (country.contains("USA") || country.contains("US)")) {
        m_widthSpin->setValue(600);
        m_heightSpin->setValue(140);
        m_currentParams.backgroundColor = Qt::white;
        m_currentParams.textColor = QColor(0, 0, 0);
        m_bgColorBtn->setStyleSheet("background-color: #FFFFFF; border: 1px solid #555;");
        m_bgColorLabel->setText("#FFFFFF");
        m_textColorBtn->setStyleSheet("background-color: #000000; border: 1px solid #555;");
        m_textColorLabel->setText("#000000");
    } else if (country.contains("France") || country.contains("F)")) {
        m_widthSpin->setValue(480);
        m_heightSpin->setValue(100);
        m_currentParams.backgroundColor = Qt::white;
        m_currentParams.textColor = QColor(0, 0, 0);
        m_bgColorBtn->setStyleSheet("background-color: #FFFFFF; border: 1px solid #555;");
        m_bgColorLabel->setText("#FFFFFF");
        m_textColorBtn->setStyleSheet("background-color: #000000; border: 1px solid #555;");
        m_textColorLabel->setText("#000000");
        m_currentParams.isReflective = true;
        m_reflectiveCheck->setChecked(true);
    }
}

void LicensePlateEditorModule::onCopyToClipboard() {
    if (m_generatedImage.isNull()) {
        onGenerate();
    }
    if (!m_generatedImage.isNull()) {
        QApplication::clipboard()->setImage(m_generatedImage);
        m_statusLabel->setText("Copied to clipboard");
    }
}

void LicensePlateEditorModule::onActivation() {
}

void LicensePlateEditorModule::onDeactivation() {
}

void LicensePlateEditorModule::onPreviewScaleChanged(int value)
{
    if (m_previewWidget) {
        m_previewWidget->setFixedHeight(value * 2);
        m_previewWidget->update();
    }
}

void LicensePlateEditorModule::onImportFromCar()
{
    QString path = QFileDialog::getExistingDirectory(nullptr, tr("Select Car Folder"));
    if (path.isEmpty()) return;
    QString stylePath = path + "/data/plate_style.ini";
    if (QFile::exists(stylePath)) {
        PlateStyle style;
        if (m_manager->loadStyle(stylePath, style)) {
            m_loadedStyles[QFileInfo(path).fileName()] = style;
            populateStyleList();
        }
    }
}

void LicensePlateEditorModule::onNewStyle()
{
    bool ok = false;
    QString name = QInputDialog::getText(nullptr, tr("New Style"), tr("Style name:"),
        QLineEdit::Normal, QString(), &ok);
    if (!ok || name.isEmpty()) return;
    PlateStyle style;
    m_loadedStyles[name] = style;
    populateStyleList();
}

void LicensePlateEditorModule::onSaveStyle()
{
    if (m_styleCombo->currentText().isEmpty()) return;
    QString path = QFileDialog::getSaveFileName(nullptr, tr("Save Style"),
        m_currentStylePath, tr("INI Files (*.ini)"));
    if (path.isEmpty()) return;
    m_currentStylePath = path;
    QSettings s(path, QSettings::IniFormat);
    s.setValue("General/name", m_styleCombo->currentText());
}

} // namespace ks
