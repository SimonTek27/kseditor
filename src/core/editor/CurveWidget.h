#pragma once

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QColor>

namespace ks {

struct KeyframePoint {
    int frame;
    float value;
};

class CurveWidget : public QWidget {
    Q_OBJECT
public:
    explicit CurveWidget(QWidget* parent = nullptr);

    void setKeyframes(const QVector<KeyframePoint>& points);
    void setKeyframeColor(const QColor& color);
    void setDisplayRange(int startFrame, int endFrame, float minVal, float maxVal);
    void clearKeyframes();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<KeyframePoint> m_keyframes;
    QColor m_curveColor = QColor(0, 120, 215);
    int m_startFrame = 0;
    int m_endFrame = 100;
    float m_minVal = -1.0f;
    float m_maxVal = 1.0f;

    QPointF mapPoint(const KeyframePoint& pt, const QRect& rect) const;
};

}
