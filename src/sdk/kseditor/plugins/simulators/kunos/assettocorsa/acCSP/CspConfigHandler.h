#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <QDir>

namespace ks {

class CspConfigHandler : public QObject {
    Q_OBJECT

public:
    explicit CspConfigHandler(QObject* parent = nullptr);
    static CspConfigHandler* instance();

    Q_INVOKABLE QVariantMap loadConfig(const QString& path);
    Q_INVOKABLE bool saveConfig(const QString& path, const QVariantMap& sections);

    Q_INVOKABLE QVariantList parseLightSeries(const QVariantMap& sections);
    Q_INVOKABLE QVariantList parseMaterialAdjustments(const QVariantMap& sections);
    Q_INVOKABLE QVariantList parseConditions(const QVariantMap& sections);
    Q_INVOKABLE QVariantList parseLights(const QVariantMap& sections);
    Q_INVOKABLE QVariantList parseShaderReplacements(const QVariantMap& sections);
    Q_INVOKABLE QVariantList parseModelReplacements(const QVariantMap& sections);
    Q_INVOKABLE QVariantList parseIncludes(const QVariantMap& sections);
    Q_INVOKABLE QVariantList parseEmissives(const QVariantMap& sections);
    Q_INVOKABLE QVariantList parseBrakeDiscs(const QVariantMap& sections);
    Q_INVOKABLE QVariantList parseCustomEmissives(const QVariantMap& sections);
    Q_INVOKABLE QVariantList parseCustomEmissiveMultis(const QVariantMap& sections);
    Q_INVOKABLE QVariantList parseMaterialDistantEmissives(const QVariantMap& sections);
    Q_INVOKABLE QVariantList parseMaterialRoomWindows(const QVariantMap& sections);
    Q_INVOKABLE QVariantList parseAbout(const QVariantMap& sections);
    Q_INVOKABLE QVariantList parseSingleSection(const QVariantMap& sections, const QString& type);

    Q_INVOKABLE QVariantMap serializeLightSeries(const QVariantList& items);
    Q_INVOKABLE QVariantMap serializeMaterialAdjustments(const QVariantList& items);
    Q_INVOKABLE QVariantMap serializeConditions(const QVariantList& items);
    Q_INVOKABLE QVariantMap serializeLights(const QVariantList& items);
    Q_INVOKABLE QVariantMap serializeShaderReplacements(const QVariantList& items);
    Q_INVOKABLE QVariantMap serializeModelReplacements(const QVariantList& items);
    Q_INVOKABLE QVariantMap serializeIncludes(const QVariantList& items);
    Q_INVOKABLE QVariantMap serializeEmissives(const QVariantList& items);
    Q_INVOKABLE QVariantMap serializeBrakeDiscs(const QVariantList& items);

    Q_INVOKABLE QVariantMap addSection(QVariantMap sections, const QString& type, const QVariantMap& data);
    Q_INVOKABLE QVariantMap removeSection(QVariantMap sections, const QString& sectionName);
    Q_INVOKABLE QVariantMap updateSection(QVariantMap sections, const QString& sectionName, const QVariantMap& data);

    Q_INVOKABLE QVariantMap loadWithIncludes(const QString& path);
    Q_INVOKABLE QVariantMap mergeConfigs(const QVariantMap& base, const QVariantMap& overlay);

    Q_INVOKABLE QStringList getAvailableSectionTypes();
    Q_INVOKABLE QStringList findSectionsByType(const QVariantMap& sections, const QString& type);
    Q_INVOKABLE QVariantList getSectionProperties(const QVariantMap& sections, const QString& sectionName);

    // Shader compilation & profiling
    Q_INVOKABLE QVariantMap compileShader(const QString& source, bool isFragment,
                                           const QString& entryPoint = "main");
    Q_INVOKABLE QVariantMap profileShader(const QString& source);
    Q_INVOKABLE QStringList compileShaderReplacement(const QVariantMap& sections,
                                                      const QString& sectionName);
    Q_INVOKABLE QVariantList profileShaderReplacements(const QVariantMap& sections);
    Q_INVOKABLE bool saveCompiledShader(const QVariantMap& sections, const QString& sectionName, const QString& outputDir);
    Q_INVOKABLE QStringList findAvailableCompilers();

signals:
    void shaderCompiled(const QString& sectionName, bool success);
    void shaderProfileReady(const QString& sectionName, const QVariantMap& profile);

private:
    static CspConfigHandler* s_instance;

    QVariantMap parseIniFile(const QString& path);
    bool writeIniFile(const QString& path, const QVariantMap& sections);

    QString resolveIncludePath(const QString& basePath, const QString& includePath);
    bool isSectionType(const QString& sectionName, const QString& type);
    QString generateSectionName(const QString& type, const QVariantMap& existingSections);
    int findNextIndex(const QVariantMap& sections, const QString& type);

    QStringList extractList(const QString& str);
    QString joinList(const QStringList& list);
};

}
