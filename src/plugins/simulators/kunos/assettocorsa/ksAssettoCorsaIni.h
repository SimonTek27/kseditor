#pragma once

#include <QString>
#include <QMap>
#include <QStringList>
#include <QFile>
#include <QTextStream>

namespace ks { namespace plugins { namespace kunos { namespace ks {

/**
 * @file ksAssettoCorsaIni.h
 * @brief INI file parser for Assetto Corsa physics configuration files.
 * 
 * This module handles all AC physics INI files used by the Kunos physics engine:
 * - car.ini           — Core car parameters (name, engine, drivetrain, aero, etc.)
 * - suspension.ini    — Suspension geometry, springs, dampers, ARBs, ride heights
 * - tyres.ini         — Tyre models, compounds, pressures, temperatures, wear
 * - brakes.ini        — Brake balance, pressure, ducts, temperatures
 * - engine.ini        — Engine curves, RPM limits, turbo, fuel consumption
 * - differential.ini  — Diff type, preload, power/coast ramps, locking
 * - aero.ini          — Wings, diffusers, drag, downforce, DRS
 * - damage.ini        — Mechanical/aero damage models, repair rates
 * - drivetrain.ini    — Clutch, gearbox, final drive, shift times
 * - transmission.ini  — Gear ratios, reverse, gearbox type
 * - limits.ini        — Rev limiter, speed limiter, pit limiter
 * - gear.ini          — Individual gear ratios, RPM drops
 * 
 * The KsIniDocument class provides load/save/section/key access.
 * All files use standard INI format with [SECTION] headers and key=value pairs.
 * Comments start with ; or #.
 */

class KsIniSection {
public:
    KsIniSection(const QString& name = QString()) : m_name(name) {}
    QString name() const { return m_name; }
    QString value(const QString& key) const { return m_values.value(key); }
    void setValue(const QString& key, const QString& val) { m_values[key] = val; }
    QMap<QString, QString> values() const { return m_values; }
    bool hasKey(const QString& key) const { return m_values.contains(key); }
    int getInt(const QString& key, int defaultVal = 0) const { return m_values.value(key).toInt(); }
    float getFloat(const QString& key, float defaultVal = 0.0f) const { return m_values.value(key).toFloat(); }
    double getDouble(const QString& key, double defaultVal = 0.0) const { return m_values.value(key).toDouble(); }
    bool getBool(const QString& key, bool defaultVal = false) const {
        QString v = m_values.value(key).toLower().trimmed();
        if (v == "1" || v == "true" || v == "yes") return true;
        if (v == "0" || v == "false" || v == "no") return false;
        return defaultVal;
    }
private:
    QString m_name;
    QMap<QString, QString> m_values;
    friend class KsIniDocument;
};

class KsIniDocument {
public:
    bool load(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
        m_sections.clear();
        QTextStream in(&file);
        KsIniSection* current = nullptr;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith(';') || line.startsWith('#') || line.startsWith("//"))
                continue;
            if (line.startsWith('[') && line.endsWith(']')) {
                QString name = line.mid(1, line.length() - 2).trimmed();
                m_sections[name] = KsIniSection(name);
                current = &m_sections[name];
                continue;
            }
            int eq = line.indexOf('=');
            if (eq > 0 && current) {
                QString key = line.left(eq).trimmed();
                QString val = line.mid(eq + 1).trimmed();
                if (val.startsWith('"') && val.endsWith('"'))
                    val = val.mid(1, val.length() - 2);
                current->setValue(key, val);
            }
        }
        file.close();
        return true;
    }

    bool save(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
        QTextStream out(&file);
        for (auto it = m_sections.begin(); it != m_sections.end(); ++it) {
            out << "[" << it.key() << "]\n";
            const auto& vals = it.value().values();
            for (auto vit = vals.begin(); vit != vals.end(); ++vit)
                out << vit.key() << "=" << vit.value() << "\n";
            out << "\n";
        }
        file.close();
        return true;
    }

    QString value(const QString& section, const QString& key) const {
        return m_sections.value(section).value(key);
    }
    void setValue(const QString& section, const QString& key, const QString& value) {
        m_sections[section].setValue(key, value);
    }
    QStringList sections() const { return m_sections.keys(); }
    QStringList keys(const QString& section) const {
        return m_sections.value(section).values().keys();
    }

    KsIniSection* section(const QString& name) {
        if (!m_sections.contains(name)) m_sections[name] = KsIniSection(name);
        return &m_sections[name];
    }

    const KsIniSection* section(const QString& name) const {
        auto it = m_sections.find(name);
        if (it != m_sections.end()) return &it.value();
        return nullptr;
    }

private:
    QMap<QString, KsIniSection> m_sections;
};

}}}} // namespace ks::plugins::kunos::ks