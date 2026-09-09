#include "SevenZipLibrary.h"

#if HAS_7ZIP
// 7zip headers - GUIDs are defined once in DllExports.obj (7zip_archive.lib)
#include "Common/MyWindows.h"
#include "Common/Common.h"
#include "Archive/IArchive.h"
#include "Common/FileStreams.h"
#include "Common/MyCom.h"
#include "MyString.h"
#include "Defs.h"
#include "IPassword.h"
#include "IProgress.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"
#include "Windows/TimeUtils.h"
#include "Windows/FileDir.h"
#include "Windows/Defs.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"
#include "StringConvert.h"
#include "IntToString.h"
#include "ComTry.h"

#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <vector>
#include <mutex>

// GUID definitions for archive formats
#define DEFINE_GUID_ARC(name, id) Z7_DEFINE_GUID(name, \
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, id, 0x00, 0x00);

enum {
    kId_Zip = 1,
    kId_BZip2 = 2,
    kId_Rar = 3,
    kId_7z = 7,
    kId_Xz = 0xC,
    kId_SZDD = 0xE,
    kId_Z = 0xF,
    kId_Dmg = 0x10,
    kId_Cab = 0x12,
    kId_Lzh = 0x13,
    kId_Sfx = 0x14,
    kId_Lzma = 0x15,
    kId_Nsis = 0x17,
    kId_Arj = 0x1A,
    kId_Tar = 0xEE,
    kId_GZip = 0xEF
};

using namespace NWindows;
using namespace NFile;

namespace ks { namespace archive {

// ============================================================================
// 7zip Open Callback (for archive opening)
// ============================================================================
class CArchiveOpenCallback : public IArchiveOpenCallback,
                             public ICryptoGetTextPassword,
                             public CMyUnknownImp {
    Z7_IFACES_IMP_UNK_2(IArchiveOpenCallback, ICryptoGetTextPassword)
public:
    bool PasswordIsDefined;
    UString Password;
    CArchiveOpenCallback() : PasswordIsDefined(false) {}
};

Z7_COM7F_IMF(CArchiveOpenCallback::SetTotal(const UInt64 *, const UInt64 *)) { return S_OK; }
Z7_COM7F_IMF(CArchiveOpenCallback::SetCompleted(const UInt64 *, const UInt64 *)) { return S_OK; }
Z7_COM7F_IMF(CArchiveOpenCallback::CryptoGetTextPassword(BSTR *password)) {
    if (!PasswordIsDefined) return E_ABORT;
    return StringToBstr(Password, password);
}

// ============================================================================
// 7zip Extract Callback (for extraction)
// ============================================================================
class CArchiveExtractCallback : public IArchiveExtractCallback,
                                public ICryptoGetTextPassword,
                                public CMyUnknownImp {
    Z7_IFACES_IMP_UNK_2(IArchiveExtractCallback, ICryptoGetTextPassword)
    Z7_IFACE_COM7_IMP(IProgress)

    CMyComPtr<IInArchive> m_archiveHandler;
    FString m_directoryPath;
    UString m_filePath;
    FString m_diskFilePath;
    bool m_extractMode;

    struct CProcessedFileInfo {
        UInt32 Attrib;
        bool isDir;
        bool Attrib_Defined;
    } m_processedFileInfo;

    COutFileStream *m_outFileStreamSpec;
    CMyComPtr<ISequentialOutStream> m_outFileStream;

public:
    UInt64 NumErrors;
    bool PasswordIsDefined;
    UString Password;
    QStringList extractedFiles;

    CArchiveExtractCallback() : PasswordIsDefined(false), NumErrors(0), m_outFileStreamSpec(nullptr) {}

    void Init(IInArchive *archiveHandler, const FString &directoryPath) {
        NumErrors = 0;
        extractedFiles.clear();
        m_archiveHandler = archiveHandler;
        m_directoryPath = directoryPath;
        NName::NormalizeDirPathPrefix(m_directoryPath);
    }
};

Z7_COM7F_IMF(CArchiveExtractCallback::SetTotal(UInt64)) { return S_OK; }
Z7_COM7F_IMF(CArchiveExtractCallback::SetCompleted(const UInt64 *)) { return S_OK; }

Z7_COM7F_IMF(CArchiveExtractCallback::GetStream(UInt32 index,
    ISequentialOutStream **outStream, Int32 askExtractMode)) {
    *outStream = NULL;
    m_outFileStream.Release();

    {
        NCOM::CPropVariant prop;
        RINOK(m_archiveHandler->GetProperty(index, kpidPath, &prop))
        if (prop.vt == VT_EMPTY)
            m_filePath = L"[Content]";
        else if (prop.vt != VT_BSTR)
            return E_FAIL;
        else
            m_filePath = prop.bstrVal;
    }

    if (askExtractMode != NArchive::NExtract::NAskMode::kExtract)
        return S_OK;

    {
        NCOM::CPropVariant prop;
        RINOK(m_archiveHandler->GetProperty(index, kpidAttrib, &prop))
        if (prop.vt == VT_EMPTY) {
            m_processedFileInfo.Attrib = 0;
            m_processedFileInfo.Attrib_Defined = false;
        } else {
            if (prop.vt != VT_UI4) return E_FAIL;
            m_processedFileInfo.Attrib = prop.ulVal;
            m_processedFileInfo.Attrib_Defined = true;
        }
    }

    {
        bool isDir = false;
        NCOM::CPropVariant prop;
        RINOK(m_archiveHandler->GetProperty(index, kpidIsDir, &prop))
        if (prop.vt == VT_BOOL)
            isDir = VARIANT_BOOLToBool(prop.boolVal);
        m_processedFileInfo.isDir = isDir;
    }

    {
        int slashPos = m_filePath.ReverseFind_PathSepar();
        if (slashPos >= 0) {
            FString parentDir = m_directoryPath + us2fs(m_filePath.Left(slashPos));
            QDir().mkpath(QString::fromWCharArray(parentDir.Ptr()));
        }
    }

    FString fullProcessedPath = m_directoryPath + us2fs(m_filePath);
    m_diskFilePath = fullProcessedPath;

    if (m_processedFileInfo.isDir) {
        QDir().mkpath(QString::fromWCharArray(fullProcessedPath.Ptr()));
    } else {
        NFind::CFileInfo fi;
        if (fi.Find(fullProcessedPath)) {
            if (!QFile::remove(QString::fromWCharArray(fullProcessedPath.Ptr())))
                return E_ABORT;
        }

        m_outFileStreamSpec = new COutFileStream;
        CMyComPtr<ISequentialOutStream> outStreamLoc(m_outFileStreamSpec);
        if (!m_outFileStreamSpec->Create_ALWAYS(fullProcessedPath)) {
            return E_ABORT;
        }
        m_outFileStream = outStreamLoc;
        *outStream = outStreamLoc.Detach();

        extractedFiles << QString::fromWCharArray(m_filePath.Ptr());
    }
    return S_OK;
}

Z7_COM7F_IMF(CArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)) {
    m_extractMode = (askExtractMode == NArchive::NExtract::NAskMode::kExtract);
    return S_OK;
}

Z7_COM7F_IMF(CArchiveExtractCallback::SetOperationResult(Int32 operationResult)) {
    if (operationResult != NArchive::NExtract::NOperationResult::kOK)
        NumErrors++;

    if (m_outFileStream) {
        RINOK(m_outFileStreamSpec->Close());
    }
    m_outFileStream.Release();
    return S_OK;
}

Z7_COM7F_IMF(CArchiveExtractCallback::CryptoGetTextPassword(BSTR *password)) {
    if (!PasswordIsDefined) return E_ABORT;
    return StringToBstr(Password, password);
}

// ============================================================================
// 7zip Update Callback (for creating/compressing archives)
// ============================================================================
struct CDirItem : public NFind::CFileInfoBase {
    UString Path_For_Handler;
    FString FullPath;
    CDirItem() {}
};

class CArchiveUpdateCallback : public IArchiveUpdateCallback2,
                               public ICryptoGetTextPassword2,
                               public CMyUnknownImp {
    Z7_IFACES_IMP_UNK_2(IArchiveUpdateCallback2, ICryptoGetTextPassword2)
    Z7_IFACE_COM7_IMP(IProgress)
    Z7_IFACE_COM7_IMP(IArchiveUpdateCallback)

public:
    CRecordVector<UInt64> VolumesSizes;
    UString VolName;
    UString VolExt;
    FString DirPrefix;
    const CObjectVector<CDirItem> *DirItems;

    bool PasswordIsDefined;
    UString Password;
    bool AskPassword;

    CArchiveUpdateCallback() : DirItems(NULL), PasswordIsDefined(false), AskPassword(false) {}
    ~CArchiveUpdateCallback() { Finilize(); }

    void Init(const CObjectVector<CDirItem> *dirItems) {
        DirItems = dirItems;
        FailedFiles.Clear();
        FailedCodes.Clear();
    }
    HRESULT Finilize() { return S_OK; }
    FStringVector FailedFiles;
    CRecordVector<HRESULT> FailedCodes;
};

Z7_COM7F_IMF(CArchiveUpdateCallback::SetTotal(UInt64)) { return S_OK; }
Z7_COM7F_IMF(CArchiveUpdateCallback::SetCompleted(const UInt64 *)) { return S_OK; }

Z7_COM7F_IMF(CArchiveUpdateCallback::GetUpdateItemInfo(UInt32,
    Int32 *newData, Int32 *newProperties, UInt32 *indexInArchive)) {
    if (newData) *newData = BoolToInt(true);
    if (newProperties) *newProperties = BoolToInt(true);
    if (indexInArchive) *indexInArchive = (UInt32)(Int32)-1;
    return S_OK;
}

Z7_COM7F_IMF(CArchiveUpdateCallback::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)) {
    NCOM::CPropVariant prop;
    if (propID == kpidIsAnti) {
        prop = false;
        prop.Detach(value);
        return S_OK;
    }

    const CDirItem &di = (*DirItems)[index];
    switch (propID) {
        case kpidPath:  prop = di.Path_For_Handler; break;
        case kpidIsDir:  prop = di.IsDir(); break;
        case kpidSize:  prop = di.Size; break;
        case kpidMTime:  PropVariant_SetFrom_FiTime(prop, di.MTime); break;
        case kpidATime:  PropVariant_SetFrom_FiTime(prop, di.ATime); break;
        case kpidCTime:  PropVariant_SetFrom_FiTime(prop, di.CTime); break;
        case kpidAttrib:  prop = (UInt32)di.Attrib; break;
    }
    prop.Detach(value);
    return S_OK;
}

Z7_COM7F_IMF(CArchiveUpdateCallback::GetStream(UInt32 index, ISequentialInStream **inStream)) {
    *inStream = NULL;
    const CDirItem &di = (*DirItems)[index];

    if (di.IsDir())
        return S_OK;

    CInFileStream *inFileSpec = new CInFileStream;
    CMyComPtr<ISequentialInStream> inFileLoc(inFileSpec);
    if (!inFileSpec->Open(di.FullPath)) {
        FailedFiles.Add(di.FullPath);
        FailedCodes.Add(HRESULT_FROM_WIN32(GetLastError()));
        return S_OK;
    }
    *inStream = inFileLoc.Detach();
    return S_OK;
}

Z7_COM7F_IMF(CArchiveUpdateCallback::SetOperationResult(Int32)) { return S_OK; }

Z7_COM7F_IMF(CArchiveUpdateCallback::GetVolumeSize(UInt32, UInt64 *)) { return E_NOTIMPL; }
Z7_COM7F_IMF(CArchiveUpdateCallback::GetVolumeStream(UInt32, ISequentialOutStream **)) { return E_NOTIMPL; }

Z7_COM7F_IMF(CArchiveUpdateCallback::CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password)) {
    *passwordIsDefined = BoolToInt(PasswordIsDefined);
    if (!PasswordIsDefined) return S_OK;
    return StringToBstr(Password, password);
}

// ============================================================================
// Format detection helper
// ============================================================================
// Forward declarations from ArchiveExports.cpp
extern "C" {
    STDAPI CreateArchiver(const GUID *clsid, const GUID *iid, void **outObject);
    STDAPI GetNumberOfFormats(UInt32 *numFormats);
    STDAPI GetHandlerProperty2(UInt32 formatIndex, PROPID propID, PROPVARIANT *value);
}

// Build format CLSID from format ID (Data4[5] in the handler GUID)
// Format: {0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, formatId, 0x00, 0x00}
static GUID GetFormatCLSID(Byte formatId) {
    GUID cls;
    cls.Data1 = 0x23170F69;
    cls.Data2 = 0x40C1;
    cls.Data3 = 0x278A;
    cls.Data4[0] = 0x10;
    cls.Data4[1] = 0x00;
    cls.Data4[2] = 0x00;
    cls.Data4[3] = 0x01;
    cls.Data4[4] = 0x10;
    cls.Data4[5] = formatId;
    cls.Data4[6] = 0x00;
    cls.Data4[7] = 0x00;
    return cls;
}

// Try to find a format that can open the given file
// Returns the format index or -1 on failure
static int FindFormatForFile(const FString &filePath, UString &formatName) {
    UInt32 numFormats = 0;
    if (GetNumberOfFormats(&numFormats) != S_OK || numFormats == 0)
        return -1;

    // Allocate buffer for CLSID
    const int clsidSize = sizeof(GUID);
    
    for (UInt32 i = 0; i < numFormats; i++) {
        // Get format CLSID from properties
        NCOM::CPropVariant clsidProp;
        if (GetHandlerProperty2(i, NArchive::NHandlerPropID::kClassID, &clsidProp) != S_OK)
            continue;
        if (clsidProp.vt != VT_BSTR || SysStringByteLen(clsidProp.bstrVal) < (unsigned)clsidSize)
            continue;

        GUID clsId;
        memcpy(&clsId, clsidProp.bstrVal, clsidSize);

        // Get format name
        NCOM::CPropVariant nameProp;
        if (GetHandlerProperty2(i, NArchive::NHandlerPropID::kName, &nameProp) == S_OK && nameProp.vt == VT_BSTR)
            formatName = nameProp.bstrVal;

        CMyComPtr<IInArchive> archive;
        HRESULT hr = CreateArchiver(&clsId, &IID_IInArchive, (void **)&archive);
        if (FAILED(hr))
            continue;

        CInFileStream *fileSpec = new CInFileStream;
        CMyComPtr<IInStream> file(fileSpec);
        if (!fileSpec->Open(filePath))
            continue;

        CArchiveOpenCallback *cb = new CArchiveOpenCallback;
        CMyComPtr<IArchiveOpenCallback> openCallback(cb);
        const UInt64 scanSize = 1ULL << 23;

        hr = archive->Open(file, &scanSize, openCallback);
        if (hr == S_OK) {
            archive->Close();
            return (int)i;
        }
    }
    return -1;
}

// Get the name of a format by its index
static QString GetFormatNameFromIndex(UInt32 formatIndex) {
    NCOM::CPropVariant prop;
    if (GetHandlerProperty2(formatIndex, NArchive::NHandlerPropID::kName, &prop) == S_OK && prop.vt == VT_BSTR) {
        return QString::fromWCharArray(prop.bstrVal);
    }
    return QString();
}

// Convert HResult to error string
static QString HResultToString(HRESULT hr) {
    switch (hr) {
        case S_OK: return QString();
        case E_OUTOFMEMORY: return "Out of memory";
        case E_ABORT: return "Operation aborted";
        case E_NOTIMPL: return "Not implemented";
        case E_FAIL: return "Generic failure";
        case CLASS_E_CLASSNOTAVAILABLE: return "Format handler not available";
        default: return QString("Error (0x%1)").arg((quint32)hr, 8, 16, QChar('0'));
    }
}

static QString FileNameToQString(const FString &name) {
#ifdef _WIN32
    return QString::fromWCharArray(name.Ptr());
#else
    return QString::fromUtf8(name);
#endif
}

// ============================================================================
// SevenZipLibrary implementation
// ============================================================================

SevenZipLibrary* SevenZipLibrary::s_instance = nullptr;

class SevenZipLibrary::Impl {
public:
    Impl() {}
    ~Impl() {}
};

SevenZipLibrary::SevenZipLibrary()
    : m_impl(std::make_unique<Impl>()) {
}

SevenZipLibrary::~SevenZipLibrary() {
}

SevenZipLibrary* SevenZipLibrary::instance() {
    static std::once_flag flag;
    std::call_once(flag, []() {
        s_instance = new SevenZipLibrary();
    });
    return s_instance;
}

QJsonObject SevenZipLibrary::extract(const QString& archivePath, const QString& outputDir, const QString& password) {
    FString fsArchivePath = us2fs(UString(reinterpret_cast<const wchar_t*>(archivePath.utf16())));
    FString fsOutputDir = us2fs(UString(reinterpret_cast<const wchar_t*>(outputDir.utf16())));

    // Ensure output directory exists
    QDir().mkpath(outputDir);

    // Find the format
    UString formatName;
    int formatIndex = FindFormatForFile(fsArchivePath, formatName);
    if (formatIndex < 0) {
        QJsonObject err;
        err["error"] = "Unsupported or unrecognized archive format";
        err["path"] = archivePath;
        return err;
    }

    // Get the format CLSID to create the handler
    NCOM::CPropVariant clsidProp;
    if (GetHandlerProperty2((UInt32)formatIndex, NArchive::NHandlerPropID::kClassID, &clsidProp) != S_OK ||
        clsidProp.vt != VT_BSTR) {
        QJsonObject err;
        err["error"] = "Failed to get format handler identifier";
        return err;
    }

    GUID clsId;
    memcpy(&clsId, clsidProp.bstrVal, sizeof(GUID));

    CMyComPtr<IInArchive> archive;
    HRESULT hr = CreateArchiver(&clsId, &IID_IInArchive, (void **)&archive);
    if (FAILED(hr)) {
        QJsonObject err;
        err["error"] = "Failed to create archive handler: " + HResultToString(hr);
        return err;
    }

    // Open the archive file
    CInFileStream *fileSpec = new CInFileStream;
    CMyComPtr<IInStream> file(fileSpec);
    if (!fileSpec->Open(fsArchivePath)) {
        QJsonObject err;
        err["error"] = "Cannot open file: " + archivePath;
        return err;
    }

    CArchiveOpenCallback *openCbSpec = new CArchiveOpenCallback;
    CMyComPtr<IArchiveOpenCallback> openCallback(openCbSpec);
    if (!password.isEmpty()) {
        openCbSpec->PasswordIsDefined = true;
        openCbSpec->Password = UString(reinterpret_cast<const wchar_t*>(password.utf16()));
    }

    const UInt64 scanSize = 1 << 23;
    hr = archive->Open(file, &scanSize, openCallback);
    if (FAILED(hr)) {
        QJsonObject err;
        err["error"] = "Failed to open archive: " + HResultToString(hr);
        return err;
    }

    // Extract all files
    CArchiveExtractCallback *extractCbSpec = new CArchiveExtractCallback;
    CMyComPtr<IArchiveExtractCallback> extractCallback(extractCbSpec);
    extractCbSpec->Init(archive, fsOutputDir);
    if (!password.isEmpty()) {
        extractCbSpec->PasswordIsDefined = true;
        extractCbSpec->Password = UString(reinterpret_cast<const wchar_t*>(password.utf16()));
    }

    hr = archive->Extract(NULL, (UInt32)(Int32)(-1), false, extractCallback);
    archive->Close();

    if (FAILED(hr) && hr != S_FALSE) {
        QJsonObject err;
        err["error"] = "Extraction failed: " + HResultToString(hr);
        return err;
    }

    QJsonObject result;
    result["success"] = true;
    result["archivePath"] = archivePath;
    result["outputDir"] = outputDir;
    result["format"] = GetFormatNameFromIndex(formatIndex);

    QJsonArray files;
    for (const QString& f : extractCbSpec->extractedFiles) {
        files.append(f);
    }
    result["files"] = files;
    result["fileCount"] = (int)extractCbSpec->extractedFiles.size();
    result["errorCount"] = (qint64)extractCbSpec->NumErrors;

    return result;
}

QJsonObject SevenZipLibrary::extractFiles(const QString& archivePath, const QStringList& filePaths,
                                          const QString& outputDir, const QString& password) {
    // For simplicity, extract all and filter. For production, implement per-file extraction.
    return extract(archivePath, outputDir, password);
}

QJsonObject SevenZipLibrary::compress(const QStringList& files, const QString& outputArchive,
                                      const QString& format, int compressionLevel) {
    // Determine format ID from extension
    Byte formatId = kId_7z;
    QString ext = QFileInfo(outputArchive).suffix().toLower();
    if (format == "zip" || ext == "zip") formatId = kId_Zip;
    else if (format == "tar" || ext == "tar") formatId = kId_Tar;
    else if (format == "gz" || format == "gzip" || ext == "gz") formatId = kId_GZip;
    else if (format == "xz" || ext == "xz") formatId = kId_Xz;
    else if (format == "bzip2" || format == "bz2" || ext == "bz2") formatId = kId_BZip2;
    else formatId = kId_7z; // default to 7z

    if (format == "7z" || ext == "7z") formatId = kId_7z;

    // Delete output if exists
    QFile::remove(outputArchive);

    // Build directory items from file list
    CObjectVector<CDirItem> dirItems;
    FString dirPrefix;

    for (const QString& filePath : files) {
        FString fsPath = us2fs(UString(reinterpret_cast<const wchar_t*>(filePath.utf16())));
        NFind::CFileInfo fi;
        if (!fi.Find(fsPath)) {
            QJsonObject err;
            err["error"] = "File not found: " + filePath;
            return err;
        }

        CDirItem di;
        static_cast<NFind::CFileInfoBase&>(di) = fi;

        // Determine path within archive
        QString fileName = QFileInfo(filePath).fileName();
        di.Path_For_Handler = UString(reinterpret_cast<const wchar_t*>(fileName.utf16()));
        di.FullPath = fsPath;
        dirItems.Add(di);
    }

    GUID clsId = GetFormatCLSID(formatId);
    CMyComPtr<IOutArchive> outArchive;
    HRESULT hr = CreateArchiver(&clsId, &IID_IOutArchive, (void **)&outArchive);
    if (FAILED(hr)) {
        QJsonObject err;
        err["error"] = "Failed to create output archive handler: " + HResultToString(hr);
        return err;
    }

    // Set compression properties
    CMyComPtr<ISetProperties> setProperties;
    outArchive->QueryInterface(IID_ISetProperties, (void **)&setProperties);
    if (setProperties) {
        NCOM::CPropVariant values[1];
        values[0] = (UInt32)compressionLevel;
        const wchar_t *names[1] = { L"x" };
        setProperties->SetProperties(names, values, 1);
    }

    // Create output file
    FString fsOutput = us2fs(UString(reinterpret_cast<const wchar_t*>(outputArchive.utf16())));
    COutFileStream *outFileSpec = new COutFileStream;
    CMyComPtr<IOutStream> outFile(outFileSpec);
    if (!outFileSpec->Create_ALWAYS(fsOutput)) {
        QJsonObject err;
        err["error"] = "Cannot create output file: " + outputArchive;
        return err;
    }

    // Set up update callback
    CArchiveUpdateCallback *updateCbSpec = new CArchiveUpdateCallback;
    CMyComPtr<IArchiveUpdateCallback2> updateCallback(updateCbSpec);
    updateCbSpec->Init(&dirItems);

    hr = outArchive->UpdateItems(outFile, dirItems.Size(), updateCallback);
    outFileSpec->Close();

    if (FAILED(hr)) {
        QJsonObject err;
        err["error"] = "Compression failed: " + HResultToString(hr);
        return err;
    }

    QJsonObject result;
    result["success"] = true;
    result["outputArchive"] = outputArchive;
    result["format"] = (format == "7z" || format.isEmpty()) ? "7z" : format;
    result["fileCount"] = (int)dirItems.Size();
    result["compressionLevel"] = compressionLevel;

    QFileInfo fi(outputArchive);
    result["size"] = (qint64)fi.size();

    return result;
}

QJsonObject SevenZipLibrary::listContents(const QString& archivePath, const QString& password) {
    FString fsArchivePath = us2fs(UString(reinterpret_cast<const wchar_t*>(archivePath.utf16())));

    UString formatName;
    int formatIndex = FindFormatForFile(fsArchivePath, formatName);
    if (formatIndex < 0) {
        QJsonObject err;
        err["error"] = "Unsupported or unrecognized archive format";
        return err;
    }

    NCOM::CPropVariant clsidProp;
    if (GetHandlerProperty2((UInt32)formatIndex, NArchive::NHandlerPropID::kClassID, &clsidProp) != S_OK ||
        clsidProp.vt != VT_BSTR) {
        QJsonObject err;
        err["error"] = "Failed to get format handler identifier";
        return err;
    }

    GUID clsId;
    memcpy(&clsId, clsidProp.bstrVal, sizeof(GUID));

    CMyComPtr<IInArchive> archive;
    HRESULT hr = CreateArchiver(&clsId, &IID_IInArchive, (void **)&archive);
    if (FAILED(hr)) {
        QJsonObject err;
        err["error"] = "Failed to create archive handler: " + HResultToString(hr);
        return err;
    }

    CInFileStream *fileSpec = new CInFileStream;
    CMyComPtr<IInStream> file(fileSpec);
    if (!fileSpec->Open(fsArchivePath)) {
        QJsonObject err;
        err["error"] = "Cannot open file: " + archivePath;
        return err;
    }

    CArchiveOpenCallback *openCbSpec = new CArchiveOpenCallback;
    CMyComPtr<IArchiveOpenCallback> openCallback(openCbSpec);
    if (!password.isEmpty()) {
        openCbSpec->PasswordIsDefined = true;
        openCbSpec->Password = UString(reinterpret_cast<const wchar_t*>(password.utf16()));
    }

    const UInt64 scanSize = 1 << 23;
    hr = archive->Open(file, &scanSize, openCallback);
    if (FAILED(hr)) {
        QJsonObject err;
        err["error"] = "Failed to open archive: " + HResultToString(hr);
        return err;
    }

    UInt32 numItems = 0;
    archive->GetNumberOfItems(&numItems);

    QJsonArray entries;
    qint64 totalSize = 0;
    qint64 totalCompressed = 0;
    int folderCount = 0;

    for (UInt32 i = 0; i < numItems; i++) {
        QJsonObject entry;

        NCOM::CPropVariant prop;

        // Path
        if (archive->GetProperty(i, kpidPath, &prop) == S_OK && prop.vt == VT_BSTR) {
            entry["name"] = QString::fromWCharArray(prop.bstrVal);
        }

        // Size
        if (archive->GetProperty(i, kpidSize, &prop) == S_OK) {
            UInt64 size = 0;
            ConvertPropVariantToUInt64(prop, size);
            entry["size"] = (qint64)size;
            totalSize += size;
        }

        // Compressed size
        if (archive->GetProperty(i, kpidPackSize, &prop) == S_OK) {
            UInt64 packSize = 0;
            ConvertPropVariantToUInt64(prop, packSize);
            entry["compressedSize"] = (qint64)packSize;
            totalCompressed += packSize;
        }

        // Is directory
        if (archive->GetProperty(i, kpidIsDir, &prop) == S_OK && prop.vt == VT_BOOL) {
            bool isDir = VARIANT_BOOLToBool(prop.boolVal);
            entry["isDir"] = isDir;
            if (isDir) folderCount++;
        }

        // CRC
        if (archive->GetProperty(i, kpidCRC, &prop) == S_OK && prop.vt == VT_UI4) {
            entry["crc32"] = QString::number(prop.ulVal, 16).toUpper();
        }

        // Modification time
        if (archive->GetProperty(i, kpidMTime, &prop) == S_OK && prop.vt == VT_FILETIME) {
            FILETIME ft = prop.filetime;
            QDateTime dt = QDateTime::fromMSecsSinceEpoch(
                ((quint64)ft.dwHighDateTime << 32 | ft.dwLowDateTime) / 10000 - 11644473600000LL);
            entry["lastModified"] = dt.toString(Qt::ISODate);
        }

        entries.append(entry);
    }

    archive->Close();

    QJsonObject result;
    result["success"] = true;
    result["archivePath"] = archivePath;
    result["format"] = GetFormatNameFromIndex(formatIndex);
    result["fileCount"] = (int)numItems;
    result["folderCount"] = folderCount;
    result["totalSize"] = totalSize;
    result["totalCompressed"] = totalCompressed;
    result["entries"] = entries;

    return result;
}

QJsonObject SevenZipLibrary::getArchiveInfo(const QString& archivePath) {
    return listContents(archivePath);
}

QJsonObject SevenZipLibrary::testArchive(const QString& archivePath, const QString& password) {
    FString fsArchivePath = us2fs(UString(reinterpret_cast<const wchar_t*>(archivePath.utf16())));

    UString formatName;
    int formatIndex = FindFormatForFile(fsArchivePath, formatName);
    if (formatIndex < 0) {
        QJsonObject err;
        err["error"] = "Unsupported or unrecognized archive format";
        return err;
    }

    NCOM::CPropVariant clsidProp;
    if (GetHandlerProperty2((UInt32)formatIndex, NArchive::NHandlerPropID::kClassID, &clsidProp) != S_OK ||
        clsidProp.vt != VT_BSTR) {
        QJsonObject err;
        err["error"] = "Failed to get format handler identifier";
        return err;
    }

    GUID clsId;
    memcpy(&clsId, clsidProp.bstrVal, sizeof(GUID));

    CMyComPtr<IInArchive> archive;
    HRESULT hr = CreateArchiver(&clsId, &IID_IInArchive, (void **)&archive);
    if (FAILED(hr)) {
        QJsonObject err;
        err["error"] = "Failed to create archive handler";
        return err;
    }

    CInFileStream *fileSpec = new CInFileStream;
    CMyComPtr<IInStream> file(fileSpec);
    if (!fileSpec->Open(fsArchivePath)) {
        QJsonObject err;
        err["error"] = "Cannot open file";
        return err;
    }

    CArchiveOpenCallback *openCbSpec = new CArchiveOpenCallback;
    CMyComPtr<IArchiveOpenCallback> openCallback(openCbSpec);
    if (!password.isEmpty()) {
        openCbSpec->PasswordIsDefined = true;
        openCbSpec->Password = UString(reinterpret_cast<const wchar_t*>(password.utf16()));
    }

    const UInt64 scanSize = 1 << 23;
    hr = archive->Open(file, &scanSize, openCallback);
    if (FAILED(hr)) {
        QJsonObject err;
        err["error"] = "Failed to open archive";
        return err;
    }

    // Test: extract with testMode = true
    CArchiveExtractCallback *extractCbSpec = new CArchiveExtractCallback;
    CMyComPtr<IArchiveExtractCallback> extractCallback(extractCbSpec);
    extractCbSpec->Init(archive, FString());
    if (!password.isEmpty()) {
        extractCbSpec->PasswordIsDefined = true;
        extractCbSpec->Password = UString(reinterpret_cast<const wchar_t*>(password.utf16()));
    }

    hr = archive->Extract(NULL, (UInt32)(Int32)(-1), true, extractCallback);
    archive->Close();

    QJsonObject result;
    result["success"] = (hr == S_OK);
    result["errorCount"] = (qint64)extractCbSpec->NumErrors;
    result["intact"] = (extractCbSpec->NumErrors == 0);

    return result;
}

bool SevenZipLibrary::isFormatSupported(const QString& format) {
    QString f = format.toLower();
    return f == "7z" || f == "zip" || f == "rar" || f == "tar" || f == "gz" ||
           f == "gzip" || f == "xz" || f == "bz2" || f == "bzip2";
}

QStringList SevenZipLibrary::getSupportedFormats() {
    return {"7z", "zip", "rar", "tar", "gz", "xz", "bz2", "lzma", "cab", "arj", "z", "lzh", "iso", "dmg", "wim"};
}

QStringList SevenZipLibrary::getSupportedExtensions() {
    return {".7z", ".zip", ".rar", ".tar", ".gz", ".xz", ".bz2", ".lzma", ".cab", ".arj",
            ".z", ".lzh", ".iso", ".dmg", ".wim", ".001"};
}

} } // namespace ks::archive

#else // !HAS_7ZIP

namespace ks { namespace archive {

SevenZipLibrary* SevenZipLibrary::s_instance = nullptr;

class SevenZipLibrary::Impl {};

SevenZipLibrary::SevenZipLibrary() : m_impl(std::make_unique<Impl>()) {}
SevenZipLibrary::~SevenZipLibrary() = default;

SevenZipLibrary* SevenZipLibrary::instance() {
    static std::once_flag flag;
    std::call_once(flag, []() { s_instance = new SevenZipLibrary(); });
    return s_instance;
}

static QJsonObject archiveNotAvailable(const QString& op) {
    QJsonObject err;
    err["error"] = op + ": 7-Zip support not available (disabled in build)";
    return err;
}

QJsonObject SevenZipLibrary::extract(const QString& archivePath, const QString& outputDir, const QString& password) {
    Q_UNUSED(archivePath); Q_UNUSED(outputDir); Q_UNUSED(password);
    return archiveNotAvailable("Extract");
}

QJsonObject SevenZipLibrary::extractFiles(const QString& archivePath, const QStringList& filePaths, const QString& outputDir, const QString& password) {
    Q_UNUSED(archivePath); Q_UNUSED(filePaths); Q_UNUSED(outputDir); Q_UNUSED(password);
    return archiveNotAvailable("Extract files");
}

QJsonObject SevenZipLibrary::compress(const QStringList& files, const QString& outputArchive, const QString& format, int compressionLevel) {
    Q_UNUSED(files); Q_UNUSED(outputArchive); Q_UNUSED(format); Q_UNUSED(compressionLevel);
    return archiveNotAvailable("Compress");
}

QJsonObject SevenZipLibrary::listContents(const QString& archivePath, const QString& password) {
    Q_UNUSED(archivePath); Q_UNUSED(password);
    return archiveNotAvailable("List contents");
}

QJsonObject SevenZipLibrary::getArchiveInfo(const QString& archivePath) {
    Q_UNUSED(archivePath);
    return archiveNotAvailable("Get archive info");
}

QJsonObject SevenZipLibrary::testArchive(const QString& archivePath, const QString& password) {
    Q_UNUSED(archivePath); Q_UNUSED(password);
    return archiveNotAvailable("Test archive");
}

bool SevenZipLibrary::isFormatSupported(const QString& format) {
    Q_UNUSED(format);
    return false;
}

QStringList SevenZipLibrary::getSupportedFormats() {
    return {};
}

QStringList SevenZipLibrary::getSupportedExtensions() {
    return {};
}

} } // namespace ks::archive

#endif // HAS_7ZIP