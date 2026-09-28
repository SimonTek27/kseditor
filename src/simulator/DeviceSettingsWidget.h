#ifndef KSENGINE_QT_FREE
#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QString>
#include <functional>

namespace ks {
namespace sim {

class DeviceSettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit DeviceSettingsWidget(QWidget* parent = nullptr);
    ~DeviceSettingsWidget() override = default;

    std::function<void()> onApplyRequested;
    std::function<void()> onRescanRequested;

private:
    void rebuildUi();
    QComboBox* m_deviceCombo = nullptr;
    QSlider* m_ffbStrength = nullptr;
    QCheckBox* m_invertSteer = nullptr;
};

} // namespace sim
} // namespace ks
#endif // !KSENGINE_QT_FREE
