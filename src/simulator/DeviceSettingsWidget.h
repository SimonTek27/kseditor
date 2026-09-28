#pragma once
/**
 * Compatibility shim: Qt widget only when KSENGINE_QT_FREE is off.
 * Prefer ks::sim::ui::DeviceSettingsOverlay for the simulator binary.
 */
#ifndef KSENGINE_QT_FREE
#include <QWidget>
class DeviceSettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit DeviceSettingsWidget(QWidget* parent = nullptr) : QWidget(parent) {}
};
#else
#include "ui/DeviceSettingsOverlay.h"
namespace ks { namespace sim {
using DeviceSettingsWidget = ui::DeviceSettingsOverlay;
}} // namespace
#endif
