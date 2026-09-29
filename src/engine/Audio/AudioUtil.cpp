#include "AudioUtil.h"

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <mutex>
#include <thread>

namespace ks::audio {

namespace {

std::wstring toWide(const std::string& utf8)
{
    if (utf8.empty()) return std::wstring();
    const int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()),
                                        nullptr, 0);
    if (len <= 0) return std::wstring();
    std::wstring out(static_cast<std::size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), out.data(), len);
    return out;
}

std::string toUtf8(const std::wstring& wide)
{
    if (wide.empty()) return std::string();
    const int len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()),
                                        nullptr, 0, nullptr, nullptr);
    if (len <= 0) return std::string();
    std::string out(static_cast<std::size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), out.data(), len,
                        nullptr, nullptr);
    return out;
}

// Glob matcher for QDir nameFilters ("*.json", "*.so"; '?' matches one char).
bool globMatch(const char* pat, const char* str)
{
    while (*pat) {
        if (*pat == '*') {
            if (globMatch(pat + 1, str)) return true;
            if (*str) {
                ++str;
                continue;
            }
            return false;
        }
        if (!*str) return false;
        if (*pat != '?' && *pat != *str) return false;
        ++pat;
        ++str;
    }
    return *str == '\0';
}

std::wstring quoteArg(const std::wstring& arg)
{
    if (arg.empty()) return L"\"\"";
    const bool needsQuotes = arg.find_first_of(L" \t") != std::wstring::npos;
    if (!needsQuotes) return arg;
    std::wstring out = L"\"";
    for (wchar_t c : arg) {
        if (c == L'"') out += L'\\';
        out += c;
    }
    out += L'"';
    return out;
}

// Shared implementation behind runProcess()/runProcessAsync(). When cb is
// non-null the stdout/stderr chunks stream to it (QProcess readyRead*), the
// aggregated result is still returned.
ProcessResult runProcessImpl(const std::string& program,
                             const std::vector<std::string>& args,
                             int timeoutMs,
                             const std::string& workingDir,
                             ProcessCallbacks* cb)
{
    ProcessResult result;

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE outRead = nullptr, outWrite = nullptr;
    HANDLE errRead = nullptr, errWrite = nullptr;
    if (!CreatePipe(&outRead, &outWrite, &sa, 0) ||
        !CreatePipe(&errRead, &errWrite, &sa, 0)) {
        result.err = "CreatePipe failed";
        return result;
    }
    // The read ends must stay in this process only.
    SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(errRead, HANDLE_FLAG_INHERIT, 0);

    std::wstring cmd = L"\"" + toWide(program) + L"\"";
    for (const std::string& a : args) cmd += L" " + quoteArg(toWide(a));
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = outWrite;
    si.hStdError = errWrite;
    si.hStdInput = nullptr;

    PROCESS_INFORMATION pi{};
    std::wstring workDir = workingDir.empty() ? std::wstring() : toWide(workingDir);
    const BOOL created = CreateProcessW(
        nullptr, cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
        workDir.empty() ? nullptr : workDir.c_str(), &si, &pi);

    // Parent must close its copies of the write ends so ReadFile sees EOF.
    CloseHandle(outWrite);
    CloseHandle(errWrite);

    if (!created) {
        CloseHandle(outRead);
        CloseHandle(errRead);
        result.err = "CreateProcessW failed for " + program;
        return result;
    }

    auto reader = [](HANDLE handle, std::string* aggregate,
                     std::function<void(const std::string&)>* sink, std::mutex* mutex) {
        char buf[4096];
        for (;;) {
            DWORD read = 0;
            if (!ReadFile(handle, buf, sizeof(buf), &read, nullptr) || read == 0) break;
            {
                std::lock_guard<std::mutex> lock(*mutex);
                aggregate->append(buf, read);
            }
            if (sink && *sink) (*sink)(std::string(buf, read));
        }
        CloseHandle(handle);
    };

    std::string outAgg, errAgg;
    std::mutex outMutex, errMutex;
    std::function<void(const std::string&)> outSink, errSink;
    if (cb) {
        outSink = cb->onStdout;
        errSink = cb->onStderr;
    }
    std::thread outThread(reader, outRead, &outAgg, &outSink, &outMutex);
    std::thread errThread(reader, errRead, &errAgg, &errSink, &errMutex);

    const DWORD wait = timeoutMs < 0 ? INFINITE : static_cast<DWORD>(timeoutMs);
    const DWORD waited = WaitForSingleObject(pi.hProcess, wait);
    if (waited == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        result.timedOut = true;
    } else if (waited != WAIT_OBJECT_0) {
        result.crashed = true;
    }
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    result.exitCode = static_cast<int>(exitCode);

    outThread.join();
    errThread.join();
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    result.out = outAgg;
    result.err = errAgg;
    if (cb && cb->onFinished) cb->onFinished(result.exitCode, result.crashed);
    return result;
}

} // namespace

std::string randomUuidNoBraces()
{
    const std::string u = mat::randomUuid();
    return u.substr(0, 8) + "-" + u.substr(8, 4) + "-" + u.substr(12, 4) + "-" +
           u.substr(16, 4) + "-" + u.substr(20, 12);
}

std::string isoDateNowUtc()
{
    const std::time_t t = static_cast<std::time_t>(nowMs() / 1000);
    std::tm tmv{};
    gmtime_s(&tmv, &t);
    std::ostringstream os;
    os << std::put_time(&tmv, "%Y-%m-%dT%H:%M:%SZ");
    return os.str();
}

bool dirExists(const std::string& path)
{
    std::error_code ec;
    return fs::is_directory(fs::path(path), ec);
}

std::string absolutePath(const std::string& path)
{
    std::error_code ec;
    fs::path p = fs::absolute(fs::path(path), ec);
    if (ec) return path;
    return p.lexically_normal().string();
}

std::string absoluteParentPath(const std::string& path)
{
    std::error_code ec;
    fs::path p = fs::absolute(fs::path(path), ec);
    if (ec) return parentPath(path);
    return p.lexically_normal().parent_path().string();
}

std::string baseName(const std::string& path)
{
    return fs::path(path).stem().string();
}

std::string suffix(const std::string& path)
{
    std::string ext = fs::path(path).extension().string();
    if (!ext.empty() && ext[0] == '.') ext.erase(0, 1);
    return ext;
}

std::string cleanPath(const std::string& path)
{
    // QDir::cleanPath semantics: forward slashes, collapse "//", drop ".",
    // resolve ".." where possible, no trailing slash (except root/UNC).
    std::string s = path;
    std::replace(s.begin(), s.end(), '\\', '/');

    const bool unc = s.rfind("//", 0) == 0;
    std::vector<std::string> parts;
    std::size_t pos = unc ? 2 : 0;
    while (pos <= s.size()) {
        const std::size_t slash = s.find('/', pos);
        const std::string part =
            s.substr(pos, slash == std::string::npos ? std::string::npos : slash - pos);
        if (part == ".." && !parts.empty() && parts.back() != "..") {
            parts.pop_back();
        } else if (!part.empty() && part != ".") {
            parts.push_back(part);
        }
        if (slash == std::string::npos) break;
        pos = slash + 1;
    }

    std::string out = unc ? "//" : "";
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) out += '/';
        out += parts[i];
    }
    if (out.empty()) out = ".";
    return out;
}

std::string currentPath()
{
    std::error_code ec;
    return fs::current_path(ec).string();
}

std::string applicationDirPath()
{
    wchar_t buf[MAX_PATH];
    const DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return std::string();
    return fs::path(std::wstring(buf, len)).parent_path().string();
}

std::string tempDir()
{
    std::error_code ec;
    return fs::temp_directory_path(ec).string();
}

std::string tempFilePath(const std::string& name)
{
    return (fs::path(tempDir()) / name).string();
}

bool readBinaryFile(const std::string& path, std::string* out, std::string* err)
{
    std::ifstream file(fs::path(path), std::ios::binary);
    if (!file) {
        if (err) *err = "cannot open " + path;
        return false;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    if (out) *out = ss.str();
    return true;
}

bool removeFile(const std::string& path)
{
    std::error_code ec;
    return fs::remove(fs::path(path), ec) && !ec;
}

bool copyFile(const std::string& from, const std::string& to)
{
    std::error_code ec;
    fs::copy(fs::path(from), fs::path(to),
             fs::copy_options::overwrite_existing, ec);
    return !ec;
}

std::vector<std::string> listFiles(const std::string& dir,
                                   const std::string& namePattern,
                                   bool recursive)
{
    return listFiles(dir, std::vector<std::string>{namePattern}, recursive);
}

std::vector<std::string> listFiles(const std::string& dir,
                                   const std::vector<std::string>& namePatterns,
                                   bool recursive)
{
    std::vector<std::string> out;
    std::error_code ec;
    const fs::path root(dir);
    if (!fs::is_directory(root, ec)) return out;

    auto matches = [&](const fs::path& p) {
        const std::string name = p.filename().string();
        for (const std::string& pattern : namePatterns) {
            if (globMatch(pattern.c_str(), name.c_str())) return true;
        }
        return false;
    };

    if (recursive) {
        fs::recursive_directory_iterator it(root, ec), end;
        for (; !ec && it != end; it.increment(ec)) {
            if (!it->is_regular_file(ec)) continue;
            if (matches(it->path())) out.push_back(it->path().string());
        }
    } else {
        fs::directory_iterator it(root, ec), end;
        for (; !ec && it != end; it.increment(ec)) {
            if (!it->is_regular_file(ec)) continue;
            if (matches(it->path())) out.push_back(it->path().string());
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

ProcessResult runProcess(const std::string& program,
                         const std::vector<std::string>& args,
                         int timeoutMs,
                         const std::string& workingDir)
{
    return runProcessImpl(program, args, timeoutMs, workingDir, nullptr);
}

void runProcessAsync(const std::string& program,
                     const std::vector<std::string>& args,
                     ProcessCallbacks callbacks,
                     int timeoutMs,
                     const std::string& workingDir)
{
    std::thread([program, args, callbacks, timeoutMs, workingDir]() mutable {
        runProcessImpl(program, args, timeoutMs, workingDir, &callbacks);
    }).detach();
}

std::string sha256Hex(const std::string& data)
{
    return sha256Hex(data.data(), data.size());
}

std::string sha256Hex(const void* data, std::size_t size)
{
    // FIPS180-4 SHA-256 (replaces QCryptographicHash::hash(data, Sha256)).
    static const std::uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
        0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
        0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
        0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
        0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
        0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
        0xc67178f2};

    std::uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                          0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

    const std::uint64_t bitLen = static_cast<std::uint64_t>(size) * 8;
    std::vector<unsigned char> msg(static_cast<const unsigned char*>(data),
                                   static_cast<const unsigned char*>(data) + size);
    msg.push_back(0x80);
    while (msg.size() % 64 != 56) msg.push_back(0);
    for (int i = 7; i >= 0; --i) msg.push_back(static_cast<unsigned char>((bitLen >> (i * 8)) & 0xFF));

    auto rotr = [](std::uint32_t x, int n) { return (x >> n) | (x << (32 - n)); };

    for (std::size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        std::uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<std::uint32_t>(msg[chunk + i * 4]) << 24) |
                   (static_cast<std::uint32_t>(msg[chunk + i * 4 + 1]) << 16) |
                   (static_cast<std::uint32_t>(msg[chunk + i * 4 + 2]) << 8) |
                   static_cast<std::uint32_t>(msg[chunk + i * 4 + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            const std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        std::uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            const std::uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            const std::uint32_t ch = (e & f) ^ (~e & g);
            const std::uint32_t temp1 = hh + s1 + ch + k[i] + w[i];
            const std::uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = s0 + maj;
            hh = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += hh;
    }

    static const char* hex = "0123456789abcdef";
    std::string out(64, '0');
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            const unsigned char byte = static_cast<unsigned char>((h[i] >> (28 - j * 4)) & 0xF);
            out[static_cast<std::size_t>(i * 8 + j)] = hex[byte];
        }
    }
    return out;
}

bool SharedLibrary::load(const std::string& path)
{
    unload();
    m_handle = reinterpret_cast<void*>(LoadLibraryW(toWide(path).c_str()));
    return m_handle != nullptr;
}

void SharedLibrary::unload()
{
    if (m_handle) {
        FreeLibrary(static_cast<HMODULE>(m_handle));
        m_handle = nullptr;
    }
}

void* SharedLibrary::resolve(const char* symbol) const
{
    if (!m_handle) return nullptr;
    return reinterpret_cast<void*>(
        GetProcAddress(static_cast<HMODULE>(m_handle), symbol));
}

} // namespace ks::audio
