#include "TerrainEditorQmlBridge.h"
#include "TrackTerrainEditor.h"
#include <QImage>
#include <QPixmap>

namespace ks {

TerrainEditorQmlBridge* TerrainEditorQmlBridge::s_instance = nullptr;

TerrainEditorQmlBridge* TerrainEditorQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new TerrainEditorQmlBridge();
    }
    return s_instance;
}

TerrainEditorQmlBridge::TerrainEditorQmlBridge(QObject* parent)
    : QObject(parent)
{
    auto* editor = TrackTerrainEditor::instance();
    connect(editor, &TrackTerrainEditor::currentTerrainChanged, this, &TerrainEditorQmlBridge::currentTerrainChanged);
    connect(editor, &TrackTerrainEditor::terrainSizeChanged, this, &TerrainEditorQmlBridge::terrainSizeChanged);
    connect(editor, &TrackTerrainEditor::brushSettingsChanged, this, &TerrainEditorQmlBridge::brushSettingsChanged);
    connect(editor, &TrackTerrainEditor::terrainModified, this, &TerrainEditorQmlBridge::terrainModified);
    connect(editor, &TrackTerrainEditor::undoAvailable, this, &TerrainEditorQmlBridge::undoAvailableChanged);
    connect(editor, &TrackTerrainEditor::redoAvailable, this, &TerrainEditorQmlBridge::redoAvailableChanged);
}

QString TerrainEditorQmlBridge::currentTerrain() const {
    return TrackTerrainEditor::instance()->currentTerrain();
}

int TerrainEditorQmlBridge::terrainWidth() const {
    return TrackTerrainEditor::instance()->terrainWidth();
}

int TerrainEditorQmlBridge::terrainHeight() const {
    return TrackTerrainEditor::instance()->terrainHeight();
}

float TerrainEditorQmlBridge::brushRadius() const {
    return TrackTerrainEditor::instance()->brushRadius();
}

float TerrainEditorQmlBridge::brushStrength() const {
    return TrackTerrainEditor::instance()->brushStrength();
}

QString TerrainEditorQmlBridge::brushType() const {
    return TrackTerrainEditor::instance()->brushType();
}

bool TerrainEditorQmlBridge::canUndo() const {
    return TrackTerrainEditor::instance()->canUndo();
}

bool TerrainEditorQmlBridge::canRedo() const {
    return TrackTerrainEditor::instance()->canRedo();
}

void TerrainEditorQmlBridge::setBrushRadius(float r) {
    TrackTerrainEditor::instance()->setBrushRadius(r);
}

void TerrainEditorQmlBridge::setBrushStrength(float s) {
    TrackTerrainEditor::instance()->setBrushStrength(s);
}

void TerrainEditorQmlBridge::setBrushType(const QString& type) {
    TrackTerrainEditor::instance()->setBrushType(type);
}

bool TerrainEditorQmlBridge::loadTerrain(const QString& heightmapPath) {
    return TrackTerrainEditor::instance()->loadTerrain(heightmapPath);
}

bool TerrainEditorQmlBridge::saveTerrain(const QString& heightmapPath) {
    return TrackTerrainEditor::instance()->saveTerrain(heightmapPath);
}

bool TerrainEditorQmlBridge::createNewTerrain(int width, int height) {
    return TrackTerrainEditor::instance()->createNewTerrain(width, height);
}

void TerrainEditorQmlBridge::applyBrush(float x, float y, float delta) {
    TrackTerrainEditor::instance()->applyBrush(x, y, delta);
}

void TerrainEditorQmlBridge::applyBrushStroke(const QVariantList& points, float totalDelta) {
    QVector<QPointF> pts;
    for (const auto& p : points) {
        QVariantMap m = p.toMap();
        pts.append(QPointF(m["x"].toDouble(), m["y"].toDouble()));
    }
    TrackTerrainEditor::instance()->applyBrushStroke(pts, totalDelta);
}

void TerrainEditorQmlBridge::smoothTerrain(float x, float y, float radius) {
    TrackTerrainEditor::instance()->smoothTerrain(x, y, radius);
}

void TerrainEditorQmlBridge::flattenTerrain(float x, float y, float radius, float targetHeight) {
    TrackTerrainEditor::instance()->flattenTerrain(x, y, radius, targetHeight);
}

void TerrainEditorQmlBridge::addNoise(float x, float y, float radius, float intensity) {
    TrackTerrainEditor::instance()->addNoise(x, y, radius, intensity);
}

float TerrainEditorQmlBridge::getHeight(float x, float y) const {
    return TrackTerrainEditor::instance()->getHeight(x, y);
}

void TerrainEditorQmlBridge::setHeight(float x, float y, float height) {
    TrackTerrainEditor::instance()->setHeight(x, y, height);
}

QVariantMap TerrainEditorQmlBridge::getTerrainStats() const {
    return TrackTerrainEditor::instance()->getTerrainStats();
}

void TerrainEditorQmlBridge::undo() {
    TrackTerrainEditor::instance()->undo();
}

void TerrainEditorQmlBridge::redo() {
    TrackTerrainEditor::instance()->redo();
}

void TerrainEditorQmlBridge::exportToOBJ(const QString& path, float scale) {
    TrackTerrainEditor::instance()->exportToOBJ(path, scale);
}

void TerrainEditorQmlBridge::exportToPNG(const QString& path) {
    TrackTerrainEditor::instance()->exportToPNG(path);
}

void TerrainEditorQmlBridge::importFromPNG(const QString& path) {
    TrackTerrainEditor::instance()->importFromPNG(path);
}

QVariantList TerrainEditorQmlBridge::getHeightmapData() const {
    QVariantList data;
    auto* editor = TrackTerrainEditor::instance();
    for (int y = 0; y < editor->terrainHeight(); ++y) {
        for (int x = 0; x < editor->terrainWidth(); ++x) {
            data.append(editor->getHeight(x, y));
        }
    }
    return data;
}

} // namespace ks
