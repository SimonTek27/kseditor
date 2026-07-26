#include "fonteditorwindow.h"

#include "fonteditor_acffile.h"
#include "fonteditor_glyphmodel.h"
#include "fonteditor_glyphtilewidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTextStream>
#include <QVBoxLayout>
#include <QtGlobal>
#include <algorithm>

FontEditorWindow::FontEditorWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    setWindowTitle(tr("ksFontGenerator"));
    resize(1150, 780);
}

void FontEditorWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);

    // ---- Top row: font selection + selected-font readout -------------
    auto *fontRow = new QHBoxLayout();
    auto *selectFontBtn = new QPushButton(tr("Seleziona font..."), central);
    connect(selectFontBtn, &QPushButton::clicked, this, &FontEditorWindow::onSelectFont);
    m_selectedFontLabel = new QLabel(tr("Nessun font selezionato"), central);
    m_rowHeightLabel = new QLabel(tr("Altezza riga: -"), central);
    fontRow->addWidget(selectFontBtn);
    fontRow->addWidget(m_selectedFontLabel, 1);
    fontRow->addWidget(m_rowHeightLabel);
    rootLayout->addLayout(fontRow);

    // ---- Second row: linked-edit toggles ------------------------------
    auto *linkGroup = new QGroupBox(tr("Modifica collegata (applica la modifica corrente a...)"), central);
    auto *linkLayout = new QHBoxLayout(linkGroup);
    m_linkLettersCheck = new QCheckBox(tr("Lettere [A-Za-z]"), linkGroup);
    m_linkNarrowCheck = new QCheckBox(tr("Punteggiatura stretta [.,:;]"), linkGroup);
    m_linkDigitsCheck = new QCheckBox(tr("Cifre [0-9-]"), linkGroup);
    m_linkGlobalVPadCheck = new QCheckBox(tr("V-Pad globale (tutti i caratteri)"), linkGroup);
    linkLayout->addWidget(m_linkLettersCheck);
    linkLayout->addWidget(m_linkNarrowCheck);
    linkLayout->addWidget(m_linkDigitsCheck);
    linkLayout->addWidget(m_linkGlobalVPadCheck);
    linkLayout->addStretch(1);
    rootLayout->addWidget(linkGroup);

    // ---- Character grid, scrollable -----------------------------------
    m_gridHost = new QWidget();
    m_gridLayout = new QGridLayout(m_gridHost);
    m_gridLayout->setSpacing(6);

    auto *scrollArea = new QScrollArea(central);
    scrollArea->setWidget(m_gridHost);
    scrollArea->setWidgetResizable(true);
    rootLayout->addWidget(scrollArea, 1);

    // ---- Bottom row: preset + export actions --------------------------
    auto *actionRow = new QHBoxLayout();
    auto *savePresetBtn = new QPushButton(tr("Salva preset (.acf)..."), central);
    auto *loadPresetBtn = new QPushButton(tr("Carica preset (.acf)..."), central);
    connect(savePresetBtn, &QPushButton::clicked, this, &FontEditorWindow::onSavePreset);
    connect(loadPresetBtn, &QPushButton::clicked, this, &FontEditorWindow::onLoadPreset);

    m_atlasSizeCombo = new QComboBox(central);
    m_atlasSizeCombo->addItem(tr("1024 x 16 px"));
    m_atlasSizeCombo->addItem(tr("2048 x 32 px"));
    m_atlasSizeCombo->addItem(tr("4096 x 64 px"));
    m_atlasSizeCombo->setCurrentIndex(1);

    auto *exportBtn = new QPushButton(tr("Esporta atlas (.png + .txt)..."), central);
    connect(exportBtn, &QPushButton::clicked, this, &FontEditorWindow::onExportAtlas);

    actionRow->addWidget(savePresetBtn);
    actionRow->addWidget(loadPresetBtn);
    actionRow->addStretch(1);
    actionRow->addWidget(new QLabel(tr("Dimensione atlas:"), central));
    actionRow->addWidget(m_atlasSizeCombo);
    actionRow->addWidget(exportBtn);
    rootLayout->addLayout(actionRow);

    setCentralWidget(central);
}

// ---------------------------------------------------------------------
// Character classification, matching the original's three regexes
// exactly ([A-Za-z], [.,:;], [0-9-]).
// ---------------------------------------------------------------------
bool FontEditorWindow::isLetter(const QString &value)
{
    static const QRegularExpression re(QStringLiteral("[A-Za-z]"));
    return re.match(value).hasMatch();
}

bool FontEditorWindow::isNarrowPunct(const QString &value)
{
    static const QRegularExpression re(QStringLiteral("[.,:;]"));
    return re.match(value).hasMatch();
}

bool FontEditorWindow::isDigitOrMinus(const QString &value)
{
    static const QRegularExpression re(QStringLiteral("[0-9-]"));
    return re.match(value).hasMatch();
}

// ---------------------------------------------------------------------
void FontEditorWindow::rebuildGrid()
{
    for (GlyphTileWidget *tile : std::as_const(m_tiles))
        tile->deleteLater();
    m_tiles.clear();
    for (GlyphModel *model : std::as_const(m_models))
        model->deleteLater();
    m_models.clear();

    const qreal pt = m_font.pointSizeF() > 0 ? m_font.pointSizeF() : m_font.pixelSize();
    m_selectedFontLabel->setText(tr("%1, %2 pt%3%4")
                                      .arg(m_font.family())
                                      .arg(pt)
                                      .arg(m_font.bold() ? tr(", grassetto") : QString())
                                      .arg(m_font.italic() ? tr(", corsivo") : QString()));

    const int columns = 10;
    int row = 0, col = 0;
    for (int code = AcfFile::kFirstCharCode; code <= AcfFile::kLastCharCode; ++code) {
        const QString value = QString(QChar(code));
        auto *model = new GlyphModel(value, code - AcfFile::kFirstCharCode, this);
        model->render(m_font);
        m_models.push_back(model);

        auto *tile = new GlyphTileWidget(model, m_gridHost);
        connect(tile, &GlyphTileWidget::edited, this, &FontEditorWindow::onTileEdited);
        m_gridLayout->addWidget(tile, row, col);
        m_tiles.push_back(tile);

        if (++col >= columns) {
            col = 0;
            ++row;
        }
    }

    // Only auto-compute the shared row height the first time - if it was
    // already set (e.g. just loaded from a preset) leave it alone. This
    // mirrors the original's `if (higherChar == 0.0)` check in loadFont().
    if (m_rowHeight <= 0.0) {
        for (GlyphModel *model : std::as_const(m_models))
            m_rowHeight = std::max(m_rowHeight, double(model->pixelHeight()));
    }

    refreshHeights();
}

void FontEditorWindow::refreshHeights()
{
    m_rowHeightLabel->setText(tr("Altezza riga: %1 px").arg(qRound(m_rowHeight)));
    const int h = qMax(1, qRound(m_rowHeight));
    for (int i = 0; i < m_models.size(); ++i) {
        m_models[i]->setPixelHeight(h);
        m_models[i]->render(m_font);
        m_tiles[i]->refreshPreview();
        m_tiles[i]->syncSpinBoxesFromModel();
    }
}

// ---------------------------------------------------------------------
void FontEditorWindow::onSelectFont()
{
    bool ok = false;
    const QFont initial = m_fontChosen ? m_font : QFont(QStringLiteral("Arial"), 48);
    const QFont chosen = QFontDialog::getFont(&ok, initial, this, tr("Seleziona il font"));
    if (!ok)
        return;

    m_font = chosen;
    m_fontChosen = true;
    m_rowHeight = 0.0; // new font -> recompute the natural row height from scratch
    rebuildGrid();
}

void FontEditorWindow::onTileEdited(GlyphTileWidget *source)
{
    GlyphModel *sourceModel = source->model();
    sourceModel->render(m_font);
    source->refreshPreview();

    // Faithfully reproducing the original's exact linked-edit rule: each
    // checkbox tests the *candidate target's* character class, not the
    // class of the character you're actually editing. So with "Lettere"
    // checked, editing ANY character (even a digit) copies its Width/
    // H-Pad/V-Pad onto every letter. That is almost certainly an
    // oversight in the original (source/target swapped in the regex
    // check) rather than an intentional feature, but it is exactly what
    // the shipped .exe does, so it is kept as-is here for parity. If you
    // want each toggle to only fire when the edited character *itself*
    // is also in that class, add e.g. `&& isLetter(sourceModel->value())`
    // to the condition below.
    for (GlyphTileWidget *tile : std::as_const(m_tiles)) {
        if (tile == source)
            continue;
        GlyphModel *m = tile->model();
        bool touched = false;

        if (m_linkLettersCheck->isChecked() && isLetter(m->value())) {
            m->setHPadding(sourceModel->hPadding());
            m->setVPadding(sourceModel->vPadding());
            m->setPixelWidth(sourceModel->pixelWidth());
            touched = true;
        }
        if (m_linkNarrowCheck->isChecked() && isNarrowPunct(m->value())) {
            m->setHPadding(sourceModel->hPadding());
            m->setVPadding(sourceModel->vPadding());
            m->setPixelWidth(sourceModel->pixelWidth());
            touched = true;
        }
        if (m_linkDigitsCheck->isChecked() && isDigitOrMinus(m->value())) {
            m->setHPadding(sourceModel->hPadding());
            m->setVPadding(sourceModel->vPadding());
            m->setPixelWidth(sourceModel->pixelWidth());
            touched = true;
        }
        if (m_linkGlobalVPadCheck->isChecked()) {
            m->setVPadding(sourceModel->vPadding());
            touched = true;
        }

        if (touched) {
            m->render(m_font);
            tile->syncSpinBoxesFromModel();
            tile->refreshPreview();
        }
    }
}

// ---------------------------------------------------------------------
void FontEditorWindow::onSavePreset()
{
    if (m_models.size() != AcfFile::kCharCount) {
        QMessageBox::information(this, tr("Salva preset"), tr("Seleziona prima un font."));
        return;
    }

    QString path = QFileDialog::getSaveFileName(this, tr("Salva preset"), QString(),
                                                  tr("File font AC (*.acf)"));
    if (path.isEmpty())
        return;
    if (!path.endsWith(QStringLiteral(".acf"), Qt::CaseInsensitive))
        path += QStringLiteral(".acf");

    AcfFile acf;
    acf.fontName = m_font.family();
    acf.family = m_font.family();
    acf.sizePt = m_font.pointSizeF() > 0 ? m_font.pointSizeF() : m_font.pixelSize();
    acf.bold = m_font.bold();
    acf.italic = m_font.italic();
    acf.height = m_rowHeight;
    acf.chars.resize(AcfFile::kCharCount);
    for (int i = 0; i < m_models.size(); ++i) {
        acf.chars[i].hPadding = m_models[i]->hPadding();
        acf.chars[i].vPadding = m_models[i]->vPadding();
        acf.chars[i].pixelWidth = m_models[i]->pixelWidth();
    }

    QString error;
    if (!acf.save(path, &error))
        QMessageBox::warning(this, tr("Errore"), error);
}

void FontEditorWindow::onLoadPreset()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Carica preset"), QString(),
                                                        tr("File font AC (*.acf)"));
    if (path.isEmpty())
        return;

    AcfFile acf;
    QString error;
    if (!acf.load(path, &error)) {
        QMessageBox::warning(this, tr("Errore"), error);
        return;
    }

    m_font = QFont(acf.family);
    m_font.setPointSizeF(acf.sizePt);
    m_font.setBold(acf.bold);
    m_font.setItalic(acf.italic);
    m_fontChosen = true;
    m_rowHeight = acf.height; // preserved from the file, same as the original

    rebuildGrid(); // m_rowHeight is already > 0, so the auto-measure pass is skipped

    for (int i = 0; i < m_models.size() && i < acf.chars.size(); ++i) {
        m_models[i]->setHPadding(acf.chars[i].hPadding);
        m_models[i]->setVPadding(acf.chars[i].vPadding);
        m_models[i]->setPixelWidth(acf.chars[i].pixelWidth);
        m_models[i]->setPixelHeight(qRound(m_rowHeight));
        m_models[i]->render(m_font);
        m_tiles[i]->syncSpinBoxesFromModel();
        m_tiles[i]->refreshPreview();
    }
    refreshHeights();
}

// ---------------------------------------------------------------------
void FontEditorWindow::onExportAtlas()
{
    if (m_rowHeight <= 0.0 || m_models.isEmpty()) {
        QMessageBox::information(this, tr("Esporta atlas"), tr("Seleziona prima un font."));
        return;
    }

    static const int kWidths[3] = {1024, 2048, 4096};
    static const int kHeights[3] = {16, 32, 64};
    const int idx = qBound(0, m_atlasSizeCombo->currentIndex(), 2);
    const int atlasWidth = kWidths[idx];
    const int atlasHeight = kHeights[idx];

    // Only ever scale *down* to fit the chosen atlas height, exactly like
    // the original (never upscale past the naturally-measured size).
    double scale = 1.0;
    if (m_rowHeight > atlasHeight)
        scale = double(atlasHeight) / m_rowHeight;

    QVector<QImage> scaled;
    scaled.reserve(m_models.size());
    for (GlyphModel *model : std::as_const(m_models)) {
        const QImage &src = model->image();
        const int w = qMax(1, qRound(src.width() * scale));
        const int h = qMax(1, qRound(src.height() * scale));
        if (w == src.width() && h == src.height())
            scaled.push_back(src);
        else
            scaled.push_back(src.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }

    QImage atlas(atlasWidth, atlasHeight, QImage::Format_ARGB32_Premultiplied);
    atlas.fill(Qt::black);
    QPainter painter(&atlas);
    QVector<double> uOffsets;
    uOffsets.reserve(scaled.size());
    double x = 0.0;
    for (const QImage &img : std::as_const(scaled)) {
        uOffsets.push_back(x / double(atlasWidth));
        painter.drawImage(QPointF(x, 0.0), img);
        x += img.width();
    }
    painter.end();

    const QString chosenPath = QFileDialog::getSaveFileName(
        this, tr("Esporta atlas - scegli il nome base"), QString(), tr("File di testo (*.txt)"));
    if (chosenPath.isEmpty())
        return;

    const QFileInfo info(chosenPath);
    const QString base = info.completeBaseName();
    const QString dir = info.absolutePath();
    const QString txtPath = dir + QLatin1Char('/') + base + QStringLiteral(".txt");
    const QString pngPath = dir + QLatin1Char('/') + base + QStringLiteral(".png");

    QFile txtFile(txtPath);
    if (!txtFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Errore"), tr("Impossibile scrivere %1").arg(txtPath));
        return;
    }
    QTextStream ts(&txtFile);
    for (double u : std::as_const(uOffsets))
        ts << QString::number(u, 'g', 8) << '\n';
    txtFile.close();

    if (!atlas.save(pngPath, "PNG")) {
        QMessageBox::warning(this, tr("Errore"), tr("Impossibile scrivere %1").arg(pngPath));
        return;
    }

    QMessageBox::information(this, tr("Esportazione completata"),
                              tr("File salvati:\n%1\n%2").arg(txtPath, pngPath));
}
