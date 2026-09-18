#include "phys_TireModel.h"
#include <QtMath>

namespace ks {

phys_TireModel::phys_TireModel()
    : m_b(6.0), m_c(1.3), m_d(1.0), m_e(-0.5)
{
    m_slipCurve.name = "Default";
    m_slipCurve.compound = "Street";
    m_slipCurve.peakSlipAngle = 8.0;
    m_slipCurve.peakSlipRatio = 0.12;
    m_slipCurve.peakLateralMu = 1.0;
    m_slipCurve.peakLongitudinalMu = 1.1;
    m_slipCurve.stiffnessLateral = 30000.0;
    m_slipCurve.stiffnessLongitudinal = 50000.0;
}

void phys_TireModel::setSlipCurve(const TireSlipCurve& curve) {
    m_slipCurve = curve;
}

double phys_TireModel::pacejkaFormula(double x, double b, double c, double d, double e) const {
    return d * qSin(c * qAtan(b * x - e * (b * x - qAtan(b * x))));
}

double phys_TireModel::calculateLateralForce(double slipAngle, double normalLoad, double frictionCoeff) {
    double x = qDegreesToRadians(slipAngle);
    double peakForce = m_slipCurve.peakLateralMu * normalLoad * frictionCoeff;
    double b = m_b / peakForce;
    return pacejkaFormula(x, b, m_c, peakForce, m_e);
}

double phys_TireModel::calculateLongitudinalForce(double slipRatio, double normalLoad, double frictionCoeff) {
    double peakForce = m_slipCurve.peakLongitudinalMu * normalLoad * frictionCoeff;
    double b = m_b / peakForce;
    return pacejkaFormula(slipRatio, b, m_c, peakForce, m_e);
}

double phys_TireModel::calculateAligningTorque(double slipAngle, double normalLoad) {
    double latForce = calculateLateralForce(slipAngle, normalLoad, 1.0);
    double pneumaticTrail = 0.05 * std::exp(-std::abs(qDegreesToRadians(slipAngle)) * 5.0);
    return latForce * pneumaticTrail;
}

double phys_TireModel::calculatePeakSlipAngle(double normalLoad) const {
    return m_slipCurve.peakSlipAngle * (1.0 + 0.1 * (normalLoad - 4000.0) / 4000.0);
}

double phys_TireModel::calculatePeakSlipRatio(double normalLoad) const {
    return m_slipCurve.peakSlipRatio * (1.0 + 0.05 * (normalLoad - 4000.0) / 4000.0);
}

void phys_TireModel::setPacejkaCoefficients(double b, double c, double d, double e) {
    m_b = b; m_c = c; m_d = d; m_e = e;
}

QVector<QPointF> phys_TireModel::generateLateralForceCurve(double normalLoad, double frictionCoeff) {
    QVector<QPointF> curve;
    for (double angle = -15.0; angle <= 15.0; angle += 0.5) {
        double force = calculateLateralForce(angle, normalLoad, frictionCoeff);
        curve.append(QPointF(angle, force));
    }
    return curve;
}

QVector<QPointF> phys_TireModel::generateLongitudinalForceCurve(double normalLoad, double frictionCoeff) {
    QVector<QPointF> curve;
    for (double slip = -1.0; slip <= 1.0; slip += 0.02) {
        double force = calculateLongitudinalForce(slip, normalLoad, frictionCoeff);
        curve.append(QPointF(slip, force));
    }
    return curve;
}

} // namespace ks