#include "LutCurveWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QValueAxis>
#include <QPainter>
#include <QComboBox>
#include <QLabel>
#include <algorithm>

namespace ks {

LutCurveWidget::LutCurveWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    auto* topBar = new QHBoxLayout;
    m_lutSelector = new QComboBox(this);
    m_infoLabel = new QLabel(tr("No LUT loaded"), this);
    m_infoLabel->setStyleSheet("color: gray;");
    topBar->addWidget(new QLabel("LUT:", this));
    topBar->addWidget(m_lutSelector, 1);
    topBar->addWidget(m_infoLabel);

    m_chart = new QChart();
    m_chart->setBackgroundVisible(false);
    m_chart->legend()->hide();

    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(220);

    layout->addLayout(topBar);
    layout->addWidget(m_chartView, 1);

    connect(m_lutSelector, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, [this](const QString& text) {
        if (!text.isEmpty() && !m_currentLut.isEmpty()) {
            QString path = QFileInfo(m_currentLut).absolutePath() + "/" + text;
            loadLutFile(path);
        }
    });
}

bool LutCurveWidget::loadLutFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_infoLabel->setText("Cannot open: " + QFileInfo(path).fileName());
        return false;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    m_xData.clear();
    m_yData.clear();

    QStringList lines = content.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#') || trimmed.startsWith(';')) continue;

        if (trimmed.contains('=')) {
            int eqPos = trimmed.indexOf('=');
            bool okX, okY;
            double x = trimmed.left(eqPos).toDouble(&okX);
            double y = trimmed.mid(eqPos + 1).toDouble(&okY);
            if (okX && okY) {
                m_xData.append(x);
                m_yData.append(y);
            }
        } else {
            bool ok;
            double val = trimmed.toDouble(&ok);
            if (ok) {
                m_xData.append(val);
                m_yData.append(val);
            }
        }
    }

    if (m_xData.isEmpty()) {
        m_infoLabel->setText("Empty or invalid LUT");
        return false;
    }

    int count = m_xData.size();
    double xMin = *std::min_element(m_xData.begin(), m_xData.end());
    double xMax = *std::max_element(m_xData.begin(), m_xData.end());
    double yMin = *std::min_element(m_yData.begin(), m_yData.end());
    double yMax = *std::max_element(m_yData.begin(), m_yData.end());

    m_chart->removeAllSeries();
    m_series = new QLineSeries(this);
    for (int i = 0; i < count; ++i) {
        m_series->append(m_xData[i], m_yData[i]);
    }
    m_chart->addSeries(m_series);

    QValueAxis* axisX = new QValueAxis();
    axisX->setTitleText("Temperature (°C)");
    axisX->setRange(xMin, xMax);
    m_chart->addAxis(axisX, Qt::AlignBottom);
    m_series->attachAxis(axisX);

    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Grip / Factor");
    axisY->setRange(yMin, yMax * 1.1);
    m_chart->addAxis(axisY, Qt::AlignLeft);
    m_series->attachAxis(axisY);

    m_infoLabel->setText(QString("%1 points | X: %2-%3 | Y: %4-%5")
        .arg(count).arg(xMin, 0, 'f', 1).arg(xMax, 0, 'f', 1)
        .arg(yMin, 0, 'f', 3).arg(yMax, 0, 'f', 3));
    m_currentLut = path;

    return true;
}

void LutCurveWidget::setData(const QVector<double>& xData,
                              const QVector<double>& yData,
                              const QString& xLabel,
                              const QString& yLabel) {
    m_xData = xData;
    m_yData = yData;

    m_chart->removeAllSeries();
    m_series = new QLineSeries(this);
    for (int i = 0; i < std::min(xData.size(), yData.size()); ++i) {
        m_series->append(xData[i], yData[i]);
    }
    m_chart->addSeries(m_series);

    double xMin = xData.isEmpty() ? 0 : *std::min_element(xData.begin(), xData.end());
    double xMax = xData.isEmpty() ? 100 : *std::max_element(xData.begin(), xData.end());
    double yMax = yData.isEmpty() ? 1 : *std::max_element(yData.begin(), yData.end());

    QValueAxis* ax = new QValueAxis();
    ax->setTitleText(xLabel);
    ax->setRange(xMin, xMax);
    m_chart->addAxis(ax, Qt::AlignBottom);
    m_series->attachAxis(ax);

    QValueAxis* ay = new QValueAxis();
    ay->setTitleText(yLabel);
    ay->setRange(0, yMax * 1.1);
    m_chart->addAxis(ay, Qt::AlignLeft);
    m_series->attachAxis(ay);
}

void LutCurveWidget::clear() {
    m_chart->removeAllSeries();
    m_xData.clear();
    m_yData.clear();
    m_infoLabel->setText(tr("No LUT loaded"));
    m_lutSelector->clear();
}

void LutCurveWidget::onPointHovered(const QPointF& point, bool isHovering) {
    if (isHovering) {
        m_infoLabel->setText(QString("Point: (%1, %2)").arg(point.x(), 0, 'f', 2).arg(point.y(), 0, 'f', 2));
    } else {
        if (!m_currentLut.isEmpty()) {
            m_infoLabel->setText("LUT: " + QFileInfo(m_currentLut).fileName());
        } else {
            m_infoLabel->setText(tr("No LUT loaded"));
        }
    }
}

QVector<double> LutCurveWidget::parseLutFile(const QString& content) const {
    QVector<double> data;
    QStringList lines = content.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#') || trimmed.startsWith(';')) continue;
        bool ok;
        double val = trimmed.toDouble(&ok);
        if (ok) data.append(val);
    }
    return data;
}

} // namespace ks