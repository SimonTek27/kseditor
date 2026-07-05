#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QTreeWidget>
#include <QLabel>
#include <QComboBox>
#include <QElapsedTimer>
#include <QAtomicInt>
#include <QThread>
#include <QMutex>

struct SearchResultItem {
    QString filePath;
    int lineNumber;
    QString lineText;
    int matchStart = -1;
    int matchLength = 0;
};

class ProjectSearchWorker : public QObject {
    Q_OBJECT
public:
    ProjectSearchWorker(const QString& rootPath, const QString& searchText,
                        const QStringList& filePatterns, bool caseSensitive, bool wholeWord);
    void stop() { m_cancelled.storeRelaxed(1); }

public slots:
    void run();

signals:
    void resultFound(const SearchResultItem& item);
    void searchCompleted(int totalFiles, int totalMatches);
    void searchError(const QString& error);

private:
    void searchFile(const QString& filePath, int& totalMatches);

    QString m_rootPath;
    QString m_searchText;
    QStringList m_filePatterns;
    bool m_caseSensitive;
    bool m_wholeWord;
    QAtomicInt m_cancelled{0};
};

class ProjectSearchWidget : public QWidget {
    Q_OBJECT
public:
    explicit ProjectSearchWidget(QWidget* parent = nullptr);
    ~ProjectSearchWidget() override;

    void setSearchRoot(const QString& path);
    QString searchRoot() const { return m_searchRoot; }
    void focusSearchInput();

signals:
    void resultActivated(const QString& filePath, int lineNumber);
    void searchStarted();
    void searchFinished(int totalFiles, int totalMatches);

public slots:
    void startSearch();
    void stopSearch();
    void clearResults();

private slots:
    void onResultFound(const SearchResultItem& item);
    void onSearchCompleted(int totalFiles, int totalMatches);
    void onSearchError(const QString& error);
    void onItemActivated(QTreeWidgetItem* item, int column);
    void onContextMenu(const QPoint& pos);
    void onReplaceAll();

private:
    void setupUI();
    QTreeWidgetItem* getOrCreateFileGroup(const QString& filePath);

    QLineEdit* m_searchInput;
    QLineEdit* m_replaceInput;
    QLineEdit* m_patternInput;
    QCheckBox* m_caseSensitive;
    QCheckBox* m_wholeWord;
    QPushButton* m_searchBtn;
    QPushButton* m_stopBtn;
    QPushButton* m_replaceAllBtn;
    QPushButton* m_clearBtn;
    QLabel* m_statusLabel;
    QTreeWidget* m_resultTree;

    QString m_searchRoot;
    QThread* m_workerThread;
    ProjectSearchWorker* m_worker;
    QMap<QString, QTreeWidgetItem*> m_fileGroups;
    int m_matchCount;
};
