#include "CurveWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>

namespace ks {

CurveWidget::CurveWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(200, 150);
}

void CurveWidget::setKeyframes(const QVector<KeyframePoint>& points)
{
    m_keyframes = points;
    update();
}

void CurveWidget::setKeyframeColor(const QColor& color)
{
    m_curveColor = color;
    update();
}

void CurveWidget::setDisplayRange(int startFrame, int endFrame, float minVal, float maxVal)
{
    m_startFrame = startFrame;
    m_endFrame = endFrame;
    m_minVal = minVal;
    m_maxVal = maxVal;
    update();
}

void CurveWidget::clearKeyframes()
{
    m_keyframes.clear();
    update();
}

QPointF CurveWidget::mapPoint(const KeyframePoint& pt, const QRect& rect) const
{
    float margin = 20.0f;
    float drawW = rect.width() - margin * 2.0f;
    float drawH = rect.height() - margin * 2.0f;

    float frameRange = (m_endFrame > m_startFrame) ? (m_endFrame - m_startFrame) : 1.0f;
    float valRange = (m_maxVal > m_minVal) ? (m_maxVal - m_minVal) : 1.0f;

    float x = margin + (pt.frame - m_startFrame) / frameRange * drawW;
    float y = margin + (1.0f - (pt.value - m_minVal) / valRange) * drawH;

    return QPointF(x, y);
}

void CurveWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect r = rect();
    int w = r.width();
    int h = r.height();

    // Background
    p.fillRect(r, QColor(26, 26, 30));

    // Grid
    p.setPen(QPen(QColor(50, 50, 55), 1));
    int gridSpacing = 32;
    for (int x = 0; x < w; x += gridSpacing)
        p.drawLine(x, 0, x, h);
    for (int y = 0; y < h; y += gridSpacing)
        p.drawLine(0, y, w, y);

    // Border
    p.setPen(QPen(QColor(62, 62, 66), 1));
    p.drawRect(r.adjusted(0, 0, -1, -1));

    if (m_keyframes.isEmpty()) {
        p.setPen(QColor(100, 100, 110));
        p.drawText(r, Qt::AlignCenter, "No keyframes");
        return;
    }

    // Draw curve polyline
    QPainterPath path;
    bool first = true;
    for (const auto& kf : m_keyframes) {
        QPointF pt = mapPoint(kf, r);
        if (first) {
            path.moveTo(pt);
            first = false;
        } else {
            path.lineTo(pt);
        }
    }

    p.setPen(QPen(m_curveColor, 2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);

    // Draw keyframe points
    p.setBrush(m_curveColor);
    p.setPen(QPen(Qt::white, 1));
    for (const auto& kf : m_keyframes) {
        QPointF pt = mapPoint(kf, r);
        p.drawEllipse(pt, 4, 4);
    }
}

}
