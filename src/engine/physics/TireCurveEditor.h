#ifndef KSENGINE_QT_FREE
#pragma once
#include <QWidget>
namespace ks { namespace physics {
class TireCurveEditor : public QWidget {
  Q_OBJECT
public:
  explicit TireCurveEditor(QWidget* parent = nullptr);
};
}} // namespace
#endif // !KSENGINE_QT_FREE
