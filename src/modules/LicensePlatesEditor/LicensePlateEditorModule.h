#pragma once

#include <QWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QImage>
#include <QFileDialog>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainter>
#include <QColorDialog>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QVector>
#include <QMap>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QScrollArea>
#include <QListWidgetItem>
#include <QString>
#include <QFont>
#include <QRegularExpression>
#include <functional>
#include <QRandomGenerator>
#include "Math/MathCore.h"
#include "QRCodeWriter.h"
#include "core/sys/ModuleManager.h"
#include "../../core/editor/EditorModule.h"

namespace ks {

// ── License Plate Core Types ───────────────────────────────────────────

struct LicensePlateResult {
    QImage texture;
    int width = 0;
    int height = 0;
    bool success = false;
    QString errorMessage;
};

struct PlateGenerationParams {
    QString text;
    QColor textColor = Qt::black;
    QColor backgroundColor = Qt::white;
    QColor borderColor = Qt::black;
    float borderWidth = 2.0f;
    int width = 512;
    int height = 128;
    QString fontFamily = "Arial";
    int fontSize = 48;
    bool isReflective = false;
    bool fontBold = false;
    QString countryCode;
    Vec2 uvOffset = Vec2(0, 0);
    Vec2 uvScale = Vec2(1, 1);

    // Enhanced rendering
    float cornerRadius = 0.0f;
    int textAlignment = 0; // 0=Center, 1=Left, 2=Right
    int backgroundType = 0; // 0=Solid, 1=Gradient, 2=Texture
    QColor gradientColor = QColor(200, 200, 200);
    QString backgroundTexturePath;
    bool holographicEnabled = false;
};

struct PlateStyle {
    QString name;
    QSize size;
    QRect backgroundRect;
    QRect textRect;
    QRect countryBandRect;
    QFont textFont;
    QColor textColor;
    QColor backgroundColor;
    QColor borderColor;
    float borderWidth;
    QString backgroundImagePath;
    QString countryCode;
    bool isReflective = false;

    std::function<QColor(const QString&)> getTextColorForText;
    std::function<QString(const QString&)> formatText;
};

struct QRCodeConfig {
    QString text;
    int moduleSize = 4;
    QColor color = Qt::black;
    QColor backgroundColor = Qt::white;
    QPoint position;
    bool enabled = false;
};

struct HolographicEffect {
    bool enabled = false;
    double angle = 45.0;
    double intensity = 0.3;
    QColor primaryColor = QColor(100, 180, 255);
    QColor secondaryColor = QColor(255, 100, 200);
};

struct CountryFormat {
    QString code;
    QString name;
    QString plateExample;
    QRegularExpression pattern;
    int maxLength;
    QColor backgroundColor;
    QColor textColor;
    QColor borderColor;
    bool hasCountryBand;
    QString countryBandText;
    bool hasEUStars;
    QString fontFamily;
};

class LicensePlatesManager {
public:
    LicensePlatesManager();
    ~LicensePlatesManager();

    bool loadStyle(const QString& stylePath, PlateStyle& outStyle);
    LicensePlateResult generatePlate(const PlateStyle& style, const QString& text);
    LicensePlateResult generatePlateSimple(const PlateGenerationParams& params);
    std::vector<LicensePlateResult> generatePlatesBatch(
        const std::vector<QString>& texts,
        const PlateGenerationParams& baseParams
    );
    QImage createAtlas(const std::vector<LicensePlateResult>& plates, int maxWidth = 4096);
    bool savePlateTexture(const LicensePlateResult& plate, const QString& outputPath);
    bool saveAsDDS(const LicensePlateResult& plate, const QString& outputPath);

    QStringList availableCountries() const;
    CountryFormat getCountryFormat(const QString& countryCode) const;
    bool validatePlateText(const QString& text, const QString& countryCode);
    void addCountryFormat(const CountryFormat& format);

    void setQRCodeConfig(const QRCodeConfig& config);
    QRCodeConfig qrCodeConfig() const { return m_qrConfig; }
    QImage generateQRCode(const QString& text, int moduleSize, const QColor& fg, const QColor& bg);

    void setHolographicEffect(const HolographicEffect& effect);
    HolographicEffect holographicEffect() const { return m_holoEffect; }
    void applyHolographicEffect(QImage& image);

private:
    QImage renderPlateImage(const PlateGenerationParams& params);
    void applyPostProcessing(QImage& image, bool isReflective);
    QString formatTextLua(const QString& text, const PlateStyle& style);
    QColor getTextColorLua(const QString& text, const PlateStyle& style);

    QMap<QString, PlateStyle> m_styleCache;
    QMap<QString, CountryFormat> m_countryFormats;
    void initDefaultCountries();
    QRCodeConfig m_qrConfig;
    HolographicEffect m_holoEffect;
};

// ── Module Types ───────────────────────────────────────────────────────

struct PlatePreset {
    QString name;
    QString countryCode;
    PlateGenerationParams params;
    QString stylePath;
};

class PlatePreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlatePreviewWidget(QWidget* parent = nullptr);
    void setImage(const QImage& image);
    void setPlateText(const QString& text);
    void setPlateStyle(const QString& styleName);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage m_image;
    QString m_plateText;
    QString m_plateStyle;
};

class LicensePlateEditorModule : public EditorModule {
    Q_OBJECT

public:
    explicit LicensePlateEditorModule(QWidget* parent = nullptr);
    ~LicensePlateEditorModule();

    QString getModuleName() const override { return "License Plate Editor"; }
    QString moduleId() const override { return "licensePlateEditor"; }
    QString getModuleIcon() const override { return ":/icons/plate.png"; }
    int getModulePriority() const override { return 35; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onGenerate();
    void onExport();
    void onExportBatch();
    void onSavePreset();
    void onLoadPreset();
    void onDeletePreset();
    void onPresetSelected(QListWidgetItem* item);
    void onTextChanged();
    void onCountryChanged(const QString& country);
    void onStyleChanged(const QString& style);
    void onTextColorClicked();
    void onBgColorClicked();
    void onBorderColorClicked();
    void onWidthChanged();
    void onHeightChanged();
    void onFontSizeChanged();
    void onReflectiveToggled(bool checked);
    void onPreviewScaleChanged(int value);
    void onCopyToClipboard();
    void onImportFromCar();
    void onNewStyle();
    void onSaveStyle();

private:
    void setupUi();
    void setupConnections();
    void populateCountryCodes();
    void populateStyleList();
    void populatePresetList();
    void updatePreview();
    void loadParameters(const PlateGenerationParams& params);
    PlateGenerationParams saveParameters();
    void applyCountryStyle(const QString& country);
    QWidget* createLeftPanel();
    QWidget* createRightPanel();
    void updateInfoLabel();

    QWidget* m_centralWidget;
    QDockWidget* m_dockWidget;
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QWidget* m_previewPanel;
    PlatePreviewWidget* m_previewWidget;
    QLabel* m_previewScaleLabel;
    QSlider* m_previewScaleSlider;
    QWidget* m_paramsPanel;
    QScrollArea* m_paramsScrollArea;
    QLineEdit* m_textEdit;
    QComboBox* m_countryCombo;
    QComboBox* m_styleCombo;
    QPushButton* m_textColorBtn;
    QPushButton* m_bgColorBtn;
    QPushButton* m_borderColorBtn;
    QLabel* m_textColorLabel;
    QLabel* m_bgColorLabel;
    QLabel* m_borderColorLabel;
    QSpinBox* m_widthSpin;
    QSpinBox* m_heightSpin;
    QSpinBox* m_fontSizeSpin;
    QSpinBox* m_borderWidthSpin;
    QDoubleSpinBox* m_borderRadiusSpin;
    QCheckBox* m_reflectiveCheck;
    QCheckBox* m_uppercaseCheck;
    QCheckBox* m_autoFormatCheck;
    QComboBox* m_fontCombo;
    QComboBox* m_textAlignCombo;
    QComboBox* m_backgroundTypeCombo;
    QListWidget* m_presetList;
    QPushButton* m_savePresetBtn;
    QPushButton* m_loadPresetBtn;
    QPushButton* m_deletePresetBtn;
    QPushButton* m_generateBtn;
    QPushButton* m_exportBtn;
    QPushButton* m_exportBatchBtn;
    QPushButton* m_copyBtn;
    QLabel* m_statusLabel;
    QLabel* m_infoLabel;
    LicensePlatesManager* m_manager;
    QMap<QString, PlateStyle> m_loadedStyles;
    QVector<PlatePreset> m_presets;
    PlateGenerationParams m_currentParams;
    QImage m_generatedImage;
    QString m_currentStylePath;
    QString m_lastExportDir;
};

} // namespace ks
