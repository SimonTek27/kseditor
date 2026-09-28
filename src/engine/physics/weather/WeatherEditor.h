#ifndef KSENGINE_QT_FREE
#pragma once
#include <QWidget>
namespace ks { namespace physics {
class WeatherEditor : public QWidget {
  Q_OBJECT
public:
  explicit WeatherEditor(QWidget* parent = nullptr);
};
}} // namespace
#endif // !KSENGINE_QT_FREE
