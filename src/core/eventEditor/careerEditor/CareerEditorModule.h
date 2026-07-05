#pragma once

#include "../../editor/EditorModule.h"
#include <QDockWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>

namespace ks {

class CareerEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit CareerEditorModule(QWidget* parent = nullptr);
    ~CareerEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Career Editor"; }
    QString moduleId() const override { return "careerEditor"; }
    int getModulePriority() const override { return 42; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onSeriesSelected(int row);
    void onLoadDir();
    void onSeriesNameChanged(const QString& t);

private:
    void setupUi();
    void loadDirToUI();

    QDockWidget* m_dockWidget = nullptr;
    QListWidget* m_seriesList = nullptr;
    QLineEdit* m_seriesNameEdit = nullptr;
    QTextEdit* m_seriesInfoEdit = nullptr;
    QPushButton* m_loadBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QVector<QPair<QString, QString>> m_series;
    int m_selectedIndex = -1;
    QString m_dir;
};

} // namespace ks
