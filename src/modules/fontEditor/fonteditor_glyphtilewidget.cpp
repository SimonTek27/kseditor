#include "fonteditor_glyphtilewidget.h"

#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

GlyphTileWidget::GlyphTileWidget(GlyphModel *model, QWidget *parent)
    : QFrame(parent), m_model(model)
{
    setFrameShape(QFrame::StyledPanel);
    setFixedWidth(112);

    m_titleLabel = new QLabel(displayLabel(model->value()), this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(QStringLiteral("font-weight: 600;"));

    m_previewLabel = new QLabel(this);
    m_previewLabel->setFixedSize(72, 72);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet(
        QStringLiteral("background-color: #1e1e1e; border: 1px solid #4a4a4a;"));

    m_widthSpin = new QSpinBox(this);
    m_widthSpin->setRange(1, 2000);
    m_widthSpin->setPrefix(QStringLiteral("W "));
    m_widthSpin->setToolTip(QStringLiteral("Larghezza cella (px)"));

    m_hPadSpin = new QSpinBox(this);
    m_hPadSpin->setRange(-500, 500);
    m_hPadSpin->setPrefix(QStringLiteral("H "));
    m_hPadSpin->setToolTip(QStringLiteral("Offset orizzontale (px)"));

    m_vPadSpin = new QSpinBox(this);
    m_vPadSpin->setRange(-500, 500);
    m_vPadSpin->setPrefix(QStringLiteral("V "));
    m_vPadSpin->setToolTip(QStringLiteral("Offset verticale (px)"));

    connect(m_widthSpin, &QSpinBox::valueChanged, this, &GlyphTileWidget::onWidthChanged);
    connect(m_hPadSpin, &QSpinBox::valueChanged, this, &GlyphTileWidget::onHPadChanged);
    connect(m_vPadSpin, &QSpinBox::valueChanged, this, &GlyphTileWidget::onVPadChanged);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(4);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_previewLabel, 0, Qt::AlignHCenter);
    layout->addWidget(m_widthSpin);
    layout->addWidget(m_hPadSpin);
    layout->addWidget(m_vPadSpin);

    syncSpinBoxesFromModel();
    refreshPreview();
}

QString GlyphTileWidget::displayLabel(const QString &value)
{
    if (value.isEmpty())
        return QStringLiteral("?");
    const QChar c = value.at(0);
    const int code = c.unicode();
    if (c == QLatin1Char(' '))
        return QStringLiteral("SPACE\n(%1)").arg(code);
    return QStringLiteral("'%1'  (%2)").arg(value).arg(code);
}

void GlyphTileWidget::refreshPreview()
{
    const QImage &img = m_model->image();
    if (img.isNull()) {
        m_previewLabel->clear();
        return;
    }
    const QPixmap pm = QPixmap::fromImage(img).scaled(
        m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_previewLabel->setPixmap(pm);
}

void GlyphTileWidget::syncSpinBoxesFromModel()
{
    m_syncing = true;
    m_widthSpin->setValue(m_model->pixelWidth());
    m_hPadSpin->setValue(m_model->hPadding());
    m_vPadSpin->setValue(m_model->vPadding());
    m_syncing = false;
}

void GlyphTileWidget::onWidthChanged(int value)
{
    if (m_syncing)
        return;
    m_model->setPixelWidth(value);
    emit edited(this);
}

void GlyphTileWidget::onHPadChanged(int value)
{
    if (m_syncing)
        return;
    m_model->setHPadding(value);
    emit edited(this);
}

void GlyphTileWidget::onVPadChanged(int value)
{
    if (m_syncing)
        return;
    m_model->setVPadding(value);
    emit edited(this);
}
