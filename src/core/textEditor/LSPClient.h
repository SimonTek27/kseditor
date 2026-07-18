#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QMap>
#include <QVector>
#include <QTcpSocket>
#include <QProcess>
#include <QTimer>
#include <QMutex>
#include <QVariant>
#include <functional>

namespace ks {

struct LSPDiagnostic {
    struct Range {
        struct Position {
            int line = 0;
            int character = 0;
        };
        Position start;
        Position end;
    };
    
    Range range;
    int severity = 1;  // 1=Error, 2=Warning, 3=Info, 4=Hint
    QString code;
    QString source;
    QString message;
    QVector<int> relatedInformation;
};

struct LSPCompletionItem {
    QString label;
    int kind = 1;  // 1=Text, 2=Method, 3=Function, 4=Constructor, 5=Field, 6=Variable, 6=Class, 7=Interface, 8=Module, 9=Property, 10=Unit, 11=Value, 12=Enum, 13=Keyword, 14=Snippet, 15=Color, 16=File, 17=Reference
    QString detail;
    QString documentation;
    QString insertText;
    QString filterText;
    QString sortText;
    bool deprecated = false;
    bool preselect = false;
};

struct LSPHover {
    QString contents;
    struct Range {
        struct Position {
            int line = 0;
            int character = 0;
        };
        Position start;
        Position end;
    } range;
};

struct LSPLocation {
    QString uri;
    struct Range {
        struct Position {
            int line = 0;
            int character = 0;
        };
        Position start;
        Position end;
    } range;
};

class LSPClient : public QObject {
    Q_OBJECT

public:
    explicit LSPClient(QObject* parent = nullptr);
    ~LSPClient() override;

    // Connection
    bool start(const QString& command, const QStringList& args, const QString& workingDir = QString());
    bool connectToServer(const QString& host, int port);
    void stop();
    bool isConnected() const { return m_connected; }

    // Initialization
    bool initialize(const QString& rootPath, const QStringList& capabilities = {});
    void shutdown();
    void exit();

    // Document synchronization
    void didOpen(const QString& filePath, const QString& languageId, int version, const QString& text);
    void didChange(const QString& filePath, int version, const QVector<QPair<int, QString>>& changes);  // (position, text)
    void didClose(const QString& filePath);
    void didSave(const QString& filePath, const QString& text = QString());

    // Completion
    void requestCompletion(const QString& filePath, int line, int character, const QString& triggerCharacter = QString());
    
    // Hover
    void requestHover(const QString& filePath, int line, int character);
    
    // Go to definition
    void gotoDefinition(const QString& filePath, int line, int character);
    
    // References
    void findReferences(const QString& filePath, int line, int character, bool includeDeclaration = true);
    
    // Document symbols
    void requestDocumentSymbols(const QString& filePath);
    
    // Workspace symbols
    void requestWorkspaceSymbols(const QString& query);
    
    // Code actions
    void requestCodeActions(const QString& filePath, int startLine, int startChar, int endLine, int endChar, const QVector<LSPDiagnostic>& diagnostics);
    
    // Formatting
    void requestFormatting(const QString& filePath);
    void requestRangeFormatting(const QString& filePath, int startLine, int startChar, int endLine, int endChar);
    void requestOnTypeFormatting(const QString& filePath, int line, int character, const QString& ch);
    
    // Rename
    void requestRename(const QString& filePath, int line, int character, const QString& newName);
    
    // Folding
    void requestFoldingRange(const QString& filePath);
    
    // Semantic tokens
    void requestSemanticTokens(const QString& filePath);
    
    // Workspace
    void addWorkspaceFolder(const QString& path, const QString& name = QString());
    void removeWorkspaceFolder(const QString& path);
    
    // Configuration
    void setConfig(const QJsonObject& config);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);
    
    // Diagnostics
    void diagnosticsReceived(const QString& file, const QVector<LSPDiagnostic>& diagnostics);
    
    // Completion
    void completionReceived(const QStringList& completions);
    void completionItemReceived(const LSPCompletionItem& item);
    
    // Hover
    void hoverReceived(const QString& file, int line, const QString& content);
    
    // Go to definition
    void definitionReceived(const QVector<LSPLocation>& locations);
    
    // References
    void referencesReceived(const QVector<LSPLocation>& locations);
    
    // Document symbols
    void documentSymbolsReceived(const QVector<LSPLocation>& symbols);
    
    // Workspace symbols
    void workspaceSymbolsReceived(const QVector<LSPLocation>& symbols);
    
    // Code actions
    void codeActionsReceived(const QJsonArray& actions);
    
    // Formatting
    void formattingReceived(const QVector<QPair<int, QString>>& edits);  // (position, newText)
    
    // Rename
    void renameReceived(const QJsonObject& workspaceEdit);
    
    // Folding
    void foldingRangeReceived(const QVector<QPair<int, int>>& ranges);  // (startLine, endLine)
    
    // Semantic tokens
    void semanticTokensReceived(const QVector<int>& tokens);  // line, char, length, tokenType, tokenModifiers

private slots:
    void onSocketReadyRead();
    void onSocketConnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void processNotification(const QJsonObject& notification);

private:
    void sendRequest(const QString& method, const QJsonObject& params);
    void sendNotification(const QString& method, const QJsonObject& params);
    void processMessage(const QJsonObject& msg);
    QJsonObject createRequest(const QString& method, const QJsonObject& params);
    void handleInitializeResponse(const QJsonObject& result);
    void handleDiagnostics(const QJsonObject& params);
    void handleCompletion(const QJsonObject& result);
    void handleHover(const QJsonObject& result);
    void handleDefinition(const QJsonObject& result);
    void handleReferences(const QJsonObject& result);
    void handleDocumentSymbols(const QJsonObject& result);
    void handleWorkspaceSymbols(const QJsonObject& result);
    void handleCodeActions(const QJsonObject& result);
    void handleFormatting(const QJsonObject& result);
    void handleRename(const QJsonObject& result);
    void handleFoldingRange(const QJsonObject& result);
    void handleSemanticTokens(const QJsonObject& result);
    void handlePublishDiagnostics(const QJsonObject& params);
    void handleLogMessage(const QJsonObject& params);
    void handleShowMessage(const QJsonObject& params);
    void handleWorkDoneProgress(const QJsonObject& params);

    QTcpSocket* m_socket = nullptr;
    QProcess* m_process = nullptr;
    bool m_connected = false;
    bool m_initialized = false;
    int m_nextRequestId = 1;
    QString m_rootPath;
    QMap<int, std::function<void(const QJsonObject&)>> m_pendingRequests;
    QMap<QString, int> m_openDocuments;  // filePath -> version
    QMutex m_mutex;
    
    // Server capabilities
    QJsonObject m_serverCapabilities;
    QString m_serverName;
    QString m_serverVersion;
    
    // Document sync
    enum class TextDocumentSyncKind {
        None = 0,
        Full = 1,
        Incremental = 2
    };
    TextDocumentSyncKind m_syncKind = TextDocumentSyncKind::Full;
    
    // Capabilities
    bool m_supportsCompletion = false;
    bool m_supportsHover = false;
    bool m_supportsDefinition = false;
    bool m_supportsReferences = false;
    bool m_supportsDocumentSymbols = false;
    bool m_supportsWorkspaceSymbols = false;
    bool m_supportsCodeActions = false;
    bool m_supportsFormatting = false;
    bool m_supportsRangeFormatting = false;
    bool m_supportsOnTypeFormatting = false;
    bool m_supportsRename = false;
    bool m_supportsFoldingRange = false;
    bool m_supportsSemanticTokens = false;
    bool m_supportsDocumentHighlight = false;
    bool m_supportsSignatureHelp = false;
    bool m_supportsCodeLens = false;
    bool m_supportsDocumentLink = false;
    bool m_supportsColorProvider = false;
    bool m_supportsInlayHint = false;
    bool m_supportsDiagnostics = true;
};

} // namespace ks