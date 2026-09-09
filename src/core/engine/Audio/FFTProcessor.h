#pragma once
#include <QVector>
#include <QObject>

namespace ks { namespace audio {

class FFTProcessor : public QObject {
    Q_OBJECT
public:
    explicit FFTProcessor(QObject* parent = nullptr) : QObject(parent) {}
    void process(const QVector<float>& /*input*/, QVector<float>& /*output*/) {}
    void spectralEdit(float /*threshold*/) {}
    void deHum(float /*freq*/) {}
    void deClick(float /*threshold*/) {}
};

}} // namespace ks::audio
