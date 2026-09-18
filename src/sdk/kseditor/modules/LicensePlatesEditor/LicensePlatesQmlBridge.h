#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include "../PaintEditor/PaintSystem.h"

namespace ks {

class LicensePlatesQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString plateText READ plateText WRITE setPlateText NOTIFY plateTextChanged)
    Q_PROPERTY(QString country READ country WRITE setCountry NOTIFY countryChanged)
    Q_PROPERTY(QString style READ style WRITE setStyle NOTIFY styleChanged)
    Q_PROPERTY(int presetCount READ presetCount NOTIFY presetCountChanged)

public:
    static LicensePlatesQmlBridge* instance();

    QString plateText() const { return m_plateText; }
    void setPlateText(const QString& text) { m_plateText = text; emit plateTextChanged(); }
    QString country() const { return m_country; }
    void setCountry(const QString& country) { m_country = country; emit countryChanged(); }
    QString style() const { return m_style; }
    void setStyle(const QString& style) { m_style = style; emit styleChanged(); }
    int presetCount() const { return m_presetCount; }

    Q_INVOKABLE void generatePlate();
    Q_INVOKABLE QString exportPlate(const QString& path);
    Q_INVOKABLE void exportBatch(const QString& directory, const QStringList& texts);
    Q_INVOKABLE void savePreset(const QString& name);
    Q_INVOKABLE void loadPreset(const QString& name);
    Q_INVOKABLE void deletePreset(const QString& name);
    Q_INVOKABLE QVariantList getPresets();
    Q_INVOKABLE QVariantMap getPreset(const QString& name);
    Q_INVOKABLE QStringList getCountries();
    Q_INVOKABLE QStringList getStyles();
    Q_INVOKABLE QVariantMap getCurrentParams();
    Q_INVOKABLE void setCurrentParams(const QVariantMap& params);
    Q_INVOKABLE QString generatePreviewImage();
    Q_INVOKABLE void setTextColor(const QString& color);
    Q_INVOKABLE void setBgColor(const QString& color);
    Q_INVOKABLE void setBorderColor(const QString& color);
    Q_INVOKABLE void setPlateWidth(int width);
    Q_INVOKABLE void setPlateHeight(int height);

    Q_INVOKABLE bool insertPlateIntoLivery(const QString& skinPath, const QString& text, const QString& country);
    Q_INVOKABLE QStringList getSupportedLiveryCountries();

    // Enhanced features
    Q_INVOKABLE QString generateQRCode(const QString& text, int size);
    Q_INVOKABLE QString exportPlateWithQR(const QString& text, const QString& qrText, const QString& path);
    Q_INVOKABLE void setBackgroundType(int type);
    Q_INVOKABLE void setCornerRadius(float radius);
    Q_INVOKABLE void setTextAlignment(int align);
    Q_INVOKABLE void setHolographicEnabled(bool enabled);
    Q_INVOKABLE void setGradientColor(const QString& color);

signals:
    void plateTextChanged();
    void countryChanged();
    void styleChanged();
    void presetCountChanged();
    void plateGenerated(const QString& imagePath);
    void presetSaved(const QString& name);
    void presetLoaded(const QString& name);
    void presetDeleted(const QString& name);
    void plateInsertedIntoLivery(const QString& skinPath, bool success);

private:
    static LicensePlatesQmlBridge* s_instance;
    QString m_plateText;
    QString m_country;
    QString m_style;
    int m_presetCount = 0;
    QString m_textColor = "#000000";
    QString m_bgColor = "#FFFFFF";
    QString m_borderColor = "#000000";
    int m_plateWidth = 520;
    int m_plateHeight = 110;
    int m_backgroundType = 0;
    float m_cornerRadius = 0.0f;
    int m_textAlignment = 0;
    bool m_holographicEnabled = false;
    QString m_gradientColor = "#C8C8C8";
};

} // namespace ks
