#pragma once

#include <QWizard>
#include <QWizardPage>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QJsonObject>
#include <QSlider>
#include <QHeaderView>

namespace ks {

struct CharacterDatabaseEntry {
    QString name;
    QString nationality;
    QString team;
    int age;
    
    int aggression;
    int consistency;
    int concentration;
    int courage;
    int adaptability;
    int technicalInsight;
    int stamina;
    int charisma;
    
    int startSkill;
    int raceSkill;
    int wetSkill;
    int qualifyingSkill;
    
    QString preferredTyre;
    int preferredTyreLife;
    double aggressionMultiplier;
    QString drivingStyle;
};

class CharacterWizard : public QWizard
{
    Q_OBJECT

public:
    enum { Page_BasicInfo = 0, Page_Attributes = 1, Page_Skills = 2, Page_Review = 3 };
    
    explicit CharacterWizard(QWidget* parent = nullptr);
    ~CharacterWizard();
    
    CharacterDatabaseEntry getCharacterEntry() const { return m_entry; }
    
signals:
    void characterCreated(const CharacterDatabaseEntry& character);
    
private:
    void setupPages();
    
    CharacterDatabaseEntry m_entry;
};

class CharacterBasicPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit CharacterBasicPage(QWidget* parent = nullptr);
    
    QString characterName() const { return m_nameEdit->text(); }
    QString nationality() const { return m_nationalityCombo->currentText(); }
    QString team() const { return m_teamEdit->text(); }
    int age() const { return m_ageSpin->value(); }
    
private:
    QLineEdit* m_nameEdit;
    QComboBox* m_nationalityCombo;
    QLineEdit* m_teamEdit;
    QSpinBox* m_ageSpin;
};

class CharacterAttributesPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit CharacterAttributesPage(QWidget* parent = nullptr);
    
    int aggression() const { return m_aggressionSlider->value(); }
    int consistency() const { return m_consistencySlider->value(); }
    int concentration() const { return m_concentrationSlider->value(); }
    int courage() const { return m_courageSlider->value(); }
    int adaptability() const { return m_adaptabilitySlider->value(); }
    int technicalInsight() const { return m_technicalSlider->value(); }
    int stamina() const { return m_staminaSlider->value(); }
    int charisma() const { return m_charismaSlider->value(); }
    
private:
    QSlider* m_aggressionSlider;
    QSlider* m_consistencySlider;
    QSlider* m_concentrationSlider;
    QSlider* m_courageSlider;
    QSlider* m_adaptabilitySlider;
    QSlider* m_technicalSlider;
    QSlider* m_staminaSlider;
    QSlider* m_charismaSlider;
    
    QLabel* m_aggressionValue;
    QLabel* m_consistencyValue;
    QLabel* m_concentrationValue;
    QLabel* m_courageValue;
    QLabel* m_adaptabilityValue;
    QLabel* m_technicalValue;
    QLabel* m_staminaValue;
    QLabel* m_charismaValue;
};

class CharacterSkillsPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit CharacterSkillsPage(QWidget* parent = nullptr);
    
    int startSkill() const { return m_startSpin->value(); }
    int raceSkill() const { return m_raceSpin->value(); }
    int wetSkill() const { return m_wetSpin->value(); }
    int qualifyingSkill() const { return m_qualifyingSpin->value(); }
    QString preferredTyre() const { return m_tyreCombo->currentText(); }
    QString drivingStyle() const { return m_styleCombo->currentText(); }
    
private:
    QSpinBox* m_startSpin;
    QSpinBox* m_raceSpin;
    QSpinBox* m_wetSpin;
    QSpinBox* m_qualifyingSpin;
    QComboBox* m_tyreCombo;
    QComboBox* m_styleCombo;
};

class CharacterReviewPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit CharacterReviewPage(QWidget* parent = nullptr);
    void setCharacterEntry(const CharacterDatabaseEntry& entry);
    
private:
    QTableWidget* m_summaryTable;
};

}