#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include "ACEPaths.h"
#include "ACEPackageParser.h"
#include "ACEProtobufDecoder.h"

namespace ks {

class ACEContentQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isAceInstalled READ isAceInstalled NOTIFY contentChanged)
    Q_PROPERTY(QString aceRoot READ getAceRoot NOTIFY contentChanged)
    Q_PROPERTY(QStringList cars READ getCars NOTIFY contentChanged)
    Q_PROPERTY(QStringList tracks READ getTracks NOTIFY contentChanged)
    Q_PROPERTY(QStringList mods READ getMods NOTIFY contentChanged)

public:
    explicit ACEContentQmlBridge(QObject* parent = nullptr);

    bool isAceInstalled() const;
    QString getAceRoot() const;
    QStringList getCars() const;
    QStringList getTracks() const;
    QStringList getMods() const;

    Q_INVOKABLE void setCustomPath(const QString& path);
    Q_INVOKABLE void refresh();

    Q_INVOKABLE QString getContentFolder() const;
    Q_INVOKABLE QString getModsFolder() const;
    Q_INVOKABLE QStringList getSkins(const QString& carName) const;

    Q_INVOKABLE QVariantMap getCarInfo(const QString& carName) const;
    Q_INVOKABLE QStringList getModContents(const QString& modFileName) const;

signals:
    void contentChanged();

private:
    ACEContentFinder* m_finder;
};

class ACEPackageQmlBridge : public QObject {
    Q_OBJECT

public:
    explicit ACEPackageQmlBridge(QObject* parent = nullptr);

    Q_INVOKABLE bool openPackage(const QString& filePath);
    Q_INVOKABLE void closePackage();
    Q_INVOKABLE bool isOpen() const;

    Q_INVOKABLE QVariantMap getManifest() const;
    Q_INVOKABLE QStringList getEntryNames() const;
    Q_INVOKABLE int getEntryCount() const;

    Q_INVOKABLE bool extractFile(const QString& entryName, const QString& outputPath);
    Q_INVOKABLE bool extractAll(const QString& outputDir);

    Q_INVOKABLE QVariantMap getEntryInfo(const QString& entryName) const;

    Q_INVOKABLE QVariantList getFilesByExtension(const QString& ext) const;
    Q_INVOKABLE qint64 getEntrySize(const QString& entryName) const;

signals:
    void packageOpened(const QString& path);
    void packageClosed();
    void error(const QString& message);

private:
    ACEPackageParser* m_parser;
};

class ACEProtobufQmlBridge : public QObject {
    Q_OBJECT

public:
    explicit ACEProtobufQmlBridge(QObject* parent = nullptr);

    Q_INVOKABLE QVariantMap decodeMessage(const QByteArray& data);
    Q_INVOKABLE QString printMessage(const QVariantMap& message);
    Q_INVOKABLE QStringList extractStrings(const QByteArray& data, int minLength = 4);

    Q_INVOKABLE QVariantMap decodeFileContent(const QString& packagePath, const QString& entryPath);
    Q_INVOKABLE QVariantMap decodeHexString(const QString& hexString);

    Q_INVOKABLE QStringList getKnownProtoFiles() const;
    Q_INVOKABLE QVariantMap getFieldNames() const;

private:
    static QStringList s_knownProtoFiles;
    static QVariantMap s_fieldNames;
};

} // namespace ks
