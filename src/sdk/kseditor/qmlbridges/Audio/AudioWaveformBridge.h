#pragma once

#include <QObject>
#include <QString>
#include <QVector>

namespace ks {
namespace audio {

class AudioWaveformBridge : public QObject
{
    Q_OBJECT

public:
    explicit AudioWaveformBridge(QObject* parent = nullptr);
    ~AudioWaveformBridge() override;

    void loadFile(const QString& path);
    bool hasData() const { return m_hasData; }

signals:
    void fileLoaded(const QString& path);
    void loadFailed(const QString& path, const QString& reason);

private:
    bool m_hasData = false;
};

} // namespace audio
} // namespace ks
