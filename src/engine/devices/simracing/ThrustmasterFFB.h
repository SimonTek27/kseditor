#pragma once
#include <string>
#include "SimRacingDevices.h"
#include <vector>
#include <mutex>

namespace ks::device {

struct ThrustmasterDeviceInfo {
    uint16_t pid = 0;
    const char* name = "";
};

class ThrustmasterFFB : public FFBBase {
public:
    ThrustmasterFFB();
    ~ThrustmasterFFB() override;

    bool initialize() override;
    void shutdown() override;
    bool isConnected() const override;
    void updateFFB(float torqueNm) override;
    std::string modelName() const;

    void setGain(float g);
    void setMaxTorque(float nm);

private:
    static const std::vector<ThrustmasterDeviceInfo>& knownDevices();
    bool openDevice();
    void closeDevice();

    uint16_t m_productId = 0;
    bool m_connected = false;
    float m_gain = 1.0f;
    float m_maxTorque = 12.0f;
    std::mutex m_mutex;
};

} // namespace ks::device
