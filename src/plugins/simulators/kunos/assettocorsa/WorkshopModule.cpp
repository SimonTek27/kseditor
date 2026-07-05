#include "WorkshopModule.h"
#include "core/modmanager/ModManager.h"
#include <QDebug>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDesktopServices>
#include <QMainWindow>
#include <QDockWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QGroupBox>
#include <QCheckBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>

namespace ks {

// ─── WorkshopDownloader ──────────────────────────────────────────────────────

WorkshopDownloader::WorkshopDownloader(QObject* parent)
    : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
}

WorkshopDownloader::~WorkshopDownloader() {
    cancelAll();
}

void WorkshopDownloader::setOutputDirectory(const QString& dir) {
    m_outputDir = dir;
}

void WorkshopDownloader::downloadItem(const WorkshopItem& item) {
    m_queue.append(item);
    if (!m_downloading) {
        startNextDownload();
    }
}

void WorkshopDownloader::downloadItems(const QVector<WorkshopItem>& items) {
    m_queue.append(items);
    if (!m_downloading) {
        startNextDownload();
    }
}

void WorkshopDownloader::cancelDownload(quint64 workshopId) {
    if (m_activeDownloads.contains(workshopId)) {
        m_activeDownloads[workshopId]->abort();
        m_activeDownloads.remove(workshopId);
    }
}

void WorkshopDownloader::cancelAll() {
    for (auto* reply : m_activeDownloads.values()) {
        reply->abort();
        reply->deleteLater();
    }
    m_activeDownloads.clear();
    m_queue.clear();
    m_downloading = false;
}

void WorkshopDownloader::startNextDownload() {
    if (m_queue.isEmpty()) {
        m_downloading = false;
        emit allDownloadsFinished();
        return;
    }

    m_downloading = true;
    WorkshopItem item = m_queue.takeFirst();

    QString installPath = getCategoryInstallPath(item.category);
    QString fileName = QString("%1_%2.acd").arg(item.workshopId).arg(item.title.replace("/", "_"));
    QString savePath = installPath + "/" + fileName;

    QUrl url = item.workshopUrl;
    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager->get(request);

    m_activeDownloads[item.workshopId] = reply;
    connect(reply, &QNetworkReply::finished, this, &WorkshopDownloader::onFinished);
    connect(reply, &QNetworkReply::readyRead, this, &WorkshopDownloader::onReadyRead);

    emit downloadStarted(item.workshopId, item.title);
}

void WorkshopDownloader::onFinished() {
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    quint64 workshopId = m_activeDownloads.key(reply);
    m_activeDownloads.remove(workshopId);

    if (reply->error() == QNetworkReply::NoError) {
        QString savePath = m_outputDir + "/" + QString::number(workshopId) + ".zip";
        QFile file(savePath);
        if (file.open(QIODevice::WriteOnly)) {
            // Use buffered data if available (from onReadyRead), otherwise read directly
            QByteArray data = m_buffers.value(workshopId);
            if (data.isEmpty()) data = reply->readAll();
            file.write(data);
            file.close();
            emit downloadFinished(workshopId, savePath);
        } else {
            emit downloadFailed(workshopId, "Failed to save file");
        }
    } else {
        emit downloadFailed(workshopId, reply->errorString());
    }

    m_buffers.remove(workshopId);
    reply->deleteLater();
    startNextDownload();
}

void WorkshopDownloader::onReadyRead() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    // Find the workshop ID for this reply
    for (auto it = m_activeDownloads.begin(); it != m_activeDownloads.end(); ++it) {
        if (it.value() == reply) {
            m_buffers[it.key()].append(reply->readAll());
            break;
        }
    }
}

QString WorkshopDownloader::getCategoryInstallPath(const QString& category) const {
    QString basePath = m_outputDir.isEmpty() ? QDir::homePath() + "/Documents/Assetto Corsa" : m_outputDir;

    if (category == "cars") return basePath + "/content/cars";
    if (category == "tracks") return basePath + "/content/tracks";
    if (category == "skins") return basePath + "/content/skins";
    if (category == "apps") return basePath + "/apps";

    return basePath;
}

// ─── WorkshopModule ──────────────────────────────────────────────────────────

WorkshopModule::WorkshopModule(QWidget* parent)
    : EditorModule(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
    m_downloader = new WorkshopDownloader(this);
}

WorkshopModule::~WorkshopModule() {
    shutdown();
}

bool WorkshopModule::initialize() {
    qDebug() << "Workshop module initialized";
    return true;
}

void WorkshopModule::shutdown() {
    if (m_downloader) {
        m_downloader->cancelAll();
    }
    m_items.clear();
    m_installedItems.clear();
}

QDockWidget* WorkshopModule::getOrCreateDockWidget(QMainWindow* mainWindow) {
    static QDockWidget* dock = nullptr;
    if (!dock) {
        dock = new QDockWidget("Workshop", mainWindow);
        dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        setupUi(mainWindow, dock);
    }
    return dock;
}

void WorkshopModule::setupUi(QMainWindow* mainWindow, QDockWidget* dock) {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);

    auto* header = new QLabel("<h3>Steam Workshop</h3>");
    header->setAlignment(Qt::AlignCenter);
    layout->addWidget(header);

    auto* categoryLabel = new QLabel("Category:");
    layout->addWidget(categoryLabel);

    auto* categoryCombo = new QComboBox();
    categoryCombo->addItem("All", "all");
    categoryCombo->addItem("Cars", "cars");
    categoryCombo->addItem("Tracks", "tracks");
    categoryCombo->addItem("Skins", "skins");
    categoryCombo->addItem("Apps", "apps");
    layout->addWidget(categoryCombo);

    auto* searchLayout = new QHBoxLayout();
    auto* searchBox = new QLineEdit();
    searchBox->setPlaceholderText("Search...");
    auto* searchBtn = new QPushButton("Search");
    searchLayout->addWidget(searchBox);
    searchLayout->addWidget(searchBtn);
    layout->addLayout(searchLayout);

    auto* refreshBtn = new QPushButton("Refresh");
    layout->addWidget(refreshBtn);

    auto* tableLabel = new QLabel("Workshop Items:");
    layout->addWidget(tableLabel);

    auto* table = new QTableWidget();
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"Title", "Author", "Size", "Status"});
    table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(table);

    auto* downloadBtn = new QPushButton("Download Selected");
    layout->addWidget(downloadBtn);

    auto* viewInstalledBtn = new QPushButton("View Installed");
    layout->addWidget(viewInstalledBtn);

    dock->setWidget(widget);

    connect(categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, categoryCombo](int index) {
                QString cat = categoryCombo->itemData(index).toString();
                browseCategory(cat);
            });

    connect(searchBtn, &QPushButton::clicked, [this, searchBox]() {
        searchItems(searchBox->text());
    });

    connect(refreshBtn, &QPushButton::clicked, this, &WorkshopModule::refreshWorkshop);
    connect(downloadBtn, &QPushButton::clicked, this, &WorkshopModule::downloadSelected);
    connect(viewInstalledBtn, &QPushButton::clicked, this, &WorkshopModule::viewInstalled);

    connect(m_downloader, &WorkshopDownloader::downloadStarted, this,
            [](quint64 id, const QString& title) {
                qDebug() << "Started download:" << title;
            });
    connect(m_downloader, &WorkshopDownloader::downloadFinished, this,
            [this](quint64 id, const QString& path) {
                qDebug() << "Downloaded to:" << path;
                emit downloadCompleted(id);
            });
}

void WorkshopModule::refreshWorkshop() {
    browseCategory(m_currentCategory);
}

void WorkshopModule::browseCategory(const QString& category) {
    m_currentCategory = category;
    QString url = getWorkshopUrlForCategory(category);

    qDebug() << "Loading workshop category:" << category << "from" << url;

    QUrl urlObj(url);
    QNetworkRequest request(urlObj);
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString html = reply->readAll();
            parseWorkshopPage(html);
            emit workshopLoaded(m_items);
        } else {
            qWarning() << "Failed to load workshop:" << reply->errorString();
        }
        reply->deleteLater();
    });
}

void WorkshopModule::searchItems(const QString& query) {
    if (query.isEmpty()) {
        browseCategory(m_currentCategory);
        return;
    }

    QString searchUrl = "https://steamcommunity.com/workshop/browse/?appid=244160&searchtext=" +
                        QUrl::toPercentEncoding(query);

    QUrl searchUrlObj(searchUrl);
    QNetworkRequest request(searchUrlObj);
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString html = reply->readAll();
            parseWorkshopPage(html);
            emit workshopLoaded(m_items);
        }
        reply->deleteLater();
    });
}

void WorkshopModule::downloadItem(const WorkshopItem& item) {
    QString defaultPath = QFileDialog::getExistingDirectory(nullptr,
        "Select Content Directory",
        QDir::homePath() + "/Documents/Assetto Corsa");

    if (!defaultPath.isEmpty()) {
        m_downloader->setOutputDirectory(defaultPath);
        m_downloader->downloadItem(item);

        // Connect to downloadFinished to handle post-download dependency resolution
        connect(m_downloader, &WorkshopDownloader::downloadFinished, this,
            [this, item](quint64 id, const QString& path) {
                if (id == item.workshopId) {
                    handlePostDownload(item, path);
                }
            }, Qt::SingleShotConnection);
    }
}

void WorkshopModule::downloadSelected() {
    if (m_items.isEmpty()) {
        QMessageBox::information(nullptr, "Workshop", "No items selected. Browse and select items to download.");
        return;
    }

    QString defaultPath = QFileDialog::getExistingDirectory(nullptr,
        "Select Content Directory",
        QDir::homePath() + "/Documents/Assetto Corsa");

    if (!defaultPath.isEmpty()) {
        m_downloader->setOutputDirectory(defaultPath);
        m_downloader->downloadItems(m_items);
    }
}

void WorkshopModule::handlePostDownload(const WorkshopItem& item, const QString& packagePath) {
    // Extract manifest to get dependencies
    QString manifestPath;
    QDir pkgDir(packagePath);
    if (pkgDir.exists()) {
        manifestPath = pkgDir.filePath("manifest.json");
    } else {
        // Try workshop_manifest.json
        manifestPath = pkgDir.filePath("workshop_manifest.json");
    }

    QStringList dependencies;
    QStringList conflicts;
    QString modName = item.title;
    QString modVersion = "1.0";

    if (QFile::exists(manifestPath)) {
        QFile mf(manifestPath);
        if (mf.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(mf.readAll());
            mf.close();
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                modName = obj["name"].toString(modName);
                modVersion = obj["version"].toString(modVersion);

                QJsonArray deps = obj["dependencies"].toArray();
                for (const auto& d : deps) {
                    dependencies.append(d.toString());
                }
                QJsonArray confs = obj["conflicts"].toArray();
                for (const auto& c : confs) {
                    conflicts.append(c.toString());
                }
            }
        }
    }

    // Use ModManager to resolve and install dependencies
    auto* modManager = ModManagerModule::instance();
    if (modManager) {
        // Check for missing dependencies
        QStringList missingDeps;
        for (const QString& dep : dependencies) {
            QString depName = dep;
            // Parse version constraint if present: "name (>= 1.0.0)"
            QRegularExpression re(R"(^(\S+)\s*\(([^)]+)\)\s*$)");
            auto match = re.match(dep);
            if (match.hasMatch()) {
                depName = match.captured(1);
            }

            bool satisfied = false;
            for (const auto& mod : modManager->mods()) {
                if (mod.name == depName && mod.enabled) {
                    satisfied = true;
                    break;
                }
            }

            if (!satisfied) {
                missingDeps.append(depName);
            }
        }

        if (!missingDeps.isEmpty()) {
            // Try to install missing dependencies from repository
            modManager->installMissingDependencies(missingDeps);
        }

        // Now install the main mod via ModManager
        // This will also apply load order
        modManager->installMod(packagePath);
    }
}

void WorkshopModule::viewInstalled() {
    m_installedItems.clear();

    QString basePath = QDir::homePath() + "/Documents/Assetto Corsa";

    QStringList dirs = {
        basePath + "/content/cars",
        basePath + "/content/tracks",
        basePath + "/content/skins",
        basePath + "/apps"
    };

    for (const QString& dirPath : dirs) {
        QDir dir(dirPath);
        if (dir.exists()) {
            for (const QString& item : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                WorkshopItem item2;
                item2.title = item;
                item2.localPath = dirPath + "/" + item;
                item2.installed = true;
                m_installedItems.append(item2);
            }
        }
    }

    qDebug() << "Found" << m_installedItems.size() << "installed items";
    emit workshopLoaded(m_installedItems);
}

void WorkshopModule::openInBrowser(const WorkshopItem& item) {
    QDesktopServices::openUrl(item.workshopUrl);
}

void WorkshopModule::loadWorkshopItems() {
    m_items.clear();

    QString url = getWorkshopUrlForCategory(m_currentCategory);
    if (url.isEmpty()) {
        emit statusMessage("Unknown workshop category: " + m_currentCategory);
        return;
    }

    QNetworkRequest request{QUrl(url)};
    request.setRawHeader("User-Agent", QByteArray("ksEditor/2.0"));
    request.setRawHeader("Accept", QByteArray("text/html"));

    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            parseWorkshopPage(QString::fromUtf8(reply->readAll()));
            emit itemsLoaded(m_items);
        } else {
            emit statusMessage("Failed to load workshop items: " + reply->errorString());
        }
        reply->deleteLater();
    });
}

void WorkshopModule::parseWorkshopPage(const QString& html) {
    m_items.clear();

    // Primary regex: extract workshop ID, title, and author from Steam Workshop HTML
    QRegularExpression itemRegex("data-publishedfileid=\"(\\d+)\"[^>]*>.*?<div class=\"workshopItemTitle\">([^<]+)</div>.*?<div class=\"workshopItemAuthor\">[^:]*:([^<]+)</div>");
    QRegularExpressionMatchIterator it = itemRegex.globalMatch(html);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        WorkshopItem item;
        item.workshopId = match.captured(1).toULongLong();
        item.title = match.captured(2).trimmed();
        item.author = match.captured(3).trimmed();
        item.workshopUrl = QUrl("https://steamcommunity.com/sharedfiles/filedetails/?id=" + QString::number(item.workshopId));
        item.category = m_currentCategory;
        item.installed = false;
        item.sizeBytes = 0;

        // Try to extract preview image URL
        QRegularExpression previewRegex("data-publishedfileid=\"" + QString::number(item.workshopId) + "\".*?src=\"([^\"]+)\"");
        QRegularExpressionMatch previewMatch = previewRegex.match(html);
        if (previewMatch.hasMatch()) {
            item.previewUrl = previewMatch.captured(1);
        }

        // Check if this item is already installed
        for (const auto& installed : m_installedItems) {
            if (installed.workshopId == item.workshopId) {
                item.installed = true;
                item.localPath = installed.localPath;
                break;
            }
        }

        m_items.append(item);
    }

    qDebug() << "Parsed" << m_items.size() << "workshop items from HTML";
}

QString WorkshopModule::getWorkshopUrlForCategory(const QString& category) {
    if (category == "cars") {
        return "https://steamcommunity.com/workshop/browse/?appid=244160&requiredtags[]=Cars&actualsort=trend&p=1";
    } else if (category == "tracks") {
        return "https://steamcommunity.com/workshop/browse/?appid=244160&requiredtags[]=Tracks&actualsort=trend&p=1";
    } else if (category == "skins") {
        return "https://steamcommunity.com/workshop/browse/?appid=244160&requiredtags[]=Skins&actualsort=trend&p=1";
    } else if (category == "apps") {
        return "https://steamcommunity.com/workshop/browse/?appid=244160&requiredtags[]=Apps&actualsort=trend&p=1";
    }
    return "https://steamcommunity.com/workshop/browse/?appid=244160&actualsort=trend&p=1";
}

// ─── WorkshopQmlBridge ───────────────────────────────────────────────────────

WorkshopQmlBridge* WorkshopQmlBridge::s_instance = nullptr;

WorkshopQmlBridge* WorkshopQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new WorkshopQmlBridge();
    }
    return s_instance;
}

void WorkshopQmlBridge::refreshWorkshop() {
    m_isLoading = true;
    emit loadingChanged();

    m_networkManager = new QNetworkAccessManager(this);
    QString url = "https://www.assettocorsa.net/forum/index.php?forums/resources.106/";

    QNetworkRequest request{QUrl{url}};
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString html = reply->readAll();
            parseWorkshopHTML(html);
        }
        reply->deleteLater();
        m_isLoading = false;
        emit loadingChanged();
        emit itemCountChanged();
    });
}

void WorkshopQmlBridge::browseCategory(const QString& category) {
    m_currentCategory = category;
    emit categoryChanged();
    refreshWorkshop();
}

void WorkshopQmlBridge::searchItems(const QString& query) {
    if (query.isEmpty()) {
        refreshWorkshop();
        return;
    }

    m_isLoading = true;
    emit loadingChanged();

    if (m_networkManager) {
        m_networkManager->deleteLater();
    }
    m_networkManager = new QNetworkAccessManager(this);

    QString searchUrl = "https://www.assettocorsa.net/forum/index.php?search/&q=" + query;
    QNetworkRequest request{QUrl{searchUrl}};
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString html = reply->readAll();
            parseWorkshopHTML(html);
        }
        reply->deleteLater();
        m_isLoading = false;
        emit loadingChanged();
        emit itemCountChanged();
    });
}

QVariantList WorkshopQmlBridge::getItems() {
    QVariantList result;
    for (const auto& item : m_items) {
        QVariantMap m;
        m["workshopId"] = static_cast<qlonglong>(item.workshopId);
        m["title"] = item.title;
        m["description"] = item.description;
        m["author"] = item.author;
        m["category"] = item.category;
        m["sizeBytes"] = static_cast<qlonglong>(item.sizeBytes);
        m["previewUrl"] = item.previewUrl;
        m["workshopUrl"] = item.workshopUrl.toString();
        m["updated"] = item.updated.toString("yyyy-MM-dd");
        m["installed"] = item.installed;
        m["localPath"] = item.localPath;
        result.append(m);
    }
    return result;
}

QVariantMap WorkshopQmlBridge::getItem(int index) {
    if (index < 0 || index >= m_items.size()) return QVariantMap();

    const auto& item = m_items[index];
    QVariantMap m;
    m["workshopId"] = static_cast<qlonglong>(item.workshopId);
    m["title"] = item.title;
    m["description"] = item.description;
    m["author"] = item.author;
    m["category"] = item.category;
    m["sizeBytes"] = static_cast<qlonglong>(item.sizeBytes);
    m["previewUrl"] = item.previewUrl;
    m["workshopUrl"] = item.workshopUrl.toString();
    m["updated"] = item.updated.toString("yyyy-MM-dd");
    m["installed"] = item.installed;
    m["localPath"] = item.localPath;
    return m;
}

void WorkshopQmlBridge::downloadItem(int index) {
    if (index < 0 || index >= m_items.size()) return;

    if (!m_downloader) {
        m_downloader = new WorkshopDownloader(this);
        connect(m_downloader, &WorkshopDownloader::downloadProgress, this, [this](quint64 id, qint64 received, qint64 total) {
            int percent = total > 0 ? static_cast<int>((received * 100) / total) : 0;
            emit downloadProgress(id, percent);
        });
        connect(m_downloader, &WorkshopDownloader::downloadFinished, this, [this](quint64 id, const QString& path) {
            emit downloadFinished(id, path);
        });
        connect(m_downloader, &WorkshopDownloader::downloadFailed, this, [this](quint64 id, const QString& error) {
            emit downloadFailed(id, error);
        });
    }

    m_downloader->downloadItem(m_items[index]);
}

void WorkshopQmlBridge::downloadItemById(qint64 workshopId) {
    for (const auto& item : m_items) {
        if (item.workshopId == static_cast<quint64>(workshopId)) {
            downloadItem(m_items.indexOf(item));
            return;
        }
    }
}

void WorkshopQmlBridge::cancelDownload(qint64 workshopId) {
    if (m_downloader) {
        m_downloader->cancelDownload(static_cast<quint64>(workshopId));
    }
}

void WorkshopQmlBridge::cancelAllDownloads() {
    if (m_downloader) {
        m_downloader->cancelAll();
    }
}

QVariantList WorkshopQmlBridge::getInstalledItems() {
    QVariantList result;
    for (const auto& item : m_items) {
        if (item.installed) {
            QVariantMap m;
            m["workshopId"] = static_cast<qlonglong>(item.workshopId);
            m["title"] = item.title;
            m["category"] = item.category;
            m["localPath"] = item.localPath;
            result.append(m);
        }
    }
    return result;
}

void WorkshopQmlBridge::openInBrowser(int index) {
    if (index < 0 || index >= m_items.size()) return;
    QDesktopServices::openUrl(m_items[index].workshopUrl);
}

void WorkshopQmlBridge::setOutputDirectory(const QString& dir) {
    m_outputDir = dir;
    if (m_downloader) {
        m_downloader->setOutputDirectory(dir);
    }
}

void WorkshopQmlBridge::parseWorkshopHTML(const QString& html) {
    m_items.clear();

    QStringList lines = html.split("\n");
    for (const auto& line : lines) {
        if (line.contains("resource-title") || line.contains("resource-author")) {
            WorkshopItem item;
            item.workshopId = m_items.size() + 1;
            item.category = m_currentCategory;
            item.installed = false;

            int titleStart = line.indexOf(">") + 1;
            int titleEnd = line.indexOf("<", titleStart);
            if (titleStart > 0 && titleEnd > titleStart) {
                item.title = line.mid(titleStart, titleEnd - titleStart).trimmed();
            }

            int authorStart = line.indexOf("author\">") + 7;
            int authorEnd = line.indexOf("<", authorStart);
            if (authorStart > 6 && authorEnd > authorStart) {
                item.author = line.mid(authorStart, authorEnd - authorStart).trimmed();
            }

            m_items.append(item);
        }
    }

    m_itemCount = m_items.size();
}

// ─── Module: Update Checking ─────────────────────────────────────────────

void WorkshopModule::checkForUpdates() {
    m_pendingUpdates.clear();

    for (const auto& installed : m_installedItems) {
        checkItemVersion(installed.workshopId, m_installedVersions.value(installed.workshopId, "1.0"));
    }

    if (!m_pendingUpdates.isEmpty()) {
        emit updatesAvailable(m_pendingUpdates.size());
        emit statusMessage(QString("%1 update(s) available").arg(m_pendingUpdates.size()));
    } else {
        emit statusMessage("All items are up to date");
    }
}

void WorkshopModule::checkItemVersion(quint64 workshopId, const QString& currentVersion) {
    // Read version from the item's local manifest if available
    for (const auto& installed : m_installedItems) {
        if (installed.workshopId == workshopId) {
            QString manifestPath = installed.localPath + "/manifest.json";
            QFile manifestFile(manifestPath);
            if (manifestFile.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll());
                manifestFile.close();

                if (doc.isObject()) {
                    QJsonObject obj = doc.object();
                    QString remoteVersion = obj["version"].toString();
                    QString remoteTitle = obj["name"].toString();

                    if (!remoteVersion.isEmpty() && remoteVersion != currentVersion) {
                        WorkshopUpdateInfo update;
                        update.workshopId = workshopId;
                        update.title = remoteTitle.isEmpty() ? installed.title : remoteTitle;
                        update.currentVersion = currentVersion;
                        update.availableVersion = remoteVersion;
                        update.changelog = obj["changelog"].toString();

                        // Semantic version comparison: check if remote is newer
                        QStringList curParts = currentVersion.split('.');
                        QStringList remParts = remoteVersion.split('.');
                        bool isNewer = false;
                        for (int i = 0; i < qMax(curParts.size(), remParts.size()); ++i) {
                            int cur = (i < curParts.size()) ? curParts[i].toInt() : 0;
                            int rem = (i < remParts.size()) ? remParts[i].toInt() : 0;
                            if (rem > cur) { isNewer = true; break; }
                            if (rem < cur) break;
                        }

                        if (isNewer) {
                            m_pendingUpdates.append(update);
                            qDebug() << "Update available for" << installed.title
                                     << ":" << currentVersion << "->" << remoteVersion;
                        }
                    }
                }
            }
            break;
        }
    }
}

void WorkshopModule::updateItem(quint64 workshopId) {
    if (m_updatingItems.contains(workshopId)) return;
    m_updatingItems.insert(workshopId);

    // Find the item and re-download
    for (const auto& item : m_installedItems) {
        if (item.workshopId == workshopId) {
            WorkshopItem updatedItem = item;
            m_downloader->downloadItem(updatedItem);

            connect(m_downloader, &WorkshopDownloader::downloadFinished, this,
                [this, workshopId](quint64 id, const QString& path) {
                    if (id == workshopId) {
                        m_installedVersions[workshopId] = "updated";
                        m_updatingItems.remove(workshopId);
                        emit updateCompleted(workshopId, true);

                        // Remove from pending updates
                        auto it = std::remove_if(m_pendingUpdates.begin(), m_pendingUpdates.end(),
                            [workshopId](const WorkshopUpdateInfo& u) { return u.workshopId == workshopId; });
                        if (it != m_pendingUpdates.end()) m_pendingUpdates.erase(it);
                    }
                });

            connect(m_downloader, &WorkshopDownloader::downloadFailed, this,
                [this, workshopId](quint64 id, const QString&) {
                    if (id == workshopId) {
                        m_updatingItems.remove(workshopId);
                        emit updateCompleted(workshopId, false);
                    }
                });
            break;
        }
    }
}

void WorkshopModule::updateAll() {
    for (const auto& update : m_pendingUpdates) {
        updateItem(update.workshopId);
    }
}

// ─── Module: Dependency Resolution ──────────────────────────────────────

void WorkshopModule::resolveDependencies(quint64 workshopId) {
    QVector<WorkshopDependency> deps;

    // Read dependency info from the item's local manifest
    for (const auto& item : m_items) {
        if (item.workshopId == workshopId) {
            QString manifestPath = item.localPath + "/manifest.json";
            if (manifestPath.isEmpty() || !QFile::exists(manifestPath)) {
                m_dependencyCache[workshopId] = deps;
                return;
            }

            QFile manifestFile(manifestPath);
            if (!manifestFile.open(QIODevice::ReadOnly)) {
                m_dependencyCache[workshopId] = deps;
                return;
            }

            QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll());
            manifestFile.close();

            if (!doc.isObject()) {
                m_dependencyCache[workshopId] = deps;
                return;
            }

            QJsonObject obj = doc.object();
            QJsonArray depsArray = obj["dependencies"].toArray();

            for (const auto& d : depsArray) {
                QJsonObject depObj = d.toObject();
                WorkshopDependency dep;
                dep.workshopId = depObj["workshopId"].toInteger();
                dep.title = depObj["title"].toString();
                dep.requiredVersion = depObj["version"].toString();
                dep.satisfied = false;

                // Check if dependency is installed
                for (const auto& installed : m_installedItems) {
                    if (installed.workshopId == dep.workshopId) {
                        dep.satisfied = true;
                        break;
                    }
                }

                deps.append(dep);
            }
            break;
        }
    }

    m_dependencyCache[workshopId] = deps;
}

QVector<WorkshopDependency> WorkshopModule::getDependencies(quint64 workshopId) const {
    return m_dependencyCache.value(workshopId);
}

QVector<WorkshopDependency> WorkshopModule::getConflicts(quint64 workshopId) const {
    QVector<WorkshopDependency> conflicts;

    // Read conflict info from the item's local manifest
    for (const auto& item : m_items) {
        if (item.workshopId == workshopId) {
            QString manifestPath = item.localPath + "/manifest.json";
            if (!manifestPath.isEmpty() && QFile::exists(manifestPath)) {
                QFile manifestFile(manifestPath);
                if (manifestFile.open(QIODevice::ReadOnly)) {
                    QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll());
                    manifestFile.close();

                    if (doc.isObject()) {
                        QJsonObject obj = doc.object();
                        QJsonArray conflictsArray = obj["conflicts"].toArray();

                        for (const auto& c : conflictsArray) {
                            QJsonObject conflictObj = c.toObject();
                            WorkshopDependency conflict;
                            conflict.workshopId = conflictObj["workshopId"].toInteger();
                            conflict.title = conflictObj["title"].toString();
                            conflict.satisfied = false;

                            // Check if the conflicting item is installed
                            for (const auto& installed : m_installedItems) {
                                if (installed.workshopId == conflict.workshopId) {
                                    conflict.satisfied = true;
                                    break;
                                }
                            }

                            if (conflict.satisfied) {
                                conflicts.append(conflict);
                            }
                        }
                    }
                }
            }
            break;
        }
    }

    // Also check reverse conflicts: items that conflict with this one
    for (const auto& installed : m_installedItems) {
        if (installed.workshopId == workshopId) continue;

        QString manifestPath = installed.localPath + "/manifest.json";
        if (!manifestPath.isEmpty() && QFile::exists(manifestPath)) {
            QFile manifestFile(manifestPath);
            if (manifestFile.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll());
                manifestFile.close();

                if (doc.isObject()) {
                    QJsonArray conflictsArray = doc.object()["conflicts"].toArray();
                    for (const auto& c : conflictsArray) {
                        QJsonObject conflictObj = c.toObject();
                        if (conflictObj["workshopId"].toInteger() == static_cast<qint64>(workshopId)) {
                            WorkshopDependency conflict;
                            conflict.workshopId = installed.workshopId;
                            conflict.title = installed.title;
                            conflict.satisfied = true;
                            conflicts.append(conflict);
                        }
                    }
                }
            }
        }
    }

    return conflicts;
}

bool WorkshopModule::canInstall(quint64 workshopId, QStringList& blockingConflicts) {
    auto conflicts = getConflicts(workshopId);
    blockingConflicts.clear();

    for (const auto& conflict : conflicts) {
        if (!conflict.satisfied) {
            blockingConflicts << conflict.title;
            emit conflictDetected(conflict.title, "Item " + QString::number(workshopId));
        }
    }

    return blockingConflicts.isEmpty();
}

} // namespace ks
