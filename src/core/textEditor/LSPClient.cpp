#include "LSPClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QUrl>
#include <QTimer>
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QEventLoop>

namespace ks {

LSPClient::LSPClient(QObject* parent)
    : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::readyRead, this, &LSPClient::onSocketReadyRead);
    connect(m_socket, &QTcpSocket::connected, this, &LSPClient::onSocketConnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &LSPClient::onSocketError);
    
    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LSPClient::onProcessFinished);
    connect(m_process, QOverload<QProcess::ProcessError>::of(&QProcess::errorOccurred),
            this, &LSPClient::onProcessError);
}

LSPClient::~LSPClient() {
    stop();
}

bool LSPClient::start(const QString& command, const QStringList& args, const QString& workingDir) {
    if (m_connected) return true;
    
    m_process->setWorkingDirectory(workingDir.isEmpty() ? QDir::currentPath() : workingDir);
    m_process->start(command, args);
    
    if (!m_process->waitForStarted(5000)) {
        emit errorOccurred("Failed to start language server: " + command);
        return false;
    }
    
    return true;
}

bool LSPClient::connectToServer(const QString& host, int port) {
    if (m_connected) return true;
    
    m_socket->connectToHost(host, port);
    if (!m_socket->waitForConnected(5000)) {
        emit errorOccurred("Failed to connect to LSP server: " + host + ":" + QString::number(port));
        return false;
    }
    
    return true;
}

void LSPClient::stop() {
    if (m_process && m_process->state() == QProcess::Running) {
        shutdown();
        m_process->waitForFinished(2000);
    }
    
    if (m_socket->state() == QTcpSocket::ConnectedState) {
        m_socket->disconnectFromHost();
        m_socket->waitForDisconnected(1000);
    }
    
    m_connected = false;
    m_initialized = false;
    m_pendingRequests.clear();
    m_openDocuments.clear();
    emit disconnected();
}

bool LSPClient::initialize(const QString& rootPath, const QStringList& capabilities) {
    m_rootPath = rootPath;
    
    QJsonObject params;
    params["processId"] = QCoreApplication::applicationPid();
    params["rootUri"] = QUrl::fromLocalFile(rootPath).toString();
    
    // Client capabilities
    QJsonObject clientCaps;
    clientCaps["textDocument"] = QJsonObject{
        {"synchronization", QJsonObject{
            {"dynamicRegistration", true},
            {"willSave", true},
            {"willSaveWaitUntil", true},
            {"didSave", true}
        }},
        {"completion", QJsonObject{
            {"dynamicRegistration", true},
            {"completionItem", QJsonObject{
                {"snippetSupport", true},
                {"commitCharactersSupport", true},
                {"documentationFormat", QJsonArray{"markdown", "plaintext"}},
                {"deprecatedSupport", true},
                {"preselectSupport", true}
            }},
            {"completionItemKind", QJsonObject{
                {"valueSet", QJsonArray{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17}}
            }},
            {"contextSupport", true}
        }},
        {"hover", QJsonObject{
            {"dynamicRegistration", true},
            {"contentFormat", QJsonArray{"markdown", "plaintext"}}
        }},
        {"definition", QJsonObject{
            {"dynamicRegistration", true},
            {"linkSupport", true}
        }},
        {"references", QJsonObject{
            {"dynamicRegistration", true}
        }},
        {"documentSymbol", QJsonObject{
            {"dynamicRegistration", true},
            {"hierarchicalDocumentSymbolSupport", true},
            {"symbolKind", QJsonObject{
                {"valueSet", QJsonArray{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26}}
            }}
        }},
        {"workspaceSymbol", QJsonObject{
            {"dynamicRegistration", true},
            {"symbolKind", QJsonObject{
                {"valueSet", QJsonArray{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26}}
            }}
        }},
        {"codeAction", QJsonObject{
            {"dynamicRegistration", true},
            {"codeActionLiteralSupport", QJsonObject{
                {"codeActionKind", QJsonObject{
                    {"valueSet", QJsonArray{"", "quickfix", "refactor", "refactor.extract", "refactor.inline", "refactor.rewrite", "source", "source.organizeImports"}}
                }}
            }}
        }},
        {"formatting", QJsonObject{
            {"dynamicRegistration", true}
        }},
        {"rangeFormatting", QJsonObject{
            {"dynamicRegistration", true}
        }},
        {"onTypeFormatting", QJsonObject{
            {"dynamicRegistration", true}
        }},
        {"rename", QJsonObject{
            {"dynamicRegistration", true},
            {"prepareSupport", true}
        }},
        {"foldingRange", QJsonObject{
            {"dynamicRegistration", true},
            {"rangeLimit", 5000},
            {"lineFoldingOnly", true}
        }},
        {"semanticTokens", QJsonObject{
            {"dynamicRegistration", true},
            {"requests", QJsonObject{
                {"full", QJsonObject{
                    {"delta", true}
                }},
                {"range", true}
            }},
            {"tokenTypes", QJsonArray{"namespace", "type", "class", "enum", "interface", "struct", "typeParameter", "parameter", "variable", "property", "enumMember", "event", "function", "method", "macro", "keyword", "modifier", "comment", "string", "number", "regexp", "operator", "decorator"}},
            {"tokenModifiers", QJsonArray{"declaration", "definition", "readonly", "static", "deprecated", "abstract", "async", "modification", "documentation", "defaultLibrary"}}
        }},
        {"codeLens", QJsonObject{
            {"dynamicRegistration", true}
        }},
        {"documentLink", QJsonObject{
            {"dynamicRegistration", true}
        }},
        {"colorProvider", QJsonObject{
            {"dynamicRegistration", true}
        }},
        {"inlayHint", QJsonObject{
            {"dynamicRegistration", true},
            {"resolveSupport", QJsonObject{
                {"properties", QJsonArray{"tooltip", "textEdits"}}
            }}
        }}
    };
    
    clientCaps["workspace"] = QJsonObject{
        {"applyEdit", true},
        {"workspaceEdit", QJsonObject{
            {"documentChanges", true},
            {"resourceOperations", QJsonArray{"create", "rename", "delete"}},
            {"failureHandling", "abort"}
        }},
        {"didChangeConfiguration", QJsonObject{
            {"dynamicRegistration", true}
        }},
        {"didChangeWatchedFiles", QJsonObject{
            {"dynamicRegistration", true}
        }},
        {"symbol", QJsonObject{
            {"dynamicRegistration", true},
            {"symbolKind", QJsonObject{
                {"valueSet", QJsonArray{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26}}
            }}
        }},
        {"executeCommand", QJsonObject{
            {"dynamicRegistration", true}
        }}
    };
    
    clientCaps["window"] = QJsonObject{
        {"workDoneProgress", true}
    };
    
    // Add user-provided capabilities
    for (const QString& cap : capabilities) {
        // Could parse and merge
    }
    
    params["capabilities"] = clientCaps;
    params["trace"] = "verbose";
    params["workspaceFolders"] = QJsonArray();
    
    sendRequest("initialize", params);
    
    // Wait for initialization response (with timeout)
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect(this, &LSPClient::connected, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.start(10000);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    loop.exec();
    
    return m_initialized;
}

void LSPClient::shutdown() {
    if (!m_connected) return;
    sendRequest("shutdown", QJsonObject());
    // Wait for response
    QEventLoop loop;
    QTimer::singleShot(2000, &loop, &QEventLoop::quit);
    loop.exec();
}

void LSPClient::exit() {
    sendNotification("exit", QJsonObject());
    stop();
}

void LSPClient::didOpen(const QString& filePath, const QString& languageId, int version, const QString& text) {
    if (!m_connected) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{
        {"uri", QUrl::fromLocalFile(filePath).toString()},
        {"languageId", languageId},
        {"version", version},
        {"text", text}
    };
    
    m_openDocuments[filePath] = version;
    sendNotification("textDocument/didOpen", params);
}

void LSPClient::didChange(const QString& filePath, int version, const QVector<QPair<int, QString>>& changes) {
    if (!m_connected) return;
    if (!m_openDocuments.contains(filePath)) return;
    
    QJsonArray contentChanges;
    for (const auto& change : changes) {
        QJsonObject changeObj;
        changeObj["text"] = change.second;
        // For incremental sync, we'd need range
        contentChanges.append(changeObj);
    }
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{
        {"uri", QUrl::fromLocalFile(filePath).toString()},
        {"version", version}
    };
    params["contentChanges"] = contentChanges;
    
    m_openDocuments[filePath] = version;
    sendNotification("textDocument/didChange", params);
}

void LSPClient::didClose(const QString& filePath) {
    if (!m_connected) return;
    if (!m_openDocuments.contains(filePath)) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{
        {"uri", QUrl::fromLocalFile(filePath).toString()}
    };
    
    m_openDocuments.remove(filePath);
    sendNotification("textDocument/didClose", params);
}

void LSPClient::didSave(const QString& filePath, const QString& text) {
    if (!m_connected) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{
        {"uri", QUrl::fromLocalFile(filePath).toString()}
    };
    if (!text.isEmpty()) {
        params["text"] = text;
    }
    
    sendNotification("textDocument/didSave", params);
}

void LSPClient::requestCompletion(const QString& filePath, int line, int character, const QString& triggerCharacter) {
    if (!m_connected || !m_supportsCompletion) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{
        {"uri", QUrl::fromLocalFile(filePath).toString()}
    };
    params["position"] = QJsonObject{
        {"line", line},
        {"character", character}
    };
    if (!triggerCharacter.isEmpty()) {
        params["context"] = QJsonObject{
            {"triggerKind", 2},  // TriggerCharacter
            {"triggerCharacter", triggerCharacter}
        };
    }
    
    sendRequest("textDocument/completion", params);
}

void LSPClient::requestHover(const QString& filePath, int line, int character) {
    if (!m_connected || !m_supportsHover) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{
        {"uri", QUrl::fromLocalFile(filePath).toString()}
    };
    params["position"] = QJsonObject{
        {"line", line},
        {"character", character}
    };
    
    sendRequest("textDocument/hover", params);
}

void LSPClient::gotoDefinition(const QString& filePath, int line, int character) {
    if (!m_connected || !m_supportsDefinition) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{
        {"uri", QUrl::fromLocalFile(filePath).toString()}
    };
    params["position"] = QJsonObject{
        {"line", line},
        {"character", character}
    };
    
    sendRequest("textDocument/definition", params);
}

void LSPClient::findReferences(const QString& filePath, int line, int character, bool includeDeclaration) {
    if (!m_connected || !m_supportsReferences) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{
        {"uri", QUrl::fromLocalFile(filePath).toString()}
    };
    params["position"] = QJsonObject{
        {"line", line},
        {"character", character}
    };
    params["context"] = QJsonObject{
        {"includeDeclaration", includeDeclaration}
    };
    
    sendRequest("textDocument/references", params);
}

void LSPClient::requestDocumentSymbols(const QString& filePath) {
    if (!m_connected || !m_supportsDocumentSymbols) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{
        {"uri", QUrl::fromLocalFile(filePath).toString()}
    };
    
    sendRequest("textDocument/documentSymbol", params);
}

void LSPClient::requestWorkspaceSymbols(const QString& query) {
    if (!m_connected || !m_supportsWorkspaceSymbols) return;
    
    QJsonObject params;
    params["query"] = query;
    
    sendRequest("workspace/symbol", params);
}

void LSPClient::requestCodeActions(const QString& filePath, int startLine, int startChar, int endLine, int endChar, const QVector<LSPDiagnostic>& diagnostics) {
    if (!m_connected || !m_supportsCodeActions) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{
        {"uri", QUrl::fromLocalFile(filePath).toString()}
    };
    params["range"] = QJsonObject{
        {"start", QJsonObject{{"line", startLine}, {"character", startChar}}},
        {"end", QJsonObject{{"line", endLine}, {"character", endChar}}}
    };
    
    QJsonArray diagArray;
    for (const auto& diag : diagnostics) {
        QJsonObject d;
        d["range"] = QJsonObject{
            {"start", QJsonObject{{"line", diag.range.start.line}, {"character", diag.range.start.character}}},
            {"end", QJsonObject{{"line", diag.range.end.line}, {"character", diag.range.end.character}}}
        };
        d["severity"] = diag.severity;
        d["message"] = diag.message;
        if (!diag.code.isEmpty()) d["code"] = diag.code;
        if (!diag.source.isEmpty()) d["source"] = diag.source;
        diagArray.append(d);
    }
    params["context"] = QJsonObject{{"diagnostics", diagArray}};
    
    sendRequest("textDocument/codeAction", params);
}

void LSPClient::requestFormatting(const QString& filePath) {
    if (!m_connected || !m_supportsFormatting) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{
        {"uri", QUrl::fromLocalFile(filePath).toString()}
    };
    params["options"] = QJsonObject{
        {"tabSize", 4},
        {"insertSpaces", true}
    };
    
    sendRequest("textDocument/formatting", params);
}

void LSPClient::requestRangeFormatting(const QString& filePath, int startLine, int startChar, int endLine, int endChar) {
    if (!m_connected || !m_supportsRangeFormatting) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{
        {"uri", QUrl::fromLocalFile(filePath).toString()}
    };
    params["range"] = QJsonObject{
        {"start", QJsonObject{{"line", startLine}, {"character", startChar}}},
        {"end", QJsonObject{{"line", endLine}, {"character", endChar}}}
    };
    params["options"] = QJsonObject{
        {"tabSize", 4},
        {"insertSpaces", true}
    };
    
    sendRequest("textDocument/rangeFormatting", params);
}

void LSPClient::requestOnTypeFormatting(const QString& filePath, int line, int character, const QString& ch) {
    if (!m_connected || !m_supportsOnTypeFormatting) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{
        {"uri", QUrl::fromLocalFile(filePath).toString()}
    };
    params["position"] = QJsonObject{
        {"line", line},
        {"character", character}
    };
    params["ch"] = ch;
    params["options"] = QJsonObject{
        {"tabSize", 4},
        {"insertSpaces", true}
    };
    
    sendRequest("textDocument/onTypeFormatting", params);
}

void LSPClient::requestRename(const QString& filePath, int line, int character, const QString& newName) {
    if (!m_connected || !m_supportsRename) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{
        {"uri", QUrl::fromLocalFile(filePath).toString()}
    };
    params["position"] = QJsonObject{
        {"line", line},
        {"character", character}
    };
    params["newName"] = newName;
    
    sendRequest("textDocument/rename", params);
}

void LSPClient::requestFoldingRange(const QString& filePath) {
    if (!m_connected || !m_supportsFoldingRange) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{
        {"uri", QUrl::fromLocalFile(filePath).toString()}
    };
    
    sendRequest("textDocument/foldingRange", params);
}

void LSPClient::requestSemanticTokens(const QString& filePath) {
    if (!m_connected || !m_supportsSemanticTokens) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{
        {"uri", QUrl::fromLocalFile(filePath).toString()}
    };
    
    sendRequest("textDocument/semanticTokens/full", params);
}

void LSPClient::addWorkspaceFolder(const QString& path, const QString& name) {
    QJsonObject params;
    QJsonObject folder;
    folder["uri"] = QUrl::fromLocalFile(path).toString();
    folder["name"] = name.isEmpty() ? QFileInfo(path).fileName() : name;
    
    params["event"] = "added";
    params["folder"] = folder;
    
    sendNotification("workspace/didChangeWorkspaceFolders", params);
}

void LSPClient::removeWorkspaceFolder(const QString& path) {
    QJsonObject params;
    QJsonObject folder;
    folder["uri"] = QUrl::fromLocalFile(path).toString();
    
    params["event"] = "removed";
    params["folder"] = folder;
    
    sendNotification("workspace/didChangeWorkspaceFolders", params);
}

void LSPClient::setConfig(const QJsonObject& config) {
    if (!m_connected) return;
    
    QJsonObject params;
    params["settings"] = config;
    sendNotification("workspace/didChangeConfiguration", params);
}

void LSPClient::sendRequest(const QString& method, const QJsonObject& params) {
    if (!m_connected) return;
    
    int requestId = m_nextRequestId++;
    QJsonObject request = createRequest(method, params);
    request["id"] = requestId;
    
    QJsonDocument doc(request);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\r\n";
    
    if (m_socket->state() == QTcpSocket::ConnectedState) {
        m_socket->write(data);
        m_socket->flush();
    }
}

void LSPClient::sendNotification(const QString& method, const QJsonObject& params) {
    if (!m_connected) return;
    
    QJsonObject notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = method;
    notification["params"] = params;
    
    QJsonDocument doc(notification);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\r\n";
    
    if (m_socket->state() == QTcpSocket::ConnectedState) {
        m_socket->write(data);
        m_socket->flush();
    }
}

QJsonObject LSPClient::createRequest(const QString& method, const QJsonObject& params) {
    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["method"] = method;
    if (!params.isEmpty()) request["params"] = params;
    return request;
}

void LSPClient::processMessage(const QJsonObject& msg) {
    if (msg.contains("id")) {
        // Response
        int id = msg["id"].toInt();
        if (m_pendingRequests.contains(id)) {
            m_pendingRequests.take(id)(msg);
        }
    } else if (msg.contains("method")) {
        // Notification
        QString method = msg["method"].toString();
        QJsonObject params = msg["params"].toObject();
        processNotification(msg);
    }
}

void LSPClient::processNotification(const QJsonObject& notification) {
    QString method = notification["method"].toString();
    QJsonObject params = notification["params"].toObject();
    
    if (method == "textDocument/publishDiagnostics") {
        handlePublishDiagnostics(params);
    } else if (method == "window/logMessage") {
        handleLogMessage(params);
    } else if (method == "window/showMessage") {
        handleShowMessage(params);
    } else if (method == "window/workDoneProgress/create") {
        handleWorkDoneProgress(params);
    } else if (method == "window/workDoneProgress/cancel") {
        // Handle cancel
    }
}

void LSPClient::handleInitializeResponse(const QJsonObject& result) {
    m_serverName = result["serverInfo"]["name"].toString();
    m_serverVersion = result["serverInfo"]["version"].toString();
    m_serverCapabilities = result["capabilities"].toObject();
    
    // Extract capabilities
    m_supportsCompletion = m_serverCapabilities.contains("completionProvider");
    m_supportsHover = m_serverCapabilities.contains("hoverProvider");
    m_supportsDefinition = m_serverCapabilities.contains("definitionProvider");
    m_supportsReferences = m_serverCapabilities.contains("referencesProvider");
    m_supportsDocumentSymbols = m_serverCapabilities.contains("documentSymbolProvider");
    m_supportsWorkspaceSymbols = m_serverCapabilities.contains("workspaceSymbolProvider");
    m_supportsCodeActions = m_serverCapabilities.contains("codeActionProvider");
    m_supportsFormatting = m_serverCapabilities.contains("documentFormattingProvider");
    m_supportsRangeFormatting = m_serverCapabilities.contains("documentRangeFormattingProvider");
    m_supportsOnTypeFormatting = m_serverCapabilities.contains("documentOnTypeFormattingProvider");
    m_supportsRename = m_serverCapabilities.contains("renameProvider");
    m_supportsFoldingRange = m_serverCapabilities.contains("foldingRangeProvider");
    m_supportsSemanticTokens = m_serverCapabilities.contains("semanticTokensProvider");
    m_supportsDocumentHighlight = m_serverCapabilities.contains("documentHighlightProvider");
    m_supportsSignatureHelp = m_serverCapabilities.contains("signatureHelpProvider");
    m_supportsCodeLens = m_serverCapabilities.contains("codeLensProvider");
    m_supportsDocumentLink = m_serverCapabilities.contains("documentLinkProvider");
    m_supportsColorProvider = m_serverCapabilities.contains("colorProvider");
    m_supportsInlayHint = m_serverCapabilities.contains("inlayHintProvider");
    m_supportsDiagnostics = m_serverCapabilities.contains("diagnosticsProvider");
    
    // Determine sync kind
    QJsonValue sync = m_serverCapabilities["textDocumentSync"];
    if (sync.isDouble()) {
        m_syncKind = static_cast<TextDocumentSyncKind>(sync.toInt());
    } else if (sync.isObject()) {
        m_syncKind = static_cast<TextDocumentSyncKind>(sync.toObject()["change"].toInt());
    }
    
    m_initialized = true;
    sendNotification("initialized", QJsonObject());
    emit connected();
}

void LSPClient::handleDiagnostics(const QJsonObject& params) {
    // Not used - handled via textDocument/publishDiagnostics
}

void LSPClient::handleCompletion(const QJsonObject& result) {
    QJsonValue val(result);
    QJsonArray items;
    if (val.isObject()) {
        QJsonObject obj = val.toObject();
        if (obj.contains("items"))
            items = obj["items"].toArray();
    } else if (val.isArray()) {
        items = val.toArray();
    }
    
    QStringList completions;
    for (const auto& item : items) {
        QJsonObject itemObj = item.toObject();
        LSPCompletionItem completion;
        completion.label = itemObj["label"].toString();
        completion.kind = itemObj["kind"].toInt(1);
        completion.detail = itemObj["detail"].toString();
        completion.documentation = itemObj["documentation"].toString();
        completion.insertText = itemObj["insertText"].toString();
        completion.filterText = itemObj["filterText"].toString();
        completion.sortText = itemObj["sortText"].toString();
        completion.deprecated = itemObj["deprecated"].toBool();
        completion.preselect = itemObj["preselect"].toBool();
        
        emit completionItemReceived(completion);
        completions.append(completion.label);
    }
    emit completionReceived(completions);
}

void LSPClient::handleHover(const QJsonObject& result) {
    if (result.isEmpty()) return;
    
    LSPHover hover;
    if (result["contents"].isString()) {
        hover.contents = result["contents"].toString();
    } else if (result["contents"].isObject()) {
        hover.contents = result["contents"].toObject()["value"].toString();
    } else if (result["contents"].isArray()) {
        QStringList parts;
        for (const auto& item : result["contents"].toArray()) {
            if (item.isString()) parts << item.toString();
            else if (item.isObject()) parts << item.toObject()["value"].toString();
        }
        hover.contents = parts.join("\n");
    }
    
    if (result.contains("range")) {
        QJsonObject rangeObj = result["range"].toObject();
        QJsonObject startObj = rangeObj["start"].toObject();
        QJsonObject endObj = rangeObj["end"].toObject();
        hover.range.start.line = startObj["line"].toInt();
        hover.range.start.character = startObj["character"].toInt();
        hover.range.end.line = endObj["line"].toInt();
        hover.range.end.character = endObj["character"].toInt();
    }
    
    emit hoverReceived("", 0, hover.contents);
}

void LSPClient::handleDefinition(const QJsonObject& result) {
    QVector<LSPLocation> locations;
    
    auto processLocation = [&](const QJsonObject& locObj) {
        LSPLocation loc;
        loc.uri = locObj["uri"].toString();
        QJsonObject rangeObj = locObj["range"].toObject();
        QJsonObject startObj = rangeObj["start"].toObject();
        QJsonObject endObj = rangeObj["end"].toObject();
        loc.range.start.line = startObj["line"].toInt();
        loc.range.start.character = startObj["character"].toInt();
        loc.range.end.line = endObj["line"].toInt();
        loc.range.end.character = endObj["character"].toInt();
        locations.append(loc);
    };
    
    QJsonValue val(result);
    if (val.isArray()) {
        for (const auto& item : val.toArray()) {
            processLocation(item.toObject());
        }
    } else if (val.isObject()) {
        processLocation(result);
    }
    
    emit definitionReceived(locations);
}

void LSPClient::handleReferences(const QJsonObject& result) {
    QVector<LSPLocation> locations;
    
    QJsonValue val(result);
    if (val.isArray()) {
        for (const auto& item : val.toArray()) {
            QJsonObject loc = item.toObject();
            LSPLocation l;
            l.uri = loc["uri"].toString();
            QJsonObject rangeObj = loc["range"].toObject();
            QJsonObject startObj = rangeObj["start"].toObject();
            QJsonObject endObj = rangeObj["end"].toObject();
            l.range.start.line = startObj["line"].toInt();
            l.range.start.character = startObj["character"].toInt();
            l.range.end.line = endObj["line"].toInt();
            l.range.end.character = endObj["character"].toInt();
            locations.append(l);
        }
    }
    
    emit referencesReceived(locations);
}

void LSPClient::handleDocumentSymbols(const QJsonObject& result) {
    QVector<LSPLocation> symbols;
    
    QJsonValue val(result);
    if (val.isArray()) {
        for (const auto& item : val.toArray()) {
            QJsonObject sym = item.toObject();
            LSPLocation loc;
            loc.uri = sym["uri"].toString();
            QJsonObject rangeObj = sym["range"].toObject();
            QJsonObject startObj = rangeObj["start"].toObject();
            QJsonObject endObj = rangeObj["end"].toObject();
            loc.range.start.line = startObj["line"].toInt();
            loc.range.start.character = startObj["character"].toInt();
            loc.range.end.line = endObj["line"].toInt();
            loc.range.end.character = endObj["character"].toInt();
            symbols.append(loc);
        }
    }
    
    emit documentSymbolsReceived(symbols);
}

void LSPClient::handleWorkspaceSymbols(const QJsonObject& result) {
    // Similar to handleDocumentSymbols
    emit workspaceSymbolsReceived(QVector<LSPLocation>());
}

void LSPClient::handleCodeActions(const QJsonObject& result) {
    emit codeActionsReceived(result["actions"].isArray() ? result["actions"].toArray() : QJsonArray());
}

void LSPClient::handleFormatting(const QJsonObject& result) {
    QVector<QPair<int, QString>> edits;
    
    QJsonValue val(result);
    if (val.isArray()) {
        for (const auto& item : val.toArray()) {
            QJsonObject edit = item.toObject();
            if (edit.contains("textEdit")) {
                QJsonObject textEdit = edit["textEdit"].toObject();
                QJsonObject rangeObj = textEdit["range"].toObject();
                QJsonObject startObj = rangeObj["start"].toObject();
                int offset = startObj["line"].toInt();
                edits.append(qMakePair(offset, textEdit["newText"].toString()));
            }
        }
    }
    
    emit formattingReceived(edits);
}

void LSPClient::handleRename(const QJsonObject& result) {
    emit renameReceived(result);
}

void LSPClient::handleFoldingRange(const QJsonObject& result) {
    QVector<QPair<int, int>> ranges;
    
    QJsonValue val(result);
    if (val.isArray()) {
        for (const auto& item : val.toArray()) {
            QJsonObject range = item.toObject();
            ranges.append(qMakePair(
                range["startLine"].toInt(),
                range["endLine"].toInt()
            ));
        }
    }
    
    emit foldingRangeReceived(ranges);
}

void LSPClient::handleSemanticTokens(const QJsonObject& result) {
    QVector<int> tokens;
    
    if (result.contains("data")) {
        QJsonArray data = result["data"].toArray();
        for (const auto& item : data) {
            tokens.append(item.toInt());
        }
    }
    
    emit semanticTokensReceived(tokens);
}

void LSPClient::handlePublishDiagnostics(const QJsonObject& params) {
    QString uri = params["uri"].toString();
    QString file = QUrl(uri).toLocalFile();
    
    QVector<LSPDiagnostic> diagnostics;
    QJsonArray diagArray = params["diagnostics"].toArray();
    
    for (const auto& item : diagArray) {
        QJsonObject d = item.toObject();
        LSPDiagnostic diag;
        QJsonObject rangeObj = d["range"].toObject();
        QJsonObject startObj = rangeObj["start"].toObject();
        QJsonObject endObj = rangeObj["end"].toObject();
        diag.range.start.line = startObj["line"].toInt();
        diag.range.start.character = startObj["character"].toInt();
        diag.range.end.line = endObj["line"].toInt();
        diag.range.end.character = endObj["character"].toInt();
        diag.severity = d["severity"].toInt(1);
        diag.code = d["code"].toString();
        diag.source = d["source"].toString();
        diag.message = d["message"].toString();
        diagnostics.append(diag);
    }
    
    emit diagnosticsReceived(file, diagnostics);
}

void LSPClient::handleLogMessage(const QJsonObject& params) {
    QString type;
    switch (params["type"].toInt(3)) {
        case 1: type = "ERROR"; break;
        case 2: type = "WARNING"; break;
        case 3: type = "INFO"; break;
        case 4: type = "LOG"; break;
    }
    qDebug() << "[LSP " << type << "] " << params["message"].toString();
}

void LSPClient::handleShowMessage(const QJsonObject& params) {
    QString type;
    switch (params["type"].toInt(3)) {
        case 1: type = "Error"; break;
        case 2: type = "Warning"; break;
        case 3: type = "Info"; break;
        case 4: type = "Log"; break;
    }
    qDebug() << "[LSP " << type << "] " << params["message"].toString();
}

void LSPClient::handleWorkDoneProgress(const QJsonObject& params) {
    // Handle progress notifications
}

void LSPClient::onSocketReadyRead() {
    while (m_socket->canReadLine()) {
        QByteArray line = m_socket->readLine();
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(line, &error);
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            processMessage(doc.object());
        }
    }
}

void LSPClient::onSocketConnected() {
    m_connected = true;
    qDebug() << "LSP: Connected to server";
}

void LSPClient::onSocketError(QAbstractSocket::SocketError error) {
    emit errorOccurred("LSP socket error: " + m_socket->errorString());
    m_connected = false;
}

void LSPClient::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    m_connected = false;
    m_initialized = false;
    qDebug() << "LSP process finished with exit code" << exitCode << exitStatus;
    emit disconnected();
}

void LSPClient::onProcessError(QProcess::ProcessError error) {
    emit errorOccurred("LSP process error: " + m_process->errorString());
}

} // namespace ks