#include "TrackWizard.h"
#include <QDebug>
#include <QFileInfo>

namespace ks {

// ─── TrackWizard ─────────────────────────────────────────────────────────────

TrackWizard::TrackWizard(QWidget* parent)
    : QWizard(parent)
{
    setWindowTitle("Track Creation Wizard");
    setMinimumSize(700, 500);
    setWizardStyle(QWizard::ModernStyle);

    loadTrackDatabase();
    setupPages();
}

TrackWizard::~TrackWizard() = default;

void TrackWizard::loadTrackDatabase()
{
    // Pre-populate with well-known real-world circuits as reference data.
    // Kept in memory — no external DB dependency required.
    m_trackDatabase.clear();

    auto add = [&](const char* name, const char* loc, const char* country,
                   const char* cat, double len, double width, int corners,
                   double elev, const char* surface, double grip) {
        TrackDatabaseEntry e{};
        e.name     = name;
        e.location = loc;
        e.country  = country;
        e.category = cat;
        e.length   = len;
        e.width    = width;
        e.corners  = corners;
        e.elevationChange = elev;
        e.surfaceType = surface;
        e.gripLevel = grip;
        e.downforceMultiplier = 1.0;
        e.dragMultiplier      = 1.0;
        e.weatherPreset = "Clear";
        e.timeOfDay     = "Midday";
        e.ambientSoundPreset = "Rural Countryside";
        m_trackDatabase.append(e);
    };

    add("Monza",           "Monza",       "Italy",           "Grand Prix Circuit", 5.793, 14.0, 11, 26,  "Historic Asphalt", 0.95);
    add("Silverstone",     "Silverstone", "United Kingdom",  "Grand Prix Circuit", 5.891, 15.0, 18, 40,  "Asphalt",          1.00);
    add("Spa-Francorchamps","Stavelot",   "Belgium",         "Grand Prix Circuit", 7.004, 14.0, 19, 100, "Asphalt",          0.98);
    add("Nürburgring GP",  "Nürburg",     "Germany",         "Grand Prix Circuit", 5.148, 14.0, 15, 60,  "Asphalt",          1.00);
    add("Suzuka",          "Suzuka",      "Japan",           "Grand Prix Circuit", 5.807, 15.0, 18, 40,  "Asphalt",          1.00);
    add("Circuit de Monaco","Monte-Carlo","Monaco",          "Street Circuit",     3.337, 9.0,  19, 42,  "Asphalt",          0.90);
    add("Laguna Seca",     "Salinas",     "United States",   "Permanent Circuit",  3.602, 12.0, 11, 60,  "Asphalt",          0.98);
    add("Brands Hatch GP", "West Kingsdown","United Kingdom","Permanent Circuit",  3.908, 12.0, 12, 70,  "Asphalt",          0.97);
    add("Mugello",         "Scarperia",   "Italy",           "Permanent Circuit",  5.245, 14.0, 15, 80,  "Asphalt",          0.99);
    add("Road Atlanta",    "Braselton",   "United States",   "Permanent Circuit",  4.168, 12.0, 12, 45,  "Asphalt",          0.97);
    add("Bathurst",        "Bathurst",    "Australia",       "Permanent Circuit",  6.213, 12.0, 23, 174, "Asphalt",          0.96);
    add("Zandvoort",       "Zandvoort",   "Netherlands",     "Grand Prix Circuit", 4.259, 12.0, 14, 32,  "Asphalt",          1.00);

    qDebug() << "TrackWizard: loaded" << m_trackDatabase.size() << "reference tracks";
}

void TrackWizard::setupPages()
{
    setPage(Page_Location, new LocationPage(this));
    setPage(Page_Physics,  new TrackPhysicsPage(this));
    setPage(Page_Ambience, new AmbiencePage(this));
    setPage(Page_Review,   new TrackReviewPage(this));

    // Collect data from all pages when the user clicks Finish.
    connect(this, &QWizard::accepted, this, [this]() {
        auto* loc  = qobject_cast<LocationPage*>(page(Page_Location));
        auto* phys = qobject_cast<TrackPhysicsPage*>(page(Page_Physics));
        auto* amb  = qobject_cast<AmbiencePage*>(page(Page_Ambience));
        auto* rev  = qobject_cast<TrackReviewPage*>(page(Page_Review));

        if (loc) {
            m_trackEntry.name     = loc->trackName();
            m_trackEntry.location = loc->location();
            m_trackEntry.country  = loc->country();
            m_trackEntry.category = loc->category();
        }
        if (phys) {
            m_trackEntry.length           = phys->trackLength();
            m_trackEntry.width            = phys->trackWidth();
            m_trackEntry.corners          = phys->cornerCount();
            m_trackEntry.elevationChange  = phys->elevationChange();
            m_trackEntry.surfaceType      = phys->surfaceType();
            m_trackEntry.gripLevel        = phys->gripLevel();
            m_trackEntry.downforceMultiplier = phys->downforceMultiplier();
            m_trackEntry.dragMultiplier      = phys->dragMultiplier();
        }
        if (amb) {
            m_trackEntry.ambientSoundPreset = amb->ambientSoundPreset();
            m_trackEntry.weatherPreset      = amb->weatherPreset();
            m_trackEntry.timeOfDay          = amb->timeOfDay();
            m_trackEntry.skyboxPath         = amb->skyboxPath();
            m_trackEntry.hasSkybox          = !amb->skyboxPath().isEmpty();
        }
        if (rev) {
            rev->setTrackEntry(m_trackEntry);
        }

        emit trackCreated(m_trackEntry);
        qDebug() << "TrackWizard: created track" << m_trackEntry.name;
    });
}

// ─── LocationPage ────────────────────────────────────────────────────────────

LocationPage::LocationPage(QWidget* parent)
    : QWizardPage(parent)
{
    setTitle("Track Location");
    setSubTitle("Enter location information for the track");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QGroupBox* basicGroup  = new QGroupBox("Basic Information");
    QFormLayout* basicLayout = new QFormLayout(basicGroup);

    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("e.g., Silverstone Circuit");
    basicLayout->addRow("Track Name:", m_nameEdit);

    m_locationEdit = new QLineEdit();
    m_locationEdit->setPlaceholderText("e.g., Silverstone, Northamptonshire");
    basicLayout->addRow("Location:", m_locationEdit);

    m_countryCombo = new QComboBox();
    m_countryCombo->addItems({
        "United Kingdom", "Germany", "Italy", "France", "Spain",
        "United States", "Japan", "Australia", "Brazil", "Monaco",
        "Belgium", "Austria", "Hungary", "Netherlands", "Other"
    });
    basicLayout->addRow("Country:", m_countryCombo);

    m_categoryCombo = new QComboBox();
    m_categoryCombo->addItems({
        "Grand Prix Circuit", "Street Circuit", "Permanent Circuit",
        "Temporary Circuit", "Drag Strip", "Oval", "Road Course", "Hillclimb"
    });
    basicLayout->addRow("Category:", m_categoryCombo);

    mainLayout->addWidget(basicGroup);
    mainLayout->addStretch();

    registerField("trackName*", m_nameEdit);
}

// ─── TrackPhysicsPage ────────────────────────────────────────────────────────

TrackPhysicsPage::TrackPhysicsPage(QWidget* parent)
    : QWizardPage(parent)
{
    setTitle("Track Physics");
    setSubTitle("Configure track dimensions and physics parameters");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Dimensions
    QGroupBox* dimGroup   = new QGroupBox("Dimensions");
    QFormLayout* dimLayout = new QFormLayout(dimGroup);

    m_lengthSpin = new QDoubleSpinBox(); m_lengthSpin->setRange(0.5, 30.0);  m_lengthSpin->setValue(5.0);  m_lengthSpin->setSuffix(" km"); m_lengthSpin->setDecimals(3);
    m_widthSpin  = new QDoubleSpinBox(); m_widthSpin->setRange(6.0, 25.0);   m_widthSpin->setValue(12.0);  m_widthSpin->setSuffix(" m");  m_widthSpin->setDecimals(1);
    m_cornersSpin = new QSpinBox();       m_cornersSpin->setRange(0, 60);      m_cornersSpin->setValue(18);
    m_elevationSpin = new QDoubleSpinBox(); m_elevationSpin->setRange(-200.0, 1000.0); m_elevationSpin->setValue(50.0); m_elevationSpin->setSuffix(" m"); m_elevationSpin->setDecimals(0);

    dimLayout->addRow("Length:",           m_lengthSpin);
    dimLayout->addRow("Width:",            m_widthSpin);
    dimLayout->addRow("Corner count:",     m_cornersSpin);
    dimLayout->addRow("Elevation change:", m_elevationSpin);

    // Surface
    QGroupBox* surfGroup   = new QGroupBox("Surface");
    QFormLayout* surfLayout = new QFormLayout(surfGroup);

    m_surfaceCombo = new QComboBox();
    m_surfaceCombo->addItems({ "Asphalt", "Concrete", "Mixed Asphalt/Concrete", "Historic Asphalt" });
    m_gripSpin = new QDoubleSpinBox(); m_gripSpin->setRange(0.5, 1.5); m_gripSpin->setValue(1.0); m_gripSpin->setDecimals(2);

    surfLayout->addRow("Surface type:", m_surfaceCombo);
    surfLayout->addRow("Grip level:",   m_gripSpin);

    // Aerodynamic effect multipliers
    QGroupBox* aeroGroup   = new QGroupBox("Aerodynamic multipliers");
    QFormLayout* aeroLayout = new QFormLayout(aeroGroup);

    m_downforceSpin = new QDoubleSpinBox(); m_downforceSpin->setRange(0.5, 2.0); m_downforceSpin->setValue(1.0); m_downforceSpin->setDecimals(2);
    m_dragSpin      = new QDoubleSpinBox(); m_dragSpin->setRange(0.5, 2.0);      m_dragSpin->setValue(1.0);      m_dragSpin->setDecimals(2);

    aeroLayout->addRow("Downforce multiplier:", m_downforceSpin);
    aeroLayout->addRow("Drag multiplier:",      m_dragSpin);

    mainLayout->addWidget(dimGroup);
    mainLayout->addWidget(surfGroup);
    mainLayout->addWidget(aeroGroup);
    mainLayout->addStretch();
}

// ─── AmbiencePage ────────────────────────────────────────────────────────────

AmbiencePage::AmbiencePage(QWidget* parent)
    : QWizardPage(parent)
{
    setTitle("Ambience & Atmosphere");
    setSubTitle("Configure sound and visual atmosphere");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QGroupBox* soundGroup   = new QGroupBox("Ambient Sound");
    QFormLayout* soundLayout = new QFormLayout(soundGroup);

    m_soundCombo = new QComboBox();
    m_soundCombo->addItems({
        "City Ambience", "Rural Countryside", "Mountain Wind", "Coastal Waves",
        "Forest Ambient", "Industrial Zone", "Desert Wind", "Night City",
        "Stadium Crowd", "None"
    });
    soundLayout->addRow("Sound preset:", m_soundCombo);

    QGroupBox* weatherGroup   = new QGroupBox("Weather");
    QFormLayout* weatherLayout = new QFormLayout(weatherGroup);

    m_weatherCombo = new QComboBox();
    m_weatherCombo->addItems({
        "Clear", "Partly Cloudy", "Overcast", "Light Rain",
        "Heavy Rain", "Fog", "Snow", "Dynamic Weather"
    });
    weatherLayout->addRow("Weather preset:", m_weatherCombo);

    QGroupBox* timeGroup   = new QGroupBox("Time of Day");
    QFormLayout* timeLayout = new QFormLayout(timeGroup);

    m_timeCombo = new QComboBox();
    m_timeCombo->addItems({
        "Dawn", "Morning", "Midday", "Afternoon",
        "Sunset", "Dusk", "Evening", "Night", "Dynamic Time"
    });
    timeLayout->addRow("Time:", m_timeCombo);

    QGroupBox* skyboxGroup   = new QGroupBox("Skybox (Optional)");
    QFormLayout* skyboxLayout = new QFormLayout(skyboxGroup);

    QHBoxLayout* skyboxInput = new QHBoxLayout();
    m_skyboxEdit = new QLineEdit();
    m_skyboxEdit->setPlaceholderText("Select skybox texture...");
    m_skyboxBrowseBtn = new QPushButton("Browse...");
    skyboxInput->addWidget(m_skyboxEdit);
    skyboxInput->addWidget(m_skyboxBrowseBtn);
    skyboxLayout->addRow("Path:", skyboxInput);

    mainLayout->addWidget(soundGroup);
    mainLayout->addWidget(weatherGroup);
    mainLayout->addWidget(timeGroup);
    mainLayout->addWidget(skyboxGroup);
    mainLayout->addStretch();

    connect(m_skyboxBrowseBtn, &QPushButton::clicked, this, &AmbiencePage::onBrowseSkybox);
}

void AmbiencePage::onBrowseSkybox()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, "Select Skybox", QDir::homePath(),
        "Image Files (*.png *.jpg *.dds);;All Files (*)");
    if (!filePath.isEmpty())
        m_skyboxEdit->setText(filePath);
}

// ─── TrackReviewPage ─────────────────────────────────────────────────────────

TrackReviewPage::TrackReviewPage(QWidget* parent)
    : QWizardPage(parent)
{
    setTitle("Review & Create");
    setSubTitle("Review track configuration before creation");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_summaryTable = new QTableWidget(0, 2);
    m_summaryTable->setHorizontalHeaderLabels({"Property", "Value"});
    m_summaryTable->horizontalHeader()->setStretchLastSection(true);
    m_summaryTable->verticalHeader()->setVisible(false);
    m_summaryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_summaryTable->setAlternatingRowColors(true);

    mainLayout->addWidget(m_summaryTable);
}

void TrackReviewPage::setTrackEntry(const TrackDatabaseEntry& entry)
{
    m_summaryTable->setRowCount(0);

    const QList<QPair<QString, QString>> items = {
        {"Name",                entry.name},
        {"Location",            entry.location},
        {"Country",             entry.country},
        {"Category",            entry.category},
        {"Length",              QString("%1 km").arg(entry.length, 0, 'f', 3)},
        {"Width",               QString("%1 m").arg(entry.width, 0, 'f', 1)},
        {"Corners",             QString::number(entry.corners)},
        {"Elevation change",    QString("%1 m").arg(entry.elevationChange, 0, 'f', 0)},
        {"Surface type",        entry.surfaceType},
        {"Grip level",          QString::number(entry.gripLevel, 'f', 2)},
        {"Downforce mult.",     QString::number(entry.downforceMultiplier, 'f', 2)},
        {"Drag mult.",          QString::number(entry.dragMultiplier, 'f', 2)},
        {"Ambient sound",       entry.ambientSoundPreset},
        {"Weather",             entry.weatherPreset},
        {"Time of day",         entry.timeOfDay},
        {"Skybox",              entry.hasSkybox ? QFileInfo(entry.skyboxPath).fileName() : "None"},
    };

    for (const auto& item : items) {
        int row = m_summaryTable->rowCount();
        m_summaryTable->insertRow(row);
        m_summaryTable->setItem(row, 0, new QTableWidgetItem(item.first));
        m_summaryTable->setItem(row, 1, new QTableWidgetItem(item.second));
    }
}

} // namespace ks
