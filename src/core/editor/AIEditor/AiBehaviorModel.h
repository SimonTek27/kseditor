#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include <QPair>
#include <QJsonObject>

/**
 * @brief AI Behavior Model for Assetto Corsa
 *
 * Simulates AI driver behavior for racing.
 * Based on:
 * - AC AI system documentation
 * - Community AI behavior discussions
 * - AC Python API documentation
 *
 * Features:
 * - AI driver profiles
 * - Race behavior simulation
 * - Overtaking logic
 * - Defending logic
 * - Pit stop strategy
 * - Tire management
 * - Adaptive difficulty
 * - Profile families & metadata
 * - Driving style scoring
 */
class AiBehaviorModel {
public:
    // ── Core Profile ────────────────────────────────────────────────────────

    struct AiDriverProfile {
        QString name;
        float skill = 0.8f;            // 0-1
        float aggression = 0.5f;       // 0-1
        float defensive = 0.5f;        // 0-1
        float consistency = 0.7f;      // 0-1
        float mistakeRate = 0.1f;      // 0-1
        float tireManagement = 0.6f;   // 0-1
        float fuelManagement = 0.6f;   // 0-1
        float wetSkill = 0.5f;         // 0-1
        float qualifyingPace = 0.8f;   // 0-1
        float racePace = 0.75f;        // 0-1
    };

    struct AiRaceState {
        int position = 0;
        int totalLaps = 0;
        int currentLap = 0;
        float lapTime = 0.0f;
        float bestLapTime = 0.0f;
        float gapAhead = 0.0f;
        float gapBehind = 0.0f;
        float fuel = 100.0f;
        float tireWear = 0.0f;
        bool inPitLane = false;
        bool needsPitStop = false;
        bool isDefending = false;
        bool isOvertaking = false;
    };

    struct AiDecision {
        float throttle = 0.0f;
        float brake = 0.0f;
        float steering = 0.0f;
        int targetGear = 0;
        bool shouldPit = false;
        bool isDefending = false;
        bool isOvertaking = false;
        QString decisionReason;
    };

    // ── Profile Metadata ────────────────────────────────────────────────────

    enum DifficultyTier {
        Novice = 0,
        Amateur,
        Intermediate,
        Advanced,
        Expert,
        Legendary
    };

    enum DrivingStyle {
        Aggressive = 0,
        Defensive,
        Consistent,
        Smooth,
        Balanced,
        Reckless,
        Tactical,
        Endurance
    };

    struct ProfileMeta {
        QString displayName;
        QString description;
        QString shortDescription;
        DifficultyTier tier = Intermediate;
        DrivingStyle style = Balanced;
        float overallRating = 0.5f;     // computed from profile values
        float overtakingRating = 0.5f;
        float defendingRating = 0.5f;
        float consistencyRating = 0.5f;
        float tireSaverRating = 0.5f;
        QStringList tags;
    };

    struct ProfileFamily {
        QString name;
        QString description;
        DifficultyTier tier;
        QVector<QPair<QString, float>> characteristicValues; // e.g. {skill, aggression, ...}
        QVector<AiDriverProfile> variants;  // slight deviations within the family
    };

    // ── Profile Management ──────────────────────────────────────────────────

    static AiDriverProfile loadProfile(const QString& name);
    static bool saveProfile(const AiDriverProfile& profile);
    static QVector<AiDriverProfile> getBuiltInProfiles();
    static QVector<ProfileMeta> getBuiltInProfileMeta();
    static QVector<ProfileFamily> getProfileFamilies();
    static AiDriverProfile getRandomProfile();
    static AiDriverProfile getProfileByStyle(DrivingStyle style, DifficultyTier tier);

    // ── Extended Presets (8 styles × 3 tiers = 24 profiles) ─────────────────

    // Tier: Novice
    static AiDriverProfile getNoviceAggressive();
    static AiDriverProfile getNoviceDefensive();
    static AiDriverProfile getNoviceConsistent();

    // Tier: Amateur
    static AiDriverProfile getAmateurAggressive();
    static AiDriverProfile getAmateurBalanced();
    static AiDriverProfile getAmateurSmooth();

    // Tier: Intermediate (base presets)
    static AiDriverProfile getAggressiveRacer();
    static AiDriverProfile getDefensiveRacer();
    static AiDriverProfile getConsistentRacer();
    static AiDriverProfile getSmoothRacer();
    static AiDriverProfile getTacticalRacer();
    static AiDriverProfile getRecklessRacer();

    // Tier: Advanced
    static AiDriverProfile getAdvancedAggressive();
    static AiDriverProfile getAdvancedTactical();
    static AiDriverProfile getAdvancedEndurance();

    // Tier: Expert
    static AiDriverProfile getVeteranDriver();
    static AiDriverProfile getExpertTactical();
    static AiDriverProfile getExpertEndurance();

    // Tier: Legendary
    static AiDriverProfile getLegendaryRacer();
    static AiDriverProfile getLegendaryQualifier();

    // Legacy aliases
    static AiDriverProfile getRookieDriver();
    static AiDriverProfile getBalancedProfile();

    // ── Profile Families ────────────────────────────────────────────────────

    static QVector<AiDriverProfile> getFamilyVariants(DrivingStyle style, int count = 3);

    // ── Adaptive Profiles ───────────────────────────────────────────────────

    struct AdaptiveWeights {
        float skill = 1.0f;
        float aggression = 1.0f;
        float defensive = 1.0f;
        float consistency = 1.0f;
        float mistakeRate = 1.0f;
        float tireManagement = 1.0f;
        float fuelManagement = 1.0f;
        float wetSkill = 1.0f;
        float qualifyingPace = 1.0f;
        float racePace = 1.0f;
    };

    static AiDriverProfile adaptProfile(const AiDriverProfile& base,
                                         const AdaptiveWeights& weights);
    static AiDriverProfile scaleToDifficulty(const AiDriverProfile& base, int difficultyPercent);
    static AiDriverProfile interpolateProfiles(const AiDriverProfile& a,
                                                const AiDriverProfile& b, float t);
    static AiDriverProfile blendWithTelemetry(const AiDriverProfile& base,
                                               const QVector<float>& telemetrySkillFactors);

    // ── Profile Comparison & Scoring ────────────────────────────────────────

    struct ProfileComparison {
        float overallScore;
        QMap<QString, float> categoryScores;
        QVector<QPair<QString, float>> strengths;    // top categories
        QVector<QPair<QString, float>> weaknesses;
        QString summary;
    };

    static ProfileComparison compareProfiles(const AiDriverProfile& a,
                                              const AiDriverProfile& b);
    static ProfileComparison scoreProfile(const AiDriverProfile& profile);
    static float overallRating(const AiDriverProfile& profile);
    static float overtakingAbility(const AiDriverProfile& profile);
    static float defendingAbility(const AiDriverProfile& profile);
    static float racecraftScore(const AiDriverProfile& profile);
    static float enduranceScore(const AiDriverProfile& profile);
    static float wetPerformance(const AiDriverProfile& profile);

    // ── Profile Suggestions ─────────────────────────────────────────────────

    struct Suggestion {
        QString category;
        float currentValue;
        float suggestedValue;
        QString reasoning;
        float impact; // 0-1 how much this would improve overall score
    };

    static QVector<Suggestion> getImprovementSuggestions(const AiDriverProfile& profile);
    static AiDriverProfile applySuggestions(const AiDriverProfile& profile,
                                             const QVector<Suggestion>& suggestions);

    // ── Behavior Calculation ────────────────────────────────────────────────

    static AiDecision calculateDecision(const AiDriverProfile& profile,
                                         const AiRaceState& state,
                                         float targetSpeed,
                                         float distanceToCarAhead);

    // Race behavior
    static float calculateRacePace(const AiDriverProfile& profile, int lap);
    static bool shouldPitStop(const AiDriverProfile& profile, const AiRaceState& state);
    static bool shouldDefend(const AiDriverProfile& profile, const AiRaceState& state);
    static bool shouldOvertake(const AiDriverProfile& profile, const AiRaceState& state);

    // Overtaking logic
    static float calculateOvertakeProbability(const AiDriverProfile& profile,
                                               float speedDifference,
                                               float distanceToCarAhead);
    static float calculateOvertakeAggression(const AiDriverProfile& profile);

    // Defending logic
    static float calculateDefendProbability(const AiDriverProfile& profile,
                                             float gapBehind);
    static float calculateDefendLine(const AiDriverProfile& profile);

    // Pit strategy
    static int calculateOptimalPitLap(const AiDriverProfile& profile, int totalLaps);
    static float calculateFuelForLap(const AiDriverProfile& profile);
    static float calculateTireWearPerLap(const AiDriverProfile& profile);

    // Mistake simulation
    static bool shouldMakeMistake(const AiDriverProfile& profile);
    static float calculateMistakeMagnitude(const AiDriverProfile& profile);

    // ── Multi-car interaction ───────────────────────────────────────────────

    struct MultiCarState {
        QVector<float> gapsAhead;
        QVector<float> gapsBehind;
        QVector<float> relativeSpeedsAhead;
        int carsNearby = 0;
        bool hasTrafficAhead = false;
        bool hasTrafficBehind = false;
        float trackPosition = 0.0f;
        float trackLength = 0.0f;
    };

    static AiDecision calculateMultiCarDecision(const AiDriverProfile& profile,
                                                  const AiRaceState& state,
                                                  const MultiCarState& multiState,
                                                  float targetSpeed);

    static bool shouldAttemptOvertakeInTraffic(const AiDriverProfile& profile,
                                                const MultiCarState& multiState);
    static bool shouldDefendMultiple(const AiDriverProfile& profile,
                                      const MultiCarState& multiState);
    static float calculateTrafficDensity(const MultiCarState& multiState, float range);
    static float calculateOptimalSpeedInTraffic(const AiDriverProfile& profile,
                                                 const MultiCarState& multiState,
                                                 float baseSpeed);
    static int findOvertakeTarget(const AiDriverProfile& profile,
                                   const MultiCarState& multiState);

    // Race strategy
    static int calculateOptimalPitLapForRace(const AiDriverProfile& profile,
                                              int totalLaps, int fieldSize);
    static float estimateFinishPosition(const AiDriverProfile& profile,
                                         float trackKnowledge, int fieldSize);

    // Validation
    static bool validateProfile(const AiDriverProfile& profile, QString* error = nullptr);

    // Utility
    static float randomFloat(float min, float max);
    static ProfileMeta computeMeta(const AiDriverProfile& profile);
    static QString tierLabel(DifficultyTier t);
    static QString styleLabel(DrivingStyle s);
};
