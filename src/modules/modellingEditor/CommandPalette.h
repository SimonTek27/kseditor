#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>
#include <QSortFilterProxyModel>
#include <functional>

namespace ks {

class CommandPalette : public QObject
{
    Q_OBJECT

public:
    static CommandPalette* instance();

    struct Command {
        QString id;
        QString name;
        QString category;
        QString shortcut;
        QString icon;
        QJsonObject data;
    };

    void registerCommand(const QString& id, const QString& name,
                          const QString& category = QString(),
                          const QString& shortcut = QString());

    void unregisterCommand(const QString& commandId);

    QVector<Command> getCommands() const { return m_commands.values(); }
    QVector<Command> searchCommands(const QString& query) const;

    void executeCommand(const QString& commandId);
    void executeCommandWithArgs(const QString& commandId, const QJsonObject& args);

    void setHandler(const QString& id, std::function<void()> handler);
    bool execute(const QString& id);
    QVector<Command> search(const QString& query) const;
    QVector<Command> getByCategory(const QString& category) const;
    QStringList getCategories() const;
    Command getCommand(const QString& id) const;
    bool hasCommand(const QString& id) const;

    void setFuzzySearch(bool enabled);
    bool isFuzzySearchEnabled() const { return m_fuzzySearch; }

    void setMaxResults(int max);
    int getMaxResults() const { return m_maxResults; }

    void showPalette();
    void hidePalette();
    void setVisible(bool visible);

    bool isVisible() const { return m_visible; }

signals:
    void paletteShown();
    void paletteHidden();
    void commandExecuted(const QString& commandId);
    void commandExecutedWithArgs(const QString& commandId, const QJsonObject& args);
    void commandRegistered(const QString& id);
    void commandUnregistered(const QString& id);
    void visibilityChanged(bool visible);

private slots:
    void onCommandExecuted(const QString& commandId);

private:
    CommandPalette(QObject* parent = nullptr);
    ~CommandPalette();
    Q_DISABLE_COPY(CommandPalette)

    static CommandPalette* s_instance;

    bool m_visible = false;
    bool m_fuzzySearch = true;
    int m_maxResults = 10;

    QMap<QString, Command> m_commands;
    QMap<QString, std::function<void()>> m_handlers;
    QStringList m_commandHistory;
    QJsonObject m_lastCommandArgs;
};

class CommandPaletteModel : public QObject
{
    Q_OBJECT

public:
    explicit CommandPaletteModel(QObject* parent = nullptr);
    ~CommandPaletteModel();

    void setCommands(const QVector<CommandPalette::Command>& commands);

    int rowCount() const { return m_filteredCommands.size(); }
    CommandPalette::Command getCommand(int row) const;

    void setFilter(const QString& filter);
    QString getFilter() const { return m_filter; }

    void setCategoryFilter(const QString& category);
    QString getCategoryFilter() const { return m_categoryFilter; }

    void sortBy(const QString& sortBy);
    QString getSortBy() const { return m_sortBy; }

signals:
    void modelReset();

private:
    void applyFilter();

    QVector<CommandPalette::Command> m_commands;
    QVector<CommandPalette::Command> m_filteredCommands;
    QString m_filter;
    QString m_categoryFilter;
    QString m_sortBy = "name";
};

class CommandPaletteDelegate : public QObject
{
    Q_OBJECT

public:
    explicit CommandPaletteDelegate(QObject* parent = nullptr);
    ~CommandPaletteDelegate();

    void setModel(CommandPaletteModel* model);
    CommandPaletteModel* getModel() const { return m_model; }

    void setRowHeight(int height);
    int getRowHeight() const { return m_rowHeight; }

    void setIconSize(int size);
    int getIconSize() const { return m_iconSize; }

    void setCategoryVisibility(bool visible);
    bool isCategoryVisible() const { return m_categoryVisible; }

signals:
    void itemSelected(int index);
    void itemActivated(int index);

private:
    CommandPaletteModel* m_model = nullptr;
    int m_rowHeight = 36;
    int m_iconSize = 24;
    bool m_categoryVisible = true;
};

class CommandPaletteSearch : public QObject
{
    Q_OBJECT

public:
    static CommandPaletteSearch* instance();

    struct SearchResult {
        CommandPalette::Command command;
        float score;
        QVector<int> matchedPositions;
    };

    void setCommands(const QVector<CommandPalette::Command>& commands);

    QVector<SearchResult> search(const QString& query) const;
    QVector<SearchResult> fuzzySearch(const QString& query) const;

    void setSearchAlgorithm(const QString& algorithm);
    QString getSearchAlgorithm() const { return m_algorithm; }

    void setThreshold(float threshold);
    float getThreshold() const { return m_threshold; }

signals:
    void searchCompleted(const QVector<SearchResult>& results);

private:
    CommandPaletteSearch(QObject* parent = nullptr);
    ~CommandPaletteSearch();
    Q_DISABLE_COPY(CommandPaletteSearch)

    static CommandPaletteSearch* s_instance;

    QVector<CommandPalette::Command> m_commands;
    QString m_algorithm = "fuzzy";
    float m_threshold = 0.3f;

    float calculateScore(const QString& text, const QString& query, QVector<int>& matched) const;
};

} // namespace ks