#include "LicensePlatesQmlBridge.h"
#include "LicensePlateEditorModule.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include "core/editor/EditorConfig.h"

namespace ks {
using paint::PaintSystem;

LicensePlatesQmlBridge* LicensePlatesQmlBridge::s_instance = nullptr;

LicensePlatesQmlBridge* LicensePlatesQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new LicensePlatesQmlBridge();
    }
    return s_instance;
}

void LicensePlatesQmlBridge::generatePlate() {
    LicensePlatesManager manager;

    PlateGenerationParams params;
    params.text = m_plateText;
    params.textColor = QColor(m_textColor.isEmpty() ? "#000000" : m_textColor);
    params.backgroundColor = QColor(m_bgColor.isEmpty() ? "#FFFFFF" : m_bgColor);
    params.borderColor = QColor(m_borderColor.isEmpty() ? "#000000" : m_borderColor);
    params.borderWidth = 2.0f;
    params.width = m_plateWidth;
    params.height = m_plateHeight;
    params.fontFamily = "Arial";
    params.fontSize = 48;
    params.countryCode = m_country;

    LicensePlateResult result = manager.generatePlateSimple(params);

    if (result.success) {
        m_presetCount++;
        emit presetCountChanged();
        emit plateGenerated("");
    }
}

QString LicensePlatesQmlBridge::exportPlate(const QString& path) {
    LicensePlatesManager manager;

    PlateGenerationParams params;
    params.text = m_plateText;
    params.textColor = QColor(m_textColor.isEmpty() ? "#000000" : m_textColor);
    params.backgroundColor = QColor(m_bgColor.isEmpty() ? "#FFFFFF" : m_bgColor);
    params.borderColor = QColor(m_borderColor.isEmpty() ? "#000000" : m_borderColor);
    params.borderWidth = 2.0f;
    params.width = m_plateWidth;
    params.height = m_plateHeight;
    params.fontFamily = "Arial";
    params.fontSize = 48;
    params.countryCode = m_country;

    LicensePlateResult result = manager.generatePlateSimple(params);

    if (result.success) {
        if (manager.savePlateTexture(result, path)) {
            return path;
        }
    }
    return "";
}

void LicensePlatesQmlBridge::exportBatch(const QString& directory, const QStringList& texts) {
    LicensePlatesManager manager;

    PlateGenerationParams params;
    params.textColor = QColor(m_textColor.isEmpty() ? "#000000" : m_textColor);
    params.backgroundColor = QColor(m_bgColor.isEmpty() ? "#FFFFFF" : m_bgColor);
    params.borderColor = QColor(m_borderColor.isEmpty() ? "#000000" : m_borderColor);
    params.borderWidth = 2.0f;
    params.width = m_plateWidth;
    params.height = m_plateHeight;
    params.fontFamily = "Arial";
    params.fontSize = 48;
    params.countryCode = m_country;

    std::vector<QString> textVec;
    for (const auto& t : texts) {
        textVec.push_back(t.toStdString().c_str());
    }

    auto results = manager.generatePlatesBatch(textVec, params);

    QDir dir(directory);
    dir.mkpath(".");

    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i].success) {
            QString path = directory + "/plate_" + QString::number(i) + ".png";
            manager.savePlateTexture(results[i], path);
        }
    }
}

void LicensePlatesQmlBridge::savePreset(const QString& name) {
    QJsonObject preset;
    preset["plateText"] = m_plateText;
    preset["country"] = m_country;
    preset["style"] = m_style;
    preset["textColor"] = m_textColor;
    preset["bgColor"] = m_bgColor;
    preset["borderColor"] = m_borderColor;
    preset["width"] = m_plateWidth;
    preset["height"] = m_plateHeight;

    QJsonDocument doc(preset);
    QString path = EditorConfig::instance().platesPath() + "/" + name + ".json";
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        m_presetCount++;
        emit presetCountChanged();
        emit presetSaved(name);
    }
}

void LicensePlatesQmlBridge::loadPreset(const QString& name) {
    QString path = EditorConfig::instance().platesPath() + "/" + name + ".json";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull() || !doc.isObject()) return;

    QJsonObject obj = doc.object();
    m_plateText = obj["plateText"].toString();
    m_country = obj["country"].toString();
    m_style = obj["style"].toString();
    m_textColor = obj["textColor"].toString();
    m_bgColor = obj["bgColor"].toString();
    m_borderColor = obj["borderColor"].toString();
    m_plateWidth = obj["width"].toInt(520);
    m_plateHeight = obj["height"].toInt(110);

    emit plateTextChanged();
    emit countryChanged();
    emit styleChanged();
    emit presetLoaded(name);
}

void LicensePlatesQmlBridge::deletePreset(const QString& name) {
    QString path = EditorConfig::instance().platesPath() + "/" + name + ".json";
    if (QFile::exists(path)) {
        QFile::remove(path);
        if (m_presetCount > 0) {
            m_presetCount--;
            emit presetCountChanged();
        }
        emit presetDeleted(name);
    }
}

QVariantList LicensePlatesQmlBridge::getPresets() {
    QVariantList presets;
    QString dirPath = EditorConfig::instance().platesPath();
    QDir dir(dirPath);

    if (!dir.exists()) return presets;

    QFileInfoList files = dir.entryInfoList(QStringList() << "*.json", QDir::Files);
    for (const auto& file : files) {
        QVariantMap p;
        p["name"] = file.baseName();
        p["path"] = file.absoluteFilePath();
        p["size"] = file.size();
        p["modified"] = file.lastModified().toString("yyyy-MM-dd");
        presets.append(p);
    }

    m_presetCount = presets.size();
    return presets;
}

QVariantMap LicensePlatesQmlBridge::getPreset(const QString& name) {
    QString path = EditorConfig::instance().platesPath() + "/" + name + ".json";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QVariantMap();

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull() || !doc.isObject()) return QVariantMap();

    return doc.object().toVariantMap();
}

QStringList LicensePlatesQmlBridge::getCountries() {
    return {"IT", "DE", "FR", "ES", "UK", "US", "JP", "AU", "BR", "NL", "BE", "AT", "CH", "SE", "NO", "DK", "FI", "PL", "CZ", "PT"};
}

QStringList LicensePlatesQmlBridge::getStyles() {
    return {"Standard", "Racing", "Vintage", "Modern", "Custom"};
}

QVariantMap LicensePlatesQmlBridge::getCurrentParams() {
    QVariantMap params;
    params["textColor"] = m_textColor.isEmpty() ? "#000000" : m_textColor;
    params["bgColor"] = m_bgColor.isEmpty() ? "#FFFFFF" : m_bgColor;
    params["borderColor"] = m_borderColor.isEmpty() ? "#000000" : m_borderColor;
    params["width"] = m_plateWidth;
    params["height"] = m_plateHeight;
    params["text"] = m_plateText;
    params["country"] = m_country;
    params["style"] = m_style;
    return params;
}

void LicensePlatesQmlBridge::setCurrentParams(const QVariantMap& params) {
    if (params.contains("textColor")) m_textColor = params["textColor"].toString();
    if (params.contains("bgColor")) m_bgColor = params["bgColor"].toString();
    if (params.contains("borderColor")) m_borderColor = params["borderColor"].toString();
    if (params.contains("width")) m_plateWidth = params["width"].toInt();
    if (params.contains("height")) m_plateHeight = params["height"].toInt();
    if (params.contains("text")) m_plateText = params["text"].toString();
    if (params.contains("country")) m_country = params["country"].toString();
    if (params.contains("style")) m_style = params["style"].toString();
}

QString LicensePlatesQmlBridge::generatePreviewImage() {
    LicensePlatesManager manager;

    PlateGenerationParams params;
    params.text = m_plateText.isEmpty() ? "AB 123 CD" : m_plateText;
    params.textColor = QColor(m_textColor.isEmpty() ? "#000000" : m_textColor);
    params.backgroundColor = QColor(m_bgColor.isEmpty() ? "#FFFFFF" : m_bgColor);
    params.borderColor = QColor(m_borderColor.isEmpty() ? "#000000" : m_borderColor);
    params.borderWidth = 2.0f;
    params.width = m_plateWidth;
    params.height = m_plateHeight;
    params.fontFamily = "Arial";
    params.fontSize = 48;
    params.countryCode = m_country;

    LicensePlateResult result = manager.generatePlateSimple(params);

    if (result.success) {
        QString previewPath = QDir::tempPath() + "/kseditor_plate_preview.png";
        if (manager.savePlateTexture(result, previewPath)) {
            return previewPath;
        }
    }
    return "";
}

void LicensePlatesQmlBridge::setTextColor(const QString& color) {
    m_textColor = color;
}

void LicensePlatesQmlBridge::setBgColor(const QString& color) {
    m_bgColor = color;
}

void LicensePlatesQmlBridge::setBorderColor(const QString& color) {
    m_borderColor = color;
}

void LicensePlatesQmlBridge::setPlateWidth(int width) {
    m_plateWidth = width;
}

void LicensePlatesQmlBridge::setPlateHeight(int height) {
    m_plateHeight = height;
}

QString LicensePlatesQmlBridge::generateQRCode(const QString& text, int size) {
    LicensePlatesManager manager;
    QImage qr = QRCodeWriter::encode(text, 0, QRCodeWriter::M, 4, QColor(0,0,0), QColor(255,255,255));
    if (qr.isNull()) return "";

    LicensePlateResult result;
    if (size > 0) {
        result.texture = qr.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else {
        result.texture = qr;
    }

    QString qrPath = QDir::tempPath() + "/kseditor_qr_temp.png";
    if (result.texture.save(qrPath)) return qrPath;
    return "";
}

QString LicensePlatesQmlBridge::exportPlateWithQR(const QString& text, const QString& qrText, const QString& path) {
    LicensePlatesManager manager;
    PlateGenerationParams params;
    params.text = text;
    params.textColor = QColor(m_textColor);
    params.backgroundColor = QColor(m_bgColor);
    params.borderColor = QColor(m_borderColor);
    params.borderWidth = 2.0f;
    params.width = m_plateWidth;
    params.height = m_plateHeight;
    params.cornerRadius = m_cornerRadius;
    params.textAlignment = m_textAlignment;
    params.backgroundType = m_backgroundType;
    params.gradientColor = QColor(m_gradientColor);
    params.holographicEnabled = m_holographicEnabled;
    params.countryCode = m_country;

    LicensePlateResult result = manager.generatePlateSimple(params);
    if (!result.success) return "";

    // Overlay QR code
    if (!qrText.isEmpty()) {
        QImage qr = QRCodeWriter::encode(qrText, 0, QRCodeWriter::M, 3, QColor(0,0,0), QColor(255,255,255));
        if (!qr.isNull()) {
            QPainter painter(&result.texture);
            int qrSize = qMin(params.width / 5, params.height / 2);
            qr = qr.scaled(qrSize, qrSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            painter.drawImage(params.width - qrSize - 8, params.height - qrSize - 8, qr);
            painter.end();
        }
    }

    if (manager.savePlateTexture(result, path)) return path;
    return "";
}

void LicensePlatesQmlBridge::setBackgroundType(int type) { m_backgroundType = type; }
void LicensePlatesQmlBridge::setCornerRadius(float radius) { m_cornerRadius = radius; }
void LicensePlatesQmlBridge::setTextAlignment(int align) { m_textAlignment = align; }
void LicensePlatesQmlBridge::setHolographicEnabled(bool enabled) { m_holographicEnabled = enabled; }
void LicensePlatesQmlBridge::setGradientColor(const QString& color) { m_gradientColor = color; }

bool LicensePlatesQmlBridge::insertPlateIntoLivery(const QString& skinPath, const QString& text, const QString& country) {
    bool ok = PaintSystem::generateLicensePlate(text, country, skinPath + "/license_plate.png");
    if (ok) {
PaintSystem::SkinConfig config = PaintSystem::loadSkinConfig(skinPath);

        PaintSystem::PaintLayer layer;
        layer.name = "license_plate";
        layer.type = "decal";
        layer.opacity = 1.0f;
        layer.position[0] = 0.7f;
        layer.position[1] = 0.3f;
        layer.size[0] = 0.25f;
        layer.size[1] = 0.1f;
        layer.texturePath = skinPath + "/license_plate.png";
        layer.visible = true;
        config.layers.append(layer);

        config.licensePlateText = text;
        config.licensePlateCountry = country;
ok = PaintSystem::saveSkinConfig(config, skinPath);
    }
    emit plateInsertedIntoLivery(skinPath, ok);
    return ok;
}

QStringList LicensePlatesQmlBridge::getSupportedLiveryCountries() {
    return PaintSystem::getSupportedCountries();
}

} // namespace ks
