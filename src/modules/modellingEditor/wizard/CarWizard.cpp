#include "CarWizard.h"
#include <QTableWidget>
#include <QMimeData>
#include <QScrollArea>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QCompleter>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QBrush>
#include <QColor>

namespace ks {

static QList<CarDatabaseEntry> initializeCarDatabase()
{
    QList<CarDatabaseEntry> database;
    
    { CarDatabaseEntry car;
        car.name = "Mercedes-AMG F1 W13"; car.manufacturer = "Mercedes"; car.category = "F1"; car.type = "Open Wheel";
        car.year = 2022; car.mass = 795; car.wheelbase = 3600; car.trackWidthFront = 1900; car.trackWidthRear = 1900;
        car.length = 4700; car.width = 2000; car.height = 950; car.engineConfig = "V8 Turbo";
        car.cylinderCount = 8; car.displacement = 1600; car.maxPower = 750; car.maxPowerRPM = 15000;
        car.maxTorque = 350; car.maxTorqueRPM = 12000; car.redlineRPM = 15000; car.turbocharged = true;
        car.topSpeed = 340; car.zeroTo100 = 2.6; car.downforceFront = 1350; car.downforceRear = 1800;
        car.dragCoefficient = 0.95; car.frontTyreSize = "13\""; car.rearTyreSize = "13\"";
        car.gearCount = 8; car.finalDrive = 3.5; car.description = "Mercedes Formula 1 car";
        database.append(car); }
    
    { CarDatabaseEntry car;
        car.name = "Red Bull Racing RB19"; car.manufacturer = "Red Bull"; car.category = "F1"; car.type = "Open Wheel";
        car.year = 2023; car.mass = 798; car.wheelbase = 3570; car.trackWidthFront = 1920; car.trackWidthRear = 1910;
        car.length = 4700; car.width = 2000; car.height = 940; car.engineConfig = "V8 Turbo";
        car.cylinderCount = 8; car.displacement = 1600; car.maxPower = 760; car.maxPowerRPM = 15000;
        car.maxTorque = 360; car.maxTorqueRPM = 12000; car.redlineRPM = 15000; car.turbocharged = true;
        car.topSpeed = 345; car.zeroTo100 = 2.5; car.downforceFront = 1400; car.downforceRear = 1850;
        car.dragCoefficient = 0.92; car.frontTyreSize = "13\""; car.rearTyreSize = "13\"";
        car.gearCount = 8; car.finalDrive = 3.4; car.description = "Dominant 2023 F1 car";
        database.append(car); }
    
    { CarDatabaseEntry car;
        car.name = "Porsche 911 GT3 R"; car.manufacturer = "Porsche"; car.category = "GT3"; car.type = "Closed Wheel";
        car.year = 2019; car.mass = 1245; car.wheelbase = 2459; car.trackWidthFront = 1680; car.trackWidthRear = 1630;
        car.length = 4560; car.width = 2038; car.height = 1194; car.engineConfig = "Flat-6";
        car.cylinderCount = 6; car.displacement = 3996; car.maxPower = 550; car.maxPowerRPM = 8750;
        car.maxTorque = 470; car.maxTorqueRPM = 6000; car.redlineRPM = 9000; car.turbocharged = false;
        car.topSpeed = 318; car.zeroTo100 = 3.4; car.downforceFront = 400; car.downforceRear = 500;
        car.dragCoefficient = 0.35; car.frontTyreSize = "18\""; car.rearTyreSize = "18\"";
        car.gearCount = 6; car.finalDrive = 3.97; car.description = "GT3 race car";
        database.append(car); }
    
    { CarDatabaseEntry car;
        car.name = "Ferrari 488 GT3 EVO"; car.manufacturer = "Ferrari"; car.category = "GT3"; car.type = "Closed Wheel";
        car.year = 2020; car.mass = 1260; car.wheelbase = 2650; car.trackWidthFront = 1740; car.trackWidthRear = 1670;
        car.length = 4710; car.width = 2030; car.height = 1160; car.engineConfig = "V8 Turbo";
        car.cylinderCount = 8; car.displacement = 3902; car.maxPower = 600; car.maxPowerRPM = 7500;
        car.maxTorque = 700; car.maxTorqueRPM = 3500; car.redlineRPM = 8000; car.turbocharged = true;
        car.topSpeed = 320; car.zeroTo100 = 3.1; car.downforceFront = 420; car.downforceRear = 530;
        car.dragCoefficient = 0.36; car.frontTyreSize = "18\""; car.rearTyreSize = "18\"";
        car.gearCount = 6; car.finalDrive = 4.0; car.description = "Evolved 488 GT3";
        database.append(car); }
    
    return database;
}

CarWizard::CarWizard(QWidget* parent)
    : QWizard(parent)
{
    setWindowTitle("Car Creation Wizard");
    setMinimumSize(800, 600);
    setWizardStyle(QWizard::ModernStyle);
    
    loadCarDatabase();
    setupPages();
    
    connect(this, &QWizard::currentIdChanged, this, [this](int id) {
        qDebug() << "[CarWizard] Current page changed to:" << id;
    });
}

CarWizard::~CarWizard() = default;

void CarWizard::loadCarDatabase()
{
    m_carDatabase = initializeCarDatabase();
    qDebug() << "Loaded" << m_carDatabase.size() << "cars from database";
}

void CarWizard::setupPages()
{
    setPage(Page_Category, new CategoryPage(this));
    setPage(Page_Engine, new EnginePage(this));
    setPage(Page_Details, new CarDetailsPage(this));
    setPage(Page_Physics, new PhysicsPage(this));
    setPage(Page_Sound, new SoundPage(this));
    setPage(Page_FBX, new FBXPage(this));
    setPage(Page_Review, new ReviewPage(this));
    
    connect(this, &QWizard::accepted, this, [this]() {
        FBXPage* fbxPage = qobject_cast<FBXPage*>(page(Page_FBX));
        if (fbxPage) {
            m_carEntry.fbxPath = fbxPage->fbxPath();
            m_carEntry.hasFbx = fbxPage->hasFbx();
        }
        ReviewPage* reviewPage = qobject_cast<ReviewPage*>(page(Page_Review));
        if (reviewPage) {
            reviewPage->setCarEntry(m_carEntry);
        }
    });
}

void CarWizard::setFbxPath(const QString& path, bool hasFbx)
{
    m_carEntry.fbxPath = path;
    m_carEntry.hasFbx = hasFbx;
}

void CarWizard::filterCarList(const QString& text) {
    m_filteredCars.clear();
    for (const auto& car : m_carDatabase) {
        if (car.name.contains(text, Qt::CaseInsensitive) ||
            car.manufacturer.contains(text, Qt::CaseInsensitive)) {
            m_filteredCars.append(car);
        }
    }
    emit carListFiltered(m_filteredCars);
}

void CarWizard::onCategorySelected(const QString& category) {
    m_selectedCategory = category;
    emit categorySelected(category);
}

void CarWizard::onCarNameChanged(const QString& name) {
    m_carEntry.name = name;
    emit carNameChanged(name);
}

void CarWizard::searchCar(const QString& partialName) {
    filterCarList(partialName);
}

void CarWizard::onCarSelected(int row) {
    if (row >= 0 && row < m_filteredCars.size()) {
        m_carEntry = m_filteredCars[row];
        emit carSelected(m_carEntry);
    }
}

void CarWizard::populateFromDatabase(const CarDatabaseEntry& entry) {
    m_carEntry = entry;
    emit carCreated(m_carEntry);
}

CategoryPage::CategoryPage(QWidget* parent)
    : QWizardPage(parent)
{
    setTitle("Select Car Category");
    setSubTitle("Choose the type and category of car");
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* typeGroup = new QGroupBox("Body Type");
    QVBoxLayout* typeLayout = new QVBoxLayout(typeGroup);
    
    QRadioButton* openWheelBtn = new QRadioButton("Open Wheel");
    openWheelBtn->setChecked(true);
    QRadioButton* closedWheelBtn = new QRadioButton("Closed Wheel / GT");
    
    m_typeGroup = new QButtonGroup(this);
    m_typeGroup->addButton(openWheelBtn, 0);
    m_typeGroup->addButton(closedWheelBtn, 1);
    
    typeLayout->addWidget(openWheelBtn);
    typeLayout->addWidget(closedWheelBtn);
    
    QGroupBox* categoryGroup = new QGroupBox("Racing Category");
    QGridLayout* categoryLayout = new QGridLayout(categoryGroup);
    
    QStringList categories = { "F1", "IndyCar", "Formula 2", "Formula 3", "GT3", "GT4", "LMP2", "Hypercar", "Touring", "Road" };
    
    m_categoryGroup = new QButtonGroup(this);
    for (int i = 0; i < categories.size(); ++i) {
        QRadioButton* btn = new QRadioButton(categories[i]);
        m_categoryGroup->addButton(btn, i);
        categoryLayout->addWidget(btn, i / 3, i % 3);
    }
    
    registerField("carType", openWheelBtn);
    registerField("carCategory", categoryGroup->findChild<QRadioButton*>());
    
    mainLayout->addWidget(typeGroup);
    mainLayout->addWidget(categoryGroup);
    mainLayout->addStretch();
}

QString CategoryPage::selectedCategory() const
{
    QAbstractButton* btn = m_categoryGroup->checkedButton();
    return btn ? btn->text() : QString();
}

QString CategoryPage::selectedType() const
{
    return m_typeGroup->checkedId() == 0 ? "Open Wheel" : "Closed Wheel";
}

EnginePage::EnginePage(QWidget* parent)
    : QWizardPage(parent)
{
    setTitle("Engine Configuration");
    setSubTitle("Select the engine configuration");
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* engineGroup = new QGroupBox("Engine Type");
    QFormLayout* engineLayout = new QFormLayout(engineGroup);
    
    m_engineConfigCombo = new QComboBox();
    m_engineConfigCombo->addItems({ "Inline-4", "Inline-6", "V6", "V6 Turbo", "V8", "V8 Turbo", "V10", "V12", "Flat-4", "Flat-6" });
    
    m_cylinderSpin = new QSpinBox();
    m_cylinderSpin->setRange(4, 12);
    m_cylinderSpin->setValue(8);
    
    m_displacementSpin = new QDoubleSpinBox();
    m_displacementSpin->setRange(1.0, 10.0);
    m_displacementSpin->setValue(4.0);
    m_displacementSpin->setSuffix(" L");
    m_displacementSpin->setDecimals(1);
    
    engineLayout->addRow("Configuration:", m_engineConfigCombo);
    engineLayout->addRow("Cylinders:", m_cylinderSpin);
    engineLayout->addRow("Displacement:", m_displacementSpin);
    
    mainLayout->addWidget(engineGroup);
    mainLayout->addStretch();
    
    registerField("engineConfig*", m_engineConfigCombo);
}

QString EnginePage::engineConfig() const { return m_engineConfigCombo->currentText(); }
int EnginePage::cylinderCount() const { return m_cylinderSpin->value(); }
double EnginePage::displacement() const { return m_displacementSpin->value(); }

CarDetailsPage::CarDetailsPage(QWidget* parent)
    : QWizardPage(parent)
{
    setTitle("Car Details");
    setSubTitle("Enter car name or search");
    
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    
    QWidget* searchPanel = new QWidget();
    QVBoxLayout* searchLayout = new QVBoxLayout(searchPanel);
    
    searchLayout->addWidget(new QLabel("Search Car:"));
    
    QHBoxLayout* searchInputLayout = new QHBoxLayout();
    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("e.g., Porsche 911 GT3");
    QPushButton* searchBtn = new QPushButton("Search");
    searchInputLayout->addWidget(m_nameEdit);
    searchInputLayout->addWidget(searchBtn);
    searchLayout->addLayout(searchInputLayout);
    
    searchLayout->addWidget(new QLabel("Search Results:"));
    m_searchResults = new QListWidget();
    m_searchResults->setMinimumHeight(200);
    searchLayout->addWidget(m_searchResults);
    
    m_carImageLabel = new QLabel();
    m_carImageLabel->setMinimumHeight(120);
    m_carImageLabel->setAlignment(Qt::AlignCenter);
    m_carImageLabel->setStyleSheet("border: 1px solid #ccc; background: #f0f0f0;");
    m_carImageLabel->setText("No image");
    searchLayout->addWidget(m_carImageLabel);
    
    QScrollArea* scrollArea = new QScrollArea();
    QWidget* formWidget = new QWidget();
    QFormLayout* formLayout = new QFormLayout(formWidget);
    
    QGroupBox* basicGroup = new QGroupBox("Basic Information");
    QFormLayout* basicLayout = new QFormLayout(basicGroup);
    
    m_manufacturerEdit = new QLineEdit();
    m_yearSpin = new QSpinBox();
    m_yearSpin->setRange(1950, 2025);
    m_yearSpin->setValue(2024);
    
    basicLayout->addRow("Manufacturer:", m_manufacturerEdit);
    basicLayout->addRow("Year:", m_yearSpin);
    
    QGroupBox* physicalGroup = new QGroupBox("Physical Specifications");
    QFormLayout* physicalLayout = new QFormLayout(physicalGroup);
    
    m_massSpin = new QDoubleSpinBox(); m_massSpin->setRange(500, 2000); m_massSpin->setValue(1200); m_massSpin->setSuffix(" kg"); m_massSpin->setDecimals(0);
    m_wheelbaseSpin = new QDoubleSpinBox(); m_wheelbaseSpin->setRange(2000, 4000); m_wheelbaseSpin->setValue(2700); m_wheelbaseSpin->setSuffix(" mm"); m_wheelbaseSpin->setDecimals(0);
    m_lengthSpin = new QDoubleSpinBox(); m_lengthSpin->setRange(3500, 5500); m_lengthSpin->setValue(4700); m_lengthSpin->setSuffix(" mm"); m_lengthSpin->setDecimals(0);
    m_widthSpin = new QDoubleSpinBox(); m_widthSpin->setRange(1500, 2500); m_widthSpin->setValue(2000); m_widthSpin->setSuffix(" mm"); m_widthSpin->setDecimals(0);
    m_heightSpin = new QDoubleSpinBox(); m_heightSpin->setRange(900, 2000); m_heightSpin->setValue(1200); m_heightSpin->setSuffix(" mm"); m_heightSpin->setDecimals(0);
    m_trackFrontSpin = new QDoubleSpinBox(); m_trackFrontSpin->setRange(1200, 2200); m_trackFrontSpin->setValue(1700); m_trackFrontSpin->setSuffix(" mm"); m_trackFrontSpin->setDecimals(0);
    m_trackRearSpin = new QDoubleSpinBox(); m_trackRearSpin->setRange(1200, 2200); m_trackRearSpin->setValue(1650); m_trackRearSpin->setSuffix(" mm"); m_trackRearSpin->setDecimals(0);
    
    physicalLayout->addRow("Mass:", m_massSpin);
    physicalLayout->addRow("Wheelbase:", m_wheelbaseSpin);
    physicalLayout->addRow("Length:", m_lengthSpin);
    physicalLayout->addRow("Width:", m_widthSpin);
    physicalLayout->addRow("Height:", m_heightSpin);
    physicalLayout->addRow("Track (F):", m_trackFrontSpin);
    physicalLayout->addRow("Track (R):", m_trackRearSpin);
    
    QGroupBox* engineGroup = new QGroupBox("Engine Specifications");
    QFormLayout* engineLayout = new QFormLayout(engineGroup);
    
    m_powerSpin = new QSpinBox(); m_powerSpin->setRange(100, 1000); m_powerSpin->setValue(550); m_powerSpin->setSuffix(" HP");
    m_maxPowerRPMSpin = new QSpinBox(); m_maxPowerRPMSpin->setRange(4000, 15000); m_maxPowerRPMSpin->setValue(7500); m_maxPowerRPMSpin->setSuffix(" RPM");
    m_torqueSpin = new QSpinBox(); m_torqueSpin->setRange(100, 1000); m_torqueSpin->setValue(500); m_torqueSpin->setSuffix(" Nm");
    m_maxTorqueRPMSpin = new QSpinBox(); m_maxTorqueRPMSpin->setRange(2000, 12000); m_maxTorqueRPMSpin->setValue(5500); m_maxTorqueRPMSpin->setSuffix(" RPM");
    m_redlineSpin = new QSpinBox(); m_redlineSpin->setRange(5000, 20000); m_redlineSpin->setValue(8500); m_redlineSpin->setSuffix(" RPM");
    m_turboCheck = new QCheckBox("Turbocharged");
    
    engineLayout->addRow("Max Power:", m_powerSpin);
    engineLayout->addRow("Power RPM:", m_maxPowerRPMSpin);
    engineLayout->addRow("Max Torque:", m_torqueSpin);
    engineLayout->addRow("Torque RPM:", m_maxTorqueRPMSpin);
    engineLayout->addRow("Redline:", m_redlineSpin);
    engineLayout->addRow("", m_turboCheck);
    
    QGroupBox* aeroGroup = new QGroupBox("Aerodynamics");
    QFormLayout* aeroLayout = new QFormLayout(aeroGroup);
    
    m_downforceFrontSpin = new QDoubleSpinBox(); m_downforceFrontSpin->setRange(0, 2000); m_downforceFrontSpin->setValue(400); m_downforceFrontSpin->setSuffix(" N"); m_downforceFrontSpin->setDecimals(0);
    m_downforceRearSpin = new QDoubleSpinBox(); m_downforceRearSpin->setRange(0, 3000); m_downforceRearSpin->setValue(500); m_downforceRearSpin->setSuffix(" N"); m_downforceRearSpin->setDecimals(0);
    m_dragSpin = new QDoubleSpinBox(); m_dragSpin->setRange(0.2, 1.5); m_dragSpin->setValue(0.35); m_dragSpin->setDecimals(2);
    
    aeroLayout->addRow("Downforce (F):", m_downforceFrontSpin);
    aeroLayout->addRow("Downforce (R):", m_downforceRearSpin);
    aeroLayout->addRow("Drag Coeff:", m_dragSpin);
    
    QGroupBox* transGroup = new QGroupBox("Transmission");
    QFormLayout* transLayout = new QFormLayout(transGroup);
    
    m_gearCountSpin = new QSpinBox(); m_gearCountSpin->setRange(4, 10); m_gearCountSpin->setValue(6);
    m_finalDriveSpin = new QDoubleSpinBox(); m_finalDriveSpin->setRange(2.0, 5.0); m_finalDriveSpin->setValue(3.7); m_finalDriveSpin->setDecimals(2);
    
    transLayout->addRow("Gear Count:", m_gearCountSpin);
    transLayout->addRow("Final Drive:", m_finalDriveSpin);
    
    formLayout->addRow(basicGroup);
    formLayout->addRow(physicalGroup);
    formLayout->addRow(engineGroup);
    formLayout->addRow(aeroGroup);
    formLayout->addRow(transGroup);
    
    scrollArea->setWidget(formWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumWidth(400);
    
    mainLayout->addWidget(searchPanel, 1);
    mainLayout->addWidget(scrollArea, 2);
    
    connect(searchBtn, &QPushButton::clicked, this, &CarDetailsPage::onSearchClicked);
    connect(m_nameEdit, &QLineEdit::returnPressed, this, &CarDetailsPage::onSearchClicked);
}

void CarDetailsPage::onSearchClicked()
{
    QString searchText = m_nameEdit->text().trimmed();
    if (searchText.isEmpty()) return;
    
    CarWizard* wizard = qobject_cast<CarWizard*>(QWizardPage::wizard());
    if (!wizard) return;
    
    const QList<CarDatabaseEntry>& database = initializeCarDatabase();
    
    m_searchResults->clear();
    
    for (const CarDatabaseEntry& car : database) {
        if (car.name.contains(searchText, Qt::CaseInsensitive) ||
            car.manufacturer.contains(searchText, Qt::CaseInsensitive) ||
            car.category.contains(searchText, Qt::CaseInsensitive)) {
            
            QString displayText = QString("%1 (%2) - %3").arg(car.name).arg(car.year).arg(car.category);
            QListWidgetItem* item = new QListWidgetItem(displayText);
            item->setData(Qt::UserRole, QVariant::fromValue(car));
            m_searchResults->addItem(item);
        }
    }
    
    if (m_searchResults->count() == 0) {
        m_searchResults->addItem("No matches found.");
    }
}

void CarDetailsPage::setCarEntry(const CarDatabaseEntry& entry)
{
    m_carEntry = entry;
    populateFields(entry);
}

void CarDetailsPage::populateFields(const CarDatabaseEntry& entry)
{
    m_nameEdit->setText(entry.name);
    m_manufacturerEdit->setText(entry.manufacturer);
    m_yearSpin->setValue(entry.year);
    m_massSpin->setValue(entry.mass);
    m_wheelbaseSpin->setValue(entry.wheelbase);
    m_lengthSpin->setValue(entry.length);
    m_widthSpin->setValue(entry.width);
    m_heightSpin->setValue(entry.height);
    m_trackFrontSpin->setValue(entry.trackWidthFront);
    m_trackRearSpin->setValue(entry.trackWidthRear);
    m_powerSpin->setValue(entry.maxPower);
    m_maxPowerRPMSpin->setValue(entry.maxPowerRPM);
    m_torqueSpin->setValue(entry.maxTorque);
    m_maxTorqueRPMSpin->setValue(entry.maxTorqueRPM);
    m_redlineSpin->setValue(entry.redlineRPM);
    m_turboCheck->setChecked(entry.turbocharged);
    m_downforceFrontSpin->setValue(entry.downforceFront);
    m_downforceRearSpin->setValue(entry.downforceRear);
    m_dragSpin->setValue(entry.dragCoefficient);
    m_gearCountSpin->setValue(entry.gearCount);
    m_finalDriveSpin->setValue(entry.finalDrive);
}

PhysicsPage::PhysicsPage(QWidget* parent)
    : QWizardPage(parent)
{
    setTitle("Physics Configuration");
    setSubTitle("Configure vehicle physics parameters");
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* suspensionGroup = new QGroupBox("Suspension");
    QFormLayout* suspensionLayout = new QFormLayout(suspensionGroup);
    
    m_suspensionSpin = new QDoubleSpinBox(); m_suspensionSpin->setRange(10000, 100000); m_suspensionSpin->setValue(45000); m_suspensionSpin->setSuffix(" N/m"); m_suspensionSpin->setDecimals(0);
    m_dampingSpin = new QDoubleSpinBox(); m_dampingSpin->setRange(500, 10000); m_dampingSpin->setValue(3000); m_dampingSpin->setSuffix(" Ns/m"); m_dampingSpin->setDecimals(0);
    
    suspensionLayout->addRow("Stiffness:", m_suspensionSpin);
    suspensionLayout->addRow("Damping:", m_dampingSpin);
    
    QGroupBox* aeroGroup = new QGroupBox("Aerodynamics");
    QFormLayout* aeroLayout = new QFormLayout(aeroGroup);
    
    m_downforceSpin = new QDoubleSpinBox(); m_downforceSpin->setRange(0, 5000); m_downforceSpin->setValue(1500); m_downforceSpin->setSuffix(" N"); m_downforceSpin->setDecimals(0);
    m_dragSpin = new QDoubleSpinBox(); m_dragSpin->setRange(0.2, 2.0); m_dragSpin->setValue(0.9); m_dragSpin->setDecimals(2);
    
    aeroLayout->addRow("Downforce:", m_downforceSpin);
    aeroLayout->addRow("Drag:", m_dragSpin);
    
    QGroupBox* massGroup = new QGroupBox("Mass Distribution");
    QFormLayout* massLayout = new QFormLayout(massGroup);
    
    m_massScaleSpin = new QDoubleSpinBox(); m_massScaleSpin->setRange(0.5, 2.0); m_massScaleSpin->setValue(1.0); m_massScaleSpin->setDecimals(2);
    
    massLayout->addRow("Mass Scale:", m_massScaleSpin);
    
    mainLayout->addWidget(suspensionGroup);
    mainLayout->addWidget(aeroGroup);
    mainLayout->addWidget(massGroup);
    mainLayout->addStretch();
}

SoundPage::SoundPage(QWidget* parent)
    : QWizardPage(parent)
{
    setTitle("Sound Configuration");
    setSubTitle("Configure engine sound settings");
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* presetGroup = new QGroupBox("Sound Preset");
    QFormLayout* presetLayout = new QFormLayout(presetGroup);
    
    m_presetCombo = new QComboBox();
    m_presetCombo->addItems({ "GT3 V8", "GT3 Flat-6", "F1 V8 Turbo", "Formula", "Touring Car", "Road Car" });
    presetLayout->addRow("Preset:", m_presetCombo);
    
    QGroupBox* settingsGroup = new QGroupBox("Audio Settings");
    QFormLayout* settingsLayout = new QFormLayout(settingsGroup);
    
    m_sampleRateCombo = new QComboBox();
    m_sampleRateCombo->addItems({ "44100 Hz", "48000 Hz" });
    settingsLayout->addRow("Sample Rate:", m_sampleRateCombo);
    
    QHBoxLayout* outputLayout = new QHBoxLayout();
    m_outputDirEdit = new QLineEdit();
    m_outputDirEdit->setPlaceholderText("Select output folder...");
    m_browseBtn = new QPushButton("Browse...");
    outputLayout->addWidget(m_outputDirEdit);
    outputLayout->addWidget(m_browseBtn);
    settingsLayout->addRow("Output:", outputLayout);
    
    mainLayout->addWidget(presetGroup);
    mainLayout->addWidget(settingsGroup);
    mainLayout->addStretch();
    
    connect(m_browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Output Directory");
        if (!dir.isEmpty()) {
            m_outputDirEdit->setText(dir);
        }
    });
}

FBXPage::FBXPage(QWidget* parent)
    : QWizardPage(parent)
{
    setTitle("Load 3D Model");
    setSubTitle("Select an FBX model (optional)");
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QLabel* infoLabel = new QLabel("Load an FBX file for the car mesh.");
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);
    
    QGroupBox* fbxGroup = new QGroupBox("FBX Model");
    QHBoxLayout* fbxLayout = new QHBoxLayout(fbxGroup);
    
    m_fbxPathEdit = new QLineEdit();
    m_fbxPathEdit->setPlaceholderText("Select FBX file...");
    m_fbxPathEdit->setReadOnly(true);
    
    m_browseBtn = new QPushButton("Browse...");
    fbxLayout->addWidget(m_fbxPathEdit);
    fbxLayout->addWidget(m_browseBtn);
    
    mainLayout->addWidget(fbxGroup);
    
    m_previewGroup = new QGroupBox("Model Preview");
    QVBoxLayout* previewLayout = new QVBoxLayout(m_previewGroup);
    
    m_previewLabel = new QLabel("No model loaded");
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumHeight(200);
    m_previewLabel->setStyleSheet("border: 2px dashed #aaa; background: #f5f5f5; color: #666;");
    previewLayout->addWidget(m_previewLabel);
    
    mainLayout->addWidget(m_previewGroup);
    
    m_statusLabel = new QLabel("Status: No model selected");
    m_statusLabel->setStyleSheet("color: #666; font-style: italic;");
    mainLayout->addWidget(m_statusLabel);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* skipBtn = new QPushButton("Skip");
    QPushButton* validateBtn = new QPushButton("Validate");
    btnLayout->addWidget(skipBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(validateBtn);
    mainLayout->addLayout(btnLayout);
    
    mainLayout->addStretch();
    
    connect(m_browseBtn, &QPushButton::clicked, this, &FBXPage::onBrowseClicked);
    connect(validateBtn, &QPushButton::clicked, this, &FBXPage::validateFbx);
    connect(skipBtn, &QPushButton::clicked, this, [this]() {
        m_hasFbx = false;
        m_fbxPath.clear();
        m_statusLabel->setText("Status: Using placeholder");
        m_statusLabel->setStyleSheet("color: #f39c12;");
    });
    
    setAcceptDrops(true);
}

FBXPage::~FBXPage() = default;

void FBXPage::onBrowseClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select FBX Model", QDir::homePath(), "FBX Files (*.fbx);;All Files (*)");
    if (!filePath.isEmpty()) {
        m_fbxPath = filePath;
        m_fbxPathEdit->setText(filePath);
        validateFbx();
    }
}

void FBXPage::validateFbx()
{
    if (m_fbxPath.isEmpty()) {
        m_statusLabel->setText("Status: Select an FBX file");
        m_statusLabel->setStyleSheet("color: #e74c3c;");
        return;
    }
    
    QFileInfo fileInfo(m_fbxPath);
    
    if (!fileInfo.exists()) {
        m_statusLabel->setText("Status: File not found!");
        m_statusLabel->setStyleSheet("color: #e74c3c;");
        return;
    }
    
    m_hasFbx = true;
    m_previewLabel->setText(QString("<b>%1</b><br>Size: %2").arg(fileInfo.fileName()).arg(formatFileSize(fileInfo.size())));
    m_previewLabel->setStyleSheet("border: 2px solid #27ae60; background: #e8f5e9; color: #2e7d32;");
    m_statusLabel->setText("Status: Ready");
    m_statusLabel->setStyleSheet("color: #27ae60;");
}

QString FBXPage::formatFileSize(qint64 bytes)
{
    const QStringList sizes = {"B", "KB", "MB", "GB"};
    double size = bytes;
    int i = 0;
    while (size >= 1024 && i < sizes.size() - 1) { size /= 1024; i++; }
    return QString("%1 %2").arg(size, 0, 'f', 1).arg(sizes[i]);
}

void FBXPage::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        QUrl url = event->mimeData()->urls().first();
        if (url.toLocalFile().toLower().endsWith(".fbx")) {
            event->acceptProposedAction();
        }
    }
}

void FBXPage::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        QString filePath = event->mimeData()->urls().first().toLocalFile();
        if (filePath.toLower().endsWith(".fbx")) {
            m_fbxPath = filePath;
            m_fbxPathEdit->setText(filePath);
            validateFbx();
            event->acceptProposedAction();
        }
    }
}

ReviewPage::ReviewPage(QWidget* parent)
    : QWizardPage(parent)
{
    setTitle("Review & Summary");
    setSubTitle("Review car configuration");
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    m_summaryTable = new QTableWidget(0, 2);
    m_summaryTable->setHorizontalHeaderLabels({"Property", "Value"});
    m_summaryTable->horizontalHeader()->setStretchLastSection(true);
    m_summaryTable->verticalHeader()->setVisible(false);
    m_summaryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    mainLayout->addWidget(m_summaryTable);
}

void ReviewPage::setCarEntry(const CarDatabaseEntry& entry)
{
    m_summaryTable->setRowCount(0);
    
    QList<QPair<QString, QString>> items = {
        {"Name", entry.name},
        {"Manufacturer", entry.manufacturer},
        {"Category", entry.category},
        {"Year", QString::number(entry.year)},
        {"Mass", QString("%1 kg").arg(entry.mass)},
        {"Engine", entry.engineConfig},
        {"Power", QString("%1 HP @ %2 RPM").arg(entry.maxPower).arg(entry.maxPowerRPM)},
        {"Redline", QString("%1 RPM").arg(entry.redlineRPM)},
        {"Gears", QString::number(entry.gearCount)},
        {"FBX", entry.hasFbx ? QFileInfo(entry.fbxPath).fileName() : "None"}
    };
    
    for (const auto& item : items) {
        int row = m_summaryTable->rowCount();
        m_summaryTable->insertRow(row);
        m_summaryTable->setItem(row, 0, new QTableWidgetItem(item.first));
        m_summaryTable->setItem(row, 1, new QTableWidgetItem(item.second));
    }
}

}