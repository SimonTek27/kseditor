#include "fonteditor_acffile.h"

#include <QFile>
#include <QTextStream>
#include <QMap>
#include <QStringList>

namespace {

// Minimal single-section INI reader: good enough for the flat CONFIG
// section this format actually uses, and deliberately tolerant of both
// "KEY = value" and "KEY=value" spacing so files from either the original
// tool or this one parse the same way.
QMap<QString, QString> readConfigSection(const QString &path, bool *ok)
{
    QMap<QString, QString> result;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        *ok = false;
        return result;
    }

    QTextStream in(&f);
    bool inConfig = false;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char(';')) || line.startsWith(QLatin1Char('#')))
            continue;

        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
            inConfig = (line.compare(QLatin1String("[CONFIG]"), Qt::CaseInsensitive) == 0);
            continue;
        }

        if (!inConfig)
            continue;

        const int eq = line.indexOf(QLatin1Char('='));
        if (eq < 0)
            continue;

        const QString key = line.left(eq).trimmed();
        const QString value = line.mid(eq + 1).trimmed();
        result.insert(key, value);
    }

    *ok = true;
    return result;
}

} // namespace

bool AcfFile::load(const QString &path, QString *error)
{
    bool ok = false;
    const QMap<QString, QString> cfg = readConfigSection(path, &ok);
    if (!ok) {
        if (error) *error = QStringLiteral("Impossibile aprire il file: %1").arg(path);
        return false;
    }

    auto require = [&](const char *key, QString *error) -> QString {
        const QString k = QString::fromLatin1(key);
        if (!cfg.contains(k) && error)
            *error = QStringLiteral("Chiave mancante nel file .acf: %1").arg(k);
        return cfg.value(k);
    };

    bool anyMissing = false;
    QString firstError;
    auto get = [&](const char *key) -> QString {
        QString err;
        QString v = require(key, &err);
        if (!err.isEmpty() && firstError.isEmpty()) { firstError = err; anyMissing = true; }
        return v;
    };

    fontName = get("FONT");
    family = get("FAMILY");
    sizePt = get("SIZE").toDouble();
    bold = (get("WEIGHT").compare(QLatin1String("B"), Qt::CaseInsensitive) == 0);
    italic = (get("STYLE").compare(QLatin1String("I"), Qt::CaseInsensitive) == 0);
    height = get("HEIGHT").toDouble();

    if (anyMissing) {
        if (error) *error = firstError;
        return false;
    }

    chars.resize(kCharCount);
    for (int i = 0; i < kCharCount; ++i) {
        const QString hKey = QStringLiteral("HPAD_%1").arg(i);
        const QString vKey = QStringLiteral("VPAD_%1").arg(i);
        const QString wKey = QStringLiteral("WIDTH_%1").arg(i);
        if (!cfg.contains(hKey) || !cfg.contains(vKey) || !cfg.contains(wKey)) {
            if (error) *error = QStringLiteral("Metriche mancanti per il carattere indice %1").arg(i);
            return false;
        }
        chars[i].hPadding = cfg.value(hKey).toInt();
        chars[i].vPadding = cfg.value(vKey).toInt();
        chars[i].pixelWidth = cfg.value(wKey).toInt();
    }

    return true;
}

bool AcfFile::save(const QString &path, QString *error) const
{
    if (chars.size() != kCharCount) {
        if (error) *error = QStringLiteral("Dati dei caratteri incompleti (attesi %1, trovati %2)")
                                 .arg(kCharCount).arg(chars.size());
        return false;
    }

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (error) *error = QStringLiteral("Impossibile scrivere il file: %1").arg(path);
        return false;
    }

    QTextStream out(&f);
    out << "[CONFIG]\n";
    out << "FONT = " << fontName << "\n";
    out << "SIZE = " << QString::number(sizePt, 'g', 8) << "\n";
    out << "FAMILY = " << family << "\n";
    out << "WEIGHT = " << (bold ? "B" : "R") << "\n";
    out << "STYLE = " << (italic ? "I" : "N") << "\n";
    out << "HEIGHT = " << QString::number(height, 'g', 8) << "\n";

    for (int i = 0; i < kCharCount; ++i) {
        out << "HPAD_" << i << " = " << chars[i].hPadding << "\n";
        out << "VPAD_" << i << " = " << chars[i].vPadding << "\n";
        out << "WIDTH_" << i << " = " << chars[i].pixelWidth << "\n";
    }

    return true;
}
