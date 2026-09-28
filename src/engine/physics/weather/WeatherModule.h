#pragma once
#include <string>
namespace ks { namespace physics {
class WeatherModule {
public:
    void setCondition(const std::string& c) { m_condition = c; }
    const std::string& condition() const { return m_condition; }
    float ambientTempC() const { return m_temp; }
    void setAmbientTempC(float t) { m_temp = t; }
private:
    std::string m_condition = "dry";
    float m_temp = 22.f;
};
}} // namespace
