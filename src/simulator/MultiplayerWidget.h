#pragma once
/**
 * Compatibility shim: Qt widget only when KSENGINE_QT_FREE is off.
 * Prefer ks::sim::ui::MultiplayerOverlay for the simulator binary.
 */
#ifndef KSENGINE_QT_FREE
#include <QWidget>
class MultiplayerWidget : public QWidget {
    Q_OBJECT
public:
    explicit MultiplayerWidget(QWidget* parent = nullptr) : QWidget(parent) {}
};
#else
#include "ui/MultiplayerOverlay.h"
namespace ks { namespace sim {
using MultiplayerWidget = ui::MultiplayerOverlay;
}} // namespace
#endif
