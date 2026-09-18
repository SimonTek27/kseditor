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
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QHeaderView>

namespace ks {

struct TrackDatabaseEntry {
    QString name;
    QString location;
    QString country;
    QString category;
    
    double length;
    double width;
    int corners;
    int turns;
    double elevationChange;
    double maxAltitude;
    double minAltitude;
    
    QString surfaceType;
    QString frictionCoeff;
    
    double downforceMultiplier;
    double dragMultiplier;
    double gripLevel;
    
    QString ambientSoundPreset;
    QString weatherPreset;
    QString timeOfDay;
    
    QString skyboxPath;
    bool hasSkybox = false;
    
    QString description;
};

class TrackWizard : public QWizard
{
    Q_OBJECT

public:
    enum { Page_Location = 0, Page_Physics = 1, Page_Ambience = 2, Page_Review = 3 };
    
    explicit TrackWizard(QWidget* parent = nullptr);
    ~TrackWizard();
    
    TrackDatabaseEntry getTrackEntry() const { return m_trackEntry; }
    
signals:
    void trackCreated(const TrackDatabaseEntry& track);
    
private:
    void setupPages();
    void loadTrackDatabase();
    
    QVector<TrackDatabaseEntry> m_trackDatabase;
    TrackDatabaseEntry m_trackEntry;
};

class LocationPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit LocationPage(QWidget* parent = nullptr);
    
    QString trackName() const { return m_nameEdit->text(); }
    QString location() const { return m_locationEdit->text(); }
    QString country() const { return m_countryCombo->currentText(); }
    QString category() const { return m_categoryCombo->currentText(); }
    
private:
    QLineEdit* m_nameEdit;
    QLineEdit* m_locationEdit;
    QComboBox* m_countryCombo;
    QComboBox* m_categoryCombo;
};

class TrackPhysicsPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit TrackPhysicsPage(QWidget* parent = nullptr);
    
    double trackLength() const { return m_lengthSpin->value(); }
    double trackWidth() const { return m_widthSpin->value(); }
    int cornerCount() const { return m_cornersSpin->value(); }
    double elevationChange() const { return m_elevationSpin->value(); }
    QString surfaceType() const { return m_surfaceCombo->currentText(); }
    double gripLevel() const { return m_gripSpin->value(); }
    double downforceMultiplier() const { return m_downforceSpin->value(); }
    double dragMultiplier() const { return m_dragSpin->value(); }
    
private:
    QDoubleSpinBox* m_lengthSpin;
    QDoubleSpinBox* m_widthSpin;
    QSpinBox* m_cornersSpin;
    QDoubleSpinBox* m_elevationSpin;
    QComboBox* m_surfaceCombo;
    QDoubleSpinBox* m_gripSpin;
    QDoubleSpinBox* m_downforceSpin;
    QDoubleSpinBox* m_dragSpin;
};

class AmbiencePage : public QWizardPage
{
    Q_OBJECT

public:
    explicit AmbiencePage(QWidget* parent = nullptr);
    
    QString ambientSoundPreset() const { return m_soundCombo->currentText(); }
    QString weatherPreset() const { return m_weatherCombo->currentText(); }
    QString timeOfDay() const { return m_timeCombo->currentText(); }
    QString skyboxPath() const { return m_skyboxEdit->text(); }
    
private slots:
    void onBrowseSkybox();
    
private:
    QComboBox* m_soundCombo;
    QComboBox* m_weatherCombo;
    QComboBox* m_timeCombo;
    QLineEdit* m_skyboxEdit;
    QPushButton* m_skyboxBrowseBtn;
};

class TrackReviewPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit TrackReviewPage(QWidget* parent = nullptr);
    void setTrackEntry(const TrackDatabaseEntry& entry);
    
private:
    QTableWidget* m_summaryTable;
};

}