#pragma once

#include <QWidget>
#include <QListWidget>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QLabel>
#include "TrackEditor.h"
#include "TrackTerrainEditor.h"

namespace ks {

class TrackEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit TrackEditorWidget(QWidget* parent = nullptr);

    void newTrack(const QString& name, int width = 512, int height = 512);
    void loadTrack(const QString& path);
    void saveTrack(const QString& path);

signals:
    void terrainModified();
    void toolChanged(int tool);

private slots:
    void onToolChanged(int index);
    void onBrushSizeChanged(double value);
    void onBrushStrengthChanged(double value);
    void onSplineSelected(QListWidgetItem* current);
    void onAddSpline();
    void onRemoveSpline();

private:
    void setupUI();

    TrackEditor* m_editor;
    TrackTerrainEditor* m_terrainEditor;

    QComboBox* m_toolCombo;
    QDoubleSpinBox* m_brushSize;
    QDoubleSpinBox* m_brushStrength;

    QListWidget* m_splineList;
    QPushButton* m_addSplineBtn;
    QPushButton* m_removeSplineBtn;

    QLabel* m_terrainInfo;
};

}
