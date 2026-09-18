#include "ACDParser.h"
#include <QDataStream>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QDebug>
#include <cstring>

// ============================================================================
// ACDParser implementation
// ============================================================================

QString ACDParser::m_lastError;

ACDParser::ACDArchive ACDParser::parse(const QString& acdPath, QString* error) {
    ACDArchive archive;
    archive.filePath = acdPath;
    archive.folderName = QFileInfo(acdPath).dir().dirName();

    QFile file(acdPath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open ACD file: " + acdPath;
        if (error) *error = m_lastError;
        return archive;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    // Check if file is encrypted by looking for INI header
    QByteArray headerBytes = file.peek(64);

    // Try to detect if this is a raw INI collection or encrypted ACD
    if (headerBytes.startsWith('[')) {
        // Looks like plain INI files concatenated
        archive.isEncrypted = false;
        archive.isValid = true;

        // Parse concatenated INI files
        QByteArray allData = file.readAll();
        QStringList fileNames = getStandardFileNames();

        // Simple heuristic: split by [SECTION] headers
        int fileIndex = 0;
        QByteArray currentContent;
        QByteArray sectionHeader = "[HEADER]";

        while (!allData.isEmpty()) {
            int nextHeader = allData.indexOf(sectionHeader, 1);
            if (nextHeader == -1) nextHeader = allData.size();

            QByteArray chunk = allData.left(nextHeader);
            allData = allData.mid(nextHeader);

            if (fileIndex < fileNames.size()) {
                ACDFileEntry entry;
                entry.fileName = fileNames[fileIndex];
                entry.offset = 0;
                entry.size = chunk.size();
                entry.data = chunk;
                archive.files.append(entry);
            }
            fileIndex++;
        }
    } else {
        // Encrypted binary ACD format
        archive.isEncrypted = true;

        // Create decryption key from folder name
        QString key = createKey(archive.folderName);
        QByteArray encryptedData = file.readAll();
        QByteArray decryptedData = decrypt(encryptedData, key);

        // Parse decrypted data
        QDataStream decryptedStream(decryptedData);
        decryptedStream.setByteOrder(QDataStream::LittleEndian);

        if (!parseHeader(decryptedStream, archive)) {
            // Try parsing as concatenated INI files
            archive.isValid = true;
            QStringList fileNames = getStandardFileNames();

            int fileIndex = 0;
            QByteArray remainingData = decryptedData;
            QByteArray sectionHeader = "[HEADER]";

            while (!remainingData.isEmpty() && fileIndex < fileNames.size()) {
                int nextHeader = remainingData.indexOf(sectionHeader, 1);
                if (nextHeader == -1) nextHeader = remainingData.size();

                QByteArray chunk = remainingData.left(nextHeader);
                remainingData = remainingData.mid(nextHeader);

                ACDFileEntry entry;
                entry.fileName = fileNames[fileIndex];
                entry.offset = 0;
                entry.size = chunk.size();
                entry.data = chunk;
                archive.files.append(entry);
                fileIndex++;
            }
        } else {
            archive.isValid = true;
            parseFileEntries(decryptedStream, archive);
            extractFileData(decryptedStream, archive);
        }
    }

    file.close();
    return archive;
}

bool ACDParser::extractAll(const QString& acdPath, const QString& outputDir, QString* error) {
    ACDArchive archive = parse(acdPath, error);
    if (!archive.isValid) {
        return false;
    }

    QDir().mkpath(outputDir);

    for (const ACDFileEntry& entry : archive.files) {
        QString outPath = outputDir + "/" + entry.fileName;
        QFile outFile(outPath);
        if (outFile.open(QIODevice::WriteOnly)) {
            outFile.write(entry.data);
            outFile.close();
        } else {
            m_lastError = "Cannot write file: " + outPath;
            if (error) *error = m_lastError;
            return false;
        }
    }

    return true;
}

bool ACDParser::createArchive(const QString& inputDir, const QString& acdPath, const QString& folderName, QString* error) {
    QDir inputDirectory(inputDir);
    if (!inputDirectory.exists()) {
        m_lastError = "Input directory does not exist: " + inputDir;
        if (error) *error = m_lastError;
        return false;
    }

    // Collect all INI files
    QStringList iniFiles = inputDirectory.entryList(QStringList() << "*.ini", QDir::Files, QDir::Name);

    QByteArray archiveData;
    QDataStream stream(&archiveData, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    // Write concatenated INI files
    for (const QString& fileName : iniFiles) {
        QFile iniFile(inputDir + "/" + fileName);
        if (iniFile.open(QIODevice::ReadOnly)) {
            stream << iniFile.readAll();
            iniFile.close();
        }
    }

    // Encrypt and write
    QString key = createKey(folderName);
    QByteArray encryptedData = encrypt(archiveData, key);

    QFile outFile(acdPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        m_lastError = "Cannot create ACD file: " + acdPath;
        if (error) *error = m_lastError;
        return false;
    }

    outFile.write(encryptedData);
    outFile.close();

    return true;
}

QString ACDParser::createKey(const QString& folderName) {
    QString lower = folderName.toLower();
    int sum = 0;
    for (QChar c : lower) sum += c.unicode();

    int octet1 = ((sum % 256) + 256) % 256;

    int n = lower.length();
    int temp = 0;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (i + 1);
    }
    int octet2 = ((temp % 256) + 256) % 256;

    temp = 0;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (n - i);
    }
    int octet3 = ((temp % 256) + 256) % 256;

    temp = 5763;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (i + 1);
    }
    int octet4 = ((temp % 256) + 256) % 256;

    temp = 66;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (n + i + 1);
    }
    int octet5 = ((temp % 256) + 256) % 256;

    temp = 101;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (n - i + 1);
    }
    int octet6 = ((temp % 256) + 256) % 256;

    temp = 171;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (i + 2);
    }
    int octet7 = ((temp % 256) + 256) % 256;

    temp = 171;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (i + 3);
    }
    int octet8 = ((temp % 256) + 256) % 256;

    return QString("%1-%2-%3-%4-%5-%6-%7-%8")
        .arg(octet1).arg(octet2).arg(octet3).arg(octet4)
        .arg(octet5).arg(octet6).arg(octet7).arg(octet8);
}

QByteArray ACDParser::decrypt(const QByteArray& data, const QString& key) {
    QStringList parts = key.split('-');
    if (parts.size() != 8) return data;

    QVector<int> keyBytes;
    for (const QString& p : parts) keyBytes.append(p.toInt());

    QByteArray result = data;
    int dataLen = data.size();
    int keyLen = keyBytes.size();

    for (int i = 0; i < dataLen; ++i) {
        int rot = keyBytes[i % keyLen];
        int val = (int)(unsigned char)data[i];
        val = ((val - rot) % 256 + 256) % 256;
        result[i] = (char)val;
    }
    return result;
}

QByteArray ACDParser::encrypt(const QByteArray& data, const QString& key) {
    QStringList parts = key.split('-');
    if (parts.size() != 8) return data;

    QVector<int> keyBytes;
    for (const QString& p : parts) keyBytes.append(p.toInt());

    QByteArray result = data;
    int dataLen = data.size();
    int keyLen = keyBytes.size();

    for (int i = 0; i < dataLen; ++i) {
        int rot = keyBytes[i % keyLen];
        int val = (int)(unsigned char)data[i];
        val = ((val + rot) % 256 + 256) % 256;
        result[i] = (char)val;
    }
    return result;
}

bool ACDParser::isValidACD(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QByteArray header = file.peek(64);
    file.close();

    // Check if it starts with INI section header
    return header.startsWith('[') || header.size() > 0;
}

bool ACDParser::isEncrypted(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QByteArray header = file.peek(64);
    file.close();

    // If it doesn't start with '[', it's likely encrypted
    return !header.startsWith('[');
}

QStringList ACDParser::getStandardFileNames() {
    return QStringList() << "car.ini" << "engine.ini" << "tyres.ini"
                         << "brakes.ini" << "drivetrain.ini" << "suspension.ini"
                         << "aero.ini" << "damage.ini" << "electronics.ini"
                         << "fuel.ini" << "turbo.ini" << "cameras.ini"
                         << "driver3d.ini" << "liveries.ini" << "skins.ini";
}

bool ACDParser::parseHeader(QDataStream& stream, ACDArchive& archive) {
    // Try to read a simple header
    quint32 magic = 0;
    stream >> magic;

    // If magic doesn't match expected patterns, rewind
    if (magic != 0x00444341 && magic != 0x41434400) {
        stream.device()->seek(0);
        return false;
    }

    quint32 fileCount = 0;
    stream >> fileCount;

    if (fileCount > 1000) { // Sanity check
        stream.device()->seek(0);
        return false;
    }

    return true;
}

bool ACDParser::parseFileEntries(QDataStream& stream, ACDArchive& archive) {
    QStringList fileNames = getStandardFileNames();

    for (int i = 0; i < fileNames.size(); ++i) {
        ACDFileEntry entry;
        entry.fileName = fileNames[i];

        quint32 offset = 0;
        quint32 size = 0;
        stream >> offset >> size;

        if (size > 10000000) { // Sanity check
            break;
        }

        entry.offset = offset;
        entry.size = size;
        archive.files.append(entry);
    }

    return true;
}

bool ACDParser::extractFileData(QDataStream& stream, ACDArchive& archive) {
    for (ACDFileEntry& entry : archive.files) {
        stream.device()->seek(entry.offset);
        entry.data = stream.device()->read(entry.size);
    }
    return true;
}

// ============================================================================
// ACDManager implementation
// ============================================================================

ACDManager::ACDManager(const QString& carPath)
    : m_carPath(carPath) {
    m_acdPath = getACDPath();
    if (!m_acdPath.isEmpty()) {
        m_archive = ACDParser::parse(m_acdPath);
    }
}

bool ACDManager::hasACD() const {
    return !m_acdPath.isEmpty() && QFile::exists(m_acdPath);
}

bool ACDManager::isACDEncrypted() const {
    if (m_acdPath.isEmpty()) return false;
    return ACDParser::isEncrypted(m_acdPath);
}

QString ACDManager::getACDPath() const {
    // Check standard locations
    QString dataPath = m_carPath + "/data/data.acd";
    if (QFile::exists(dataPath)) return dataPath;

    dataPath = m_carPath + "/data/acd_0.accd";
    if (QFile::exists(dataPath)) return dataPath;

    return QString();
}

bool ACDManager::extractToFolder(const QString& outputDir) {
    if (m_acdPath.isEmpty()) {
        m_errors << "No ACD file found";
        return false;
    }

    QString outDir = outputDir;
    if (outDir.isEmpty()) {
        outDir = m_carPath + "/data_extracted";
    }

    QString error;
    bool result = ACDParser::extractAll(m_acdPath, outDir, &error);
    if (!result) {
        m_errors << error;
    }
    return result;
}

bool ACDManager::extractToDefaultLocation() {
    return extractToFolder(m_carPath + "/data_extracted");
}

bool ACDManager::repackFromFolder(const QString& inputDir) {
    QString inDir = inputDir;
    if (inDir.isEmpty()) {
        inDir = m_carPath + "/data_extracted";
    }

    if (!QDir(inDir).exists()) {
        m_errors << "Extracted folder does not exist: " + inDir;
        return false;
    }

    QString folderName = QFileInfo(m_carPath).fileName();
    QString error;
    bool result = ACDParser::createArchive(inDir, m_acdPath, folderName, &error);
    if (!result) {
        m_errors << error;
    }
    return result;
}

bool ACDManager::repackFromDefaultLocation() {
    return repackFromFolder(m_carPath + "/data_extracted");
}

QByteArray ACDManager::getFileContent(const QString& fileName) const {
    for (const ACDParser::ACDFileEntry& entry : m_archive.files) {
        if (entry.fileName == fileName) {
            return entry.data;
        }
    }
    return QByteArray();
}

QStringList ACDManager::getFileList() const {
    QStringList files;
    for (const ACDParser::ACDFileEntry& entry : m_archive.files) {
        files << entry.fileName;
    }
    return files;
}

bool ACDManager::hasFile(const QString& fileName) const {
    for (const ACDParser::ACDFileEntry& entry : m_archive.files) {
        if (entry.fileName == fileName) return true;
    }
    return false;
}

QString ACDManager::getStatus() const {
    if (m_acdPath.isEmpty()) return "No ACD file";
    if (!m_archive.isValid) return "Invalid ACD";
    if (m_archive.isEncrypted) return "Encrypted (" + QString::number(m_archive.files.size()) + " files)";
    return "Unencrypted (" + QString::number(m_archive.files.size()) + " files)";
}
