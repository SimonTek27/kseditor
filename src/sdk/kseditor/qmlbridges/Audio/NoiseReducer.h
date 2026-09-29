#pragma once

#include <QObject>
#include <QVector>

class NoiseReducer : public QObject
{
    Q_OBJECT
public:
    explicit NoiseReducer(QObject *parent = nullptr);
    ~NoiseReducer() override = default;

    void captureNoiseProfile(const QVector<float> &samples);
    void setReductionAmount(float amount);
    QVector<float> reduceNoise(const QVector<float> &samples) const;
    bool hasProfile() const;

private:
    QVector<float> m_noiseProfile;
    float m_reductionAmount = 0.5f;
    bool m_hasProfile = false;
};
