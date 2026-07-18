#include "AiBehaviorModel.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <cmath>
#include <random>
#include <algorithm>

static std::mt19937& rng() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

float AiBehaviorModel::randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng());
}

// ============================================================================
// Label Helpers
// ============================================================================

QString AiBehaviorModel::tierLabel(DifficultyTier t) {
    switch (t) {
        case Novice:       return "Novice";
        case Amateur:      return "Amateur";
        case Intermediate: return "Intermediate";
        case Advanced:     return "Advanced";
        case Expert:       return "Expert";
        case Legendary:    return "Legendary";
    }
    return "Unknown";
}

QString AiBehaviorModel::styleLabel(DrivingStyle s) {
    switch (s) {
        case Aggressive:  return "Aggressive";
        case Defensive:   return "Defensive";
        case Consistent:  return "Consistent";
        case Smooth:      return "Smooth";
        case Balanced:    return "Balanced";
        case Reckless:    return "Reckless";
        case Tactical:    return "Tactical";
        case Endurance:   return "Endurance";
    }
    return "Unknown";
}

// ============================================================================
// Profile Meta
// ============================================================================

AiBehaviorModel::ProfileMeta AiBehaviorModel::computeMeta(const AiDriverProfile& p) {
    ProfileMeta m;
    m.displayName = p.name;
    m.overallRating = overallRating(p);
    m.overtakingRating = overtakingAbility(p);
    m.defendingRating = defendingAbility(p);
    m.consistencyRating = p.consistency;
    m.tireSaverRating = p.tireManagement;

    if (p.aggression >= 0.7f && p.skill >= 0.7f) {
        m.style = Aggressive;
        m.description = "High-risk, high-reward driver who attacks every opportunity.";
        m.shortDescription = "Attacks constantly";
        m.tags << "attacking" << "high-risk" << "exciting";
    } else if (p.defensive >= 0.7f && p.aggression <= 0.6f) {
        m.style = Defensive;
        m.description = "Masters of position defense who rarely give up a place.";
        m.shortDescription = "Tough to pass";
        m.tags << "defensive" << "blocker" << "positional";
    } else if (p.consistency >= 0.85f && p.mistakeRate <= 0.05f) {
        m.style = Consistent;
        m.description = "Brings the car home lap after lap with minimal errors.";
        m.shortDescription = "Rock steady";
        m.tags << "reliable" << "low-mistake" << "steady";
    } else if (p.tireManagement >= 0.8f && p.aggression <= 0.5f) {
        m.style = Smooth;
        m.description = "Gentle on equipment with silky smooth inputs.";
        m.shortDescription = "Easy on tires";
        m.tags << "smooth" << "tire-saver" << "efficient";
    } else if (p.aggression >= 0.8f && p.consistency <= 0.4f) {
        m.style = Reckless;
        m.description = "Unpredictable and dangerous — crashes or wins.";
        m.shortDescription = "Wild card";
        m.tags << "unpredictable" << "dangerous" << "high-crash";
    } else if (p.defensive >= 0.6f && p.consistency >= 0.7f && p.aggression >= 0.5f) {
        m.style = Tactical;
        m.description = "Thinks several moves ahead with strategic racecraft.";
        m.shortDescription = "Strategic thinker";
        m.tags << "strategic" << "calculated" << "smart";
    } else if (p.tireManagement >= 0.7f && p.fuelManagement >= 0.7f && p.consistency >= 0.7f) {
        m.style = Endurance;
        m.description = "Built for the long game with excellent conservation.";
        m.shortDescription = "Long-run specialist";
        m.tags << "endurance" << "conservation" << "long-run";
    } else {
        m.style = Balanced;
        m.description = "Well-rounded driver with no major weaknesses.";
        m.shortDescription = "All-rounder";
        m.tags << "balanced" << "versatile";
    }

    // Difficulty tier
    float rating = m.overallRating;
    if (rating < 0.35f) m.tier = Novice;
    else if (rating < 0.50f) m.tier = Amateur;
    else if (rating < 0.65f) m.tier = Intermediate;
    else if (rating < 0.80f) m.tier = Advanced;
    else if (rating < 0.92f) m.tier = Expert;
    else m.tier = Legendary;

    return m;
}

// ============================================================================
// Profile Loading / Saving
// ============================================================================

AiBehaviorModel::AiDriverProfile AiBehaviorModel::loadProfile(const QString& name) {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                   + "/ai_profiles/" + name + ".json";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        // fallback: check built-in
        auto builtIn = getBuiltInProfiles();
        for (const auto& p : builtIn) {
            if (p.name == name) return p;
        }
        return AiDriverProfile();
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) return AiDriverProfile();

    QJsonObject obj = doc.object();
    AiDriverProfile profile;
    profile.name = obj["name"].toString();
    profile.skill = static_cast<float>(obj["skill"].toDouble());
    profile.aggression = static_cast<float>(obj["aggression"].toDouble());
    profile.defensive = static_cast<float>(obj["defensive"].toDouble());
    profile.consistency = static_cast<float>(obj["consistency"].toDouble());
    profile.mistakeRate = static_cast<float>(obj["mistakeRate"].toDouble());
    profile.tireManagement = static_cast<float>(obj["tireManagement"].toDouble());
    profile.fuelManagement = static_cast<float>(obj["fuelManagement"].toDouble());
    profile.wetSkill = static_cast<float>(obj["wetSkill"].toDouble());
    profile.qualifyingPace = static_cast<float>(obj["qualifyingPace"].toDouble());
    profile.racePace = static_cast<float>(obj["racePace"].toDouble());
    return profile;
}

bool AiBehaviorModel::saveProfile(const AiDriverProfile& profile) {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                  + "/ai_profiles";
    QDir().mkpath(dir);

    QString path = dir + "/" + profile.name + ".json";
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;

    QJsonObject obj;
    obj["name"] = profile.name;
    obj["skill"] = profile.skill;
    obj["aggression"] = profile.aggression;
    obj["defensive"] = profile.defensive;
    obj["consistency"] = profile.consistency;
    obj["mistakeRate"] = profile.mistakeRate;
    obj["tireManagement"] = profile.tireManagement;
    obj["fuelManagement"] = profile.fuelManagement;
    obj["wetSkill"] = profile.wetSkill;
    obj["qualifyingPace"] = profile.qualifyingPace;
    obj["racePace"] = profile.racePace;

    file.write(QJsonDocument(obj).toJson());
    file.close();
    return true;
}

// ============================================================================
// Built-in Profile Lists
// ============================================================================

QVector<AiBehaviorModel::AiDriverProfile> AiBehaviorModel::getBuiltInProfiles() {
    return {
        getNoviceAggressive(), getNoviceDefensive(), getNoviceConsistent(),
        getAmateurAggressive(), getAmateurBalanced(), getAmateurSmooth(),
        getAggressiveRacer(), getDefensiveRacer(), getConsistentRacer(),
        getSmoothRacer(), getTacticalRacer(), getRecklessRacer(),
        getAdvancedAggressive(), getAdvancedTactical(), getAdvancedEndurance(),
        getVeteranDriver(), getExpertTactical(), getExpertEndurance(),
        getLegendaryRacer(), getLegendaryQualifier()
    };
}

QVector<AiBehaviorModel::ProfileMeta> AiBehaviorModel::getBuiltInProfileMeta() {
    auto profiles = getBuiltInProfiles();
    QVector<ProfileMeta> meta;
    for (const auto& p : profiles) {
        meta.append(computeMeta(p));
    }
    return meta;
}

// ============================================================================
// Profile Families
// ============================================================================

QVector<AiBehaviorModel::ProfileFamily> AiBehaviorModel::getProfileFamilies() {
    QVector<ProfileFamily> families;

    // Aggressive family
    {
        ProfileFamily f;
        f.name = "Aggressive";
        f.description = "Attack-minded drivers who prioritize positions over preservation.";
        f.tier = Intermediate;
        f.characteristicValues = {{"aggression", 0.85f}, {"skill", 0.75f},
                                  {"consistency", 0.55f}, {"mistakeRate", 0.15f}};
        f.variants = {getAmateurAggressive(), getAggressiveRacer(), getAdvancedAggressive()};
        families.append(f);
    }

    // Defensive family
    {
        ProfileFamily f;
        f.name = "Defensive";
        f.description = "Masters of defense who make passing nearly impossible.";
        f.tier = Intermediate;
        f.characteristicValues = {{"defensive", 0.9f}, {"consistency", 0.8f},
                                  {"aggression", 0.45f}, {"mistakeRate", 0.05f}};
        f.variants = {getNoviceDefensive(), getDefensiveRacer()};
        families.append(f);
    }

    // Consistent family
    {
        ProfileFamily f;
        f.name = "Consistent";
        f.description = "Reliable lap-after-lap performers with minimal errors.";
        f.tier = Intermediate;
        f.characteristicValues = {{"consistency", 0.92f}, {"mistakeRate", 0.03f},
                                  {"tireManagement", 0.8f}, {"skill", 0.65f}};
        f.variants = {getNoviceConsistent(), getConsistentRacer()};
        families.append(f);
    }

    // Tactical family
    {
        ProfileFamily f;
        f.name = "Tactical";
        f.description = "Strategic drivers who think several moves ahead.";
        f.tier = Advanced;
        f.characteristicValues = {{"defensive", 0.7f}, {"consistency", 0.8f},
                                  {"aggression", 0.6f}, {"skill", 0.8f}};
        f.variants = {getTacticalRacer(), getAdvancedTactical(), getExpertTactical()};
        families.append(f);
    }

    // Endurance family
    {
        ProfileFamily f;
        f.name = "Endurance";
        f.description = "Built for the long game with exceptional conservation skills.";
        f.tier = Advanced;
        f.characteristicValues = {{"tireManagement", 0.9f}, {"fuelManagement", 0.9f},
                                  {"consistency", 0.85f}, {"aggression", 0.45f}};
        f.variants = {getAdvancedEndurance(), getExpertEndurance()};
        families.append(f);
    }

    // Legendary family
    {
        ProfileFamily f;
        f.name = "Legendary";
        f.description = "The absolute elite — maximum skill in every dimension.";
        f.tier = Legendary;
        f.characteristicValues = {{"skill", 0.97f}, {"consistency", 0.95f},
                                  {"qualifyingPace", 0.98f}, {"racePace", 0.95f}};
        f.variants = {getLegendaryRacer(), getLegendaryQualifier()};
        families.append(f);
    }

    return families;
}

QVector<AiBehaviorModel::AiDriverProfile> AiBehaviorModel::getFamilyVariants(
    DrivingStyle style, int count)
{
    auto families = getProfileFamilies();
    for (const auto& f : families) {
        QString fLower = f.name.toLower();
        if (fLower == styleLabel(style).toLower()) {
            QVector<AiDriverProfile> result;
            for (int i = 0; i < qMin(count, f.variants.size()); ++i) {
                result.append(f.variants[i]);
            }
            return result;
        }
    }

    // Fallback: generate variants by scaling
    auto base = getProfileByStyle(style, Intermediate);
    QVector<AiDriverProfile> result;
    for (int i = 0; i < count; ++i) {
        float scale = 0.5f + (static_cast<float>(i) / qMax(1, count - 1)) * 0.5f;
        result.append(scaleToDifficulty(base, static_cast<int>(scale * 100)));
    }
    return result;
}

// ============================================================================
// Scale & Adapt
// ============================================================================

AiBehaviorModel::AiDriverProfile AiBehaviorModel::scaleToDifficulty(
    const AiDriverProfile& base, int difficultyPercent)
{
    float factor = qBound(0.1f, difficultyPercent / 100.0f, 1.0f);
    AdaptiveWeights w;
    w.skill = factor;
    w.aggression = 0.5f + factor * 0.5f;
    w.defensive = 0.3f + factor * 0.7f;
    w.consistency = 0.3f + factor * 0.7f;
    w.mistakeRate = 1.0f - factor * 0.8f;
    w.tireManagement = 0.3f + factor * 0.7f;
    w.fuelManagement = 0.3f + factor * 0.7f;
    w.wetSkill = 0.2f + factor * 0.8f;
    w.qualifyingPace = 0.3f + factor * 0.7f;
    w.racePace = 0.3f + factor * 0.7f;
    return adaptProfile(base, w);
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::adaptProfile(
    const AiDriverProfile& base, const AdaptiveWeights& weights)
{
    AiDriverProfile p = base;
    auto apply = [](float val, float weight) {
        return qBound(0.0f, val * qBound(0.1f, weight, 2.0f), 1.0f);
    };
    p.skill = apply(base.skill, weights.skill);
    p.aggression = apply(base.aggression, weights.aggression);
    p.defensive = apply(base.defensive, weights.defensive);
    p.consistency = apply(base.consistency, weights.consistency);
    p.mistakeRate = apply(base.mistakeRate, weights.mistakeRate);
    p.tireManagement = apply(base.tireManagement, weights.tireManagement);
    p.fuelManagement = apply(base.fuelManagement, weights.fuelManagement);
    p.wetSkill = apply(base.wetSkill, weights.wetSkill);
    p.qualifyingPace = apply(base.qualifyingPace, weights.qualifyingPace);
    p.racePace = apply(base.racePace, weights.racePace);
    return p;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::interpolateProfiles(
    const AiDriverProfile& a, const AiDriverProfile& b, float t)
{
    t = qBound(0.0f, t, 1.0f);
    auto lerp = [t](float va, float vb) { return va + (vb - va) * t; };
    AiDriverProfile r;
    r.name = a.name + "_x_" + b.name;
    r.skill = lerp(a.skill, b.skill);
    r.aggression = lerp(a.aggression, b.aggression);
    r.defensive = lerp(a.defensive, b.defensive);
    r.consistency = lerp(a.consistency, b.consistency);
    r.mistakeRate = lerp(a.mistakeRate, b.mistakeRate);
    r.tireManagement = lerp(a.tireManagement, b.tireManagement);
    r.fuelManagement = lerp(a.fuelManagement, b.fuelManagement);
    r.wetSkill = lerp(a.wetSkill, b.wetSkill);
    r.qualifyingPace = lerp(a.qualifyingPace, b.qualifyingPace);
    r.racePace = lerp(a.racePace, b.racePace);
    return r;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::blendWithTelemetry(
    const AiDriverProfile& base, const QVector<float>& telemetrySkillFactors)
{
    if (telemetrySkillFactors.isEmpty()) return base;

    AiDriverProfile p = base;
    float avgSkill = 0;
    for (float f : telemetrySkillFactors) avgSkill += f;
    avgSkill /= telemetrySkillFactors.size();

    // Blend base profile toward telemetry-observed skill
    float blend = qBound(0.0f, avgSkill, 1.0f);
    p.skill = (base.skill + blend) * 0.5f;
    p.consistency = (base.consistency + telemetrySkillFactors.value(1, base.consistency)) * 0.5f;
    p.racePace = (base.racePace + telemetrySkillFactors.value(2, base.racePace)) * 0.5f;
    p.qualifyingPace = (base.qualifyingPace + telemetrySkillFactors.value(3, base.qualifyingPace)) * 0.5f;

    // Ensure values stay in range
    auto clamp = [](float v) { return qBound(0.0f, v, 1.0f); };
    p.skill = clamp(p.skill);
    p.consistency = clamp(p.consistency);
    p.racePace = clamp(p.racePace);
    p.qualifyingPace = clamp(p.qualifyingPace);

    return p;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getProfileByStyle(
    DrivingStyle style, DifficultyTier tier)
{
    auto profiles = getBuiltInProfiles();
    QVector<QPair<AiDriverProfile, float>> scored;
    for (const auto& p : profiles) {
        auto m = computeMeta(p);
        float styleMatch = (m.style == style) ? 1.0f : 0.0f;
        float tierDiff = qAbs(static_cast<int>(m.tier) - static_cast<int>(tier));
        float tierMatch = qMax(0.0f, 1.0f - tierDiff * 0.25f);
        scored.append({p, styleMatch * 0.6f + tierMatch * 0.4f});
    }

    std::sort(scored.begin(), scored.end(),
              [](const QPair<AiDriverProfile, float>& a,
                 const QPair<AiDriverProfile, float>& b) {
                  return a.second > b.second;
              });

    return scored.isEmpty() ? getBalancedProfile() : scored.first().first;
}

// ============================================================================
// Profile Comparison & Scoring
// ============================================================================

AiBehaviorModel::ProfileComparison AiBehaviorModel::compareProfiles(
    const AiDriverProfile& a, const AiDriverProfile& b)
{
    auto scoreA = scoreProfile(a);
    auto scoreB = scoreProfile(b);

    ProfileComparison comp;
    comp.overallScore = scoreA.overallScore - scoreB.overallScore;

    QStringList cats = {"skill", "aggression", "defensive", "consistency",
                        "tireManagement", "fuelManagement", "wetSkill",
                        "qualifyingPace", "racePace"};

    for (const auto& cat : cats) {
        float sa = scoreA.categoryScores.value(cat, 0);
        float sb = scoreB.categoryScores.value(cat, 0);
        comp.categoryScores[cat] = sa - sb;
    }

    // Strengths = categories where profile A significantly leads
    // Weaknesses = categories where profile A significantly trails
    for (const auto& cat : cats) {
        float diff = comp.categoryScores[cat];
        if (diff > 0.1f)
            comp.strengths.append({cat, diff});
        else if (diff < -0.1f)
            comp.weaknesses.append({cat, -diff});
    }

    std::sort(comp.strengths.begin(), comp.strengths.end(),
              [](const QPair<QString, float>& x, const QPair<QString, float>& y) {
                  return x.second > y.second;
              });
    std::sort(comp.weaknesses.begin(), comp.weaknesses.end(),
              [](const QPair<QString, float>& x, const QPair<QString, float>& y) {
                  return x.second > y.second;
              });

    if (comp.overallScore > 0.05f)
        comp.summary = QString("%1 outperforms %2 by %3 points")
            .arg(a.name).arg(b.name).arg(comp.overallScore, 0, 'f', 1);
    else if (comp.overallScore < -0.05f)
        comp.summary = QString("%1 outperforms %2 by %3 points")
            .arg(b.name).arg(a.name).arg(-comp.overallScore, 0, 'f', 1);
    else
        comp.summary = QString("%1 and %2 are evenly matched").arg(a.name).arg(b.name);

    return comp;
}

AiBehaviorModel::ProfileComparison AiBehaviorModel::scoreProfile(
    const AiDriverProfile& profile)
{
    ProfileComparison comp;
    comp.overallScore = overallRating(profile);

    comp.categoryScores["skill"] = profile.skill;
    comp.categoryScores["aggression"] = overtakingAbility(profile);
    comp.categoryScores["defensive"] = defendingAbility(profile);
    comp.categoryScores["consistency"] = profile.consistency;
    comp.categoryScores["tireManagement"] = profile.tireManagement;
    comp.categoryScores["fuelManagement"] = profile.fuelManagement;
    comp.categoryScores["wetSkill"] = wetPerformance(profile);
    comp.categoryScores["qualifyingPace"] = profile.qualifyingPace;
    comp.categoryScores["racePace"] = profile.racePace;
    comp.categoryScores["racecraft"] = racecraftScore(profile);
    comp.categoryScores["endurance"] = enduranceScore(profile);

    // Top 3 categories
    QVector<QPair<QString, float>> sorted;
    for (auto it = comp.categoryScores.begin(); it != comp.categoryScores.end(); ++it)
        sorted.append({it.key(), it.value()});
    std::sort(sorted.begin(), sorted.end(),
              [](const QPair<QString, float>& a, const QPair<QString, float>& b) {
                  return a.second > b.second;
              });

    for (int i = 0; i < qMin(3, sorted.size()); ++i)
        comp.strengths.append(sorted[i]);
    for (int i = sorted.size() - 1; i >= qMax(0, sorted.size() - 2); --i)
        comp.weaknesses.append(sorted[i]);

    comp.summary = QString("%1 — Overall: %2%")
        .arg(profile.name).arg(qRound(comp.overallScore * 100));
    return comp;
}

float AiBehaviorModel::overallRating(const AiDriverProfile& p) {
    return (p.skill * 0.25f + p.consistency * 0.15f + p.racePace * 0.15f
            + p.qualifyingPace * 0.10f + overtakingAbility(p) * 0.10f
            + defendingAbility(p) * 0.10f + p.tireManagement * 0.05f
            + p.fuelManagement * 0.03f + p.wetSkill * 0.05f
            + (1.0f - p.mistakeRate) * 0.02f);
}

float AiBehaviorModel::overtakingAbility(const AiDriverProfile& p) {
    return qBound(0.0f, p.aggression * 0.5f + p.skill * 0.3f + p.qualifyingPace * 0.2f, 1.0f);
}

float AiBehaviorModel::defendingAbility(const AiDriverProfile& p) {
    return qBound(0.0f, p.defensive * 0.5f + p.consistency * 0.3f + p.skill * 0.2f, 1.0f);
}

float AiBehaviorModel::racecraftScore(const AiDriverProfile& p) {
    return qBound(0.0f, (overtakingAbility(p) + defendingAbility(p) + p.skill) / 3.0f, 1.0f);
}

float AiBehaviorModel::enduranceScore(const AiDriverProfile& p) {
    return qBound(0.0f, (p.tireManagement + p.fuelManagement + p.consistency) / 3.0f, 1.0f);
}

float AiBehaviorModel::wetPerformance(const AiDriverProfile& p) {
    return qBound(0.0f, p.wetSkill * 0.6f + p.skill * 0.2f + p.consistency * 0.2f, 1.0f);
}

// ============================================================================
// Improvement Suggestions
// ============================================================================

QVector<AiBehaviorModel::Suggestion> AiBehaviorModel::getImprovementSuggestions(
    const AiDriverProfile& profile)
{
    QVector<Suggestion> suggestions;
    auto meta = computeMeta(profile);

    auto addSuggestion = [&](const QString& cat, float current, float target,
                             const QString& reason, float impact) {
        if (current >= target) return;
        Suggestion s;
        s.category = cat;
        s.currentValue = current;
        s.suggestedValue = target;
        s.reasoning = reason;
        s.impact = impact;
        suggestions.append(s);
    };

    addSuggestion("skill", profile.skill, qMin(1.0f, profile.skill + 0.15f),
                  "Higher skill improves all aspects of driving", 0.25f);

    if (profile.mistakeRate > 0.15f)
        addSuggestion("mistakeRate", profile.mistakeRate, 0.08f,
                      "Reducing mistakes saves positions and time", 0.15f);

    if (profile.consistency < 0.6f)
        addSuggestion("consistency", profile.consistency, 0.75f,
                      "More consistent laps mean better race results", 0.18f);

    if (profile.tireManagement < 0.5f)
        addSuggestion("tireManagement", profile.tireManagement, 0.65f,
                      "Better tire management reduces pit stops", 0.12f);

    if (profile.aggression < 0.3f && profile.skill > 0.6f)
        addSuggestion("aggression", profile.aggression, 0.45f,
                      "Slightly more aggression can yield overtaking opportunities", 0.08f);

    if (profile.defensive < 0.3f && profile.consistency > 0.6f)
        addSuggestion("defensive", profile.defensive, 0.5f,
                      "Better defense prevents losing positions", 0.10f);

    if (profile.wetSkill < 0.4f)
        addSuggestion("wetSkill", profile.wetSkill, 0.55f,
                      "Improving wet driving adds consistency in changing conditions", 0.08f);

    if (profile.qualifyingPace < profile.racePace)
        addSuggestion("qualifyingPace", profile.qualifyingPace,
                      profile.racePace + 0.05f,
                      "Better qualifying pace improves starting position", 0.10f);

    // Sort by impact
    std::sort(suggestions.begin(), suggestions.end(),
              [](const Suggestion& a, const Suggestion& b) {
                  return a.impact > b.impact;
              });

    return suggestions;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::applySuggestions(
    const AiDriverProfile& profile, const QVector<Suggestion>& suggestions)
{
    auto p = profile;
    for (const auto& s : suggestions) {
        float val = qBound(0.0f, s.suggestedValue, 1.0f);
        if (s.category == "skill") p.skill = val;
        else if (s.category == "mistakeRate") p.mistakeRate = val;
        else if (s.category == "consistency") p.consistency = val;
        else if (s.category == "tireManagement") p.tireManagement = val;
        else if (s.category == "aggression") p.aggression = val;
        else if (s.category == "defensive") p.defensive = val;
        else if (s.category == "wetSkill") p.wetSkill = val;
        else if (s.category == "qualifyingPace") p.qualifyingPace = val;
    }
    return p;
}

// ============================================================================
// Random Profile
// ============================================================================

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getRandomProfile() {
    int styleIdx = static_cast<int>(randomFloat(0, 7.99f));
    int tierIdx = static_cast<int>(randomFloat(0, 5.99f));
    AiDriverProfile p = getProfileByStyle(static_cast<DrivingStyle>(styleIdx),
                                          static_cast<DifficultyTier>(tierIdx));
    // Randomize numeric attributes so consecutive calls are distinct.
    // Skill is biased to a competent range so simulated races reliably progress.
    p.skill          = randomFloat(0.5f, 0.99f);
    p.aggression     = randomFloat(0.0f, 1.0f);
    p.defensive      = randomFloat(0.0f, 1.0f);
    p.consistency    = randomFloat(0.0f, 1.0f);
    p.mistakeRate    = randomFloat(0.0f, 0.5f);
    p.tireManagement = randomFloat(0.0f, 1.0f);
    p.fuelManagement = randomFloat(0.0f, 1.0f);
    p.wetSkill       = randomFloat(0.0f, 1.0f);
    p.qualifyingPace = randomFloat(0.0f, 1.0f);
    p.racePace       = randomFloat(0.0f, 1.0f);
    return p;
}

// ============================================================================
// NOVICE TIER
// ============================================================================

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getNoviceAggressive() {
    AiDriverProfile p;
    p.name = "Novice Aggressive";
    p.skill = 0.30f; p.aggression = 0.70f; p.defensive = 0.15f; p.consistency = 0.25f;
    p.mistakeRate = 0.35f; p.tireManagement = 0.20f; p.fuelManagement = 0.25f;
    p.wetSkill = 0.20f; p.qualifyingPace = 0.30f; p.racePace = 0.25f;
    return p;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getNoviceDefensive() {
    AiDriverProfile p;
    p.name = "Novice Defensive";
    p.skill = 0.25f; p.aggression = 0.20f; p.defensive = 0.60f; p.consistency = 0.40f;
    p.mistakeRate = 0.30f; p.tireManagement = 0.25f; p.fuelManagement = 0.25f;
    p.wetSkill = 0.20f; p.qualifyingPace = 0.20f; p.racePace = 0.25f;
    return p;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getNoviceConsistent() {
    AiDriverProfile p;
    p.name = "Novice Consistent";
    p.skill = 0.28f; p.aggression = 0.25f; p.defensive = 0.25f; p.consistency = 0.55f;
    p.mistakeRate = 0.25f; p.tireManagement = 0.30f; p.fuelManagement = 0.30f;
    p.wetSkill = 0.20f; p.qualifyingPace = 0.22f; p.racePace = 0.28f;
    return p;
}

// ============================================================================
// AMATEUR TIER
// ============================================================================

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getAmateurAggressive() {
    AiDriverProfile p;
    p.name = "Amateur Aggressive";
    p.skill = 0.50f; p.aggression = 0.80f; p.defensive = 0.30f; p.consistency = 0.40f;
    p.mistakeRate = 0.22f; p.tireManagement = 0.30f; p.fuelManagement = 0.35f;
    p.wetSkill = 0.35f; p.qualifyingPace = 0.55f; p.racePace = 0.45f;
    return p;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getAmateurBalanced() {
    AiDriverProfile p;
    p.name = "Amateur Balanced";
    p.skill = 0.45f; p.aggression = 0.40f; p.defensive = 0.40f; p.consistency = 0.50f;
    p.mistakeRate = 0.18f; p.tireManagement = 0.40f; p.fuelManagement = 0.40f;
    p.wetSkill = 0.35f; p.qualifyingPace = 0.40f; p.racePace = 0.42f;
    return p;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getAmateurSmooth() {
    AiDriverProfile p;
    p.name = "Amateur Smooth";
    p.skill = 0.42f; p.aggression = 0.25f; p.defensive = 0.35f; p.consistency = 0.60f;
    p.mistakeRate = 0.15f; p.tireManagement = 0.55f; p.fuelManagement = 0.50f;
    p.wetSkill = 0.35f; p.qualifyingPace = 0.35f; p.racePace = 0.40f;
    return p;
}

// ============================================================================
// INTERMEDIATE TIER
// ============================================================================

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getAggressiveRacer() {
    AiDriverProfile p;
    p.name = "Aggressive";
    p.skill = 0.85f; p.aggression = 0.90f; p.defensive = 0.60f; p.consistency = 0.60f;
    p.mistakeRate = 0.15f; p.tireManagement = 0.40f; p.fuelManagement = 0.50f;
    p.wetSkill = 0.60f; p.qualifyingPace = 0.90f; p.racePace = 0.80f;
    return p;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getDefensiveRacer() {
    AiDriverProfile p;
    p.name = "Defensive";
    p.skill = 0.75f; p.aggression = 0.50f; p.defensive = 0.95f; p.consistency = 0.85f;
    p.mistakeRate = 0.05f; p.tireManagement = 0.70f; p.fuelManagement = 0.70f;
    p.wetSkill = 0.60f; p.qualifyingPace = 0.60f; p.racePace = 0.75f;
    return p;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getConsistentRacer() {
    AiDriverProfile p;
    p.name = "Consistent";
    p.skill = 0.70f; p.aggression = 0.50f; p.defensive = 0.50f; p.consistency = 0.95f;
    p.mistakeRate = 0.02f; p.tireManagement = 0.80f; p.fuelManagement = 0.80f;
    p.wetSkill = 0.70f; p.qualifyingPace = 0.65f; p.racePace = 0.70f;
    return p;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getSmoothRacer() {
    AiDriverProfile p;
    p.name = "Smooth";
    p.skill = 0.80f; p.aggression = 0.30f; p.defensive = 0.50f; p.consistency = 0.92f;
    p.mistakeRate = 0.02f; p.tireManagement = 0.95f; p.fuelManagement = 0.85f;
    p.wetSkill = 0.80f; p.qualifyingPace = 0.70f; p.racePace = 0.82f;
    return p;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getTacticalRacer() {
    AiDriverProfile p;
    p.name = "Tactical";
    p.skill = 0.82f; p.aggression = 0.60f; p.defensive = 0.75f; p.consistency = 0.80f;
    p.mistakeRate = 0.05f; p.tireManagement = 0.70f; p.fuelManagement = 0.75f;
    p.wetSkill = 0.70f; p.qualifyingPace = 0.75f; p.racePace = 0.80f;
    return p;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getRecklessRacer() {
    AiDriverProfile p;
    p.name = "Reckless";
    p.skill = 0.65f; p.aggression = 0.95f; p.defensive = 0.20f; p.consistency = 0.30f;
    p.mistakeRate = 0.30f; p.tireManagement = 0.20f; p.fuelManagement = 0.30f;
    p.wetSkill = 0.30f; p.qualifyingPace = 0.70f; p.racePace = 0.50f;
    return p;
}

// ============================================================================
// ADVANCED TIER
// ============================================================================

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getAdvancedAggressive() {
    AiDriverProfile p;
    p.name = "Advanced Aggressive";
    p.skill = 0.88f; p.aggression = 0.92f; p.defensive = 0.70f; p.consistency = 0.72f;
    p.mistakeRate = 0.08f; p.tireManagement = 0.55f; p.fuelManagement = 0.60f;
    p.wetSkill = 0.72f; p.qualifyingPace = 0.92f; p.racePace = 0.85f;
    return p;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getAdvancedTactical() {
    AiDriverProfile p;
    p.name = "Advanced Tactical";
    p.skill = 0.88f; p.aggression = 0.65f; p.defensive = 0.82f; p.consistency = 0.85f;
    p.mistakeRate = 0.03f; p.tireManagement = 0.78f; p.fuelManagement = 0.80f;
    p.wetSkill = 0.78f; p.qualifyingPace = 0.82f; p.racePace = 0.86f;
    return p;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getAdvancedEndurance() {
    AiDriverProfile p;
    p.name = "Advanced Endurance";
    p.skill = 0.78f; p.aggression = 0.40f; p.defensive = 0.65f; p.consistency = 0.88f;
    p.mistakeRate = 0.02f; p.tireManagement = 0.92f; p.fuelManagement = 0.92f;
    p.wetSkill = 0.80f; p.qualifyingPace = 0.65f; p.racePace = 0.78f;
    return p;
}

// ============================================================================
// EXPERT TIER
// ============================================================================

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getVeteranDriver() {
    AiDriverProfile p;
    p.name = "Veteran";
    p.skill = 0.90f; p.aggression = 0.60f; p.defensive = 0.80f; p.consistency = 0.90f;
    p.mistakeRate = 0.03f; p.tireManagement = 0.90f; p.fuelManagement = 0.90f;
    p.wetSkill = 0.85f; p.qualifyingPace = 0.75f; p.racePace = 0.88f;
    return p;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getExpertTactical() {
    AiDriverProfile p;
    p.name = "Expert Tactical";
    p.skill = 0.93f; p.aggression = 0.70f; p.defensive = 0.88f; p.consistency = 0.90f;
    p.mistakeRate = 0.02f; p.tireManagement = 0.85f; p.fuelManagement = 0.85f;
    p.wetSkill = 0.88f; p.qualifyingPace = 0.88f; p.racePace = 0.92f;
    return p;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getExpertEndurance() {
    AiDriverProfile p;
    p.name = "Expert Endurance";
    p.skill = 0.85f; p.aggression = 0.45f; p.defensive = 0.75f; p.consistency = 0.93f;
    p.mistakeRate = 0.01f; p.tireManagement = 0.97f; p.fuelManagement = 0.95f;
    p.wetSkill = 0.88f; p.qualifyingPace = 0.72f; p.racePace = 0.85f;
    return p;
}

// ============================================================================
// LEGENDARY TIER
// ============================================================================

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getLegendaryRacer() {
    AiDriverProfile p;
    p.name = "Legendary";
    p.skill = 0.97f; p.aggression = 0.75f; p.defensive = 0.90f; p.consistency = 0.95f;
    p.mistakeRate = 0.01f; p.tireManagement = 0.92f; p.fuelManagement = 0.92f;
    p.wetSkill = 0.95f; p.qualifyingPace = 0.95f; p.racePace = 0.96f;
    return p;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getLegendaryQualifier() {
    AiDriverProfile p;
    p.name = "Legendary Qualifier";
    p.skill = 0.98f; p.aggression = 0.55f; p.defensive = 0.85f; p.consistency = 0.96f;
    p.mistakeRate = 0.005f; p.tireManagement = 0.90f; p.fuelManagement = 0.88f;
    p.wetSkill = 0.92f; p.qualifyingPace = 0.99f; p.racePace = 0.92f;
    return p;
}

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getBalancedProfile() {
    AiDriverProfile p;
    p.name = "Balanced";
    p.skill = 0.65f; p.aggression = 0.50f; p.defensive = 0.50f; p.consistency = 0.65f;
    p.mistakeRate = 0.10f; p.tireManagement = 0.55f; p.fuelManagement = 0.55f;
    p.wetSkill = 0.50f; p.qualifyingPace = 0.60f; p.racePace = 0.60f;
    return p;
}

// ============================================================================
// Legacy aliases
// ============================================================================

AiBehaviorModel::AiDriverProfile AiBehaviorModel::getRookieDriver() {
    AiDriverProfile p;
    p.name = "Rookie";
    p.skill = 0.35f; p.aggression = 0.45f; p.defensive = 0.40f; p.consistency = 0.35f;
    p.mistakeRate = 0.30f; p.tireManagement = 0.30f; p.fuelManagement = 0.30f;
    p.wetSkill = 0.25f; p.qualifyingPace = 0.35f; p.racePace = 0.30f;
    return p;
}

// ============================================================================
// Existing behavior calculation methods (unchanged)
// ============================================================================

AiBehaviorModel::AiDecision AiBehaviorModel::calculateDecision(
    const AiDriverProfile& profile, const AiRaceState& state,
    float targetSpeed, float distanceToCarAhead)
{
    AiDecision decision;

    float skillFactor = profile.skill * (0.9f + profile.consistency * 0.1f);

    if (shouldMakeMistake(profile)) {
        float mag = calculateMistakeMagnitude(profile);
        decision.throttle = qMax(0.0f, targetSpeed * 0.01f - mag * 0.3f);
        decision.brake = mag * 0.5f;
        decision.steering = randomFloat(-0.3f, 0.3f) * mag;
        decision.decisionReason = "mistake";
        return decision;
    }

    if (shouldPitStop(profile, state)) {
        decision.shouldPit = true;
        decision.throttle = 0.3f;
        decision.brake = 0.5f;
        decision.decisionReason = "pit_stop";
        return decision;
    }

    if (distanceToCarAhead < 20.0f && shouldOvertake(profile, state)) {
        float overtakeAggr = calculateOvertakeAggression(profile);
        decision.isOvertaking = true;
        decision.throttle = skillFactor * (0.8f + overtakeAggr * 0.2f);
        decision.brake = 0.0f;
        decision.steering = overtakeAggr * 0.3f;
        decision.decisionReason = "overtake";
    } else if (state.isDefending || shouldDefend(profile, state)) {
        float defendLine = calculateDefendLine(profile);
        decision.isDefending = true;
        decision.throttle = skillFactor * 0.9f;
        decision.brake = defendLine * 0.2f;
        decision.steering = defendLine * 0.15f;
        decision.decisionReason = "defend";
    } else {
        decision.throttle = skillFactor * 0.95f;
        decision.brake = 0.0f;
        decision.steering = 0.0f;
        decision.decisionReason = "normal";
    }

    if (state.tireWear > 80.0f) {
        decision.throttle *= 0.9f;
    }
    if (state.fuel < 20.0f) {
        decision.throttle *= 0.95f;
    }

    return decision;
}

float AiBehaviorModel::calculateRacePace(const AiDriverProfile& profile, int lap) {
    float base = profile.racePace;
    float tireDeg = profile.tireManagement * 0.1f;
    float fatigue = qMax(0.0f, (lap - 10) * 0.005f * (1.0f - profile.consistency));
    return qMax(0.5f, base - tireDeg * (lap * 0.01f) - fatigue);
}

bool AiBehaviorModel::shouldPitStop(const AiDriverProfile& profile, const AiRaceState& state) {
    if (state.inPitLane) return false;
    if (state.needsPitStop) return true;

    float tireThreshold = 80.0f - profile.tireManagement * 20.0f;
    float fuelThreshold = 15.0f + profile.fuelManagement * 10.0f;

    return state.tireWear > tireThreshold || state.fuel < fuelThreshold;
}

bool AiBehaviorModel::shouldDefend(const AiDriverProfile& profile, const AiRaceState& state) {
    if (state.gapBehind < 1.5f && profile.defensive > 0.3f) return true;
    if (state.isDefending) return true;
    return state.gapBehind < 2.0f && profile.defensive > 0.5f && profile.aggression > 0.4f;
}

bool AiBehaviorModel::shouldOvertake(const AiDriverProfile& profile, const AiRaceState& state) {
    float prob = calculateOvertakeProbability(
        profile, state.gapBehind < 3.0f ? 10.0f : 0.0f, state.gapAhead);
    return prob > 0.5f;
}

float AiBehaviorModel::calculateOvertakeProbability(
    const AiDriverProfile& profile, float speedDifference, float distanceToCarAhead)
{
    float base = profile.aggression * 0.5f + profile.skill * 0.3f;
    float distanceFactor = qMax(0.0f, 1.0f - distanceToCarAhead / 50.0f);
    float speedFactor = qBound(0.0f, speedDifference / 20.0f, 1.0f);
    return qBound(0.0f, base + distanceFactor * 0.3f + speedFactor * 0.2f, 1.0f);
}

float AiBehaviorModel::calculateOvertakeAggression(const AiDriverProfile& profile) {
    return profile.aggression * (0.7f + profile.skill * 0.3f);
}

float AiBehaviorModel::calculateDefendProbability(const AiDriverProfile& profile, float gapBehind) {
    float base = profile.defensive * 0.6f;
    float gapFactor = qMax(0.0f, 1.0f - gapBehind / 5.0f);
    return qBound(0.0f, base + gapFactor * 0.4f, 1.0f);
}

float AiBehaviorModel::calculateDefendLine(const AiDriverProfile& profile) {
    return profile.defensive * profile.aggression;
}

int AiBehaviorModel::calculateOptimalPitLap(const AiDriverProfile& profile, int totalLaps) {
    float tireWearPerLap = calculateTireWearPerLap(profile);
    float fuelPerLap = calculateFuelForLap(profile);

    float maxLapsOnTires = (tireWearPerLap > 0.001f) ? (100.0f / tireWearPerLap) : totalLaps;
    float maxLapsOnFuel = (fuelPerLap > 0.001f) ? (100.0f / fuelPerLap) : totalLaps;

    float pitWindow = qMin(maxLapsOnTires, maxLapsOnFuel) * 0.85f;
    return qBound(1, static_cast<int>(pitWindow), totalLaps - 5);
}

float AiBehaviorModel::calculateFuelForLap(const AiDriverProfile& profile) {
    return 1.5f - profile.fuelManagement * 0.5f;
}

float AiBehaviorModel::calculateTireWearPerLap(const AiDriverProfile& profile) {
    return 1.2f - profile.tireManagement * 0.4f;
}

bool AiBehaviorModel::shouldMakeMistake(const AiDriverProfile& profile) {
    float mistakeProb = profile.mistakeRate * (1.0f - profile.consistency * 0.5f);
    return randomFloat(0.0f, 1.0f) < mistakeProb;
}

float AiBehaviorModel::calculateMistakeMagnitude(const AiDriverProfile& profile) {
    return randomFloat(0.1f, 1.0f) * (1.0f - profile.consistency);
}

// ============================================================================
// Multi-car interaction
// ============================================================================

AiBehaviorModel::AiDecision AiBehaviorModel::calculateMultiCarDecision(
    const AiDriverProfile& profile, const AiRaceState& state,
    const MultiCarState& multiState, float targetSpeed)
{
    AiDecision decision;
    float skillFactor = profile.skill * (0.9f + profile.consistency * 0.1f);

    if (shouldMakeMistake(profile)) {
        float mag = calculateMistakeMagnitude(profile);
        decision.throttle = qMax(0.0f, targetSpeed * 0.01f - mag * 0.3f);
        decision.brake = mag * 0.5f;
        decision.steering = randomFloat(-0.3f, 0.3f) * mag;
        decision.decisionReason = "mistake (traffic)";
        return decision;
    }

    if (multiState.hasTrafficAhead) {
        float optSpeed = calculateOptimalSpeedInTraffic(profile, multiState, targetSpeed);
        int targetIdx = findOvertakeTarget(profile, multiState);

        if (targetIdx >= 0 && shouldAttemptOvertakeInTraffic(profile, multiState)) {
            decision.isOvertaking = true;
            decision.throttle = skillFactor * (0.85f + profile.aggression * 0.15f);
            decision.brake = 0.0f;
            decision.steering = profile.aggression * 0.25f;
            decision.decisionReason = QString("multi_overtake_car_%1").arg(targetIdx);
        } else {
            decision.throttle = qMin(1.0f, optSpeed / qMax(1.0f, targetSpeed));
            decision.brake = (optSpeed < targetSpeed * 0.8f) ? 0.3f : 0.0f;
            decision.steering = 0.05f * (1.0f - profile.consistency);
            decision.decisionReason = "traffic_follow";
        }
    } else if (shouldDefendMultiple(profile, multiState)) {
        decision.isDefending = true;
        float defendIntensity = qMin(1.0f, multiState.carsNearby * 0.3f);
        decision.throttle = skillFactor * (0.95f - defendIntensity * 0.1f);
        decision.brake = defendIntensity * 0.15f;
        decision.steering = defendIntensity * 0.2f;
        decision.decisionReason = "defend_multiple";
    } else {
        decision.throttle = skillFactor * 0.95f;
        decision.brake = 0.0f;
        decision.steering = 0.0f;
        decision.decisionReason = "normal_racing";
    }

    int carsInDraft = 0;
    for (float gap : multiState.gapsAhead) {
        if (gap > 0 && gap < 15.0f) carsInDraft++;
    }
    if (carsInDraft > 0) {
        decision.throttle *= (1.0f + carsInDraft * 0.02f);
    }

    if (state.tireWear > 80.0f) decision.throttle *= 0.9f;
    if (state.fuel < 20.0f) decision.throttle *= 0.95f;

    return decision;
}

bool AiBehaviorModel::shouldAttemptOvertakeInTraffic(
    const AiDriverProfile& profile, const MultiCarState& multiState)
{
    if (!multiState.hasTrafficAhead || multiState.gapsAhead.isEmpty()) return false;

    float density = calculateTrafficDensity(multiState, 30.0f);
    float overtakeUrgency = profile.aggression * 0.6f + (1.0f - density) * 0.4f;

    float nearestGap = multiState.gapsAhead[0];
    float speedAdvantage = multiState.relativeSpeedsAhead.isEmpty() ? 0.0f : multiState.relativeSpeedsAhead[0];

    return overtakeUrgency > 0.4f && nearestGap < 15.0f && speedAdvantage > -5.0f;
}

bool AiBehaviorModel::shouldDefendMultiple(
    const AiDriverProfile& profile, const MultiCarState& multiState)
{
    if (multiState.gapsBehind.isEmpty()) return false;
    float density = calculateTrafficDensity(multiState, 20.0f);
    float defendNeed = profile.defensive * 0.5f + density * 0.5f;
    return defendNeed > 0.4f;
}

float AiBehaviorModel::calculateTrafficDensity(const MultiCarState& multiState, float range)
{
    int carsInRange = 0;
    for (float gap : multiState.gapsAhead) {
        if (gap > 0 && gap < range) carsInRange++;
    }
    for (float gap : multiState.gapsBehind) {
        if (gap > 0 && gap < range) carsInRange++;
    }
    return qMin(1.0f, carsInRange / 5.0f);
}

float AiBehaviorModel::calculateOptimalSpeedInTraffic(
    const AiDriverProfile& profile, const MultiCarState& multiState, float baseSpeed)
{
    if (!multiState.hasTrafficAhead || multiState.gapsAhead.isEmpty()) return baseSpeed;

    float density = calculateTrafficDensity(multiState, 25.0f);
    float skillFactor = profile.skill * 0.5f + profile.consistency * 0.3f;
    float minGap = 1e6f;
    for (float g : multiState.gapsAhead) {
        if (g > 0 && g < minGap) minGap = g;
    }

    float gapFactor = qMin(1.0f, minGap / 30.0f);
    float speedReduction = (1.0f - skillFactor) * 0.3f + density * 0.2f + (1.0f - gapFactor) * 0.2f;
    return baseSpeed * (1.0f - speedReduction);
}

int AiBehaviorModel::findOvertakeTarget(
    const AiDriverProfile& profile, const MultiCarState& multiState)
{
    if (multiState.gapsAhead.isEmpty()) return -1;

    for (int i = 0; i < multiState.gapsAhead.size(); ++i) {
        float gap = multiState.gapsAhead[i];
        float relSpeed = (i < multiState.relativeSpeedsAhead.size()) ? multiState.relativeSpeedsAhead[i] : 0.0f;

        bool closeEnough = gap < 15.0f * (1.0f + profile.aggression);
        bool fastEnough = relSpeed > -10.0f * (1.0f - profile.skill);
        bool noTrafficBehind = true;
        for (float bg : multiState.gapsBehind) {
            if (bg > 0 && bg < 5.0f) { noTrafficBehind = false; break; }
        }

        if (closeEnough && fastEnough && noTrafficBehind) {
            return i;
        }
    }
    return -1;
}

int AiBehaviorModel::calculateOptimalPitLapForRace(
    const AiDriverProfile& profile, int totalLaps, int fieldSize)
{
    float tireWearPerLap = calculateTireWearPerLap(profile);
    float fuelPerLap = calculateFuelForLap(profile);
    float tireLaps = (tireWearPerLap > 0.001f) ? 100.0f / tireWearPerLap : totalLaps;
    float fuelLaps = (fuelPerLap > 0.001f) ? 100.0f / fuelPerLap : totalLaps;
    float pitWindow = qMin(tireLaps, fuelLaps) * 0.8f;
    int idealLap = qBound(1, static_cast<int>(pitWindow), totalLaps - 3);

    float fieldFactor = qBound(0.8f, 1.0f - fieldSize * 0.01f, 1.2f);
    return qBound(1, static_cast<int>(idealLap * fieldFactor), totalLaps - 3);
}

float AiBehaviorModel::estimateFinishPosition(
    const AiDriverProfile& profile, float trackKnowledge, int fieldSize)
{
    float skillScore = profile.skill * 0.4f + profile.consistency * 0.2f
                      + profile.racePace * 0.2f + profile.qualifyingPace * 0.1f
                      + trackKnowledge * 0.1f;
    float estimatedPos = (1.0f - skillScore) * fieldSize + 1.0f;
    return qBound(1.0f, estimatedPos, static_cast<float>(fieldSize));
}

bool AiBehaviorModel::validateProfile(const AiDriverProfile& profile, QString* error) {
    auto check = [&](float val, const QString& name, float min, float max) {
        if (val < min || val > max) {
            if (error) *error = QString("%1 must be between %2 and %3").arg(name).arg(min).arg(max);
            return false;
        }
        return true;
    };

    if (!check(profile.skill, "skill", 0, 1)) return false;
    if (!check(profile.aggression, "aggression", 0, 1)) return false;
    if (!check(profile.defensive, "defensive", 0, 1)) return false;
    if (!check(profile.consistency, "consistency", 0, 1)) return false;
    if (!check(profile.mistakeRate, "mistakeRate", 0, 1)) return false;
    if (!check(profile.tireManagement, "tireManagement", 0, 1)) return false;
    if (!check(profile.fuelManagement, "fuelManagement", 0, 1)) return false;
    if (!check(profile.wetSkill, "wetSkill", 0, 1)) return false;
    if (!check(profile.qualifyingPace, "qualifyingPace", 0, 1)) return false;
    if (!check(profile.racePace, "racePace", 0, 1)) return false;
    return true;
}
