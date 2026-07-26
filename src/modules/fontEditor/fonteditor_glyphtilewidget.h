#pragma once

#include <QFrame>

#include "fonteditor_glyphmodel.h"

class QLabel;
class QSpinBox;

// One tile in the character grid: a live preview of the rasterised glyph
// plus three spin boxes (Width / H-Pad / V-Pad) that let the user nudge
// that one character. Functionally this replaces the original app's
// `FontControl` UserControl - same three adjustable quantities, same
// "any edit fires a notification the main window can react to" shape,
// just QSpinBox (built-in +/- buttons and typed entry in one widget)
// standing in for the original's separate textbox-plus-two-buttons groups.
class GlyphTileWidget : public QFrame
{
    Q_OBJECT

public:
    explicit GlyphTileWidget(GlyphModel *model, QWidget *parent = nullptr);

    GlyphModel *model() const { return m_model; }

    // Repaints the preview QLabel from the model's current image().
    void refreshPreview();

    // Pushes the model's current Width/HPad/VPad into the spin boxes
    // without re-triggering onWidthChanged() etc. Used when another tile's
    // edit propagates into this one (linked editing) or a preset is loaded.
    void syncSpinBoxesFromModel();

signals:
    // Emitted after the user changes Width, H-Pad or V-Pad *through this
    // tile's own controls* (not when synced programmatically). MainWindow
    // listens to this to re-render, propagate linked edits, and refresh
    // the shared row height label - equivalent to the original's
    // PngChangeEvent / fc_PngChange.
    void edited(GlyphTileWidget *self);

private slots:
    void onWidthChanged(int value);
    void onHPadChanged(int value);
    void onVPadChanged(int value);

private:
    static QString displayLabel(const QString &value);

    GlyphModel *m_model;
    QLabel *m_titleLabel;
    QLabel *m_previewLabel;
    QSpinBox *m_widthSpin;
    QSpinBox *m_hPadSpin;
    QSpinBox *m_vPadSpin;
    bool m_syncing = false; // guards against feedback loops while syncing
};
