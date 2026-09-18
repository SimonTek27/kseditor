#include "QRCodeWriter.h"
#include <QtMath>
#include <QDebug>
#include <QPainter>
#include <algorithm>
#include <cstring>

namespace ks {

bool QRCodeWriter::s_tablesInit = false;
int QRCodeWriter::s_gfLog[256];
int QRCodeWriter::s_gfExp[512];

static const int ALPHANUMERIC_TABLE[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    36, -1, -1, -1, 37, 38, -1, -1, 39, 40, -1, 41, 42, 43, 44, 45,
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  -1, -1, 46, 47, 48, -1,
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1
};

int QRCodeWriter::alphanumericValue(const QChar& c) {
    ushort u = c.unicode();
    if (u < 128) return ALPHANUMERIC_TABLE[u];
    return -1;
}

// Data codewords per version for each ECC level
static const int DATA_CODEWORDS[41][4] = {
    {0,0,0,0}, {19,16,13,9}, {34,28,22,16}, {55,44,34,26}, {80,64,48,36},
    {108,86,62,46}, {136,108,76,60}, {156,124,88,66}, {194,154,110,86},
    {232,182,132,100}, {274,216,154,122}, {324,254,180,140}, {370,290,206,158},
    {428,334,244,180}, {461,365,261,197}, {523,415,295,223}, {589,453,325,253},
    {647,507,367,283}, {721,563,397,313}, {795,627,445,341},
    {861,669,485,385}, {932,714,512,406}, {1006,782,568,442}, {1094,860,614,464},
    {1174,914,664,514}, {1276,1000,718,538}, {1370,1062,754,596},
    {1468,1128,808,628}, {1531,1193,871,661}, {1631,1267,911,701},
    {1735,1373,985,745}, {1843,1455,1033,793}, {1955,1541,1115,845},
    {2071,1631,1171,901}, {2191,1725,1231,961}, {2306,1812,1286,986},
    {2434,1914,1354,1054}, {2566,1992,1426,1096}, {2702,2102,1502,1142},
    {2812,2216,1582,1212}, {2956,2334,1666,1278}
};

// Total codewords per version
static const int TOTAL_CODEWORDS[41] = {
    0, 26, 44, 70, 100, 134, 172, 196, 242, 292, 346, 404, 466, 532, 581, 655,
    733, 815, 901, 991, 1085, 1156, 1258, 1364, 1474, 1588, 1706, 1828, 1921,
    2051, 2185, 2323, 2465, 2611, 2761, 2876, 3034, 3196, 3362, 3532, 3706
};

// ECC codewords per block for each ECC level
static const int ECC_PER_BLOCK[41][4] = {
    {0,0,0,0}, {7,10,13,17}, {10,16,22,28}, {15,26,18,22}, {20,18,26,16},
    {26,24,18,22}, {18,16,24,28}, {20,18,18,26}, {24,22,22,26}, {30,22,20,24},
    {18,26,24,28}, {20,30,28,24}, {24,22,26,28}, {26,22,24,22}, {30,24,20,24},
    {22,24,30,24}, {24,28,24,30}, {28,28,28,28}, {30,28,28,30}, {28,26,26,26},
    {28,26,30,28}, {28,30,24,30}, {28,28,30,30}, {30,28,28,30}, {30,28,30,30},
    {26,28,30,30}, {28,28,28,30}, {30,30,30,30}, {30,30,30,30}, {30,30,30,30},
    {30,30,30,30}, {30,30,30,30}, {30,30,30,30}, {30,30,30,30}, {30,30,30,30},
    {30,30,30,30}, {30,30,30,30}, {30,30,30,30}, {30,30,30,30}, {30,30,30,30}
};

// Number of blocks per version for each ECC level
static const int BLOCKS[41][4] = {
    {0,0,0,0}, {1,1,1,1}, {1,1,1,1}, {1,1,2,2}, {1,2,2,4},
    {1,2,4,2}, {2,4,2,4}, {2,4,4,5}, {2,4,4,6}, {2,4,6,8},
    {2,4,6,8}, {4,4,4,8}, {2,4,8,10}, {4,6,8,10}, {3,6,8,12},
    {5,6,10,12}, {5,6,10,12}, {1,6,12,16}, {5,8,12,16}, {5,8,12,16},
    {5,8,12,16}, {6,10,12,16}, {7,10,12,16}, {8,12,12,16}, {8,10,12,16},
    {8,12,12,16}, {9,14,12,16}, {9,11,14,16}, {9,13,14,16}, {9,13,14,16},
    {5,13,14,16}, {5,13,15,16}, {5,13,15,16}, {5,13,15,16}, {5,13,15,16},
    {5,13,15,16}, {5,13,15,16}, {5,13,15,16}, {5,13,15,16}, {5,13,15,16}
};

// Alignment pattern positions per version
static const int ALIGNMENT[41][7] = {
    {}, {6,18,0,0,0,0,0}, {6,22,0,0,0,0,0}, {6,26,0,0,0,0,0}, {6,30,0,0,0,0,0},
    {6,34,0,0,0,0,0}, {6,22,38,0,0,0,0}, {6,24,42,0,0,0,0}, {6,26,46,0,0,0,0},
    {6,28,50,0,0,0,0}, {6,30,54,0,0,0,0}, {6,32,58,0,0,0,0}, {6,34,62,0,0,0,0},
    {6,26,46,66,0,0,0}, {6,26,48,70,0,0,0}, {6,26,50,74,0,0,0}, {6,30,54,78,0,0,0},
    {6,30,56,82,0,0,0}, {6,30,58,86,0,0,0}, {6,32,60,88,0,0,0}, {6,34,62,90,0,0,0},
    {6,28,50,72,94,0,0}, {6,26,50,74,98,0,0}, {6,30,54,78,102,0,0},
    {6,28,54,80,106,0,0}, {6,32,58,84,110,0,0}, {6,30,58,86,114,0,0},
    {6,34,62,90,118,0,0}, {6,26,50,74,98,122,0}, {6,30,54,78,102,126,0},
    {6,26,52,78,104,130,0}, {6,30,56,82,108,134,0}, {6,34,60,86,112,138,0},
    {6,30,58,86,114,142,0}, {6,34,62,90,118,146,0}, {6,30,54,78,102,126,150},
    {6,24,50,76,102,128,154}, {6,28,54,80,106,132,158}, {6,32,58,84,110,136,162},
    {6,26,54,82,110,138,166}, {6,30,58,86,114,142,170}
};

static const int VERSION_BITS[40] = {
    0, 0, 0, 0, 0, 0, 0, 0x07C94, 0x085BC, 0x09A99, 0x0A4D3,
    0x0BBF6, 0x0C762, 0x0D847, 0x0E60D, 0x0F928, 0x10B78, 0x1145D,
    0x12A17, 0x13532, 0x149A6, 0x15683, 0x168C9, 0x177EC, 0x18EC4,
    0x191E1, 0x1AFAB, 0x1B08E, 0x1CC1A, 0x1D33F, 0x1ED75, 0x1F250,
    0x209D5, 0x216F0, 0x228BA, 0x2379F, 0x24B0B, 0x2542E, 0x26A64,
    0x27541
};

int QRCodeWriter::dataCodewords(int version, EccLevel eccLevel) {
    if (version < 1 || version > 40) return 0;
    return DATA_CODEWORDS[version][eccLevel];
}

int QRCodeWriter::totalCodewords(int version) {
    if (version < 1 || version > 40) return 0;
    return TOTAL_CODEWORDS[version];
}

int QRCodeWriter::eccCodewords(int version, EccLevel eccLevel) {
    if (version < 1 || version > 40) return 0;
    return ECC_PER_BLOCK[version][eccLevel];
}

int QRCodeWriter::blocks(int version, EccLevel eccLevel) {
    if (version < 1 || version > 40) return 0;
    return BLOCKS[version][eccLevel];
}

int QRCodeWriter::blockDataWords(int version, EccLevel eccLevel) {
    int totalData = dataCodewords(version, eccLevel);
    int numBlocks = blocks(version, eccLevel);
    if (numBlocks == 0) return 0;
    return totalData / numBlocks;
}

int QRCodeWriter::chooseVersion(const QString& text, EccLevel eccLevel) {
    int len = text.length();
    for (int v = 1; v <= 6; ++v) {
        int cap = 0;
        if (eccLevel == L) cap = v <= 1 ? 25 : v == 2 ? 47 : v == 3 ? 77 : v == 4 ? 114 : v == 5 ? 154 : 195;
        else if (eccLevel == M) cap = v <= 1 ? 20 : v == 2 ? 38 : v == 3 ? 61 : v == 4 ? 90 : v == 5 ? 122 : 154;
        else if (eccLevel == Q) cap = v <= 1 ? 16 : v == 2 ? 29 : v == 3 ? 47 : v == 4 ? 67 : v == 5 ? 87 : 108;
        else cap = v <= 1 ? 10 : v == 2 ? 20 : v == 3 ? 35 : v == 4 ? 50 : v == 5 ? 64 : 84;
        if (len <= cap) return v;
    }
    return 6;
}

int QRCodeWriter::gfMul(int a, int b) {
    if (a == 0 || b == 0) return 0;
    return s_gfExp[s_gfLog[a] + s_gfLog[b]];
}

void QRCodeWriter::initTables() {
    if (s_tablesInit) return;
    int x = 1;
    for (int i = 0; i < 255; ++i) {
        s_gfExp[i] = x;
        s_gfLog[x] = i;
        x = (x * 2) ^ ((x >> 7) * 0x11D);
    }
    for (int i = 255; i < 512; ++i) s_gfExp[i] = s_gfExp[i - 255];
    s_tablesInit = true;
}

QVector<unsigned char> QRCodeWriter::encodeAlphanumeric(const QString& text, int version, EccLevel eccLevel) {
    initTables();
    int capacity = dataCodewords(version, eccLevel);
    QVector<unsigned char> result;

    int modeBits = 0x02;
    int charCount = text.length();
    int charCountBits = version <= 9 ? 9 : version <= 26 ? 11 : 13;

    int bitPos = 0;
    unsigned int buffer = 0;
    int bitsInBuffer = 0;

    auto writeBits = [&](unsigned int value, int count) {
        buffer = (buffer << count) | value;
        bitsInBuffer += count;
        while (bitsInBuffer >= 8) {
            result.append(static_cast<unsigned char>(buffer >> (bitsInBuffer - 8)));
            bitsInBuffer -= 8;
        }
    };

    writeBits(modeBits, 4);
    writeBits(charCount, charCountBits);

    int i = 0;
    while (i < text.length()) {
        if (i + 1 < text.length()) {
            int v = alphanumericValue(text[i]) * 45 + alphanumericValue(text[i + 1]);
            writeBits(v, 11);
            i += 2;
        } else {
            int v = alphanumericValue(text[i]);
            writeBits(v, 6);
            i += 1;
        }
    }

    // Terminator
    int remaining = capacity * 8 - (bitPos + bitsInBuffer + 4);
    if (remaining > 0) writeBits(0, qMin(4, remaining));
    else writeBits(0, 0);

    // Flush
    if (bitsInBuffer > 0) {
        result.append(static_cast<unsigned char>(buffer >> (bitsInBuffer - 8)));
    }

    // Pad to capacity
    while (result.size() < capacity) {
        result.append(0xEC);
        if (result.size() < capacity) result.append(0x11);
    }

    return result;
}

QVector<unsigned char> QRCodeWriter::encodeByte(const QString& text, int version, EccLevel eccLevel) {
    initTables();
    int capacity = dataCodewords(version, eccLevel);
    QVector<unsigned char> result;

    int modeBits = 0x04;
    int charCount = text.length();
    int charCountBits = version <= 9 ? 8 : version <= 26 ? 16 : 16;

    int bitPos = 0;
    unsigned int buffer = 0;
    int bitsInBuffer = 0;

    auto writeBits = [&](unsigned int value, int count) {
        buffer = (buffer << count) | value;
        bitsInBuffer += count;
        while (bitsInBuffer >= 8) {
            result.append(static_cast<unsigned char>(buffer >> (bitsInBuffer - 8)));
            bitsInBuffer -= 8;
        }
    };

    writeBits(modeBits, 4);
    writeBits(charCount, charCountBits);

    QByteArray utf8 = text.toUtf8();
    for (char c : utf8) {
        writeBits(static_cast<unsigned char>(c), 8);
    }

    int totalBits = bitPos + bitsInBuffer;
    int terminator = 4;
    int remaining = capacity * 8 - totalBits - terminator;
    if (remaining > 0) writeBits(0, qMin(4, remaining));

    if (bitsInBuffer > 0) {
        result.append(static_cast<unsigned char>(buffer >> (bitsInBuffer - 8)));
    }

    while (result.size() < capacity) {
        result.append(0xEC);
        if (result.size() < capacity) result.append(0x11);
    }

    return result;
}

QVector<unsigned char> QRCodeWriter::rsEncode(const QVector<unsigned char>& data, int eccCount) {
    initTables();
    QVector<unsigned char> gen(eccCount + 1, 0);
    gen[0] = 1;
    for (int i = 0; i < eccCount; ++i) {
        for (int j = i; j > 0; --j) {
            gen[j] = gen[j - 1] ^ gfMul(gen[j], s_gfExp[i]);
        }
        gen[0] = gfMul(gen[0], s_gfExp[i]);
    }

    QVector<unsigned char> msg(data.size() + eccCount, 0);
    for (int i = 0; i < data.size(); ++i) msg[i] = data[i];

    for (int i = 0; i < data.size(); ++i) {
        if (msg[i] != 0) {
            for (int j = 0; j < gen.size(); ++j) {
                msg[i + j] ^= gfMul(gen[j], msg[i]);
            }
        }
    }

    QVector<unsigned char> ecc(eccCount);
    for (int i = 0; i < eccCount; ++i) {
        ecc[i] = msg[data.size() + i];
    }
    return ecc;
}

QVector<unsigned char> QRCodeWriter::interleave(const QVector<unsigned char>& data,
                                                  const QVector<unsigned char>& ecc,
                                                  int version, EccLevel eccLevel)
{
    int numBlocks = blocks(version, eccLevel);
    int blockSize = blockDataWords(version, eccLevel);
    int eccSize = eccCodewords(version, eccLevel);

    QVector<QVector<unsigned char>> dataBlocks(numBlocks);
    QVector<QVector<unsigned char>> eccBlocks(numBlocks);

    for (int b = 0; b < numBlocks; ++b) {
        dataBlocks[b].resize(blockSize);
        for (int i = 0; i < blockSize; ++i) {
            dataBlocks[b][i] = data[b * blockSize + i];
        }
        QVector<unsigned char> block(data.data() + b * blockSize, data.data() + (b + 1) * blockSize);
        eccBlocks[b] = rsEncode(block, eccSize);
    }

    QVector<unsigned char> interleaved;
    for (int i = 0; i < blockSize; ++i) {
        for (int b = 0; b < numBlocks; ++b) {
            interleaved.append(dataBlocks[b][i]);
        }
    }
    for (int i = 0; i < eccSize; ++i) {
        for (int b = 0; b < numBlocks; ++b) {
            interleaved.append(eccBlocks[b][i]);
        }
    }

    return interleaved;
}

void QRCodeWriter::placeFinder(QImage& image, int row, int col, int ms) {
    QPainter painter(&image);
    painter.setPen(Qt::NoPen);

    // Outer 7x7 dark
    painter.setBrush(Qt::black);
    painter.drawRect(col, row, 7 * ms, 7 * ms);

    // Inner 5x5 light
    painter.setBrush(Qt::white);
    painter.drawRect(col + ms, row + ms, 5 * ms, 5 * ms);

    // Center 3x3 dark
    painter.setBrush(Qt::black);
    painter.drawRect(col + 2 * ms, row + 2 * ms, 3 * ms, 3 * ms);

    // Separator (white border)
    painter.setBrush(Qt::white);
    painter.drawRect(col - ms, row, ms, 7 * ms);
    painter.drawRect(col + 7 * ms, row, ms, 7 * ms);
    painter.drawRect(col, row - ms, 7 * ms, ms);
    painter.drawRect(col, row + 7 * ms, 7 * ms, ms);

    painter.end();
}

void QRCodeWriter::placeAlignment(QImage& image, int row, int col, int ms) {
    QPainter painter(&image);
    painter.setPen(Qt::NoPen);

    painter.setBrush(Qt::black);
    painter.drawRect(col, row, 5 * ms, 5 * ms);

    painter.setBrush(Qt::white);
    painter.drawRect(col + ms, row + ms, 3 * ms, 3 * ms);

    painter.setBrush(Qt::black);
    painter.drawRect(col + 2 * ms, row + 2 * ms, ms, ms);

    painter.end();
}

void QRCodeWriter::placeTiming(QImage& image, int moduleSize, int version) {
    QPainter painter(&image);
    painter.setPen(Qt::NoPen);
    int size = (17 + version * 4) * moduleSize;

    for (int i = 8; i < size / moduleSize - 8; ++i) {
        bool dark = (i % 2 == 0);
        painter.setBrush(dark ? Qt::black : Qt::white);

        int pos = i * moduleSize;
        // Horizontal
        int y = 6 * moduleSize;
        painter.drawRect(pos, y, moduleSize, moduleSize);
        // Vertical
        painter.drawRect(y, pos, moduleSize, moduleSize);
    }
    painter.end();
}

static const unsigned int FORMAT_MASK_DATA[32] = {
    0x5412, 0x5125, 0x5E7C, 0x5B4B, 0x45F9, 0x40CE, 0x4F97, 0x4AA0,
    0x77C4, 0x72F3, 0x7DAA, 0x789D, 0x662F, 0x6318, 0x6C41, 0x6976,
    0x1689, 0x13BE, 0x1CE7, 0x19D0, 0x0762, 0x0255, 0x0D0C, 0x083B,
    0x355F, 0x3068, 0x3F31, 0x3A06, 0x24B4, 0x2183, 0x2EDA, 0x2BED
};

void QRCodeWriter::placeFormat(QImage& image, int mask, int moduleSize, EccLevel eccLevel) {
    int ecIdx = static_cast<int>(eccLevel);
    int formatIdx = ecIdx * 8 + mask;
    unsigned int formatBits = FORMAT_MASK_DATA[formatIdx];

    QPainter painter(&image);
    painter.setPen(Qt::NoPen);
    int size = image.width();

    auto setModule = [&](int row, int col, bool dark) {
        int x = col * moduleSize;
        int y = row * moduleSize;
        if (x >= 0 && x < size && y >= 0 && y < size) {
            painter.setBrush(dark ? Qt::black : Qt::white);
            painter.drawRect(x, y, moduleSize, moduleSize);
        }
    };

    for (int i = 0; i < 15; ++i) {
        bool bit = (formatBits >> (14 - i)) & 1;
        // Horizontal at row 8
        if (i <= 5) setModule(8, i, bit);
        else if (i == 6) setModule(8, 7, bit);
        else if (i <= 7) setModule(8, 8, bit);
        else setModule(8, 14 - i + 7 + 1, bit);

        // Vertical at col 8
        if (i <= 5) setModule(i, 8, bit);
        else if (i == 6) setModule(7, 8, bit);
        else if (i <= 7) setModule(8 - (i - 6), 8, bit);
        else setModule(14 - i + 7 + 1, 8, bit);
    }

    // Dark module
    setModule(size / moduleSize - 8, 8, true);

    // Format bits on right/bottom side
    for (int i = 0; i < 15; ++i) {
        bool bit = (formatBits >> i) & 1;
        // Bottom-left horizontal
        if (i <= 5) setModule(size / moduleSize - 1 - i, 8, bit);
        else if (i <= 6) setModule(size / moduleSize - 1 - 6, 8, bit);
        else if (i <= 7) setModule(size / moduleSize - 1 - (8 - (i - 6)), 8, bit);
        else setModule(size / moduleSize - 1 - (i - 7), 8, bit);

        // Right vertical
        if (i <= 5) setModule(8, size / moduleSize - 1 - i, bit);
        else if (i <= 6) setModule(8, size / moduleSize - 1 - 6, bit);
        else if (i <= 7) setModule(8, size / moduleSize - 1 - (8 - (i - 6)), bit);
        else setModule(8, size / moduleSize - 1 - (i - 7), bit);
    }

    painter.end();
}

void QRCodeWriter::placeVersion(QImage& image, int version, int moduleSize) {
    if (version < 7) return;
    unsigned int versionBits = static_cast<unsigned int>(VERSION_BITS[version - 1]);

    QPainter painter(&image);
    painter.setPen(Qt::NoPen);
    int size = image.width() / moduleSize;

    for (int i = 0; i < 18; ++i) {
        bool bit = (versionBits >> (17 - i)) & 1;
        // Bottom-left
        int row = size - 11 + i / 3;
        int col = i % 3;
        painter.setBrush(bit ? Qt::black : Qt::white);
        painter.drawRect(col * moduleSize, row * moduleSize, moduleSize, moduleSize);
        // Top-right
        painter.drawRect((size - 11 + i / 3) * moduleSize, (i % 3) * moduleSize, moduleSize, moduleSize);
    }
    painter.end();
}

int QRCodeWriter::applyMask(int row, int col, int mask) {
    switch (mask) {
        case 0: return (row + col) % 2 == 0;
        case 1: return row % 2 == 0;
        case 2: return col % 3 == 0;
        case 3: return (row + col) % 3 == 0;
        case 4: return (row / 2 + col / 3) % 2 == 0;
        case 5: return (row * col) % 2 + (row * col) % 3 == 0;
        case 6: return ((row * col) % 2 + (row * col) % 3) % 2 == 0;
        case 7: return ((row + col) % 2 + (row * col) % 3) % 2 == 0;
    }
    return 0;
}

void QRCodeWriter::placeData(QImage& image, const QVector<unsigned char>& codewords,
                              int version, int moduleSize, int mask)
{
    int modules = 17 + version * 4;
    QPainter painter(&image);
    painter.setPen(Qt::NoPen);

    QVector<QVector<bool>> reserved(modules, QVector<bool>(modules, false));
    QVector<QVector<bool>> dataBits(modules, QVector<bool>(modules, false));

    // Mark reserved areas
    auto markRect = [&](int r, int c, int h, int w) {
        for (int y = r; y < r + h && y < modules; ++y)
            for (int x = c; x < c + w && x < modules; ++x)
                if (y >= 0 && x >= 0) reserved[y][x] = true;
    };

    // Finder patterns + separators
    markRect(0, 0, 9, 9);
    markRect(0, modules - 8, 9, 8);
    markRect(modules - 8, 0, 8, 9);

    // Timing patterns
    for (int i = 8; i < modules - 8; ++i) {
        reserved[6][i] = true;
        reserved[i][6] = true;
    }

    // Dark module
    reserved[modules - 8][8] = true;

    // Format info areas
    for (int i = 0; i <= 8; ++i) {
        if (i == 6) continue;
        reserved[8][i] = true;
        reserved[i][8] = true;
    }
    for (int i = modules - 8; i < modules; ++i) {
        reserved[8][i] = true;
        reserved[i][8] = true;
    }

    // Alignment patterns
    const int* alignPos = ALIGNMENT[version];
    for (int ai = 0; ai < 7 && alignPos[ai] != 0; ++ai) {
        for (int aj = 0; aj < 7 && alignPos[aj] != 0; ++aj) {
            int ar = alignPos[ai] - 2;
            int ac = alignPos[aj] - 2;
            if (ar >= 0 && ac >= 0) {
                bool overlapsFinder = (ar <= 9 && ac <= 9)
                    || (ar <= 9 && ac >= modules - 10)
                    || (ar >= modules - 10 && ac <= 9);
                if (!overlapsFinder) {
                    markRect(ar, ac, 5, 5);
                }
            }
        }
    }

    // Place codeword bits
    int bitIdx = 0;
    int col = modules - 1;
    int dir = -1;

    while (col >= 1) {
        if (col == 6) { col = 5; continue; }

        for (int row = (dir == -1) ? modules - 1 : 0;
             row >= 0 && row < modules;
             row += dir)
        {
            for (int c = 0; c < 2; ++c) {
                int cx = col - c;
                if (cx < 0) continue;
                if (!reserved[row][cx]) {
                    if (bitIdx < codewords.size() * 8) {
                        int byteIdx = bitIdx / 8;
                        int bitInByte = 7 - (bitIdx % 8);
                        bool bit = (codewords[byteIdx] >> bitInByte) & 1;
                        bool masked = bit ^ (applyMask(row, cx, mask) != 0);
                        dataBits[row][cx] = masked;
                    }
                    ++bitIdx;
                }
            }
        }
        dir = -dir;
        col -= 2;
    }

    // Draw data modules
    for (int r = 0; r < modules; ++r) {
        for (int c = 0; c < modules; ++c) {
            if (!reserved[r][c]) {
                painter.setBrush(dataBits[r][c] ? Qt::black : Qt::white);
                painter.drawRect(c * moduleSize, r * moduleSize, moduleSize, moduleSize);
            }
        }
    }

    painter.end();
}

int QRCodeWriter::calculatePenalty(const QImage& image, int moduleSize) {
    int modules = image.width() / moduleSize;
    int penalty = 0;

    // Penalty 1: Adjacent modules in same color (5+ in a row)
    auto countRuns = [&](bool horizontal) {
        int p = 0;
        for (int i = 0; i < modules; ++i) {
            int runLen = 0;
            bool prev = false;
            bool first = true;
            for (int j = 0; j < modules; ++j) {
                int r = horizontal ? i * moduleSize : j * moduleSize;
                int c = horizontal ? j * moduleSize : i * moduleSize;
                bool dark = qGray(image.pixel(c, r)) < 128;
                if (first || dark == prev) {
                    ++runLen;
                } else {
                    if (runLen >= 5) p += runLen - 2;
                    runLen = 1;
                }
                prev = dark;
                first = false;
            }
            if (runLen >= 5) p += runLen - 2;
        }
        return p;
    };
    penalty += countRuns(true);
    penalty += countRuns(false);

    // Penalty 2: 2x2 blocks of same color
    for (int r = 0; r < modules - 1; ++r) {
        for (int c = 0; c < modules - 1; ++c) {
            bool tl = qGray(image.pixel(c * moduleSize, r * moduleSize)) < 128;
            bool tr = qGray(image.pixel((c + 1) * moduleSize, r * moduleSize)) < 128;
            bool bl = qGray(image.pixel(c * moduleSize, (r + 1) * moduleSize)) < 128;
            bool br = qGray(image.pixel((c + 1) * moduleSize, (r + 1) * moduleSize)) < 128;
            if (tl == tr && tl == bl && tl == br) penalty += 3;
        }
    }

    // Penalty 3: Finder-like patterns (1011101 or 1110101)
    for (int i = 0; i < modules; ++i) {
        for (int j = 0; j < modules - 10; ++j) {
            bool horizontal = true;
            bool vertical = true;
            int pattern1[7] = {1,0,1,1,1,0,1};
            int pattern2[7] = {1,1,1,0,1,0,1};
            for (int k = 0; k < 7; ++k) {
                bool hMod = qGray(image.pixel((j + k) * moduleSize, i * moduleSize)) < 128;
                bool vMod = qGray(image.pixel(i * moduleSize, (j + k) * moduleSize)) < 128;
                if (hMod != (pattern1[k] != 0)) horizontal = false;
                if (vMod != (pattern1[k] != 0)) vertical = false;
            }
            if (horizontal) {
                for (int k = 0; k < 4; ++k) {
                    int idx = j + 7 + k;
                    if (idx < modules) {
                        bool hMod = qGray(image.pixel(idx * moduleSize, i * moduleSize)) < 128;
                        if (k < 3 && hMod != 0) horizontal = false;
                        if (k == 3 && hMod != 1) horizontal = false;
                    }
                }
            }
            if (vertical) {
                for (int k = 0; k < 4; ++k) {
                    int idx = j + 7 + k;
                    if (idx < modules) {
                        bool vMod = qGray(image.pixel(i * moduleSize, idx * moduleSize)) < 128;
                        if (k < 3 && vMod != 0) vertical = false;
                        if (k == 3 && vMod != 1) vertical = false;
                    }
                }
            }
            if (horizontal) penalty += 40;
            if (vertical) penalty += 40;
        }
    }

    // Penalty 4: Dark module ratio
    int darkCount = 0;
    for (int r = 0; r < modules; ++r)
        for (int c = 0; c < modules; ++c)
            if (qGray(image.pixel(c * moduleSize, r * moduleSize)) < 128)
                ++darkCount;
    int total = modules * modules;
    int percent = (darkCount * 100) / total;
    int prev5 = (percent / 5) * 5;
    int next5 = prev5 + 5;
    int pDist = qMin(qAbs(percent - prev5 - 50), qAbs(percent - next5 - 50));
    penalty += (pDist / 5) * 10;

    return penalty;
}

int QRCodeWriter::bestMask(const QImage& image, int moduleSize, int version) {
    int bestScore = INT_MAX;
    int bestM = 0;

    for (int m = 0; m < 8; ++m) {
        QImage testImage = image.copy();
        QPainter painter(&testImage);
        painter.setPen(Qt::NoPen);

        int modules = 17 + version * 4;
        QVector<QVector<bool>> reserved(modules, QVector<bool>(modules, false));

        auto markRect = [&](int r, int c, int h, int w) {
            for (int y = r; y < r + h && y < modules; ++y)
                for (int x = c; x < c + w && x < modules; ++x)
                    if (y >= 0 && x >= 0) reserved[y][x] = true;
        };
        markRect(0, 0, 9, 9);
        markRect(0, modules - 8, 9, 8);
        markRect(modules - 8, 0, 8, 9);
        for (int i = 8; i < modules - 8; ++i) { reserved[6][i] = true; reserved[i][6] = true; }
        reserved[modules - 8][8] = true;
        for (int i = 0; i <= 8; ++i) { if (i != 6) { reserved[8][i] = true; reserved[i][8] = true; } }
        for (int i = modules - 8; i < modules; ++i) { reserved[8][i] = true; reserved[i][8] = true; }
        const int* alignPos = ALIGNMENT[version];
        for (int ai = 0; ai < 7 && alignPos[ai] != 0; ++ai)
            for (int aj = 0; aj < 7 && alignPos[aj] != 0; ++aj) {
                int ar = alignPos[ai] - 2, ac = alignPos[aj] - 2;
                if (ar >= 0 && ac >= 0) {
                    bool ov = (ar <= 9 && ac <= 9) || (ar <= 9 && ac >= modules - 10) || (ar >= modules - 10 && ac <= 9);
                    if (!ov) markRect(ar, ac, 5, 5);
                }
            }

        for (int r = 0; r < modules; ++r) {
            for (int c = 0; c < modules; ++c) {
                if (!reserved[r][c]) {
                    int gray = qGray(testImage.pixel(c * moduleSize, r * moduleSize));
                    bool dark = gray < 128;
                    bool masked = dark ^ (applyMask(r, c, m) != 0);
                    painter.setBrush(masked ? Qt::black : Qt::white);
                    painter.drawRect(c * moduleSize, r * moduleSize, moduleSize, moduleSize);
                }
            }
        }
        painter.end();

        int score = calculatePenalty(testImage, moduleSize);
        if (score < bestScore) {
            bestScore = score;
            bestM = m;
        }
    }

    return bestM;
}

QImage QRCodeWriter::encode(const QString& text, int version, EccLevel eccLevel,
                             int moduleSize, const QColor& fg, const QColor& bg)
{
    initTables();

    if (text.isEmpty()) return QImage();
    if (version <= 0) version = chooseVersion(text, eccLevel);
    if (version < 1 || version > 6) version = 6;

    int modules = 17 + version * 4;
    int imgSize = modules * moduleSize;

    QImage image(imgSize, imgSize, QImage::Format_ARGB32);
    image.fill(bg);

    QVector<unsigned char> data = encodeAlphanumeric(text, version, eccLevel);
    int numBlocks = blocks(version, eccLevel);
    int blockSize = blockDataWords(version, eccLevel);
    int eccSize = eccCodewords(version, eccLevel);

    QVector<unsigned char> allData;
    QVector<unsigned char> allEcc;

    for (int b = 0; b < numBlocks; ++b) {
        QVector<unsigned char> blockData(blockSize);
        for (int i = 0; i < blockSize; ++i) {
            blockData[i] = data[b * blockSize + i];
        }
        for (unsigned char c : blockData) allData.append(c);

        QVector<unsigned char> blockEcc = rsEncode(blockData, eccSize);
        for (unsigned char c : blockEcc) allEcc.append(c);
    }

    QVector<unsigned char> interleaved = interleave(data, allEcc, version, eccLevel);

    // Place fixed patterns
    placeFinder(image, 0, 0, moduleSize);
    placeFinder(image, 0, (modules - 7) * moduleSize, moduleSize);
    placeFinder(image, (modules - 7) * moduleSize, 0, moduleSize);

    placeTiming(image, moduleSize, version);

    const int* alignPos = ALIGNMENT[version];
    for (int ai = 0; ai < 7 && alignPos[ai] != 0; ++ai) {
        for (int aj = 0; aj < 7 && alignPos[aj] != 0; ++aj) {
            int ar = alignPos[ai] - 2;
            int ac = alignPos[aj] - 2;
            if (ar >= 0 && ac >= 0) {
                bool overlapsFinder = (ar <= 9 && ac <= 9)
                    || (ar <= 9 && ac >= modules - 10)
                    || (ar >= modules - 10 && ac <= 9);
                if (!overlapsFinder) {
                    placeAlignment(image, ar * moduleSize, ac * moduleSize, moduleSize);
                }
            }
        }
    }

    int mask = bestMask(image, moduleSize, version);
    placeData(image, interleaved, version, moduleSize, mask);
    placeFormat(image, mask, moduleSize, eccLevel);
    placeVersion(image, version, moduleSize);

    // Apply colors
    if (fg != Qt::black || bg != Qt::white) {
        for (int y = 0; y < imgSize; ++y) {
            for (int x = 0; x < imgSize; ++x) {
                QRgb pixel = image.pixel(x, y);
                int gray = qGray(pixel);
                if (gray < 128) {
                    image.setPixel(x, y, fg.rgba());
                } else {
                    image.setPixel(x, y, bg.rgba());
                }
            }
        }
    }

    return image;
}

} // namespace ks
