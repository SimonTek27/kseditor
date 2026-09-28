#ifndef KSENGINE_QT_FREE
#pragma once
#include <QWidget>
namespace ks { namespace print3d {
class PrintPreview : public QWidget {
  Q_OBJECT
public:
  explicit PrintPreview(QWidget* parent = nullptr);
};
}} // namespace
#endif // !KSENGINE_QT_FREE
