#pragma once

#include <QMainWindow>
#include <QFont>
#include <QVector>

class QComboBox;
class QLabel;
class QCheckBox;
class QGridLayout;
class QWidget;

class GlyphModel;
class GlyphTileWidget;

// Main application window: font picker, the 95-character grid, the four
// "linked editing" toggles, and the preset/export actions. Functionally
// equivalent to the original app's MainWindow.xaml(.cs).
class FontEditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FontEditorWindow(QWidget *parent = nullptr);

private slots:
    void onSelectFont();
    void onSavePreset();
    void onLoadPreset();
    void onExportAtlas();
    void onTileEdited(GlyphTileWidget *source);

private:
    void buildUi();

    // Equivalent to the original's loadFont(): (re)creates all 95
    // GlyphModel/GlyphTileWidget pairs for m_font, auto-measuring each one
    // and - only the first time (m_rowHeight == 0) - computing the shared
    // row height from the tallest natural glyph.
    void rebuildGrid();

    // Equivalent to refreshHeights(): forces every glyph's PixelHeight to
    // m_rowHeight, re-renders them all, and updates the on-screen label.
    void refreshHeights();

    static bool isLetter(const QString &value);
    static bool isNarrowPunct(const QString &value);
    static bool isDigitOrMinus(const QString &value);

    QFont m_font;
    bool m_fontChosen = false;
    double m_rowHeight = 0.0; // 0 = "not yet computed"; mirrors the original's `higherChar`

    QVector<GlyphModel *> m_models;
    QVector<GlyphTileWidget *> m_tiles;

    QLabel *m_selectedFontLabel = nullptr;
    QLabel *m_rowHeightLabel = nullptr;
    QComboBox *m_atlasSizeCombo = nullptr;
    QCheckBox *m_linkLettersCheck = nullptr;
    QCheckBox *m_linkNarrowCheck = nullptr;
    QCheckBox *m_linkDigitsCheck = nullptr;
    QCheckBox *m_linkGlobalVPadCheck = nullptr;

    QWidget *m_gridHost = nullptr;
    QGridLayout *m_gridLayout = nullptr;
};
