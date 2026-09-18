#include "AnimationTimeline.h"
#include <QDebug>

namespace ks {

AnimationTimeline* AnimationTimeline::s_instance = nullptr;

AnimationTimeline* AnimationTimeline::instance() {
    if (!s_instance) s_instance = new AnimationTimeline();
    return s_instance;
}

AnimationTimeline::AnimationTimeline(QObject* parent) : QObject(parent) {}
AnimationTimeline::~AnimationTimeline() = default;

void AnimationTimeline::createAnimation(const QString&) {}
void AnimationTimeline::deleteAnimation(const QString&) {}
void AnimationTimeline::setCurrentAnimation(const QString&) {}
void AnimationTimeline::renameAnimation(const QString&, const QString&) {}
void AnimationTimeline::duplicateAnimation(const QString&) {}
QVector<AnimationTimeline::Animation> AnimationTimeline::getAnimations() const { return {}; }
AnimationTimeline::Animation AnimationTimeline::getAnimation(const QString&) const { return {}; }

void AnimationTimeline::addTrack(const QString&, const Track&) {}
void AnimationTimeline::removeTrack(const QString&, const QString&) {}
void AnimationTimeline::setTrackLocked(const QString&, const QString&, bool) {}
void AnimationTimeline::setTrackMuted(const QString&, const QString&, bool) {}
void AnimationTimeline::setTrackSolo(const QString&, const QString&, bool) {}
QVector<AnimationTimeline::Track> AnimationTimeline::getTracks(const QString&) const { return {}; }

void AnimationTimeline::addKeyframe(const QString&, const QString&, const Keyframe&) {}
void AnimationTimeline::removeKeyframe(const QString&, const QString&, int) {}
void AnimationTimeline::setKeyframeValue(const QString&, const QString&, int, float) {}
void AnimationTimeline::setKeyframeInterpolation(const QString&, const QString&, int, const QString&) {}
float AnimationTimeline::evaluateTrack(const QString&, const QString&, int) const { return 0.0f; }

void AnimationTimeline::setCurrentFrame(int) {}
void AnimationTimeline::setPlayRange(int, int) {}
void AnimationTimeline::getPlayRange(int& start, int& end) const { start = m_rangeStart; end = m_rangeEnd; }
void AnimationTimeline::setFrameRate(int) {}
void AnimationTimeline::setLoop(bool) {}
void AnimationTimeline::play() {}
void AnimationTimeline::pause() {}
void AnimationTimeline::stop() {}
void AnimationTimeline::togglePlayPause() {}
void AnimationTimeline::stepForward() {}
void AnimationTimeline::stepBackward() {}
void AnimationTimeline::goToStart() {}
void AnimationTimeline::goToEnd() {}

QMap<int, float> AnimationTimeline::bakeTrack(const QString&, const QString&, int) const { return {}; }

void AnimationTimeline::advanceFrame() {}

} // namespace ks
