#include "PngCodec.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>

namespace ks::image {
namespace {

void setError(std::string* err, const std::string& message)
{
    if (err) *err = message;
}

// ---------------------------------------------------------------------------
// Bit IO (deflate packs the least-significant bit of a byte first)
// ---------------------------------------------------------------------------

struct BitReader {
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;
    std::size_t pos = 0;
    std::uint32_t bitBuf = 0;
    int bitCnt = 0;
    bool failed = false;

    int getBits(int n)
    {
        while (bitCnt < n) {
            if (pos >= size) { failed = true; return 0; }
            bitBuf |= static_cast<std::uint32_t>(data[pos++]) << bitCnt;
            bitCnt += 8;
        }
        const std::uint32_t v = bitBuf & ((1u << n) - 1u);
        bitBuf >>= n;
        bitCnt -= n;
        return static_cast<int>(v);
    }

    // Returns false when the stream is truncated.
    bool needBits(int n)
    {
        while (bitCnt < n) {
            if (pos >= size) { failed = true; return false; }
            bitBuf |= static_cast<std::uint32_t>(data[pos++]) << bitCnt;
            bitCnt += 8;
        }
        return true;
    }

    void alignByte()
    {
        bitBuf = 0;
        bitCnt = 0;
    }
};

struct BitWriter {
    std::vector<std::uint8_t>& out;

    explicit BitWriter(std::vector<std::uint8_t>& out) : out(out) {}
    std::uint32_t acc = 0;
    int nbits = 0;

    void put(std::uint32_t value, int n)
    {
        acc |= (value & ((n >= 32) ? 0xFFFFFFFFu : ((1u << n) - 1u))) << nbits;
        nbits += n;
        while (nbits >= 8) {
            out.push_back(static_cast<std::uint8_t>(acc & 0xFF));
            acc >>= 8;
            nbits -= 8;
        }
    }

    void flush()
    {
        if (nbits > 0) {
            out.push_back(static_cast<std::uint8_t>(acc & 0xFF));
            acc = 0;
            nbits = 0;
        }
    }
};

int reverseBits(int code, int len)
{
    int out = 0;
    for (int i = 0; i < len; ++i) {
        out = (out << 1) | ((code >> i) & 1);
    }
    return out;
}

// ---------------------------------------------------------------------------
// CRC / Adler
// ---------------------------------------------------------------------------

const std::array<std::uint32_t, 256>& crc32Table()
{
    static const std::array<std::uint32_t, 256> table = [] {
        std::array<std::uint32_t, 256> t{};
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t c = i;
            for (int k = 0; k < 8; ++k)
                c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            t[i] = c;
        }
        return t;
    }();
    return table;
}

// ---------------------------------------------------------------------------
// Canonical Huffman: length-limited code lengths + encoder/decoder tables
// ---------------------------------------------------------------------------

constexpr int kMaxBits = 15;
constexpr int kCodeLenBits = 7;

// Package-merge length-limited Huffman (RFC 1951 limits codes to 15 bits).
// Returns false when the alphabet is empty or limit is not satisfiable.
bool buildLengths(const std::uint32_t* freq, int n, int limit,
                  std::uint8_t* lengths)
{
    std::memset(lengths, 0, static_cast<std::size_t>(n));

    std::vector<int> order;
    order.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i)
        if (freq[i] > 0) order.push_back(i);
    if (order.empty()) return false;
    if (order.size() == 1) { lengths[order[0]] = 1; return true; }

    std::sort(order.begin(), order.end(), [&](int a, int b) {
        if (freq[a] != freq[b]) return freq[a] < freq[b];
        return a < b;
    });

    // Item: weight + the set of symbols it contains (as a small index list).
    struct Item {
        std::int64_t weight;
        std::vector<int> symbols;
    };

    std::vector<Item> items;
    items.reserve(order.size());
    for (int sym : order)
        items.push_back(Item{freq[sym], {sym}});

    std::vector<Item> packages;
    for (int level = 0; level < limit; ++level) {
        // Candidate list: original items + packages produced on the previous
        // level, merged in ascending weight order.
        std::vector<Item> list;
        list.reserve(items.size() + packages.size());
        list.insert(list.end(), items.begin(), items.end());
        list.insert(list.end(), packages.begin(), packages.end());
        std::stable_sort(list.begin(), list.end(),
                         [](const Item& a, const Item& b) { return a.weight < b.weight; });

        packages.clear();
        for (std::size_t i = 0; i + 1 < list.size(); i += 2) {
            Item p;
            p.weight = list[i].weight + list[i + 1].weight;
            p.symbols = std::move(list[i].symbols);
            p.symbols.insert(p.symbols.end(), list[i + 1].symbols.begin(),
                             list[i + 1].symbols.end());
            packages.push_back(std::move(p));
        }
        if (packages.empty()) break;
    }

    for (const Item& p : packages)
        for (int sym : p.symbols)
            if (lengths[sym] < 255) lengths[sym]++;

    // Every active symbol must have received a length.
    for (int i = 0; i < n; ++i)
        if (freq[i] > 0 && lengths[i] == 0) return false;
    return true;
}

struct HuffTable {
    std::array<std::uint16_t, kMaxBits + 1> count{};
    std::vector<std::uint16_t> symbols; // sorted by (length, symbol)

    void build(const std::uint8_t* lengths, int n)
    {
        count.fill(0);
        for (int i = 0; i < n; ++i) count[lengths[i]]++;
        count[0] = 0;
        std::array<std::uint16_t, kMaxBits + 2> offs{};
        for (int i = 1; i <= kMaxBits; ++i)
            offs[i + 1] = static_cast<std::uint16_t>(offs[i] + count[i]);
        symbols.resize(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i)
            if (lengths[i] != 0) symbols[offs[lengths[i]]++] = static_cast<std::uint16_t>(i);

        // Canonical codes, stored per symbol for O(1) encoding.
        symLen.assign(lengths, lengths + n);
        symCode.assign(static_cast<std::size_t>(n), 0);
        int c = 0;
        int idx = 0;
        for (int l = 1; l <= kMaxBits; ++l) {
            for (int i = 0; i < count[l]; ++i, ++idx)
                symCode[symbols[idx]] = static_cast<std::uint16_t>(c++);
            c <<= 1;
        }
    }

    // Canonical MSB-first code for a symbol.
    bool codeFor(int symbol, int* code, int* len) const
    {
        if (symbol < 0 || static_cast<std::size_t>(symbol) >= symLen.size()) return false;
        if (symLen[static_cast<std::size_t>(symbol)] == 0) return false;
        *code = symCode[static_cast<std::size_t>(symbol)];
        *len = symLen[static_cast<std::size_t>(symbol)];
        return true;
    }

    std::vector<std::uint8_t> symLen;
    std::vector<std::uint16_t> symCode;
};

// Decodes one symbol; returns -1 on a corrupt/overlong code.
int huffDecode(BitReader& br, const HuffTable& h)
{
    int code = 0, first = 0, index = 0;
    for (int len = 1; len <= kMaxBits; ++len) {
        if (!br.needBits(1)) return -1;
        code |= br.getBits(1);
        const int count = h.count[len];
        if (code - first < count) return h.symbols[index + (code - first)];
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Inflate (RFC 1950 zlib wrapper + RFC 1951 deflate)
// ---------------------------------------------------------------------------

struct LenState {
    HuffTable lit;
    HuffTable dist;
};

void buildFixedTables(LenState& t)
{
    std::array<std::uint8_t, 288> litLen{};
    for (int i = 0; i < 144; ++i) litLen[i] = 8;
    for (int i = 144; i < 256; ++i) litLen[i] = 9;
    for (int i = 256; i < 280; ++i) litLen[i] = 7;
    for (int i = 280; i < 288; ++i) litLen[i] = 8;
    t.lit.build(litLen.data(), 288);

    std::array<std::uint8_t, 32> distLen{};
    distLen.fill(5);
    t.dist.build(distLen.data(), 32);
}

const std::array<int, 29>& lenBase()
{
    static const std::array<int, 29> v = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19,
                                          23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115,
                                          131, 163, 195, 227, 258};
    return v;
}

const std::array<int, 29>& lenExtra()
{
    static const std::array<int, 29> v = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
                                          3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
    return v;
}

const std::array<int, 30>& distBase()
{
    static const std::array<int, 30> v = {1,  2,  3,  4,  5,  7,   9,   13,  17,  25,
                                          33, 49, 65, 97, 129, 193, 257, 385, 513, 769,
                                          1025, 1537, 2049, 3073, 4097, 6145, 8193,
                                          12289, 16385, 24577};
    return v;
}

const std::array<int, 30>& distExtra()
{
    static const std::array<int, 30> v = {0, 0, 0, 0, 1, 1, 2,  2,  3,  3,  4,  4,  5,  5, 6,
                                          6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
    return v;
}

bool inflateBlocks(BitReader& br, std::vector<std::uint8_t>& out,
                   std::size_t maxOut, std::string* err)
{
    LenState fixed;
    buildFixedTables(fixed);
    bool finalBlock = false;

    while (!finalBlock) {
        if (!br.needBits(3)) { setError(err, "truncated deflate stream"); return false; }
        finalBlock = (br.getBits(1) != 0);
        const int type = br.getBits(2);

        if (type == 0) {
            br.alignByte();
            if (br.pos + 4 > br.size) { setError(err, "truncated stored block"); return false; }
            const int len = br.data[br.pos] | (br.data[br.pos + 1] << 8);
            const int nlen = br.data[br.pos + 2] | (br.data[br.pos + 3] << 8);
            br.pos += 4;
            br.bitBuf = 0;
            br.bitCnt = 0;
            if ((len ^ 0xFFFF) != nlen) { setError(err, "bad stored block length"); return false; }
            if (br.pos + static_cast<std::size_t>(len) > br.size) {
                setError(err, "truncated stored block data");
                return false;
            }
            if (maxOut && out.size() + static_cast<std::size_t>(len) > maxOut) {
                setError(err, "decompressed data exceeds limit");
                return false;
            }
            out.insert(out.end(), br.data + br.pos, br.data + br.pos + len);
            br.pos += static_cast<std::size_t>(len);
            continue;
        }

        const LenState* tables = nullptr;
        LenState dyn;
        if (type == 1) {
            tables = &fixed;
        } else if (type == 2) {
            if (!br.needBits(14)) { setError(err, "truncated dynamic header"); return false; }
            const int hlit = br.getBits(5) + 257;
            const int hdist = br.getBits(5) + 1;
            const int hclen = br.getBits(4) + 4;
            if (hlit > 288 || hdist > 32) { setError(err, "bad dynamic header"); return false; }

            static const std::array<int, 19> kOrder = {16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
                                                       11, 4,  12, 3, 13, 2, 14, 1, 15};
            std::array<std::uint8_t, 19> clLen{};
            for (int i = 0; i < hclen; ++i) clLen[kOrder[static_cast<std::size_t>(i)]] =
                static_cast<std::uint8_t>(br.getBits(3));

            HuffTable clTable;
            clTable.build(clLen.data(), 19);

            std::vector<std::uint8_t> lengths(static_cast<std::size_t>(hlit + hdist), 0);
            int i = 0;
            while (i < hlit + hdist) {
                const int sym = huffDecode(br, clTable);
                if (sym < 0) { setError(err, "bad code-length symbol"); return false; }
                if (sym < 16) {
                    lengths[static_cast<std::size_t>(i++)] = static_cast<std::uint8_t>(sym);
                } else if (sym == 16) {
                    if (i == 0) { setError(err, "repeat with no previous length"); return false; }
                    const int prev = lengths[static_cast<std::size_t>(i - 1)];
                    int repeat = 3 + br.getBits(2);
                    while (repeat-- > 0 && i < hlit + hdist)
                        lengths[static_cast<std::size_t>(i++)] = static_cast<std::uint8_t>(prev);
                } else if (sym == 17) {
                    int repeat = 3 + br.getBits(3);
                    while (repeat-- > 0 && i < hlit + hdist) lengths[static_cast<std::size_t>(i++)] = 0;
                } else {
                    int repeat = 11 + br.getBits(7);
                    while (repeat-- > 0 && i < hlit + hdist) lengths[static_cast<std::size_t>(i++)] = 0;
                }
            }

            dyn.lit.build(lengths.data(), hlit);
            dyn.dist.build(lengths.data() + hlit, hdist);
            if (dyn.lit.count[0] == 0 && dyn.lit.symbols.empty()) {
                setError(err, "empty literal table");
                return false;
            }
            tables = &dyn;
        } else {
            setError(err, "reserved deflate block type");
            return false;
        }

        for (;;) {
            const int sym = huffDecode(br, tables->lit);
            if (sym < 0) { setError(err, "bad literal/length code"); return false; }
            if (sym < 256) {
                if (maxOut && out.size() + 1 > maxOut) {
                    setError(err, "decompressed data exceeds limit");
                    return false;
                }
                out.push_back(static_cast<std::uint8_t>(sym));
                continue;
            }
            if (sym == 256) break;

            const int lenIdx = sym - 257;
            if (lenIdx >= 29) { setError(err, "bad length code"); return false; }
            const int length = lenBase()[static_cast<std::size_t>(lenIdx)] +
                               br.getBits(lenExtra()[static_cast<std::size_t>(lenIdx)]);

            const int distSym = huffDecode(br, tables->dist);
            if (distSym < 0 || distSym >= 30) { setError(err, "bad distance code"); return false; }
            const std::size_t distance =
                static_cast<std::size_t>(distBase()[static_cast<std::size_t>(distSym)] +
                                         br.getBits(distExtra()[static_cast<std::size_t>(distSym)]));
            if (distance > out.size()) { setError(err, "distance too far back"); return false; }
            if (maxOut && out.size() + static_cast<std::size_t>(length) > maxOut) {
                setError(err, "decompressed data exceeds limit");
                return false;
            }
            std::size_t from = out.size() - distance;
            for (int k = 0; k < length; ++k)
                out.push_back(out[from++]);
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Deflate (LZ77 + dynamic Huffman, falling back to the fixed tables)
// ---------------------------------------------------------------------------

constexpr int kWindowSize = 32768;
constexpr int kMinMatch = 3;
constexpr int kMaxMatch = 258;
constexpr int kHashBits = 15;
constexpr int kHashSize = 1 << kHashBits;
constexpr int kMaxChain = 128;

struct Token {
    std::uint16_t litLen;   // literal, or length for a match
    std::uint16_t dist;     // 0 for a literal
};

void buildLitFreq(const std::vector<Token>& tokens, std::uint32_t* litFreq,
                  std::uint32_t* distFreq)
{
    std::memset(litFreq, 0, sizeof(std::uint32_t) * 288);
    std::memset(distFreq, 0, sizeof(std::uint32_t) * 32);
    litFreq[256] = 1; // end of block
    for (const Token& t : tokens) {
        if (t.dist == 0) {
            litFreq[t.litLen]++;
            continue;
        }
        const int len = t.litLen;
        for (int i = 28; i >= 0; --i) {
            if (len >= lenBase()[static_cast<std::size_t>(i)]) {
                litFreq[257 + i] += 1;
                break;
            }
        }
        const int dist = t.dist;
        for (int i = 29; i >= 0; --i) {
            if (dist >= distBase()[static_cast<std::size_t>(i)]) {
                distFreq[i] += 1;
                break;
            }
        }
    }
}

void writeFixedLiteral(BitWriter& bw, int sym)
{
    int code = 0, len = 0;
    if (sym < 144) { code = 0x30 + sym; len = 8; }
    else if (sym < 256) { code = 0x190 + (sym - 144); len = 9; }
    else if (sym < 280) { code = sym - 256; len = 7; }
    else { code = 0xC0 + (sym - 280); len = 8; }
    bw.put(static_cast<std::uint32_t>(reverseBits(code, len)), len);
}

void writeCode(BitWriter& bw, const HuffTable& h, int symbol)
{
    int code = 0, len = 0;
    if (!h.codeFor(symbol, &code, &len)) { return; }
    bw.put(static_cast<std::uint32_t>(reverseBits(code, len)), len);
}

void writeLengthDistance(BitWriter& bw, int len, int dist, const HuffTable* lit,
                         bool fixed, const HuffTable* distTable)
{
    int lenIdx = 0;
    for (int i = 28; i >= 0; --i) {
        if (len >= lenBase()[static_cast<std::size_t>(i)]) { lenIdx = i; break; }
    }
    int distIdx = 0;
    for (int i = 29; i >= 0; --i) {
        if (dist >= distBase()[static_cast<std::size_t>(i)]) { distIdx = i; break; }
    }

    const int litSym = 257 + lenIdx;
    if (fixed) writeFixedLiteral(bw, litSym);
    else writeCode(bw, *lit, litSym);
    const int lenBits = lenExtra()[static_cast<std::size_t>(lenIdx)];
    if (lenBits) bw.put(static_cast<std::uint32_t>(len - lenBase()[static_cast<std::size_t>(lenIdx)]), lenBits);

    if (fixed) {
        bw.put(static_cast<std::uint32_t>(reverseBits(distIdx, 5)), 5);
    } else {
        writeCode(bw, *distTable, distIdx);
    }
    const int dExtra = distExtra()[static_cast<std::size_t>(distIdx)];
    if (dExtra) bw.put(static_cast<std::uint32_t>(dist - distBase()[static_cast<std::size_t>(distIdx)]), dExtra);
}

void lz77(const std::uint8_t* src, std::size_t len, std::vector<Token>& tokens)
{
    tokens.clear();
    if (len == 0) return;
    tokens.reserve(len / 3 + 16);

    std::vector<int> head(kHashSize, -1);
    std::vector<int> prev(len > 0 ? len : 1, -1);

    auto hash3 = [&](std::size_t i) -> int {
        const std::uint32_t v = (static_cast<std::uint32_t>(src[i]) << 16) |
                                (static_cast<std::uint32_t>(src[i + 1]) << 8) |
                                static_cast<std::uint32_t>(src[i + 2]);
        return static_cast<int>((v * 0x9E3779B1u) >> (32 - kHashBits));
    };

    std::size_t i = 0;
    while (i < len) {
        int bestLen = 0;
        std::size_t bestDist = 0;
        if (i + kMinMatch <= len) {
            const int h = hash3(i);
            int cand = head[h];
            int chain = 0;
            const std::size_t maxDist = std::min<std::size_t>(kWindowSize, i);
            while (cand >= 0 && chain++ < kMaxChain) {
                const std::size_t d = i - static_cast<std::size_t>(cand);
                if (d > maxDist) break;
                int l = 0;
                const int maxL = static_cast<int>(std::min<std::size_t>(kMaxMatch, len - i));
                while (l < maxL && src[cand + l] == src[i + l]) ++l;
                if (l > bestLen) {
                    bestLen = l;
                    bestDist = d;
                    if (l >= kMaxMatch) break;
                }
                cand = prev[static_cast<std::size_t>(cand)];
            }
        }

        if (bestLen >= kMinMatch) {
            tokens.push_back(Token{static_cast<std::uint16_t>(bestLen),
                                   static_cast<std::uint16_t>(bestDist)});
            for (std::size_t k = 0; k < static_cast<std::size_t>(bestLen) && i + k + 2 < len; ++k) {
                const int h = hash3(i + k);
                prev[i + k] = head[h];
                head[h] = static_cast<int>(i + k);
            }
            i += static_cast<std::size_t>(bestLen);
        } else {
            tokens.push_back(Token{static_cast<std::uint16_t>(src[i]), 0});
            if (i + 2 < len) {
                const int h = hash3(i);
                prev[i] = head[h];
                head[h] = static_cast<int>(i);
            }
            ++i;
        }
    }
}

void writeDynamicHeader(BitWriter& bw, const std::vector<std::uint8_t>& litLen,
                        const std::vector<std::uint8_t>& distLen)
{
    static const std::array<int, 19> kOrder = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5,
                                               11, 4, 12, 3, 13, 2, 14, 1, 15};

    std::vector<std::uint8_t> cl(19, 0);
    std::vector<std::uint8_t> seq;
    std::vector<std::uint16_t> seqExtra; // extra value per 16/17/18 run
    std::vector<std::uint8_t> seqKind;   // 0 = literal, 16/17/18 = run

    int hlit = static_cast<int>(litLen.size());
    while (hlit > 257 && litLen[static_cast<std::size_t>(hlit - 1)] == 0) --hlit;
    int hdist = static_cast<int>(distLen.size());
    while (hdist > 1 && distLen[static_cast<std::size_t>(hdist - 1)] == 0) --hdist;

    // HLIT/HDIST tell the decoder exactly how many lengths it must read, so
    // the RLE sequence has to cover that range and nothing more.
    const std::size_t total =
        static_cast<std::size_t>(hlit) + static_cast<std::size_t>(hdist);
    std::vector<std::uint8_t> all(total);
    std::copy(litLen.begin(), litLen.begin() + hlit, all.begin());
    std::copy(distLen.begin(), distLen.begin() + hdist, all.begin() + hlit);

    std::size_t i = 0;
    while (i < total) {
        const std::uint8_t cur = all[i];
        std::size_t run = 1;
        while (i + run < total && all[i + run] == cur) ++run;
        if (cur == 0) {
            std::size_t remaining = run;
            while (remaining >= 11) {
                const std::size_t take = std::min<std::size_t>(remaining, 138);
                seq.push_back(18);
                seqKind.push_back(18);
                seqExtra.push_back(static_cast<std::uint16_t>(take - 11));
                remaining -= take;
            }
            while (remaining >= 3) {
                const std::size_t take = std::min<std::size_t>(remaining, 10);
                seq.push_back(17);
                seqKind.push_back(17);
                seqExtra.push_back(static_cast<std::uint16_t>(take - 3));
                remaining -= take;
            }
            for (std::size_t k = 0; k < remaining; ++k) {
                seq.push_back(0);
                seqKind.push_back(0);
                seqExtra.push_back(0);
            }
        } else {
            // First occurrence is emitted literally, repeats use code 16.
            seq.push_back(cur);
            seqKind.push_back(0);
            seqExtra.push_back(0);
            std::size_t remaining = run - 1;
            while (remaining >= 3) {
                const std::size_t take = std::min<std::size_t>(remaining, 6);
                seq.push_back(16);
                seqKind.push_back(16);
                seqExtra.push_back(static_cast<std::uint16_t>(take - 3));
                remaining -= take;
            }
            for (std::size_t k = 0; k < remaining; ++k) {
                seq.push_back(cur);
                seqKind.push_back(0);
                seqExtra.push_back(0);
            }
        }
        i += run;
    }

    std::uint32_t clFreq[19] = {};
    for (std::uint8_t s : seq) clFreq[s]++;
    std::uint8_t clLen[19] = {};
    buildLengths(clFreq, 19, kCodeLenBits, clLen);

    // zlib rejects an incomplete code-length table, so a single-symbol
    // alphabet gets a dummy second symbol to make the code complete.
    int active = 0;
    int only = -1;
    for (int s = 0; s < 19; ++s) {
        if (clLen[s] != 0) { ++active; only = s; }
    }
    if (active == 1 && only >= 0) {
        const int other = (only == 0) ? 1 : 0;
        clLen[only] = 1;
        clLen[other] = 1;
    }

    std::copy(clLen, clLen + 19, cl.begin());

    int hclen = 19;
    while (hclen > 4 && cl[static_cast<std::size_t>(kOrder[static_cast<std::size_t>(hclen - 1)])] == 0)
        --hclen;

    bw.put(static_cast<std::uint32_t>(hlit - 257), 5);
    bw.put(static_cast<std::uint32_t>(hdist - 1), 5);
    bw.put(static_cast<std::uint32_t>(hclen - 4), 4);
    for (int i2 = 0; i2 < hclen; ++i2)
        bw.put(cl[static_cast<std::size_t>(kOrder[static_cast<std::size_t>(i2)])], 3);

    HuffTable clTable;
    clTable.build(cl.data(), 19);
    for (std::size_t k = 0; k < seq.size(); ++k) {
        writeCode(bw, clTable, seq[k]);
        const std::uint8_t kind = seqKind[k];
        if (kind == 16) bw.put(seqExtra[k], 2);
        else if (kind == 17) bw.put(seqExtra[k], 3);
        else if (kind == 18) bw.put(seqExtra[k], 7);
    }
}

void writeTokensFixed(BitWriter& bw, const std::vector<Token>& tokens)
{
    for (const Token& t : tokens) {
        if (t.dist == 0) writeFixedLiteral(bw, t.litLen);
        else writeLengthDistance(bw, t.litLen, t.dist, nullptr, true, nullptr);
    }
    writeFixedLiteral(bw, 256);
}

// Builds one complete deflate block (final bit + type + payload).
bool buildFixedBlock(const std::vector<Token>& tokens, std::vector<std::uint8_t>* block)
{
    BitWriter bw(*block);
    bw.put(1, 1); // final block
    bw.put(1, 2); // fixed Huffman
    writeTokensFixed(bw, tokens);
    bw.flush();
    return true;
}

bool buildDynamicBlock(const std::vector<Token>& tokens, std::vector<std::uint8_t>* block)
{
    std::uint32_t litFreq[288];
    std::uint32_t distFreq[32];
    buildLitFreq(tokens, litFreq, distFreq);
    litFreq[256] = 1;

    std::uint8_t litLen[288];
    if (!buildLengths(litFreq, 288, kMaxBits, litLen)) return false;

    // A single distance code still needs a length of one bit, as in zlib.
    bool anyDist = false;
    for (int i = 0; i < 30; ++i) {
        if (distFreq[i] > 0) { anyDist = true; break; }
    }
    std::uint8_t distLen[32] = {};
    if (!anyDist) {
        distLen[0] = 1;
    } else if (!buildLengths(distFreq, 30, kMaxBits, distLen)) {
        return false;
    }

    const std::vector<std::uint8_t> litLenVec(litLen, litLen + 288);
    const std::vector<std::uint8_t> distLenVec(distLen, distLen + 32);

    BitWriter bw(*block);
    bw.put(1, 1); // final block
    bw.put(2, 2); // dynamic Huffman
    writeDynamicHeader(bw, litLenVec, distLenVec);

    HuffTable litTable;
    litTable.build(litLen, 288);
    HuffTable distTable;
    distTable.build(distLen, 32);

    for (const Token& t : tokens) {
        if (t.dist == 0) {
            writeCode(bw, litTable, t.litLen);
        } else {
            writeLengthDistance(bw, t.litLen, t.dist, &litTable, false, &distTable);
        }
    }
    writeCode(bw, litTable, 256);
    bw.flush();
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// Public zlib API
// ---------------------------------------------------------------------------

std::uint32_t crc32(const void* data, std::size_t len, std::uint32_t seed)
{
    const auto* p = static_cast<const std::uint8_t*>(data);
    std::uint32_t c = seed ^ 0xFFFFFFFFu;
    const auto& table = crc32Table();
    for (std::size_t i = 0; i < len; ++i)
        c = table[(c ^ p[i]) & 0xFFu] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

std::uint32_t adler32(const void* data, std::size_t len, std::uint32_t seed)
{
    constexpr std::uint32_t kMod = 65521;
    std::uint32_t a = seed & 0xFFFFu;
    std::uint32_t b = (seed >> 16) & 0xFFFFu;
    const auto* p = static_cast<const std::uint8_t*>(data);
    std::size_t i = 0;
    while (i < len) {
        const std::size_t chunk = std::min<std::size_t>(len - i, 5552);
        for (std::size_t k = 0; k < chunk; ++k) {
            a += p[i + k];
            b += a;
        }
        a %= kMod;
        b %= kMod;
        i += chunk;
    }
    return (b << 16) | a;
}

bool zlibDecompress(const std::uint8_t* src, std::size_t len,
                    std::vector<std::uint8_t>* out, std::size_t maxOut,
                    std::string* err)
{
    if (!src || len < 2) { setError(err, "empty zlib stream"); return false; }
    const int cmf = src[0];
    const int flg = src[1];
    if ((cmf & 0x0F) != 8) { setError(err, "unsupported zlib compression method"); return false; }
    if (((cmf << 8) | flg) % 31 != 0) { setError(err, "bad zlib header check"); return false; }
    if (flg & 0x20) { setError(err, "preset dictionaries are not supported"); return false; }

    BitReader br;
    br.data = src + 2;
    br.size = len - 2;
    if (!inflateBlocks(br, *out, maxOut, err)) return false;

    // Adler-32 trailer (big endian) when the stream is complete.
    if (br.pos + 4 <= br.size) {
        const std::uint32_t expected =
            (static_cast<std::uint32_t>(br.data[br.pos]) << 24) |
            (static_cast<std::uint32_t>(br.data[br.pos + 1]) << 16) |
            (static_cast<std::uint32_t>(br.data[br.pos + 2]) << 8) |
            static_cast<std::uint32_t>(br.data[br.pos + 3]);
        const std::uint32_t actual = adler32(out->empty() ? nullptr : out->data(), out->size());
        if (expected != actual) { setError(err, "zlib adler32 mismatch"); return false; }
    }
    return true;
}

bool zlibCompress(const std::uint8_t* src, std::size_t len,
                  std::vector<std::uint8_t>* out, bool storeOnly, std::string* err)
{
    if (!out) { setError(err, "null output buffer"); return false; }
    out->clear();
    out->reserve(len / 4 + 64);
    out->push_back(0x78);
    out->push_back(0x01);

    if (storeOnly) {
        // Raw stored blocks, for callers that explicitly want no compression.
        BitWriter bw(*out);
        std::size_t off = 0;
        while (off < len || (len == 0 && off == 0)) {
            const std::size_t chunk = std::min<std::size_t>(len - off, 65535);
            const bool last = (off + chunk >= len);
            bw.put(last ? 1u : 0u, 1);
            bw.put(0, 2);
            bw.flush();
            out->push_back(static_cast<std::uint8_t>(chunk & 0xFF));
            out->push_back(static_cast<std::uint8_t>((chunk >> 8) & 0xFF));
            const int nlen = static_cast<int>(~chunk) & 0xFFFF;
            out->push_back(static_cast<std::uint8_t>(nlen & 0xFF));
            out->push_back(static_cast<std::uint8_t>((nlen >> 8) & 0xFF));
            out->insert(out->end(), src + off, src + off + chunk);
            off += chunk;
            if (len == 0) break;
        }
    } else {
        std::vector<Token> tokens;
        lz77(src, len, tokens);

        std::vector<std::uint8_t> fixedBlock;
        fixedBlock.reserve(len / 3 + 64);
        buildFixedBlock(tokens, &fixedBlock);

        std::vector<std::uint8_t> dynamicBlock;
        dynamicBlock.reserve(len / 3 + 64);
        const bool dynamicOk = buildDynamicBlock(tokens, &dynamicBlock);

        const std::vector<std::uint8_t>* chosen = &fixedBlock;
        if (dynamicOk && dynamicBlock.size() < fixedBlock.size()) chosen = &dynamicBlock;
        out->insert(out->end(), chosen->begin(), chosen->end());
    }

    const std::uint32_t ad = adler32(src, len);
    out->push_back(static_cast<std::uint8_t>((ad >> 24) & 0xFF));
    out->push_back(static_cast<std::uint8_t>((ad >> 16) & 0xFF));
    out->push_back(static_cast<std::uint8_t>((ad >> 8) & 0xFF));
    out->push_back(static_cast<std::uint8_t>(ad & 0xFF));
    return true;
}

// ---------------------------------------------------------------------------
// PNG
// ---------------------------------------------------------------------------

std::uint32_t readBe32(const std::uint8_t* p)
{
    return (static_cast<std::uint32_t>(p[0]) << 24) | (static_cast<std::uint32_t>(p[1]) << 16) |
           (static_cast<std::uint32_t>(p[2]) << 8) | static_cast<std::uint32_t>(p[3]);
}

void writeBe32(std::vector<std::uint8_t>& out, std::uint32_t v)
{
    out.push_back(static_cast<std::uint8_t>(v >> 24));
    out.push_back(static_cast<std::uint8_t>(v >> 16));
    out.push_back(static_cast<std::uint8_t>(v >> 8));
    out.push_back(static_cast<std::uint8_t>(v));
}

int paeth(int a, int b, int c)
{
    const int p = a + b - c;
    const int pa = std::abs(p - a);
    const int pb = std::abs(p - b);
    const int pc = std::abs(p - c);
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

bool unfilterScanlines(std::vector<std::uint8_t>& data, int width, int height,
                       int bytesPerPixel, int rowBytes, std::string* err)
{
    const std::size_t need = static_cast<std::size_t>(height) *
                             (static_cast<std::size_t>(rowBytes) + 1);
    if (data.size() < need) { setError(err, "truncated PNG image data"); return false; }

    const std::size_t stride = static_cast<std::size_t>(rowBytes) + 1;
    for (int y = 0; y < height; ++y) {
        std::uint8_t* row = data.data() + static_cast<std::size_t>(y) * stride;
        const int filter = row[0];
        std::uint8_t* cur = row + 1;
        const std::uint8_t* prev =
            (y > 0) ? data.data() + static_cast<std::size_t>(y - 1) * stride + 1 : nullptr;
        switch (filter) {
        case 0:
            break;
        case 1:
            for (int i = bytesPerPixel; i < rowBytes; ++i)
                cur[i] = static_cast<std::uint8_t>(cur[i] + cur[i - bytesPerPixel]);
            break;
        case 2:
            if (prev)
                for (int i = 0; i < rowBytes; ++i)
                    cur[i] = static_cast<std::uint8_t>(cur[i] + prev[i]);
            break;
        case 3:
            for (int i = 0; i < rowBytes; ++i) {
                const int left = (i >= bytesPerPixel) ? cur[i - bytesPerPixel] : 0;
                const int up = prev ? prev[i] : 0;
                cur[i] = static_cast<std::uint8_t>(cur[i] + ((left + up) >> 1));
            }
            break;
        case 4:
            for (int i = 0; i < rowBytes; ++i) {
                const int left = (i >= bytesPerPixel) ? cur[i - bytesPerPixel] : 0;
                const int up = prev ? prev[i] : 0;
                const int upLeft = (prev && i >= bytesPerPixel) ? prev[i - bytesPerPixel] : 0;
                cur[i] = static_cast<std::uint8_t>(cur[i] + paeth(left, up, upLeft));
            }
            break;
        default:
            setError(err, "unsupported PNG filter type");
            return false;
        }
        (void)width;
    }
    return true;
}

bool decodePng(const std::uint8_t* data, std::size_t len, RawImage* out, std::string* err)
{
    static const std::uint8_t kSig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    if (!data || len < 8 || std::memcmp(data, kSig, 8) != 0) {
        setError(err, "not a PNG file");
        return false;
    }

    int width = 0, height = 0, bitDepth = 0, colorType = 0, interlace = 0;
    std::vector<std::uint8_t> palette;
    std::vector<std::uint8_t> paletteAlpha;
    std::vector<int> trnsGray;
    std::vector<int> trnsRgb;
    std::vector<std::uint8_t> idat;
    bool haveIhdr = false;

    std::size_t pos = 8;
    while (pos + 8 <= len) {
        const std::uint32_t chunkLen = readBe32(data + pos);
        if (pos + 12 + chunkLen > len) { setError(err, "truncated PNG chunk"); return false; }
        const char type[5] = {static_cast<char>(data[pos + 4]), static_cast<char>(data[pos + 5]),
                              static_cast<char>(data[pos + 6]), static_cast<char>(data[pos + 7]), 0};
        const std::uint8_t* body = data + pos + 8;

        if (std::strcmp(type, "IHDR") == 0) {
            if (chunkLen < 13) { setError(err, "bad IHDR"); return false; }
            width = static_cast<int>(readBe32(body));
            height = static_cast<int>(readBe32(body + 4));
            bitDepth = body[8];
            colorType = body[9];
            interlace = body[12];
            haveIhdr = true;
            if (body[10] != 0 || body[11] != 0) { setError(err, "bad PNG compression/filter method"); return false; }
            if (interlace != 0) { setError(err, "interlaced PNG files are not supported"); return false; }
            if (width <= 0 || height <= 0 || width > 32768 || height > 32768) {
                setError(err, "bad PNG dimensions");
                return false;
            }
        } else if (std::strcmp(type, "PLTE") == 0) {
            palette.assign(body, body + chunkLen);
        } else if (std::strcmp(type, "tRNS") == 0) {
            if (colorType == 3) {
                paletteAlpha.assign(body, body + chunkLen);
            } else if (colorType == 0 && chunkLen >= 2) {
                trnsGray.push_back((body[0] << 8) | body[1]);
            } else if (colorType == 2 && chunkLen >= 6) {
                trnsRgb.push_back((body[0] << 8) | body[1]);
                trnsRgb.push_back((body[2] << 8) | body[3]);
                trnsRgb.push_back((body[4] << 8) | body[5]);
            }
        } else if (std::strcmp(type, "IDAT") == 0) {
            idat.insert(idat.end(), body, body + chunkLen);
        } else if (std::strcmp(type, "IEND") == 0) {
            break;
        }
        pos += 12 + chunkLen;
    }

    if (!haveIhdr) { setError(err, "PNG has no IHDR"); return false; }
    if (idat.empty()) { setError(err, "PNG has no image data"); return false; }

    static const int kChannels[7] = {1, 0, 3, 1, 2, 0, 4};
    if (colorType > 6 || kChannels[colorType] == 0) { setError(err, "bad PNG color type"); return false; }
    if (bitDepth != 1 && bitDepth != 2 && bitDepth != 4 && bitDepth != 8 && bitDepth != 16) {
        setError(err, "unsupported PNG bit depth");
        return false;
    }
    if (colorType == 3 && palette.empty()) { setError(err, "indexed PNG has no palette"); return false; }

    const int channels = kChannels[colorType];
    const int rowBytes = (width * channels * bitDepth + 7) / 8;
    const int bytesPerPixel = std::max(1, (channels * bitDepth) / 8);

    std::vector<std::uint8_t> raw;
    const std::size_t maxRaw =
        (static_cast<std::size_t>(rowBytes) + 1) * static_cast<std::size_t>(height);
    if (!zlibDecompress(idat.data(), idat.size(), &raw, maxRaw + 16, err)) return false;
    if (raw.size() < maxRaw) { setError(err, "PNG image data is incomplete"); return false; }

    if (!unfilterScanlines(raw, width, height, bytesPerPixel, rowBytes, err)) return false;

    out->resize(width, height);
    // Full sample value (1..16 bits, big endian for 16 bit data).
    auto sampleRaw = [&](const std::uint8_t* row, int index) -> int {
        if (bitDepth == 16) return (row[index * 2] << 8) | row[index * 2 + 1];
        if (bitDepth == 8) return row[index];
        const int bitPos = index * bitDepth;
        const int shift = 8 - bitDepth - (bitPos % 8);
        const int mask = (1 << bitDepth) - 1;
        return (row[bitPos / 8] >> shift) & mask;
    };
    // Sample scaled to 0..255 (16 bit data keeps its high byte).
    auto sample8 = [&](const std::uint8_t* row, int index) -> int {
        const int v = sampleRaw(row, index);
        if (bitDepth == 16) return v >> 8;
        if (bitDepth == 8) return v;
        const int maxV = (1 << bitDepth) - 1;
        return maxV ? (v * 255 + maxV / 2) / maxV : 0;
    };

    const std::size_t stride = static_cast<std::size_t>(rowBytes) + 1;
    for (int y = 0; y < height; ++y) {
        const std::uint8_t* row = raw.data() + static_cast<std::size_t>(y) * stride + 1;
        std::uint8_t* dst = out->rgba.data() +
                            static_cast<std::size_t>(y) * static_cast<std::size_t>(width) * 4;
        for (int x = 0; x < width; ++x) {
            int r = 0, g = 0, b = 0, a = 255;
            switch (colorType) {
            case 0: {
                const int v = sampleRaw(row, x);
                r = g = b = sample8(row, x);
                if (trnsGray.size() == 1 && v == trnsGray[0]) a = 0;
                break;
            }
            case 2: {
                r = sample8(row, x * 3);
                g = sample8(row, x * 3 + 1);
                b = sample8(row, x * 3 + 2);
                if (trnsRgb.size() == 3) {
                    if (sampleRaw(row, x * 3) == trnsRgb[0] &&
                        sampleRaw(row, x * 3 + 1) == trnsRgb[1] &&
                        sampleRaw(row, x * 3 + 2) == trnsRgb[2]) a = 0;
                }
                break;
            }
            case 3: {
                const int idx = sampleRaw(row, x);
                const std::size_t base = static_cast<std::size_t>(idx) * 3;
                if (base + 2 >= palette.size()) {
                    setError(err, "PNG palette index out of range");
                    return false;
                }
                r = palette[base];
                g = palette[base + 1];
                b = palette[base + 2];
                if (static_cast<std::size_t>(idx) < paletteAlpha.size()) a = paletteAlpha[idx];
                break;
            }
            case 4: {
                r = g = b = sample8(row, x * 2);
                a = sample8(row, x * 2 + 1);
                break;
            }
            default: {
                r = sample8(row, x * 4);
                g = sample8(row, x * 4 + 1);
                b = sample8(row, x * 4 + 2);
                a = sample8(row, x * 4 + 3);
                break;
            }
            }
            dst[x * 4 + 0] = static_cast<std::uint8_t>(r);
            dst[x * 4 + 1] = static_cast<std::uint8_t>(g);
            dst[x * 4 + 2] = static_cast<std::uint8_t>(b);
            dst[x * 4 + 3] = static_cast<std::uint8_t>(a);
        }
    }
    return true;
}

bool encodePng(const RawImage& image, std::vector<std::uint8_t>* out, std::string* err)
{
    if (image.isNull() || !out) { setError(err, "empty image"); return false; }
    const int width = image.width;
    const int height = image.height;
    const int rowBytes = width * 4;
    const int bpp = 4;

    static const std::uint8_t kSig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    out->clear();
    out->insert(out->end(), kSig, kSig + 8);

    auto pushChunk = [&](const char* type, const std::vector<std::uint8_t>& body) {
        writeBe32(*out, static_cast<std::uint32_t>(body.size()));
        const std::size_t typePos = out->size();
        out->insert(out->end(), type, type + 4);
        out->insert(out->end(), body.begin(), body.end());
        const std::uint32_t crc = crc32(out->data() + typePos, 4 + body.size());
        writeBe32(*out, crc);
    };

    std::vector<std::uint8_t> ihdr;
    writeBe32(ihdr, static_cast<std::uint32_t>(width));
    writeBe32(ihdr, static_cast<std::uint32_t>(height));
    ihdr.push_back(8);  // bit depth
    ihdr.push_back(6);  // RGBA
    ihdr.push_back(0);  // deflate
    ihdr.push_back(0);  // adaptive filtering
    ihdr.push_back(0);  // no interlace
    pushChunk("IHDR", ihdr);

    // Filtered scanlines: pick the filter with the smallest absolute sum.
    std::vector<std::uint8_t> filtered(static_cast<std::size_t>(height) *
                                       (static_cast<std::size_t>(rowBytes) + 1));
    std::vector<std::uint8_t> candidate(rowBytes);
    for (int y = 0; y < height; ++y) {
        const std::uint8_t* row = image.rgba.data() +
                                  static_cast<std::size_t>(y) * static_cast<std::size_t>(rowBytes);
        const std::uint8_t* prev = (y > 0)
                                       ? image.rgba.data() +
                                             static_cast<std::size_t>(y - 1) *
                                                 static_cast<std::size_t>(rowBytes)
                                       : nullptr;
        int bestFilter = 0;
        int bestScore = 0x7FFFFFFF;
        std::vector<std::uint8_t> bestRow(rowBytes, 0);
        for (int f = 0; f < 5; ++f) {
            int score = 0;
            for (int i = 0; i < rowBytes; ++i) {
                const int raw = row[i];
                const int left = (i >= bpp) ? row[i - bpp] : 0;
                const int up = prev ? prev[i] : 0;
                const int upLeft = (prev && i >= bpp) ? prev[i - bpp] : 0;
                int v = 0;
                switch (f) {
                case 0: v = raw; break;
                case 1: v = raw - left; break;
                case 2: v = raw - up; break;
                case 3: v = raw - ((left + up) >> 1); break;
                default: v = raw - paeth(left, up, upLeft); break;
                }
                const int b = v & 0xFF;
                candidate[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(b);
                score += (b < 128) ? b : (256 - b);
            }
            if (score < bestScore) {
                bestScore = score;
                bestFilter = f;
                bestRow = candidate;
            }
        }
        std::uint8_t* dst = filtered.data() + static_cast<std::size_t>(y) *
                                                   (static_cast<std::size_t>(rowBytes) + 1);
        dst[0] = static_cast<std::uint8_t>(bestFilter);
        std::memcpy(dst + 1, bestRow.data(), static_cast<std::size_t>(rowBytes));
    }

    std::vector<std::uint8_t> z;
    if (!zlibCompress(filtered.data(), filtered.size(), &z, false, err)) return false;
    pushChunk("IDAT", z);

    pushChunk("IEND", {});
    return true;
}

// ---------------------------------------------------------------------------
// BMP (24/32 bit, uncompressed)
// ---------------------------------------------------------------------------

bool decodeBmp(const std::uint8_t* data, std::size_t len, RawImage* out, std::string* err)
{
    if (!data || len < 54 || data[0] != 'B' || data[1] != 'M') {
        setError(err, "not a BMP file");
        return false;
    }
    auto le32 = [&](std::size_t off) -> std::uint32_t {
        return static_cast<std::uint32_t>(data[off]) | (static_cast<std::uint32_t>(data[off + 1]) << 8) |
               (static_cast<std::uint32_t>(data[off + 2]) << 16) |
               (static_cast<std::uint32_t>(data[off + 3]) << 24);
    };
    auto le16 = [&](std::size_t off) -> std::uint32_t {
        return static_cast<std::uint32_t>(data[off]) | (static_cast<std::uint32_t>(data[off + 1]) << 8);
    };

    const std::uint32_t pixelOffset = le32(10);
    const std::uint32_t dibSize = le32(14);
    if (dibSize < 40) { setError(err, "unsupported BMP header"); return false; }
    const int width = static_cast<int>(le32(18));
    const int rawHeight = static_cast<int>(le32(22));
    const int bpp = static_cast<int>(le16(28));
    const std::uint32_t compression = le32(30);
    if (compression != 0 && compression != 3) { setError(err, "compressed BMP is not supported"); return false; }
    if (bpp != 24 && bpp != 32) { setError(err, "unsupported BMP bit depth"); return false; }
    const bool topDown = rawHeight < 0;
    const int height = topDown ? -rawHeight : rawHeight;
    if (width <= 0 || height <= 0 || pixelOffset >= len) { setError(err, "bad BMP dimensions"); return false; }

    const int bytesPerPixel = bpp / 8;
    const int rowSize = ((width * bytesPerPixel + 3) / 4) * 4;
    if (pixelOffset + static_cast<std::size_t>(rowSize) * height > len) {
        setError(err, "truncated BMP pixel data");
        return false;
    }

    out->resize(width, height);
    for (int y = 0; y < height; ++y) {
        const int srcY = topDown ? y : (height - 1 - y);
        const std::uint8_t* row = data + pixelOffset +
                                  static_cast<std::size_t>(srcY) * static_cast<std::size_t>(rowSize);
        std::uint8_t* dst = out->rgba.data() + static_cast<std::size_t>(y) * width * 4;
        for (int x = 0; x < width; ++x) {
            const std::uint8_t* p = row + static_cast<std::size_t>(x) * bytesPerPixel;
            dst[x * 4 + 0] = p[2];
            dst[x * 4 + 1] = p[1];
            dst[x * 4 + 2] = p[0];
            dst[x * 4 + 3] = (bytesPerPixel == 4) ? p[3] : 255;
        }
    }
    return true;
}

bool encodeBmp(const RawImage& image, std::vector<std::uint8_t>* out, std::string* err)
{
    if (image.isNull() || !out) { setError(err, "empty image"); return false; }
    const int width = image.width;
    const int height = image.height;
    const int rowSize = ((width * 4 + 3) / 4) * 4;
    const std::uint32_t dataSize = static_cast<std::uint32_t>(rowSize) * height;

    out->assign(54, 0);
    (*out)[0] = 'B';
    (*out)[1] = 'M';
    auto putLe32 = [&](std::size_t off, std::uint32_t v) {
        (*out)[off] = static_cast<std::uint8_t>(v);
        (*out)[off + 1] = static_cast<std::uint8_t>(v >> 8);
        (*out)[off + 2] = static_cast<std::uint8_t>(v >> 16);
        (*out)[off + 3] = static_cast<std::uint8_t>(v >> 24);
    };
    auto putLe16 = [&](std::size_t off, std::uint16_t v) {
        (*out)[off] = static_cast<std::uint8_t>(v);
        (*out)[off + 1] = static_cast<std::uint8_t>(v >> 8);
    };
    putLe32(2, 54 + dataSize);
    putLe32(10, 54);
    putLe32(14, 40);
    putLe32(18, static_cast<std::uint32_t>(width));
    putLe32(22, static_cast<std::uint32_t>(height)); // bottom-up
    putLe16(26, 1);
    putLe16(28, 32);
    putLe32(34, dataSize);
    putLe32(38, 2835);
    putLe32(42, 2835);

    out->resize(54 + dataSize);
    for (int y = 0; y < height; ++y) {
        const int dstY = height - 1 - y;
        std::uint8_t* row = out->data() + 54 + static_cast<std::size_t>(dstY) * rowSize;
        const std::uint8_t* src = image.rgba.data() +
                                  static_cast<std::size_t>(y) * static_cast<std::size_t>(width) * 4;
        for (int x = 0; x < width; ++x) {
            row[x * 4 + 0] = src[x * 4 + 2];
            row[x * 4 + 1] = src[x * 4 + 1];
            row[x * 4 + 2] = src[x * 4 + 0];
            row[x * 4 + 3] = src[x * 4 + 3];
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// TGA (type 2 uncompressed / type 10 RLE truecolor, 24/32 bit)
// ---------------------------------------------------------------------------

bool decodeTga(const std::uint8_t* data, std::size_t len, RawImage* out, std::string* err)
{
    if (!data || len < 18) { setError(err, "not a TGA file"); return false; }
    const int idLength = data[0];
    const int colorMapType = data[1];
    const int imageType = data[2];
    const int width = data[12] | (data[13] << 8);
    const int height = data[14] | (data[15] << 8);
    const int depth = data[16];
    const int descriptor = data[17];
    if (colorMapType != 0) { setError(err, "color-mapped TGA is not supported"); return false; }
    if (imageType != 2 && imageType != 10) { setError(err, "unsupported TGA image type"); return false; }
    if (depth != 24 && depth != 32) { setError(err, "unsupported TGA bit depth"); return false; }
    if (width <= 0 || height <= 0 || width > 32768 || height > 32768) {
        setError(err, "bad TGA dimensions");
        return false;
    }

    const int bytesPerPixel = depth / 8;
    std::size_t pos = 18 + static_cast<std::size_t>(idLength);
    if (pos >= len) { setError(err, "truncated TGA header"); return false; }

    out->resize(width, height);
    const bool topDown = (descriptor & 0x20) != 0;
    const std::size_t pixelCount = static_cast<std::size_t>(width) * height;

    auto putPixel = [&](std::size_t index, const std::uint8_t* p) {
        const int x = static_cast<int>(index % static_cast<std::size_t>(width));
        const int y = static_cast<int>(index / static_cast<std::size_t>(width));
        const int dstY = topDown ? y : (height - 1 - y);
        std::uint8_t* dst = out->rgba.data() +
                            (static_cast<std::size_t>(dstY) * width + x) * 4;
        dst[0] = p[2];
        dst[1] = p[1];
        dst[2] = p[0];
        dst[3] = (bytesPerPixel == 4) ? p[3] : 255;
    };

    if (imageType == 2) {
        if (pos + pixelCount * bytesPerPixel > len) { setError(err, "truncated TGA data"); return false; }
        for (std::size_t i = 0; i < pixelCount; ++i)
            putPixel(i, data + pos + i * bytesPerPixel);
        return true;
    }

    std::size_t i = 0;
    while (i < pixelCount) {
        if (pos >= len) { setError(err, "truncated TGA RLE data"); return false; }
        const int headerByte = data[pos++];
        const int count = (headerByte & 0x7F) + 1;
        if (headerByte & 0x80) {
            if (pos + bytesPerPixel > len) { setError(err, "truncated TGA RLE packet"); return false; }
            const std::uint8_t* p = data + pos;
            pos += bytesPerPixel;
            for (int k = 0; k < count && i < pixelCount; ++k) putPixel(i++, p);
        } else {
            if (pos + static_cast<std::size_t>(count) * bytesPerPixel > len) {
                setError(err, "truncated TGA raw packet");
                return false;
            }
            for (int k = 0; k < count && i < pixelCount; ++k) {
                putPixel(i, data + pos);
                pos += bytesPerPixel;
                ++i;
            }
        }
    }
    return true;
}

bool encodeTga(const RawImage& image, std::vector<std::uint8_t>* out, std::string* err)
{
    if (image.isNull() || !out) { setError(err, "empty image"); return false; }
    const int width = image.width;
    const int height = image.height;
    out->assign(18, 0);
    (*out)[2] = 2; // uncompressed truecolor
    (*out)[12] = static_cast<std::uint8_t>(width & 0xFF);
    (*out)[13] = static_cast<std::uint8_t>(width >> 8);
    (*out)[14] = static_cast<std::uint8_t>(height & 0xFF);
    (*out)[15] = static_cast<std::uint8_t>(height >> 8);
    (*out)[16] = 32;
    (*out)[17] = 0x28; // top-left origin, 8 alpha bits
    out->reserve(18 + static_cast<std::size_t>(width) * height * 4);
    for (int y = 0; y < height; ++y) {
        const std::uint8_t* row = image.rgba.data() +
                                  static_cast<std::size_t>(y) * static_cast<std::size_t>(width) * 4;
        for (int x = 0; x < width; ++x) {
            out->push_back(row[x * 4 + 2]);
            out->push_back(row[x * 4 + 1]);
            out->push_back(row[x * 4 + 0]);
            out->push_back(row[x * 4 + 3]);
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Dispatch + file helpers
// ---------------------------------------------------------------------------

bool decodeImage(const std::uint8_t* data, std::size_t len, RawImage* out, std::string* err)
{
    if (!data || len < 4 || !out) { setError(err, "no image data"); return false; }
    static const std::uint8_t kSig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    if (len >= 8 && std::memcmp(data, kSig, 8) == 0) return decodePng(data, len, out, err);
    if (data[0] == 'B' && data[1] == 'M') return decodeBmp(data, len, out, err);
    return decodeTga(data, len, out, err);
}

bool encodeImage(const RawImage& image, const std::string& path,
                 std::vector<std::uint8_t>* out, std::string* err)
{
    const std::size_t dot = path.find_last_of('.');
    std::string ext = (dot == std::string::npos) ? std::string() : path.substr(dot + 1);
    for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (ext == "bmp") return encodeBmp(image, out, err);
    if (ext == "tga") return encodeTga(image, out, err);
    return encodePng(image, out, err);
}

bool loadImageFile(const std::string& path, RawImage* out, std::string* err)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) { setError(err, "cannot open file: " + path); return false; }
    const std::streamsize size = file.tellg();
    if (size <= 0) { setError(err, "empty file: " + path); return false; }
    std::vector<std::uint8_t> buffer(static_cast<std::size_t>(size));
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        setError(err, "cannot read file: " + path);
        return false;
    }
    std::string localErr;
    if (!decodeImage(buffer.data(), buffer.size(), out, &localErr)) {
        setError(err, path + ": " + localErr);
        return false;
    }
    return true;
}

bool saveImageFile(const std::string& path, const RawImage& image, std::string* err)
{
    std::vector<std::uint8_t> bytes;
    std::string localErr;
    if (!encodeImage(image, path, &bytes, &localErr)) {
        setError(err, path + ": " + localErr);
        return false;
    }
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) { setError(err, "cannot open file for writing: " + path); return false; }
    file.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    if (!file.good()) { setError(err, "cannot write file: " + path); return false; }
    return true;
}

} // namespace ks::image
