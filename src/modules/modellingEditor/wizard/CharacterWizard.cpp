#include "CharacterWizard.h"
#include <QDebug>

namespace ks {

// ─── CharacterWizard ─────────────────────────────────────────────────────────

CharacterWizard::CharacterWizard(QWidget* parent)
    : QWizard(parent)
{
    setWindowTitle("Character Creation Wizard");
    setMinimumSize(700, 550);
    setWizardStyle(QWizard::ModernStyle);

    setupPages();
}

CharacterWizard::~CharacterWizard() = default;

void CharacterWizard::setupPages()
{
    setPage(Page_BasicInfo,  new CharacterBasicPage(this));
    setPage(Page_Attributes, new CharacterAttributesPage(this));
    setPage(Page_Skills,     new CharacterSkillsPage(this));
    setPage(Page_Review,     new CharacterReviewPage(this));

    // Collect page data and populate m_entry before emitting.
    connect(this, &QWizard::accepted, this, [this]() {
        auto* basic  = qobject_cast<CharacterBasicPage*>(page(Page_BasicInfo));
        auto* attrs  = qobject_cast<CharacterAttributesPage*>(page(Page_Attributes));
        auto* skills = qobject_cast<CharacterSkillsPage*>(page(Page_Skills));
        auto* rev    = qobject_cast<CharacterReviewPage*>(page(Page_Review));

        if (basic) {
            m_entry.name        = basic->characterName();
            m_entry.nationality = basic->nationality();
            m_entry.team        = basic->team();
            m_entry.age         = basic->age();
        }
        if (attrs) {
            m_entry.aggression       = attrs->aggression();
            m_entry.consistency      = attrs->consistency();
            m_entry.concentration    = attrs->concentration();
            m_entry.courage          = attrs->courage();
            m_entry.adaptability     = attrs->adaptability();
            m_entry.technicalInsight = attrs->technicalInsight();
            m_entry.stamina          = attrs->stamina();
            m_entry.charisma         = attrs->charisma();
        }
        if (skills) {
            m_entry.startSkill      = skills->startSkill();
            m_entry.raceSkill       = skills->raceSkill();
            m_entry.wetSkill        = skills->wetSkill();
            m_entry.qualifyingSkill = skills->qualifyingSkill();
            m_entry.preferredTyre   = skills->preferredTyre();
            m_entry.drivingStyle    = skills->drivingStyle();
            m_entry.aggressionMultiplier = 1.0 + (m_entry.aggression - 50) * 0.01;
        }
        if (rev) {
            rev->setCharacterEntry(m_entry);
        }

        emit characterCreated(m_entry);
        qDebug() << "CharacterWizard: created driver" << m_entry.name;
    });
}

// ─── CharacterBasicPage ──────────────────────────────────────────────────────

CharacterBasicPage::CharacterBasicPage(QWidget* parent)
    : QWizardPage(parent)
{
    setTitle("Basic Information");
    setSubTitle("Enter the driver's personal details");

    QVBoxLayout* mainLayout  = new QVBoxLayout(this);
    QGroupBox* personalGroup  = new QGroupBox("Personal Details");
    QFormLayout* personalLayout = new QFormLayout(personalGroup);

    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("e.g., Lewis Hamilton");
    personalLayout->addRow("Driver Name:", m_nameEdit);

    m_nationalityCombo = new QComboBox();
    m_nationalityCombo->addItems({
        "United Kingdom", "Germany", "Italy", "France", "Spain",
        "United States", "Japan", "Australia", "Brazil", "Netherlands",
        "Belgium", "Austria", "Finland", "Mexico", "Canada", "Monaco", "Other"
    });
    personalLayout->addRow("Nationality:", m_nationalityCombo);

    m_teamEdit = new QLineEdit();
    m_teamEdit->setPlaceholderText("e.g., Mercedes-AMG Petronas");
    personalLayout->addRow("Team:", m_teamEdit);

    m_ageSpin = new QSpinBox();
    m_ageSpin->setRange(18, 55);
    m_ageSpin->setValue(28);
    personalLayout->addRow("Age:", m_ageSpin);

    mainLayout->addWidget(personalGroup);
    mainLayout->addStretch();

    registerField("driverName*", m_nameEdit);
}

// ─── CharacterAttributesPage ─────────────────────────────────────────────────

CharacterAttributesPage::CharacterAttributesPage(QWidget* parent)
    : QWizardPage(parent)
{
    setTitle("Driver Attributes");
    setSubTitle("Configure personality and mental attributes (1–100)");

    QVBoxLayout* mainLayout  = new QVBoxLayout(this);
    QGroupBox* mentalGroup   = new QGroupBox("Mental Attributes");
    QGridLayout* mentalLayout = new QGridLayout(mentalGroup);

    // Helper: create label + slider + live value display in one row.
    auto makeRow = [&](const QString& label, QSlider*& slider, QLabel*& valLabel, int row) {
        mentalLayout->addWidget(new QLabel(label), row, 0);
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(1, 100);
        slider->setValue(50);
        mentalLayout->addWidget(slider, row, 1);
        valLabel = new QLabel("50");
        valLabel->setMinimumWidth(30);
        valLabel->setAlignment(Qt::AlignCenter);
        mentalLayout->addWidget(valLabel, row, 2);
        connect(slider, &QSlider::valueChanged, this,
                [valLabel](int v) { valLabel->setText(QString::number(v)); });
    };

    makeRow("Aggression:",       m_aggressionSlider,    m_aggressionValue,    0);
    makeRow("Consistency:",      m_consistencySlider,   m_consistencyValue,   1);
    makeRow("Concentration:",    m_concentrationSlider, m_concentrationValue, 2);
    makeRow("Courage:",          m_courageSlider,       m_courageValue,       3);
    makeRow("Adaptability:",     m_adaptabilitySlider,  m_adaptabilityValue,  4);
    makeRow("Technical insight:",m_technicalSlider,     m_technicalValue,     5);
    makeRow("Stamina:",          m_staminaSlider,       m_staminaValue,       6);
    makeRow("Charisma:",         m_charismaSlider,      m_charismaValue,      7);

    mentalLayout->setColumnStretch(1, 1);

    mainLayout->addWidget(mentalGroup);
    mainLayout->addStretch();
}

// ─── CharacterSkillsPage ─────────────────────────────────────────────────────

CharacterSkillsPage::CharacterSkillsPage(QWidget* parent)
    : QWizardPage(parent)
{
    setTitle("Racing Skills");
    setSubTitle("Set discipline-specific performance scores (1–100) and preferences");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QGroupBox* skillsGroup   = new QGroupBox("Skill Scores");
    QFormLayout* skillsLayout = new QFormLayout(skillsGroup);

    auto makeSpin = [](int def) {
        auto* s = new QSpinBox();
        s->setRange(1, 100);
        s->setValue(def);
        return s;
    };

    m_startSpin      = makeSpin(75);
    m_raceSpin       = makeSpin(80);
    m_wetSpin        = makeSpin(70);
    m_qualifyingSpin = makeSpin(78);

    skillsLayout->addRow("Start skill:",      m_startSpin);
    skillsLayout->addRow("Race skill:",       m_raceSpin);
    skillsLayout->addRow("Wet skill:",        m_wetSpin);
    skillsLayout->addRow("Qualifying skill:", m_qualifyingSpin);

    QGroupBox* prefsGroup   = new QGroupBox("Preferences");
    QFormLayout* prefsLayout = new QFormLayout(prefsGroup);

    m_tyreCombo = new QComboBox();
    m_tyreCombo->addItems({ "Soft", "Medium", "Hard", "Intermediate", "Wet" });
    prefsLayout->addRow("Preferred tyre:", m_tyreCombo);

    m_styleCombo = new QComboBox();
    m_styleCombo->addItems({ "Clean", "Balanced", "Aggressive", "Defensive", "Wheel-to-Wheel" });
    prefsLayout->addRow("Driving style:", m_styleCombo);

    mainLayout->addWidget(skillsGroup);
    mainLayout->addWidget(prefsGroup);
    mainLayout->addStretch();
}

// ─── CharacterReviewPage ─────────────────────────────────────────────────────

CharacterReviewPage::CharacterReviewPage(QWidget* parent)
    : QWizardPage(parent)
{
    setTitle("Review & Create");
    setSubTitle("Review driver configuration before creation");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_summaryTable = new QTableWidget(0, 2);
    m_summaryTable->setHorizontalHeaderLabels({"Property", "Value"});
    m_summaryTable->horizontalHeader()->setStretchLastSection(true);
    m_summaryTable->verticalHeader()->setVisible(false);
    m_summaryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_summaryTable->setAlternatingRowColors(true);

    mainLayout->addWidget(m_summaryTable);
}

void CharacterReviewPage::setCharacterEntry(const CharacterDatabaseEntry& entry)
{
    m_summaryTable->setRowCount(0);

    const QList<QPair<QString, QString>> items = {
        {"Name",              entry.name},
        {"Nationality",       entry.nationality},
        {"Team",              entry.team},
        {"Age",               QString::number(entry.age)},
        {"Aggression",        QString::number(entry.aggression)},
        {"Consistency",       QString::number(entry.consistency)},
        {"Concentration",     QString::number(entry.concentration)},
        {"Courage",           QString::number(entry.courage)},
        {"Adaptability",      QString::number(entry.adaptability)},
        {"Technical insight", QString::number(entry.technicalInsight)},
        {"Stamina",           QString::number(entry.stamina)},
        {"Charisma",          QString::number(entry.charisma)},
        {"Start skill",       QString::number(entry.startSkill)},
        {"Race skill",        QString::number(entry.raceSkill)},
        {"Wet skill",         QString::number(entry.wetSkill)},
        {"Qualifying skill",  QString::number(entry.qualifyingSkill)},
        {"Preferred tyre",    entry.preferredTyre},
        {"Driving style",     entry.drivingStyle},
        {"Aggression mult.",  QString::number(entry.aggressionMultiplier, 'f', 3)},
    };

    for (const auto& item : items) {
        int row = m_summaryTable->rowCount();
        m_summaryTable->insertRow(row);
        m_summaryTable->setItem(row, 0, new QTableWidgetItem(item.first));
        m_summaryTable->setItem(row, 1, new QTableWidgetItem(item.second));
    }
}

} // namespace ks
