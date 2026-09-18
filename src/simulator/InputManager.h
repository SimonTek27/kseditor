#pragma once

#include <memory>
#include <unordered_set>

#ifdef _WIN32
namespace ks::device { class XInputDevice; }
#endif

namespace ks::sim {

class InputManager {
public:
    InputManager();
    ~InputManager();

    bool initialize();
    void update();

    double throttle() const { return m_throttle; }
    double brake()    const { return m_brake; }
    double steer()    const { return m_steer; }
    double clutch()   const { return m_clutch; }

    double rawThrottle() const { return m_rawThrottle; }
    double rawBrake()    const { return m_rawBrake; }
    double rawSteer()    const { return m_rawSteer; }

    bool shiftUp()   const { return m_shiftUp; }
    bool shiftDown() const { return m_shiftDown; }

    bool hasXInput() const { return m_xinput != nullptr; }
    bool isXInputConnected() const;
#ifdef _WIN32
    ks::device::XInputDevice* xinput() const { return m_xinput.get(); }
#endif

    void setKeyDown(int key) { m_keys.insert(key); }
    void setKeyUp(int key) { m_keys.erase(key); }
    bool isKeyDown(int key) const { return m_keys.count(key) > 0; }

    void setSteerGamma(double g) { m_steeringGamma = g; }
    void setDeadZone(double dz) { m_deadZone = dz; }
    void setInvertSteer(bool i) { m_invertSteer = i; }

    void reset();

private:
    void processKeyboard();
    void processXInput();
    void applyDeadZone(double& value, double deadZone) const;

    double m_throttle = 0, m_brake = 0, m_steer = 0, m_clutch = 0;
    double m_rawThrottle = 0, m_rawBrake = 0, m_rawSteer = 0;

    bool m_shiftUp = false, m_shiftDown = false;
    bool m_prevE = false, m_prevQ = false;

    double m_steeringGamma = 1.5;
    double m_deadZone = 0.05;
    bool m_invertSteer = false;

    std::unordered_set<int> m_keys;

#ifdef _WIN32
    std::unique_ptr<ks::device::XInputDevice> m_xinput;
#endif
};

} // namespace ks::sim
