#include "core/animation/AnimationSystem.h"
#include <QtTest/QtTest>

using namespace ks;

typedef GraphEditor::FCurve GraphFCurve;

class TestAnimationSystem : public QObject {
    Q_OBJECT

private slots:
    void testGraphCurveEvaluateLinear();
    void testGraphCurveEvaluateConstant();
    void testGraphCurveAddRemoveKeyframe();
    void testGraphCurveSort();
    void testAnimationTimelineCreateAnimation();
    void testAnimationTimelineTracks();
    void testAnimationTimelineKeyframes();
    void testAnimationTimelinePlayback();
    void testGraphEditorCurve();
    void testGraphEditorKeyframes();
    void testGraphEditorInterpolation();
    void testDopeSheetChannels();
    void testNLAEditorTracks();
    void testNLAEditorStrips();
    void testDriversEditorLifecycle();
};

void TestAnimationSystem::testGraphCurveEvaluateLinear()
{
    GraphCurve curve;
    Keyframe kf1, kf2;
    kf1.frame = 0;
    kf1.value = 0.0f;
    kf1.interpolation = Keyframe::InterpolationLinear;
    kf2.frame = 10;
    kf2.value = 100.0f;
    kf2.interpolation = Keyframe::InterpolationLinear;

    curve.addKeyframe(kf1);
    curve.addKeyframe(kf2);

    float mid = curve.evaluate(5.0f);
    QVERIFY(qAbs(mid - 50.0f) < 0.001f);

    float start = curve.evaluate(0.0f);
    QCOMPARE(start, 0.0f);

    float end = curve.evaluate(10.0f);
    QCOMPARE(end, 100.0f);
}

void TestAnimationSystem::testGraphCurveEvaluateConstant()
{
    GraphCurve curve;
    Keyframe kf1, kf2;
    kf1.frame = 0;
    kf1.value = 10.0f;
    kf1.interpolation = Keyframe::InterpolationConstant;
    kf2.frame = 10;
    kf2.value = 20.0f;
    kf2.interpolation = Keyframe::InterpolationConstant;

    curve.addKeyframe(kf1);
    curve.addKeyframe(kf2);

    float at5 = curve.evaluate(5.0f);
    QCOMPARE(at5, 10.0f);

    float at10 = curve.evaluate(10.0f);
    QCOMPARE(at10, 20.0f);
}

void TestAnimationSystem::testGraphCurveAddRemoveKeyframe()
{
    GraphCurve curve;
    Keyframe kf;
    kf.frame = 5;
    kf.value = 42.0f;
    curve.addKeyframe(kf);
    QVERIFY(curve.getKeyframe(5) != nullptr);
    QCOMPARE(curve.getKeyframe(5)->value, 42.0f);

    curve.removeKeyframe(5);
    QVERIFY(curve.getKeyframe(5) == nullptr);
}

void TestAnimationSystem::testGraphCurveSort()
{
    GraphCurve curve;
    Keyframe kf1, kf2, kf3;
    kf1.frame = 30; kf1.value = 3.0f;
    kf2.frame = 10; kf2.value = 1.0f;
    kf3.frame = 20; kf3.value = 2.0f;

    curve.addKeyframe(kf1);
    curve.addKeyframe(kf2);
    curve.addKeyframe(kf3);
    curve.sortKeyframes();

    QCOMPARE(curve.keyframes[0].frame, 10);
    QCOMPARE(curve.keyframes[1].frame, 20);
    QCOMPARE(curve.keyframes[2].frame, 30);
}

void TestAnimationSystem::testAnimationTimelineCreateAnimation()
{
    AnimationTimeline* tl = AnimationTimeline::instance();
    tl->createAnimation("Walk");
    tl->createAnimation("Run");

    QVector<AnimationTimeline::Animation> anims = tl->getAnimations();
    bool foundWalk = false, foundRun = false;
    for (const auto& a : anims) {
        if (a.name == "Walk") foundWalk = true;
        if (a.name == "Run") foundRun = true;
    }
    QVERIFY(foundWalk);
    QVERIFY(foundRun);
}

void TestAnimationSystem::testAnimationTimelineTracks()
{
    AnimationTimeline* tl = AnimationTimeline::instance();
    tl->createAnimation("Test");

    AnimationTimeline::Track track;
    track.id = "posX";
    track.name = "Position X";
    track.property = "position.x";
    tl->addTrack("Test", track);

    QVector<AnimationTimeline::Track> tracks = tl->getTracks("Test");
    QCOMPARE(tracks.size(), 1);
    QCOMPARE(tracks[0].name, QString("Position X"));

    tl->removeTrack("Test", "posX");
    tracks = tl->getTracks("Test");
    QCOMPARE(tracks.size(), 0);
}

void TestAnimationSystem::testAnimationTimelineKeyframes()
{
    AnimationTimeline* tl = AnimationTimeline::instance();
    tl->createAnimation("Bounce");
    AnimationTimeline::Track track;
    track.id = "height";
    track.name = "Height";
    track.property = "position.y";
    tl->addTrack("Bounce", track);

    AnimationTimeline::Keyframe kf;
    kf.frame = 0;
    kf.value = 0.0f;
    tl->addKeyframe("Bounce", "height", kf);
    kf.frame = 10;
    kf.value = 100.0f;
    tl->addKeyframe("Bounce", "height", kf);

    float mid = tl->evaluateTrack("Bounce", "height", 5);
    QVERIFY(qAbs(mid - 50.0f) < 0.001f);
}

void TestAnimationSystem::testAnimationTimelinePlayback()
{
    AnimationTimeline* tl = AnimationTimeline::instance();
    tl->createAnimation("WalkCycle");
    tl->setCurrentAnimation("WalkCycle");
    tl->setFrameRate(30);
    tl->setLoop(true);
    tl->setPlayRange(0, 60);

    QCOMPARE(tl->fps(), 30);
    QVERIFY(tl->isLooping());
    QCOMPARE(tl->frameStart(), 0);
    QCOMPARE(tl->frameEnd(), 60);
}

void TestAnimationSystem::testGraphEditorCurve()
{
    GraphEditor editor;
    editor.addCurve("position.x");
    GraphFCurve* curve = editor.getCurve("position.x");
    QVERIFY(curve != nullptr);
    QCOMPARE(curve->dataPath, QString("position.x"));

    editor.removeCurve("position.x");
    QVERIFY(editor.getCurve("position.x") == nullptr);
}

void TestAnimationSystem::testGraphEditorKeyframes()
{
    GraphEditor editor;
    editor.addCurve("rotation.y");

    GraphFCurve* curve = editor.getCurve("rotation.y");
    QVERIFY(curve != nullptr);

    curve->addKeyframe(0, 0.0f);
    curve->addKeyframe(30, 360.0f);

    QCOMPARE(curve->keyframes.size(), 2);
    float mid = curve->evaluate(15.0f);
    QVERIFY(qAbs(mid - 180.0f) < 0.001f);

    curve->removeKeyframe(0);
    QCOMPARE(curve->keyframes.size(), 1);
}

void TestAnimationSystem::testGraphEditorInterpolation()
{
    GraphEditor editor;
    editor.addCurve("scale.x");
    editor.setInterpolation("scale.x", Keyframe::InterpolationLinear);
}

void TestAnimationSystem::testDopeSheetChannels()
{
    DopeSheet sheet;
    DopeSheet::Action act;
    act.name = "idle";
    act.frameStart = 0;
    act.frameEnd = 30;
    QVector<DopeSheet::Action> actions = {act};
    act.name = "walk";
    act.frameStart = 40;
    act.frameEnd = 70;
    actions.append(act);

    sheet.setActions(actions);
    QCOMPARE(sheet.getChannels().size(), 0);
}

void TestAnimationSystem::testNLAEditorTracks()
{
    NLAEditor nla;
    nla.addTrack("Track1");
    nla.addTrack("Track2");
    QCOMPARE(nla.getTracks().size(), 2);

    nla.removeTrack(0);
    QCOMPARE(nla.getTracks().size(), 1);
}

void TestAnimationSystem::testNLAEditorStrips()
{
    NLAEditor nla;
    nla.addTrack("Main");
    nla.addTrack("Overlay");

    NLAEditor::NLAStrip strip;
    strip.name = "Jump";
    strip.actionName = "jump_anim";
    strip.frameStart = 0;
    strip.frameEnd = 20;
    strip.blendIn = 2.0f;
    strip.blendOut = 2.0f;

    nla.addStrip(0, strip);
    QCOMPARE(nla.getTracks()[0].strips.size(), 1);

    nla.removeStrip(0, 0);
    QCOMPARE(nla.getTracks()[0].strips.size(), 0);
}

void TestAnimationSystem::testDriversEditorLifecycle()
{
    DriversEditor drivers;
    drivers.addDriver("rotation.wheel_FL", DriversEditor::Driver::Type::Average);
    drivers.addDriver("rotation.wheel_FR", DriversEditor::Driver::Type::Sum);

    DriversEditor::DriverVariable var;
    var.name = "speed";
    var.type = DriversEditor::DriverVariable::Type::SingleProperty;
    var.dataPath = "chassis.speed";
    drivers.addVariable("rotation.wheel_FL", var);

    drivers.evaluateDriver(DriversEditor::Driver(), 1.0f);
}

QTEST_MAIN(TestAnimationSystem)
#include "test_AnimationSystem.moc"
