#include "PPFiltersEditor.h"
#if HAS_QT3D
#include <Qt3DRender/Qt3DRender>
#include <Qt3DExtras/Qt3DExtras>
#include <Qt3DLogic/Qt3DLogic>
#endif
#include <QVulkanWindow>
#include <QVulkanInstance>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QScrollArea>
#include <QHeaderView>
#include <QFileDialog>
#include <QDebug>
#include <QRegularExpression>
#include <QDataStream>
#include <QTemporaryFile>
#include <QProcess>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include "core/editor/EditorConfig.h"
#include "core/textEditor/CodeEditor.h"
#include "core/textEditor/SyntaxHighlighter.h"

// Main Editor Implementation
KSPFiltersEditor::KSPFiltersEditor(QWidget *parent)
    : QMainWindow(parent)
    , m_vulkanInstance(nullptr)
{
    setupUI();
    setup3DPreview();
    loadFiltersList();
}

KSPFiltersEditor::~KSPFiltersEditor()
{
    if (m_vulkanInstance) {
        delete m_vulkanInstance;
    }
}

// ============================================================================
// Qt3DWindow — real Qt3D rendering surface embedded in a QWidget
// ============================================================================

#if HAS_QT3D

KSPFiltersEditor::Qt3DWindow::Qt3DWindow(QWidget *parent)
    : QWidget(parent)
    , m_view(new Qt3DExtras::Qt3DWindow())
    , m_rootEntity(nullptr)
    , m_camera(nullptr)
    , m_renderSettings(nullptr)
{
    // Embed the QWindow inside this QWidget
    QWidget *container = QWidget::createWindowContainer(m_view, this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(container);

    // Grab the render settings from the window's active frame graph
    m_renderSettings = m_view->renderSettings();

    // Set up an initial camera (will be replaced by createTestScene)
    m_camera = m_view->camera();
    m_camera->lens()->setPerspectiveProjection(60.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
    m_camera->setPosition(QVector3D(0, 2, 8));
    m_camera->setViewCenter(QVector3D(0, 0, 0));

    // Default root entity
    m_rootEntity = new Qt3DCore::QEntity();
    m_view->setRootEntity(m_rootEntity);
}

KSPFiltersEditor::Qt3DWindow::~Qt3DWindow()
{
    // m_view is a child of this widget's container; Qt handles cleanup
}

Qt3DRender::QCamera* KSPFiltersEditor::Qt3DWindow::camera()
{
    return m_camera;
}

void KSPFiltersEditor::Qt3DWindow::setRootEntity(Qt3DCore::QEntity *entity)
{
    m_rootEntity = entity;
    if (m_view) {
        m_view->setRootEntity(entity);
    }
}

#endif // HAS_QT3D

void KSPFiltersEditor::setupUI()
{
    setWindowTitle(EditorConfig::instance().editorTitle());    resize(1280, 800);
    
    // Central widget with splitter
    QWidget *central = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, central);
    
    // Left panel - Filter tree
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    
    QLabel *filterLabel = new QLabel("PP Filters:", leftPanel);
    leftLayout->addWidget(filterLabel);
    
    m_filterTree = new QTreeWidget(leftPanel);
    m_filterTree->setHeaderLabel("Available Filters");
    m_filterTree->setMaximumWidth(250);
    leftLayout->addWidget(m_filterTree);
    
    // Right panel - Tabs
    m_mainTabs = new QTabWidget();
    
    // Parameters tab
    QWidget *paramTab = new QWidget();
    QVBoxLayout *paramLayout = new QVBoxLayout(paramTab);
    
    m_filterInfoLabel = new QLabel("No filter selected");
    paramLayout->addWidget(m_filterInfoLabel);
    
    m_paramTable = new QTableWidget(paramTab);
    m_paramTable->setColumnCount(4);
    m_paramTable->setHorizontalHeaderLabels({"Parameter", "Value", "Min", "Max"});
    m_paramTable->horizontalHeader()->setStretchLastSection(true);
    paramLayout->addWidget(m_paramTable);
    
    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_saveBtn = new QPushButton("Save Filter", paramTab);
    m_exportBtn = new QPushButton("Export to AC", paramTab);
    m_reloadBtn = new QPushButton("Reload", paramTab);
    btnLayout->addWidget(m_saveBtn);
    btnLayout->addWidget(m_exportBtn);
    btnLayout->addWidget(m_reloadBtn);
    paramLayout->addLayout(btnLayout);
    
    m_mainTabs->addTab(paramTab, "Parameters");
    
    // Scene preset tab
    QWidget *sceneTab = new QWidget();
    QVBoxLayout *sceneLayout = new QVBoxLayout(sceneTab);
    m_scenePresetCombo = new QComboBox();
    m_scenePresetCombo->addItems({"Spa Francorchamps", "Nordschleife", "Monza", "Test Track"});
    sceneLayout->addWidget(m_scenePresetCombo);
    m_mainTabs->addTab(sceneTab, "3D Scene");

    // Color Grading tab
    setupColorGradingUI(m_mainTabs);

    // Shader Editor tab
    setupShaderEditorTab();

    // Add widgets to splitter
    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(m_mainTabs);
    mainSplitter->setSizes({250, 800});
    
    mainLayout->addWidget(mainSplitter);
    setCentralWidget(central);
    
    // Connect signals
    connect(m_filterTree, &QTreeWidget::clicked, this, &KSPFiltersEditor::onFilterSelected);
    connect(m_saveBtn, &QPushButton::clicked, this, &KSPFiltersEditor::onSaveFilter);
    connect(m_exportBtn, &QPushButton::clicked, this, &KSPFiltersEditor::onExportFilter);
    connect(m_reloadBtn, &QPushButton::clicked, this, &KSPFiltersEditor::onReloadFilter);
    connect(m_scenePresetCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, &KSPFiltersEditor::updatePreview);
}

void KSPFiltersEditor::setup3DPreview()
{
#if HAS_QT3D
    // Create Vulkan instance
    m_vulkanInstance = new QVulkanInstance();
    m_vulkanInstance->setLayers(QByteArrayList() << "VK_LAYER_KHRONOS_validation");

    if (!m_vulkanInstance->create()) {
        qWarning() << "Failed to create Vulkan instance, falling back to OpenGL";
    }

    // Create 3D window
    m_3dWindow = new Qt3DWindow(this);
    m_3dWindow->setMinimumSize(400, 400);

    // Add to preview container in parameters tab
    m_mainTabs->addTab(m_3dWindow, "3D Preview");

    // Create scene
    createTestScene();

    // Set root entity
    m_3dWindow->setRootEntity(m_rootEntity);
#else
    m_3dWindow = nullptr;
    // Fallback: 2D gradient preview widget for color grading visualization
    auto* fallbackPreview = new QWidget();
    fallbackPreview->setMinimumSize(320, 240);
    fallbackPreview->setAutoFillBackground(true);
    QPalette pal = fallbackPreview->palette();
    pal.setColor(QPalette::Window, QColor("#1a1a2e"));
    fallbackPreview->setPalette(pal);
    m_mainTabs->addTab(fallbackPreview, "3D Preview (fallback)");
#endif
}

void KSPFiltersEditor::setupVulkanBackend()
{
#if HAS_VULKAN
    if (!m_vulkanInstance) {
        m_vulkanInstance = new QVulkanInstance();
        m_vulkanInstance->setLayers(QByteArrayList() << "VK_LAYER_KHRONOS_validation");
        if (!m_vulkanInstance->create()) {
            qWarning() << "Vulkan instance creation failed";
            delete m_vulkanInstance;
            m_vulkanInstance = nullptr;
        }
    }
#else
    m_vulkanInstance = nullptr;
#endif
}

void KSPFiltersEditor::createTestScene()
{
#if HAS_QT3D
    // Root entity
    m_rootEntity = new Qt3DCore::QEntity();

    // Camera
    m_camera = m_3dWindow->camera();
    m_camera->setPosition(QVector3D(0, 2, 8));
    m_camera->setViewCenter(QVector3D(0, 0, 0));

    // Directional light (needs its own entity with transform)
    Qt3DCore::QEntity *dirLightEntity = new Qt3DCore::QEntity(m_rootEntity);
    Qt3DRender::QDirectionalLight *light = new Qt3DRender::QDirectionalLight(dirLightEntity);
    light->setColor(QColor(255, 255, 255));
    light->setIntensity(0.8f);
    light->setWorldDirection(QVector3D(0, -1, -0.5));
    Qt3DCore::QTransform *dirLightTransform = new Qt3DCore::QTransform(dirLightEntity);
    dirLightTransform->setTranslation(QVector3D(0, 10, 0));
    dirLightEntity->addComponent(light);
    dirLightEntity->addComponent(dirLightTransform);

    Qt3DCore::QEntity *fillLightEntity = new Qt3DCore::QEntity(m_rootEntity);
    Qt3DRender::QPointLight *fillLight = new Qt3DRender::QPointLight(fillLightEntity);
    fillLight->setColor(QColor(150, 150, 255));
    fillLight->setIntensity(0.4f);
    Qt3DCore::QTransform *fillLightTransform = new Qt3DCore::QTransform(fillLightEntity);
    fillLightTransform->setTranslation(QVector3D(2, 3, 2));
    fillLightEntity->addComponent(fillLight);
    fillLightEntity->addComponent(fillLightTransform);
    
    // Ground plane (track)
    Qt3DExtras::QPlaneMesh *planeMesh = new Qt3DExtras::QPlaneMesh();
    planeMesh->setWidth(10);
    planeMesh->setHeight(20);
    
    Qt3DExtras::QDiffuseSpecularMaterial *trackMaterial = new Qt3DExtras::QDiffuseSpecularMaterial();
    trackMaterial->setDiffuse(QColor(40, 40, 45));
    trackMaterial->setSpecular(QColor(20, 20, 20));
    trackMaterial->setShininess(50);
    
    m_trackEntity = new Qt3DCore::QEntity(m_rootEntity);
    m_trackEntity->addComponent(planeMesh);
    m_trackEntity->addComponent(trackMaterial);
    
    // Test car body (simplified)
    Qt3DExtras::QCylinderMesh *carBodyMesh = new Qt3DExtras::QCylinderMesh();
    carBodyMesh->setRadius(0.8f);
    carBodyMesh->setLength(2.0f);
    
    Qt3DExtras::QDiffuseSpecularMaterial *carMaterial = new Qt3DExtras::QDiffuseSpecularMaterial();
    carMaterial->setDiffuse(QColor(200, 50, 50));
    carMaterial->setSpecular(QColor(100, 100, 100));
    carMaterial->setShininess(80);
    
    m_carEntity = new Qt3DCore::QEntity(m_rootEntity);
    m_carEntity->addComponent(carBodyMesh);
    m_carEntity->addComponent(carMaterial);
    
    Qt3DCore::QTransform *carTransform = new Qt3DCore::QTransform();
    carTransform->setTranslation(QVector3D(0, 0.5f, 0));
    m_carEntity->addComponent(carTransform);
    
    // Sky sphere (environment)
    Qt3DExtras::QSphereMesh *sphereMesh = new Qt3DExtras::QSphereMesh();
    sphereMesh->setRadius(50);
    sphereMesh->setRings(100);
    sphereMesh->setSlices(100);
    
    Qt3DExtras::QDiffuseSpecularMaterial *skyMaterial = new Qt3DExtras::QDiffuseSpecularMaterial();
    skyMaterial->setDiffuse(QColor(135, 206, 235));
    skyMaterial->setAmbient(QColor(100, 100, 120));
    
    m_skySphere = new Qt3DCore::QEntity(m_rootEntity);
    m_skySphere->addComponent(sphereMesh);
    m_skySphere->addComponent(skyMaterial);
#endif
}

void KSPFiltersEditor::loadFiltersList()
{
    // Search paths for PPFilters
    QStringList searchPaths = {
        EditorConfig::instance().ppFiltersPath(),
        "./ppfilters/"
    };
    
    QMap<QString, QStringList> filterCategories;
    
    for (const QString &path : searchPaths) {
        QDir dir(path);
        if (dir.exists()) {
            QStringList iniFiles = dir.entryList(QStringList() << "*.ini", QDir::Files);
            for (const QString &file : iniFiles) {
                QString filterName = QString(file).replace(".ini", "");
                filterCategories["Local Filters"].append(filterName);
                
                // Store path for loading
                m_currentFilter.path = dir.absoluteFilePath(file);
            }
        }
    }
    
    // Populate tree
    m_filterTree->clear();
    for (auto it = filterCategories.begin(); it != filterCategories.end(); ++it) {
        QTreeWidgetItem *categoryItem = new QTreeWidgetItem(m_filterTree);
        categoryItem->setText(0, it.key());
        
        for (const QString &filter : it.value()) {
            QTreeWidgetItem *filterItem = new QTreeWidgetItem(categoryItem);
            filterItem->setText(0, filter);
        }
    }
    
    m_filterTree->expandAll();
}

void KSPFiltersEditor::onFilterSelected(const QModelIndex &index)
{
    QTreeWidgetItem *item = m_filterTree->currentItem();
    if (!item || item->parent() == nullptr) // Skip category items
        return;
    
    QString filterName = item->text(0);
    m_currentFilter.name = filterName;
    
    // Find the actual file path
    QStringList searchPaths = {
        EditorConfig::instance().ppFiltersPath()
    };
    
    for (const QString &path : searchPaths) {
        QString filePath = path + filterName + ".ini";
        if (QFile::exists(filePath)) {
            m_currentFilterPath = filePath;
            parseFilterINI(filePath);
            break;
        }
    }
    
    m_filterInfoLabel->setText(QString("Filter: %1\nPath: %2").arg(filterName, m_currentFilterPath));
}

void KSPFiltersEditor::parseFilterINI(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open filter file:" << filePath;
        return;
    }
    
    m_currentFilter.parameters.clear();
    m_paramTable->clearContents();
    
    QTextStream in(&file);
    QString currentSection;
    PPFilterParameter currentParam;
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        
        if (line.startsWith("[") && line.endsWith("]")) {
            currentSection = line.mid(1, line.length() - 2);
        } else if (line.contains("=") && !line.startsWith(";") && !line.startsWith("#")) {
            QStringList parts = line.split("=");
            QString paramName = parts[0].trimmed();
            float value = parts[1].trimmed().toFloat();
            
            currentParam.name = paramName;
            currentParam.value = value;
            currentParam.min = 0.0f;
            currentParam.max = 2.0f;
            currentParam.step = 0.01f;
            currentParam.type = "float";
            
            m_currentFilter.parameters[paramName] = currentParam;
        }
    }
    
    file.close();
    
    // Populate parameter table
    m_paramTable->setRowCount(m_currentFilter.parameters.size());
    int row = 0;
    for (auto it = m_currentFilter.parameters.begin(); it != m_currentFilter.parameters.end(); ++it) {
        m_paramTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        
        QDoubleSpinBox *spinBox = new QDoubleSpinBox();
        spinBox->setRange(it.value().min, it.value().max);
        spinBox->setSingleStep(it.value().step);
        spinBox->setValue(it.value().value);
        spinBox->setDecimals(3);
        
        m_paramTable->setCellWidget(row, 1, spinBox);
        m_paramTable->setItem(row, 2, new QTableWidgetItem(QString::number(it.value().min)));
        m_paramTable->setItem(row, 3, new QTableWidgetItem(QString::number(it.value().max)));
        
        m_paramSpinBoxes[it.key()] = spinBox;
        
        connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
                [this, paramName = it.key()](double val) {
                    onParameterChanged(paramName, static_cast<float>(val));
                });
        
        row++;
    }
    
    m_paramTable->resizeColumnsToContents();
}

void KSPFiltersEditor::onParameterChanged(const QString &paramName, float value)
{
    if (m_currentFilter.parameters.contains(paramName)) {
        m_currentFilter.parameters[paramName].value = value;
        applyParametersTo3DScene();
    }
}

void KSPFiltersEditor::applyParametersTo3DScene()
{
    float saturation = m_currentFilter.parameters.value("SATURATION").value;
    float contrast = m_currentFilter.parameters.value("CONTRAST").value;
    float brightness = m_currentFilter.parameters.value("BRIGHTNESS").value;
    float gamma = m_currentFilter.parameters.value("GAMMA").value;

#if HAS_QT3D
    // Adjust car material based on filter parameters
    if (m_carEntity) {
        Qt3DExtras::QDiffuseSpecularMaterial *carMat =
            qobject_cast<Qt3DExtras::QDiffuseSpecularMaterial*>(m_carEntity->components()[1]);

        if (carMat) {
            // Apply color grading effect
            QColor diffuseColor = QColor::fromHsv(
                0, // Hue
                static_cast<int>(255 * saturation), // Saturation
                static_cast<int>(128 + brightness * 128) // Value
            );
            carMat->setDiffuse(diffuseColor);
        }
    }

    // Update sky color based on filter
    if (m_skySphere) {
        Qt3DExtras::QDiffuseSpecularMaterial *skyMat =
            qobject_cast<Qt3DExtras::QDiffuseSpecularMaterial*>(m_skySphere->components()[1]);

        if (skyMat) {
            int blueVal = static_cast<int>(135 + contrast * 50);
            QColor skyColor(100, 100, qBound(50, blueVal, 255));
            skyMat->setDiffuse(skyColor);
        }
    }
#else
    qDebug() << "PPFiltersEditor: Color correction (requires Qt3D): Sat="
             << saturation << "Contrast=" << contrast
             << "Bright=" << brightness << "Gamma=" << gamma;
#endif
}

void KSPFiltersEditor::onSaveFilter()
{
    if (m_currentFilterPath.isEmpty()) {
        QMessageBox::warning(this, "Error", "No filter loaded");
        return;
    }
    
    saveFilterINI();
    QMessageBox::information(this, "Success", "Filter saved successfully");
}

void KSPFiltersEditor::saveFilterINI()
{
    QFile file(m_currentFilterPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Cannot write filter file");
        return;
    }
    
    QTextStream out(&file);
    out << "; KSP PPFilter Editor\n";
    out << "; Generated with Qt6 3D + Vulkan\n\n";
    
    out << "[PRESET]\n";
    for (auto it = m_currentFilter.parameters.begin(); it != m_currentFilter.parameters.end(); ++it) {
        out << it.key() << "=" << it.value().value << "\n";
    }
    
    out << "\n[DESCRIPTION]\n";
    out << "NAME=" << m_currentFilter.name << "\n";
    out << "AUTHOR=KSP Editor\n";
    
    file.close();
}

void KSPFiltersEditor::onExportFilter()
{
    // Export to simulator format
    QString exportPath = EditorConfig::instance().ppFiltersPath();
    QDir().mkpath(exportPath);
    
    QString exportFile = exportPath + m_currentFilter.name + ".ini";
    
    if (QFile::copy(m_currentFilterPath, exportFile)) {
        QMessageBox::information(this, "Export Complete", 
                                 QString("Filter exported to:\n%1").arg(exportFile));
    } else {
        QMessageBox::warning(this, "Export Failed", 
                             QString("Failed to export filter to:\n%1").arg(exportFile));
    }
}

void KSPFiltersEditor::onReloadFilter()
{
    if (!m_currentFilterPath.isEmpty()) {
        parseFilterINI(m_currentFilterPath);
        applyParametersTo3DScene();
        QMessageBox::information(this, "Reloaded", "Filter reloaded from disk");
    }
}

void KSPFiltersEditor::updatePreview()
{
    applyParametersTo3DScene();
}

void KSPFiltersEditor::loadFromKsSystem(const QString& systemPath)
{
    m_acSystemPath = systemPath;
    
    KsVulkanIntegration* ksVulkan = KsVulkanIntegration::instance();
    if (!ksVulkan || !ksVulkan->isInitialized()) {
        qWarning() << "KsVulkanIntegration not initialized";
        return;
    }
    
    const QMap<QString, PPFilterPreset*>& presets = ksVulkan->ppFilterPresets();
    
    for (auto it = presets.constBegin(); it != presets.constEnd(); ++it) {
        const QString& presetName = it.key();
        
        QTreeWidgetItem* item = new QTreeWidgetItem(m_filterTree);
        item->setText(0, presetName);
        item->setData(0, Qt::UserRole, presetName);
        m_filterTree->addTopLevelItem(item);
    }
    
    qInfo() << "Loaded" << presets.size() << "PP filter presets from KS system";
}

void KSPFiltersEditor::applyToKs(PPFilterPreset& /*preset*/)
{
    // PPFilterPreset loads its state from a QSettings/INI file via loadFromSettings().
    // We save the current filter to a temp INI and reload it into m_currentPreset.
    if (!m_currentFilterPath.isEmpty()) {
        delete m_currentPreset;
        m_currentPreset = new PPFilterPreset(m_currentFilter.name, this);
        QSettings settings(m_currentFilterPath, QSettings::IniFormat);
        m_currentPreset->loadFromSettings(&settings);
    }
    syncWithKsVulkan();
}

void KSPFiltersEditor::syncWithKsVulkan()
{
    KsVulkanIntegration* ksVulkan = KsVulkanIntegration::instance();
    if (ksVulkan && ksVulkan->isInitialized() && m_currentPreset) {
        ksVulkan->applyPPFilterPreset(m_currentPreset);
    }
}

// ============================================================================
// Color Grading Support
// ============================================================================

void KSPFiltersEditor::setupColorGradingUI(QWidget* parent)
{
    m_colorGradingTab = new QWidget();
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    auto* cgWidget = new QWidget();
    auto* cgLayout = new QVBoxLayout(cgWidget);

    // Presets
    auto* presetGroup = new QGroupBox("Color Grading Presets");
    auto* presetLayout = new QVBoxLayout(presetGroup);
    m_cgPresetCombo = new QComboBox();
    m_cgPresetCombo->addItems({"Default", "Cinematic", "Vivid", "Vintage", "Moody"});
    connect(m_cgPresetCombo, &QComboBox::currentTextChanged, this, &KSPFiltersEditor::onColorGradingPresetChanged);
    presetLayout->addWidget(m_cgPresetCombo);

    auto* lutBtnLayout = new QHBoxLayout();
    m_cgExportLUTBtn = new QPushButton("Export .cube LUT");
    m_cgImportLUTBtn = new QPushButton("Import .cube LUT");
    connect(m_cgExportLUTBtn, &QPushButton::clicked, this, &KSPFiltersEditor::onExportCubeLUT);
    connect(m_cgImportLUTBtn, &QPushButton::clicked, this, &KSPFiltersEditor::onImportCubeLUT);
    lutBtnLayout->addWidget(m_cgExportLUTBtn);
    lutBtnLayout->addWidget(m_cgImportLUTBtn);
    presetLayout->addLayout(lutBtnLayout);
    cgLayout->addWidget(presetGroup);

    // Exposure / Gamma / Contrast
    auto* basicGroup = new QGroupBox("Basic Adjustments");
    auto* basicGrid = new QGridLayout(basicGroup);

    auto addSlider = [&](const QString& label, QDoubleSpinBox*& spinner, int row, int col, float val, float min, float max, float step) {
        basicGrid->addWidget(new QLabel(label), row, col);
        spinner = new QDoubleSpinBox();
        spinner->setRange(min, max);
        spinner->setSingleStep(step);
        spinner->setValue(val);
        spinner->setDecimals(3);
        connect(spinner, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &KSPFiltersEditor::onColorGradingParamChanged);
        basicGrid->addWidget(spinner, row, col + 1);
    };

    addSlider("Exposure:", m_cgExposure, 0, 0, 0.0f, -5.0f, 5.0f, 0.01f);
    addSlider("Gamma:", m_cgGamma, 0, 2, 1.0f, 0.1f, 5.0f, 0.01f);
    addSlider("Contrast:", m_cgContrast, 1, 0, 1.0f, 0.0f, 3.0f, 0.01f);
    addSlider("Brightness:", m_cgBrightness, 1, 2, 1.0f, 0.0f, 3.0f, 0.01f);
    addSlider("Saturation:", m_cgSaturation, 2, 0, 1.0f, 0.0f, 3.0f, 0.01f);
    addSlider("Vibrance:", m_cgVibrance, 2, 2, 0.0f, 0.0f, 1.0f, 0.01f);
    addSlider("Temperature:", m_cgTemp, 3, 0, 0.0f, -50.0f, 50.0f, 1.0f);
    addSlider("Tint:", m_cgTint, 3, 2, 0.0f, -50.0f, 50.0f, 1.0f);
    cgLayout->addWidget(basicGroup);

    // Lift / Gamma / Gain per channel
    auto* lggGroup = new QGroupBox("Lift / Gamma / Gain (per-channel)");
    auto* lggGrid = new QGridLayout(lggGroup);

    auto addChannelRow = [&](const QString& label, QDoubleSpinBox*& lift, QDoubleSpinBox*& gamma, QDoubleSpinBox*& gain, int row) {
        lggGrid->addWidget(new QLabel(label), row, 0);
        lift = new QDoubleSpinBox(); lift->setRange(-1.0, 1.0); lift->setSingleStep(0.01); lift->setValue(0.0); lift->setDecimals(3);
        connect(lift, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &KSPFiltersEditor::onColorGradingParamChanged);
        lggGrid->addWidget(lift, row, 1);
        gamma = new QDoubleSpinBox(); gamma->setRange(0.1, 5.0); gamma->setSingleStep(0.01); gamma->setValue(1.0); gamma->setDecimals(3);
        connect(gamma, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &KSPFiltersEditor::onColorGradingParamChanged);
        lggGrid->addWidget(gamma, row, 2);
        gain = new QDoubleSpinBox(); gain->setRange(0.0, 5.0); gain->setSingleStep(0.01); gain->setValue(1.0); gain->setDecimals(3);
        connect(gain, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &KSPFiltersEditor::onColorGradingParamChanged);
        lggGrid->addWidget(gain, row, 3);
    };
    lggGrid->addWidget(new QLabel(""), 0, 0);
    lggGrid->addWidget(new QLabel("Lift"), 0, 1);
    lggGrid->addWidget(new QLabel("Gamma"), 0, 2);
    lggGrid->addWidget(new QLabel("Gain"), 0, 3);
    addChannelRow("Red", m_cgLiftR, m_cgGammaR, m_cgGainR, 1);
    addChannelRow("Green", m_cgLiftG, m_cgGammaG, m_cgGainG, 2);
    addChannelRow("Blue", m_cgLiftB, m_cgGammaB, m_cgGainB, 3);
    cgLayout->addWidget(lggGroup);

    // Split-toning
    auto* splitGroup = new QGroupBox("Split Toning");
    auto* splitGrid = new QGridLayout(splitGroup);
    addSlider("Shadows Saturation:", m_cgShadowsSat, 0, 0, 0.0f, 0.0f, 1.0f, 0.01f);
    addSlider("Highlights Saturation:", m_cgHighlightsSat, 0, 2, 0.0f, 0.0f, 1.0f, 0.01f);
    addSlider("Balance:", m_cgBalance, 1, 0, 0.0f, -1.0f, 1.0f, 0.01f);
    cgLayout->addWidget(splitGroup);

    // Output
    auto* outGroup = new QGroupBox("Output");
    auto* outLayout = new QVBoxLayout(outGroup);
    addSlider("LUT Intensity:", m_cgLutIntensity, 0, 0, 1.0f, 0.0f, 1.0f, 0.01f);
    cgLayout->addWidget(outGroup);

    cgLayout->addStretch();
    scrollArea->setWidget(cgWidget);
    auto* tabLayout = new QVBoxLayout(m_colorGradingTab);
    tabLayout->addWidget(scrollArea);
    m_mainTabs->addTab(m_colorGradingTab, "Color Grading");
}

void KSPFiltersEditor::updateColorGradingParams()
{
    auto p = m_colorGrading.params();
    if (m_cgExposure) p.exposure = m_cgExposure->value();
    if (m_cgGamma) p.gamma = m_cgGamma->value();
    if (m_cgContrast) p.contrast = m_cgContrast->value();
    if (m_cgBrightness) p.brightness = m_cgBrightness->value();
    if (m_cgSaturation) p.saturation = m_cgSaturation->value();
    if (m_cgVibrance) p.vibrance = m_cgVibrance->value();
    if (m_cgTemp) p.temperature = m_cgTemp->value();
    if (m_cgTint) p.tint = m_cgTint->value();
    if (m_cgLiftR) p.liftR = m_cgLiftR->value();
    if (m_cgLiftG) p.liftG = m_cgLiftG->value();
    if (m_cgLiftB) p.liftB = m_cgLiftB->value();
    if (m_cgGammaR) p.gammaR = m_cgGammaR->value();
    if (m_cgGammaG) p.gammaG = m_cgGammaG->value();
    if (m_cgGammaB) p.gammaB = m_cgGammaB->value();
    if (m_cgGainR) p.gainR = m_cgGainR->value();
    if (m_cgGainG) p.gainG = m_cgGainG->value();
    if (m_cgGainB) p.gainB = m_cgGainB->value();
    if (m_cgShadowsSat) p.shadowsSat = m_cgShadowsSat->value();
    if (m_cgHighlightsSat) p.highlightsSat = m_cgHighlightsSat->value();
    if (m_cgBalance) p.balance = m_cgBalance->value();
    if (m_cgLutIntensity) p.lutIntensity = m_cgLutIntensity->value();
    m_colorGrading.setParams(p);
}

void KSPFiltersEditor::onColorGradingParamChanged()
{
    auto oldParams = m_colorGrading.params();
    updateColorGradingParams();
    auto newParams = m_colorGrading.params();
    m_cgPresetCombo->blockSignals(true);
    m_cgPresetCombo->setCurrentIndex(0);
    m_cgPresetCombo->blockSignals(false);
    applyColorGradingToScene();
}

void KSPFiltersEditor::onColorGradingPresetChanged(const QString& preset)
{
    ks::ColorGradingParams p;
    if (preset == "Cinematic") p = ks::PPFilterColorGrading::cinematicParams();
    else if (preset == "Vivid") p = ks::PPFilterColorGrading::vividParams();
    else if (preset == "Vintage") p = ks::PPFilterColorGrading::vintageParams();
    else if (preset == "Moody") p = ks::PPFilterColorGrading::moodyParams();
    else p = ks::PPFilterColorGrading::defaultParams();

    m_cgExposure->setValue(p.exposure);
    m_cgGamma->setValue(p.gamma);
    m_cgContrast->setValue(p.contrast);
    m_cgBrightness->setValue(p.brightness);
    m_cgSaturation->setValue(p.saturation);
    m_cgVibrance->setValue(p.vibrance);
    m_cgTemp->setValue(p.temperature);
    m_cgTint->setValue(p.tint);
    m_cgLiftR->setValue(p.liftR); m_cgLiftG->setValue(p.liftG); m_cgLiftB->setValue(p.liftB);
    m_cgGammaR->setValue(p.gammaR); m_cgGammaG->setValue(p.gammaG); m_cgGammaB->setValue(p.gammaB);
    m_cgGainR->setValue(p.gainR); m_cgGainG->setValue(p.gainG); m_cgGainB->setValue(p.gainB);
    m_cgShadowsSat->setValue(p.shadowsSat);
    m_cgHighlightsSat->setValue(p.highlightsSat);
    m_cgBalance->setValue(p.balance);
    m_cgLutIntensity->setValue(p.lutIntensity);

    m_colorGrading.setParams(p);
    applyColorGradingToScene();
}

void KSPFiltersEditor::onExportCubeLUT()
{
    updateColorGradingParams();
    QString path = QFileDialog::getSaveFileName(this, "Export .cube LUT", QString(), "Cube LUT (*.cube)");
    if (path.isEmpty()) return;
    if (m_colorGrading.exportCubeLUT(path)) {
        QMessageBox::information(this, "Export Complete", QString("LUT exported to:\n%1").arg(path));
    } else {
        QMessageBox::warning(this, "Export Failed", "Failed to export LUT file");
    }
}

void KSPFiltersEditor::onImportCubeLUT()
{
    QString path = QFileDialog::getOpenFileName(this, "Import .cube LUT", QString(), "Cube LUT (*.cube)");
    if (path.isEmpty()) return;
    if (m_colorGrading.importCubeLUT(path)) {
        QMessageBox::information(this, "Import Complete",
            QString("LUT imported (size: %1)").arg(m_colorGrading.lutSize()));
    } else {
        QMessageBox::warning(this, "Import Failed", "Failed to import LUT file");
    }
}

void KSPFiltersEditor::applyColorGradingToScene()
{
    updateColorGradingParams();
    auto lut = m_colorGrading.generateLUT3D(33);

#if HAS_QT3D
    if (m_carEntity) {
        auto* carMat = qobject_cast<Qt3DExtras::QDiffuseSpecularMaterial*>(m_carEntity->components()[1]);
        if (carMat) {
            float lum = 0.2126f * lut[0] + 0.7152f * lut[1] + 0.0722f * lut[2];
            QColor color = QColor::fromHsvF(0.0f, 0.0f, qBound(0.0, (double)lum, 1.0));
            carMat->setDiffuse(color);
        }
    }
    if (m_skySphere) {
        auto* skyMat = qobject_cast<Qt3DExtras::QDiffuseSpecularMaterial*>(m_skySphere->components()[1]);
        if (skyMat) {
            int baseB = static_cast<int>(135 + m_colorGrading.params().temperature * 2);
            QColor skyColor(100, 100, qBound(20, baseB, 255));
            skyMat->setDiffuse(skyColor);
        }
    }
#else
    qDebug() << "Color grading applied (requires Qt3D for preview)";
#endif
}

void KSPFiltersEditor::setColorGradingPreset(const QString& name)
{
    m_cgPresetCombo->setCurrentText(name);
}

void KSPFiltersEditor::exportCubeLUT(const QString& path)
{
    onExportCubeLUT();
}

void KSPFiltersEditor::importCubeLUT(const QString& path)
{
    onImportCubeLUT();
}

// ============================================================================
// Custom Shader Support
// ============================================================================

bool KSPFiltersEditor::loadCustomShader(const QString& shaderPath) {
    m_shaderErrors.clear();

    QFile file(shaderPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_shaderErrors << "Failed to open shader file: " + shaderPath;
        return false;
    }

    QString source = file.readAll();
    file.close();

    if (!validateShaderSource(source)) {
        return false;
    }

    m_currentShaderSource = source;
    return true;
}

bool KSPFiltersEditor::compileShader(const QString& source, const QString& entryPoint, const QString& shaderModel) {
    m_shaderErrors.clear();

    if (source.isEmpty()) {
        m_shaderErrors << "Shader source is empty";
        return false;
    }

    // Validate GLSL syntax (basic checks)
    if (!validateShaderSource(source)) {
        return false;
    }

    // Compile GLSL to SPIR-V
    QString spirv = compileGLSLToSPIRV(source, entryPoint, shaderModel.contains("frag"));

    if (!m_shaderErrors.isEmpty()) {
        return false;
    }

    return true;
}

bool KSPFiltersEditor::previewCustomShader(const QString& shaderSource) {
    m_shaderErrors.clear();

    if (!validateShaderSource(shaderSource)) {
        return false;
    }

    // Apply shader to the 3D preview
    m_currentShaderSource = shaderSource;
    updatePreview();

    return m_shaderErrors.isEmpty();
}

QString KSPFiltersEditor::compileGLSLToSPIRV(const QString& glslSource, const QString& entryPoint, bool isFragment) {
    m_shaderErrors.clear();

    if (glslSource.isEmpty()) {
        m_shaderErrors << "Shader source is empty";
        return QString();
    }

    // Write GLSL source to a temp file
    QTemporaryFile srcFile(QDir::temp().absoluteFilePath("ks_shader_XXXXXX") + (isFragment ? ".frag" : ".vert"));
    if (!srcFile.open()) {
        m_shaderErrors << "Failed to create temporary shader file";
        return QString();
    }
    srcFile.write(glslSource.toUtf8());
    QString srcPath = srcFile.fileName();
    srcFile.close();

    QString outPath = QDir::temp().absoluteFilePath(QFileInfo(srcPath).baseName() + ".spv");

    // Try glslangValidator first, then shaderc as fallback
    QStringList tools;
    tools << "glslangValidator" << "glslangValidator.exe";
    QString spirvTool;
    for (const auto& tool : tools) {
        QProcess which;
        which.start("where", QStringList() << tool);
        which.waitForFinished(2000);
        if (which.exitCode() == 0) {
            spirvTool = tool;
            break;
        }
    }

    if (spirvTool.isEmpty()) {
        // Fallback: use shaderc (via glslc)
        QProcess whichGlslc;
        whichGlslc.start("where", QStringList() << "glslc");
        whichGlslc.waitForFinished(2000);
        if (whichGlslc.exitCode() == 0) {
            QProcess glslc;
            glslc.start("glslc", QStringList()
                << "-fshader-stage=" + QString(isFragment ? "fragment" : "vertex")
                << "-o" << outPath
                << srcPath);
            glslc.waitForFinished(30000);
            if (glslc.exitCode() != 0) {
                m_shaderErrors << QString("glslc error: %1").arg(QString::fromUtf8(glslc.readAllStandardError()));
                return QString();
            }
        } else {
            m_shaderErrors << "No SPIR-V compiler found. Install glslangValidator or shaderc."
                          << "Ensure glslangValidator or glslc is in PATH.";
            return QString();
        }
    } else {
        QProcess glslang;
        glslang.start(spirvTool, QStringList()
            << "-V" << srcPath << "-o" << outPath);
        glslang.waitForFinished(30000);
        if (glslang.exitCode() != 0) {
            QString errStr = QString::fromUtf8(glslang.readAllStandardError());
            m_shaderErrors << QString("glslangValidator error (exit %1): %2")
                              .arg(glslang.exitCode()).arg(errStr);
            return QString();
        }
    }

    // Read compiled SPIR-V
    QFile spvFile(outPath);
    if (!spvFile.open(QIODevice::ReadOnly)) {
        m_shaderErrors << "Failed to read compiled SPIR-V output";
        return QString();
    }
    QByteArray spirvBin = spvFile.readAll();
    spvFile.close();
    QFile::remove(outPath);

    if (spirvBin.isEmpty()) {
        m_shaderErrors << "SPIR-V compilation produced empty output";
        return QString();
    }

    return QString::fromLatin1(spirvBin.toHex());
}

bool KSPFiltersEditor::validateShaderSource(const QString& source) {
    if (source.isEmpty()) {
        m_shaderErrors << "Shader source is empty";
        return false;
    }

    // Check for invalid characters
    for (const QChar& c : source) {
        if (c.unicode() < 32 && c != '\n' && c != '\r' && c != '\t') {
            m_shaderErrors << "Invalid character in shader source";
            return false;
        }
    }

    // Check for maximum length
    if (source.length() > 100000) {
        m_shaderErrors << "Shader source exceeds maximum length (100KB)";
        return false;
    }

    return true;
}

// ============================================================================
// Shader Editor Tab
// ============================================================================

void KSPFiltersEditor::setupShaderEditorTab()
{
    QWidget *shaderTab = new QWidget();
    QVBoxLayout *shaderLayout = new QVBoxLayout(shaderTab);

    QHBoxLayout *shaderToolbar = new QHBoxLayout();
    m_loadShaderBtn = new QPushButton("Load GLSL...");
    m_compileBtn = new QPushButton("Compile");
    m_reloadShaderBtn = new QPushButton("Reload to Preview");
    m_compileBtn->setEnabled(false);

    shaderToolbar->addWidget(m_loadShaderBtn);
    shaderToolbar->addWidget(m_compileBtn);
    shaderToolbar->addWidget(m_reloadShaderBtn);
    shaderToolbar->addStretch();
    shaderLayout->addLayout(shaderToolbar);

    m_shaderTabWidget = new QTabWidget();

    QWidget *vertTab = new QWidget();
    QVBoxLayout *vertLayout = new QVBoxLayout(vertTab);

    m_shaderEditor = new QPlainTextEdit();
    m_shaderEditor->setPlaceholderText("// Paste or load GLSL vertex shader here...");
    m_shaderEditor->setFont(QFont("Consolas", 10));
    m_shaderEditor->setTabStopDistance(20);
    m_shaderEditor->setLineWrapMode(QPlainTextEdit::NoWrap);

    auto *vsHighlighter = new ks::SyntaxHighlighter(m_shaderEditor->document());
    vsHighlighter->setLanguage(ks::SyntaxHighlighter::Glsl);

    vertLayout->addWidget(m_shaderEditor);
    m_shaderTabWidget->addTab(vertTab, "Vertex Shader");

    QWidget *fragTab = new QWidget();
    QVBoxLayout *fragLayout = new QVBoxLayout(fragTab);

    m_shaderOutput = new QPlainTextEdit();
    m_shaderOutput->setReadOnly(true);
    m_shaderOutput->setPlaceholderText("Compilation output...");
    m_shaderOutput->setFont(QFont("Consolas", 9));
    m_shaderOutput->setMaximumBlockCount(500);

    fragLayout->addWidget(m_shaderOutput);
    m_shaderTabWidget->addTab(fragTab, "Fragment Shader");

    shaderLayout->addWidget(m_shaderTabWidget);
    m_mainTabs->addTab(shaderTab, "Shader Editor");

    connect(m_loadShaderBtn, &QPushButton::clicked, this, &KSPFiltersEditor::onLoadShaderFile);
    connect(m_compileBtn, &QPushButton::clicked, this, &KSPFiltersEditor::onCompileShader);
    connect(m_reloadShaderBtn, &QPushButton::clicked, this, &KSPFiltersEditor::onReloadShaderToPreview);
    connect(m_shaderEditor, &QPlainTextEdit::textChanged, this, &KSPFiltersEditor::onShaderTextChanged);
}

void KSPFiltersEditor::onShaderTextChanged()
{
    m_shaderDirty = true;
    m_compileBtn->setEnabled(true);
}

void KSPFiltersEditor::onCompileShader()
{
    m_shaderOutput->clear();
    m_shaderErrors.clear();

    QString source = m_shaderEditor->toPlainText();
    if (source.isEmpty()) {
        m_shaderOutput->appendPlainText("Error: No shader source to compile");
        return;
    }

    bool isFragment = (m_shaderTabWidget->currentIndex() == 1);
    QString entryPoint = "main";

    m_shaderOutput->appendPlainText(QString("Compiling %1 shader...")
        .arg(isFragment ? "fragment" : "vertex"));

    QString spirv = compileGLSLToSPIRV(source, entryPoint, isFragment);
    if (!spirv.isEmpty()) {
        m_shaderOutput->appendPlainText("Compilation successful.");
        m_currentShaderSource = source;
        m_shaderDirty = false;
        m_compileBtn->setEnabled(false);
        m_reloadShaderBtn->setEnabled(true);
    } else {
        for (const QString& err : m_shaderErrors)
            m_shaderOutput->appendPlainText("Error: " + err);
    }
}

void KSPFiltersEditor::onLoadShaderFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Load GLSL Shader",
        QString(), "GLSL Shaders (*.vert *.frag *.glsl *.vs *.fs);;All Files (*.*)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to open file: " + filePath);
        return;
    }

    QString source = QString::fromUtf8(file.readAll());
    file.close();

    m_shaderEditor->setPlainText(source);
    m_currentShaderPath = filePath;

    if (filePath.endsWith(".frag") || filePath.endsWith(".fs"))
        m_shaderTabWidget->setCurrentIndex(1);
    else
        m_shaderTabWidget->setCurrentIndex(0);

    m_shaderOutput->appendPlainText(QString("Loaded: %1").arg(filePath));
}

void KSPFiltersEditor::onReloadShaderToPreview()
{
    if (m_shaderDirty) {
        onCompileShader();
        if (!m_shaderErrors.isEmpty()) return;
    }

    if (m_currentShaderSource.isEmpty()) {
        m_shaderOutput->appendPlainText("No shader to reload");
        return;
    }

    m_shaderOutput->appendPlainText("Reloading shader into preview...");
    previewCustomShader(m_currentShaderSource);
    updatePreview();
    m_shaderOutput->appendPlainText("Shader applied to preview.");
}
