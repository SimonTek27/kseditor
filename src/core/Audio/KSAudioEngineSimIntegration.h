#ifndef KSAUDIOENGINESIMINTEGRATION_H
#define KSAUDIOENGINESIMINTEGRATION_H

#include <QObject>
#include <QString>
#include <QVector>
#include "KSRPMRecorder.h"
#include "KSCarAcoustics.h"
#include "KSAudioGenerator.h"
#include "KSRPMProfile.h"
#include "KSAudioModuleBridge.h"

namespace ks {
namespace audio {

class KSAudioEngineSimIntegration : public QObject {
    Q_OBJECT
public:
    static KSAudioEngineSimIntegration* instance();

    KSRPMRecorder* recorder() const { return m_recorder; }
    KSCarAcoustics* acoustics() const { return m_acoustics; }
    KSAudioGenerator* audioGenerator() const { return m_audioGenerator; }
    KSAudioModuleBridge* bridge() const { return m_bridge; }

    bool initialize();
    void shutdown();

    QString workspacePath() const { return m_workspacePath; }
    void setWorkspacePath(const QString& path) { m_workspacePath = path; }

signals:
    void initialized();
    void shutdowned();
    void error(const QString& message);

private:
    explicit KSAudioEngineSimIntegration(QObject* parent = nullptr);
    ~KSAudioEngineSimIntegration();

    static KSAudioEngineSimIntegration* s_instance;

    KSRPMRecorder* m_recorder = nullptr;
    KSCarAcoustics* m_acoustics = nullptr;
    KSAudioGenerator* m_audioGenerator = nullptr;
    KSAudioModuleBridge* m_bridge = nullptr;
    QString m_workspacePath;
};

}
}

#endif
