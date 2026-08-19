#include "BvhImporter.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

namespace ks {

bool BvhImporter::load(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QTextStream ts(&file);
    const QString content = ts.readAll();
    file.close();

    const QStringList lines = content.split(QRegularExpression("[\r\n]+"));
    int idx = 0;

    // Find HIERARCHY / ROOT.
    while (idx < lines.size() && !lines[idx].trimmed().startsWith("ROOT"))
        ++idx;
    if (idx >= lines.size()) return false;

    // Recursive hierarchy parse using a stack of open braces.
    struct StackEntry { int jointIdx = -1; bool seenOffset = false; };
    QVector<StackEntry> stack;
    int channelOffset = 0;

    // First pass is line-driven; joints are created in document order.
    QVector<int> openParents; // parallel to m_joints: parent idx
    while (idx < lines.size()) {
        const QString line = lines[idx].trimmed();
        ++idx;
        if (line.isEmpty()) continue;

        if (line.startsWith("JOINT") || line.startsWith("ROOT") || line.startsWith("End Site")) {
            // End Site has no name; name it after the parent.
            const QString name = line.startsWith("End Site")
                ? (m_joints.isEmpty() ? QStringLiteral("EndSite") : m_joints.last().name + "_End")
                : line.section(' ', 1).trimmed();
            int parent = stack.isEmpty() ? -1 : stack.last().jointIdx;
            BvhJoint j;
            j.name = name;
            j.parent = parent;
            if (parent >= 0) {
                m_joints[parent].children.append(m_joints.size());
            }
            m_joints.append(j);
            stack.append(StackEntry{ int(m_joints.size()) - 1, false });
            continue;
        }

        if (line.startsWith("OFFSET")) {
            const QStringList parts = line.split(QRegularExpression("\\s+"));
            if (parts.size() >= 4 && !stack.isEmpty()) {
                const int ji = stack.last().jointIdx;
                m_joints[ji].offset = QVector3D(parts[1].toFloat(), parts[2].toFloat(), parts[3].toFloat());
                stack.last().seenOffset = true;
            }
            continue;
        }

        if (line.startsWith("CHANNELS")) {
            const QStringList parts = line.split(QRegularExpression("\\s+"));
            if (parts.size() >= 3 && !stack.isEmpty()) {
                const int ji = stack.last().jointIdx;
                const int n = parts[1].toInt();
                m_joints[ji].channelOffset = channelOffset;
                for (int i = 0; i < n && i + 2 < parts.size(); ++i)
                    m_joints[ji].channels.append(parts[i + 2]);
                channelOffset += n;
            }
            continue;
        }

        if (line == "{") continue;
        if (line == "}") {
            if (!stack.isEmpty()) stack.removeLast();
            continue;
        }

        if (line.startsWith("MOTION")) break;
    }

    m_channelCount = channelOffset;
    if (m_joints.isEmpty() || m_channelCount == 0) return false;

    // Motion section.
    bool seenFrames = false, seenFrameTime = false;
    while (idx < lines.size()) {
        const QString line = lines[idx].trimmed();
        ++idx;
        if (line.startsWith("Frames:")) {
            m_frameCount = line.section(':', 1).trimmed().toInt();
            seenFrames = true;
        } else if (line.startsWith("Frame Time:")) {
            m_frameTime = line.section(':', 1).trimmed().toFloat();
            seenFrameTime = true;
        } else if (line.startsWith("MOTION")) {
            // nothing
        } else if (seenFrames && !line.isEmpty()) {
            // A motion line: parse all channels.
            const QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() < m_channelCount) continue;
            QVector<float> f(m_channelCount);
            for (int i = 0; i < m_channelCount; ++i)
                f[i] = parts[i].toFloat();
            m_frames.append(f);
        }
    }
    if (!seenFrames || m_frames.isEmpty()) return false;
    if (!seenFrameTime || m_frameTime <= 0.0f) m_frameTime = 1.0f / 30.0f;
    m_frameCount = m_frames.size();
    return true;
}

QVector<float> BvhImporter::frame(int f) const
{
    return (f >= 0 && f < m_frames.size()) ? m_frames[f] : QVector<float>();
}

} // namespace ks
