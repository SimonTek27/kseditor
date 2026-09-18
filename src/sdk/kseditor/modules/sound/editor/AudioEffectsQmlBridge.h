#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>

namespace ks {
namespace audio {

class AudioEffectsQmlBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int effectCount READ effectCount NOTIFY effectChainChanged)
    Q_PROPERTY(bool masterBypassed READ masterBypassed WRITE setMasterBypass NOTIFY masterBypassChanged)
    Q_PROPERTY(int eqBandCount READ eqBandCount CONSTANT)

public:
    static AudioEffectsQmlBridge* instance();

    explicit AudioEffectsQmlBridge(QObject* parent = nullptr);
    ~AudioEffectsQmlBridge();

    int effectCount() const;
    bool masterBypassed() const;
    int eqBandCount() const;

    // Effect chain management
    Q_INVOKABLE QStringList availableEffectTypes() const;
    Q_INVOKABLE QStringList effectParameters(const QString& type) const;
    Q_INVOKABLE int addEffect(const QString& type);
    Q_INVOKABLE void removeEffect(int index);
    Q_INVOKABLE void clearEffects();
    Q_INVOKABLE QString effectTypeAt(int index) const;
    Q_INVOKABLE bool isEffectBypassed(int index) const;
    Q_INVOKABLE void bypassEffect(int index, bool bypassed);
    Q_INVOKABLE void moveEffect(int from, int to);
    Q_INVOKABLE void resetEffect(int index);

    // Per-effect parameters
    Q_INVOKABLE void setEffectParam(int index, const QString& param, double value);
    Q_INVOKABLE double getEffectParam(int index, const QString& param) const;
    Q_INVOKABLE QVariantMap getEffectParams(int index) const;
    Q_INVOKABLE QVariantList getEffectChain() const;

    // Master chain controls
    Q_INVOKABLE void setMasterBypass(bool bypass);
    Q_INVOKABLE void setEqBand(int band, float gain);
    Q_INVOKABLE float getEqBand(int band) const;
    Q_INVOKABLE void setCompressor(float threshold, float ratio, float attack, float release);
    Q_INVOKABLE void setReverb(float roomSize, float damping, float wet, float dry);
    Q_INVOKABLE void setDelay(float time, float feedback, float mix);
    Q_INVOKABLE void setLimiter(float threshold, float release);
    Q_INVOKABLE void resetMasterChain();

    // Presets
    Q_INVOKABLE QVariantMap savePreset(const QString& name) const;
    Q_INVOKABLE void loadPreset(const QVariantMap& preset);
    Q_INVOKABLE void applyEqPreset(const QString& name);

    // Process audio through the entire chain
    Q_INVOKABLE QVariantList processAudio(const QVariantList& samples, int sampleRate);

signals:
    void effectChainChanged();
    void effectParamChanged(int index, const QString& param, double value);
    void masterBypassChanged();
    void masterSettingsChanged();

private:
    static AudioEffectsQmlBridge* s_instance;
};

} // namespace audio
} // namespace ks
