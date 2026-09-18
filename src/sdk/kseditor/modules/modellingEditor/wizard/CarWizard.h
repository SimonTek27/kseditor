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
#include <QListWidget>
#include <QPushButton>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMap>
#include <QString>
#include <QVector>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>

namespace ks {

struct CarDatabaseEntry {
    QString name;
    QString manufacturer;
    QString category;
    QString type;
    
    double year;
    double mass;
    double wheelbase;
    double trackWidthFront;
    double trackWidthRear;
    double length;
    double width;
    double height;
    
    QString engineConfig;
    int cylinderCount;
    double displacement;
    double maxPower;
    int maxPowerRPM;
    double maxTorque;
    int maxTorqueRPM;
    int redlineRPM;
    bool turbocharged;
    
    double topSpeed;
    double zeroTo100;
    double downforceFront;
    double downforceRear;
    double dragCoefficient;
    
    QString frontTyreSize;
    QString rearTyreSize;
    
    QString transmission;
    int gearCount;
    double finalDrive;
    
    QString description;
    
    QString fbxPath;
    bool hasFbx = false;
};

class CarWizard : public QWizard
{
    Q_OBJECT

public:
    enum { Page_Category = 0, Page_Engine = 1, Page_Details = 2, Page_Physics = 3, Page_Sound = 4, Page_FBX = 5, Page_Review = 6 };
    
    explicit CarWizard(QWidget* parent = nullptr);
    ~CarWizard();
    
    CarDatabaseEntry getCarEntry() const { return m_carEntry; }
    bool hasValidFbx() const { return m_carEntry.hasFbx && !m_carEntry.fbxPath.isEmpty(); }
    
    void setFbxPath(const QString& path, bool hasFbx);
    
signals:
    void carCreated(const CarDatabaseEntry& car);
    void carListFiltered(const QList<CarDatabaseEntry>& cars);
    void categorySelected(const QString& category);
    void carNameChanged(const QString& name);
    void carSelected(const CarDatabaseEntry& entry);
    
private slots:
    void onCategorySelected(const QString& category);
    void onCarNameChanged(const QString& name);
    void searchCar(const QString& partialName);
    void onCarSelected(int row);
    void populateFromDatabase(const CarDatabaseEntry& entry);
    
private:
    void setupPages();
    void loadCarDatabase();
    void filterCarList(const QString& text);
    
    CarDatabaseEntry m_carEntry;
    QList<CarDatabaseEntry> m_carDatabase;
    QList<CarDatabaseEntry> m_filteredCars;
    QString m_selectedCategory;
    QListWidget* m_carListWidget;
    QTableWidget* m_specsTable;
    QLineEdit* m_nameEdit;
    QComboBox* m_categoryCombo;
    QComboBox* m_engineConfigCombo;
};

class CategoryPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit CategoryPage(QWidget* parent = nullptr);
    QString selectedCategory() const;
    QString selectedType() const;
    
private:
    QButtonGroup* m_typeGroup;
    QButtonGroup* m_categoryGroup;
};

class EnginePage : public QWizardPage
{
    Q_OBJECT

public:
    explicit EnginePage(QWidget* parent = nullptr);
    
    QString engineConfig() const;
    int cylinderCount() const;
    double displacement() const;
    
private:
    QComboBox* m_engineConfigCombo;
    QSpinBox* m_cylinderSpin;
    QDoubleSpinBox* m_displacementSpin;
};

class CarDetailsPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit CarDetailsPage(QWidget* parent = nullptr);
    
    QString carName() const { return m_nameEdit->text(); }
    CarDatabaseEntry getCarEntry() const { return m_carEntry; }
    
public slots:
    void setCarEntry(const CarDatabaseEntry& entry);
    void onSearchClicked();
    
signals:
    void carNameChanged(const QString& name);
    void carSelected(const CarDatabaseEntry& entry);
    
private:
    void populateFields(const CarDatabaseEntry& entry);
    
    QLineEdit* m_nameEdit;
    QLineEdit* m_manufacturerEdit;
    QSpinBox* m_yearSpin;
    QDoubleSpinBox* m_massSpin;
    QDoubleSpinBox* m_wheelbaseSpin;
    
    QSpinBox* m_powerSpin;
    QSpinBox* m_maxPowerRPMSpin;
    QSpinBox* m_torqueSpin;
    QSpinBox* m_maxTorqueRPMSpin;
    QSpinBox* m_redlineSpin;
    QCheckBox* m_turboCheck;
    
    QDoubleSpinBox* m_lengthSpin;
    QDoubleSpinBox* m_widthSpin;
    QDoubleSpinBox* m_heightSpin;
    QDoubleSpinBox* m_trackFrontSpin;
    QDoubleSpinBox* m_trackRearSpin;
    
    QDoubleSpinBox* m_downforceFrontSpin;
    QDoubleSpinBox* m_downforceRearSpin;
    QDoubleSpinBox* m_dragSpin;
    
    QSpinBox* m_gearCountSpin;
    QDoubleSpinBox* m_finalDriveSpin;
    
    QListWidget* m_searchResults;
    QLabel* m_carImageLabel;
    
    CarDatabaseEntry m_carEntry;
    
    friend class CarWizard;
};

class PhysicsPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PhysicsPage(QWidget* parent = nullptr);
    
    double suspensionStiffness() const { return m_suspensionSpin->value(); }
    double damping() const { return m_dampingSpin->value(); }
    double downforceCoeff() const { return m_downforceSpin->value(); }
    double dragCoeff() const { return m_dragSpin->value(); }
    
private:
    QDoubleSpinBox* m_suspensionSpin;
    QDoubleSpinBox* m_dampingSpin;
    QDoubleSpinBox* m_downforceSpin;
    QDoubleSpinBox* m_dragSpin;
    QDoubleSpinBox* m_massScaleSpin;
};

class SoundPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit SoundPage(QWidget* parent = nullptr);
    
    QString soundPreset() const { return m_presetCombo->currentText(); }
    int sampleRate() const { return m_sampleRateCombo->currentText().replace(" Hz", "").toInt(); }
    QString outputDirectory() const { return m_outputDirEdit->text(); }
    
private:
    QComboBox* m_presetCombo;
    QComboBox* m_sampleRateCombo;
    QLineEdit* m_outputDirEdit;
    QPushButton* m_browseBtn;
};

class FBXPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit FBXPage(QWidget* parent = nullptr);
    ~FBXPage();
    
    QString fbxPath() const { return m_fbxPath; }
    bool hasFbx() const { return m_hasFbx; }
    
private slots:
    void onBrowseClicked();
    void validateFbx();
    
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    
private:
    QString m_fbxPath;
    bool m_hasFbx = false;
    
    QLineEdit* m_fbxPathEdit;
    QPushButton* m_browseBtn;
    QLabel* m_previewLabel;
    QLabel* m_statusLabel;
    QGroupBox* m_previewGroup;
    
    QString formatFileSize(qint64 bytes);
};

class ReviewPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit ReviewPage(QWidget* parent = nullptr);
    void setCarEntry(const CarDatabaseEntry& entry);
    
private:
    QTableWidget* m_summaryTable;
};

}