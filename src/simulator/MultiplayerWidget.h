#ifndef KSENGINE_QT_FREE
#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QString>
#include <QVector>
#include <functional>
#include <string>

namespace ks {
namespace sim {

class MultiplayerWidget : public QWidget {
    Q_OBJECT
public:
    explicit MultiplayerWidget(QWidget* parent = nullptr);
    ~MultiplayerWidget() override = default;

    void setHostMode(bool host);
    bool isHostMode() const;

    std::function<void(const QString& address, int port)> onConnectRequested;
    std::function<void()> onDisconnectRequested;
    std::function<void()> onHostRequested;

private:
    void rebuildUi();
    QLineEdit* m_addressEdit = nullptr;
    QSpinBox* m_portSpin = nullptr;
    QListWidget* m_playerList = nullptr;
    bool m_host = false;
};

} // namespace sim
} // namespace ks
#endif // !KSENGINE_QT_FREE
