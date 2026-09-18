#pragma once

#include <string>

namespace ks::sim {

class SetupGarage {
public:
    SetupGarage();
    ~SetupGarage();

    void update(float dt);
    // Stubbed - no QPainter in pure Win32/Vulkan path
    void render(int width, int height);

    bool handleKeyPress(int key);
    bool handleKeyRelease(int key);

    void setVisible(bool v) { m_visible = v; }
    bool isVisible() const { return m_visible; }
    void toggleVisible() { m_visible = !m_visible; }
    void setOpacity(float o) { m_opacity = o; }

    struct SetupData {
        float tirePressureFL = 2.2f;
        float tirePressureFR = 2.2f;
        float tirePressureRL = 2.0f;
        float tirePressureRR = 2.0f;
        float brakeBias = 0.56f;
        float rideHeightFront = 30.0f;
        float rideHeightRear = 35.0f;
        float springRateFront = 150.0f;
        float springRateRear = 180.0f;
        float frontWingAngle = 10.0f;
        float rearWingAngle = 12.0f;
        float diffPreload = 30.0f;
    };

    const SetupData& currentSetup() const { return m_setup; }
    void setSetup(const SetupData& s) { m_setup = s; }

private:
    bool m_visible = false;
    float m_opacity = 0.85f;

    int m_selectedRow = 0;
    int m_rowCount = 10;

    float m_fadeIn = 0.0f;
    float m_slideOffset = 0.0f;

    SetupData m_setup;

    bool m_shiftHeld = false;
};

} // namespace ks::sim
