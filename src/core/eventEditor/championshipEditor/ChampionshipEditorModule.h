#pragma once

#include "../../editor/EditorModule.h"
#include <QDockWidget>
#include <QListWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QTabWidget>
#include <QStackedWidget>
#include <QMap>

namespace ks {

struct ChampionshipGoal {
    int points = 0;
    int ranking = 0;
    int tier1 = 0;
    int tier2 = 0;
    int tier3 = 0;
};

struct ConditionGoal {
    QString type;       // TIME / POINTS / POSITION / AI
    int objective = 0;
};

struct ChampionshipEvent {
    QString name;
    QString description;
    QString track;
    QString configTrack;
    QString model;
    QString modelConfig;
    QString skin;
    int cars = 1;
    int aiLevel = 100;
    int raceLaps = 0;
    int penalties = 1;
    bool driftMode = false;
    bool fixedSetup = false;

    int sunAngle = 48;
    double timeMult = 1.0;
    double cloudSpeed = 0.2;

    int ambientTemp = 26;
    int roadTemp = 32;

    int dynamicTrackPreset = 5;

    QString sessionName;
    int sessionType = 3;
    QString spawnSet = "START";
    int startingPosition = 0;
    int sessionLaps = 0;
    int durationMinutes = 0;

    QVector<ConditionGoal> conditions;
};

struct OpponentDriver {
    QString name;
    QString car;
    QString skin;
    int level = 100;
    QString modelConfig;
};

struct ChampionshipEntry {
    QString dirName;
    QString name;
    QString code;
    QString description;
    QString model;
    QString requires;
    ChampionshipGoal goals;
    QVector<int> pointsForPlace;
    QVector<ChampionshipEvent> events;
    QVector<OpponentDriver> opponents;
};

class ChampionshipEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit ChampionshipEditorModule(QWidget* parent = nullptr);
    ~ChampionshipEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Championship Editor"; }
    QString moduleId() const override { return "championshipEditor"; }
    int getModulePriority() const override { return 43; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onChampionshipSelected(int row);
    void onEventSelected(int row);
    void onLoadDir();
    void onSaveDir();
    void onAddChampionship();
    void onRemoveChampionship();
    void onAddEvent();
    void onRemoveEvent();
    void onChampionshipPropChanged();
    void onEventPropChanged();

private:
    void setupUi();
    void loadDirToUI();
    void saveChampionship(const ChampionshipEntry& entry, const QString& dir);
    void saveAll();
    void populateChampionshipProps(int index);
    void populateEventProps(int champIdx, int eventIdx);
    QWidget* createChampionshipProps();
    QWidget* createEventProps();
    QWidget* createEventConditions(int champIdx, int eventIdx);

    QDockWidget* m_dockWidget = nullptr;

    QListWidget* m_championshipList = nullptr;
    QListWidget* m_eventList = nullptr;
    QStackedWidget* m_propsStack = nullptr;

    QWidget* m_champPropsWidget = nullptr;
    QWidget* m_eventPropsWidget = nullptr;

    // Championship props
    QLineEdit* m_champNameEdit = nullptr;
    QLineEdit* m_champCodeEdit = nullptr;
    QTextEdit* m_champDescEdit = nullptr;
    QLineEdit* m_champModelEdit = nullptr;
    QLineEdit* m_champRequiresEdit = nullptr;
    QSpinBox* m_goalPointsSpin = nullptr;
    QSpinBox* m_goalRankingSpin = nullptr;
    QSpinBox* m_goalTier1Spin = nullptr;
    QSpinBox* m_goalTier2Spin = nullptr;
    QSpinBox* m_goalTier3Spin = nullptr;

    // Event props
    QLineEdit* m_evNameEdit = nullptr;
    QTextEdit* m_evDescEdit = nullptr;
    QLineEdit* m_evTrackEdit = nullptr;
    QLineEdit* m_evConfigTrackEdit = nullptr;
    QLineEdit* m_evModelEdit = nullptr;
    QLineEdit* m_evSkinEdit = nullptr;
    QSpinBox* m_evCarsSpin = nullptr;
    QSpinBox* m_evAiLevelSpin = nullptr;
    QSpinBox* m_evRaceLapsSpin = nullptr;
    QSpinBox* m_evSunAngleSpin = nullptr;
    QSpinBox* m_evAmbientSpin = nullptr;
    QSpinBox* m_evRoadSpin = nullptr;
    QComboBox* m_evSessionTypeCombo = nullptr;
    QLineEdit* m_evSessionNameEdit = nullptr;
    QSpinBox* m_evSessionLapsSpin = nullptr;
    QSpinBox* m_evDurationSpin = nullptr;

    QPushButton* m_addChampBtn = nullptr;
    QPushButton* m_removeChampBtn = nullptr;
    QPushButton* m_addEventBtn = nullptr;
    QPushButton* m_removeEventBtn = nullptr;
    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QLabel* m_statusLabel = nullptr;

    QVector<ChampionshipEntry> m_championships;
    int m_selectedChamp = -1;
    int m_selectedEvent = -1;
    QString m_dir;
};

} // namespace ks
