#pragma once

#include <QString>
#include <QVector>

// AcfFile reads and writes the ".acf" preset format used by the original
// ksFontGenerator. It is a plain INI file with a single [CONFIG] section:
//
//   [CONFIG]
//   FONT = Fixation
//   SIZE = 48
//   FAMILY = Fixation
//   WEIGHT = R          ('R'egular or 'B'old)
//   STYLE = N           ('N'ormal or 'I'talic)
//   HEIGHT = 85         (shared cell height, in px, for every character)
//   HPAD_0 = 0          (per-character horizontal offset, px)
//   VPAD_0 = 13         (per-character vertical offset, px)
//   WIDTH_0 = 21        (per-character cell width, px)
//   ... repeated for index 0..94, one entry per ASCII code 32.126 ...
//
// This exact layout was recovered from a real sample file plus the IL of
// the original butSavePreset_Click/butLoadPreset_Click handlers, so a
// file produced here round-trips through the original .exe (and vice
// versa) without any changes to field names or the 0..94 indexing scheme
// (index = ASCII code - 32).
class AcfFile
{
public:
    static constexpr int kFirstCharCode = 32;  // ' '
    static constexpr int kLastCharCode = 126;  // '~'
    static constexpr int kCharCount = kLastCharCode - kFirstCharCode + 1; // 95

    struct CharMetrics
    {
        int hPadding = 0;
        int vPadding = 0;
        int pixelWidth = 0;
    };

    QString fontName;     // FONT key (display name)
    QString family;       // FAMILY key
    double sizePt = 48.0;  // SIZE key, in points
    bool bold = false;    // WEIGHT: B/R
    bool italic = false;  // STYLE: I/N
    double height = 0.0;  // HEIGHT key: shared row height in px

    QVector<CharMetrics> chars; // always kCharCount entries when valid

    // Reads `path`. Returns false (and sets *error, if given) on failure -
    // missing file, unreadable, or missing required keys.
    bool load(const QString &path, QString *error = nullptr);

    // Writes `path`, overwriting it if it already exists.
    bool save(const QString &path, QString *error = nullptr) const;
};
