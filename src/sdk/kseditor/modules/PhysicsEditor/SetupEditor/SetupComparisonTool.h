#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include <QColor>

/**
 * @brief Setup Comparison Tool for Assetto Corsa
 *
 * Compares car setups and shows differences.
 * Based on:
 * - lonemeow/acc-setup-diff
 * - Content Manager setup comparison
 *
 * Features:
 * - Side-by-side setup comparison
 * - Difference highlighting
 * - Impact analysis
 * - Recommendation generation
 * - Export comparison report
 */
class SetupComparisonTool {
public:
    struct SetupData {
        QString name;
        QString carName;
        QString trackName;
        QMap<QString, float> values;
        QMap<QString, QString> stringValues;
    };

    struct ComparisonDiff {
        QString parameter;
        QString category;
        float valueA = 0.0f;
        float valueB = 0.0f;
        float difference = 0.0f;
        float percentChange = 0.0f;
        QString impact;        // "high", "medium", "low"
        QString recommendation;
    };

    struct ComparisonReport {
        SetupData setupA;
        SetupData setupB;
        QVector<ComparisonDiff> differences;
        int totalDifferences = 0;
        int highImpactCount = 0;
        int mediumImpactCount = 0;
        int lowImpactCount = 0;
        QString summary;
    };

    // Comparison operations
    static ComparisonReport compareSetups(const SetupData& setupA, const SetupData& setupB);
    static QVector<ComparisonDiff> findDifferences(const SetupData& setupA, const SetupData& setupB);

    // Data loading
    static SetupData loadSetup(const QString& setupPath);
    static SetupData loadSetupFromIni(const QString& iniPath);
    static bool saveSetup(const SetupData& setup, const QString& setupPath);

    // Analysis
    static QString analyzeImpact(const ComparisonDiff& diff);
    static QString generateRecommendation(const ComparisonDiff& diff);
    static QVector<QString> getHighImpactParameters();

    // Export
    static bool exportComparison(const ComparisonReport& report, const QString& filePath);
    static bool exportToHtml(const ComparisonReport& report, const QString& filePath);
    static bool exportToCsv(const ComparisonReport& report, const QString& filePath);

    // Utility
    static QString formatDifference(float diff, const QString& unit);
    static QString formatPercentChange(float percent);
    static QColor getImpactColor(const QString& impact);
    static QString getCategoryForParameter(const QString& parameter);

private:
    static float calculateDifference(float valueA, float valueB);
    static float calculatePercentChange(float valueA, float valueB);
};

/**
 * @brief Setup Comparison Manager - High-level interface
 */
class SetupComparisonManager {
public:
    SetupComparisonManager();

    // Operations
    bool loadSetupA(const QString& path);
    bool loadSetupB(const QString& path);
    SetupComparisonTool::ComparisonReport compare();

    // Access
    SetupComparisonTool::SetupData getSetupA() const { return m_setupA; }
    SetupComparisonTool::SetupData getSetupB() const { return m_setupB; }
    SetupComparisonTool::ComparisonReport getLastReport() const { return m_lastReport; }

    // Export
    bool exportReport(const QString& filePath, const QString& format = "html");

private:
    SetupComparisonTool::SetupData m_setupA;
    SetupComparisonTool::SetupData m_setupB;
    SetupComparisonTool::ComparisonReport m_lastReport;
};
