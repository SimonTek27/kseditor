#ifndef KSENGINE_QT_FREE
#pragma once
// Qt WeatherModule — editor only. See git history for full body.
#include <QObject>
#include <QString>
namespace ks { namespace physics {
class WeatherModule : public QObject {
  Q_OBJECT
public:
  explicit WeatherModule(QObject* parent = nullptr);
};
}} // namespace
#endif // !KSENGINE_QT_FREE
