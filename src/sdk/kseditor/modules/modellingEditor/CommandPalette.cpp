#include "CommandPalette.h"

#include <QDebug>
#include <algorithm>

namespace ks {

CommandPalette* CommandPalette::s_instance = nullptr;

CommandPalette* CommandPalette::instance()
{
    if (!s_instance)
        s_instance = new CommandPalette();
    return s_instance;
}

CommandPalette::CommandPalette(QObject* parent)
    : QObject(parent)
{}

CommandPalette::~CommandPalette()
{
    s_instance = nullptr;
}

void CommandPalette::registerCommand(const QString& id,
                                      const QString& name,
                                      const QString& category,
                                      const QString& shortcut)
{
    Command cmd;
    cmd.id       = id;
    cmd.name     = name;
    cmd.category = category;
    cmd.shortcut = shortcut;
    m_commands.insert(id, cmd);
    emit commandRegistered(id);
}

void CommandPalette::unregisterCommand(const QString& id)
{
    if (m_commands.remove(id))
        emit commandUnregistered(id);
}

void CommandPalette::setHandler(const QString& id, std::function<void()> handler)
{
    m_handlers.insert(id, std::move(handler));
}

void CommandPalette::setHandlerWithArgs(const QString& id, std::function<void(const QJsonObject&)> handler)
{
    m_handlersWithArgs.insert(id, std::move(handler));
}

bool CommandPalette::execute(const QString& id)
{
    if (!m_commands.contains(id)) {
        qWarning() << "CommandPalette: unknown command" << id;
        return false;
    }
    auto it = m_handlers.find(id);
    if (it != m_handlers.end()) {
        (*it)();
        emit commandExecuted(id);
        return true;
    }
    auto itArgs = m_handlersWithArgs.find(id);
    if (itArgs != m_handlersWithArgs.end()) {
        (*itArgs)(m_lastCommandArgs);
        emit commandExecuted(id);
        return true;
    }
    qWarning() << "CommandPalette: no handler for" << id;
    return false;
}

QVector<CommandPalette::Command> CommandPalette::search(const QString& query) const
{
    QVector<Command> results;
    const QString q = query.toLower();
    for (const auto& cmd : m_commands) {
        if (cmd.name.toLower().contains(q)     ||
            cmd.category.toLower().contains(q) ||
            cmd.shortcut.toLower().contains(q))
        {
            results << cmd;
        }
    }
    // Sort by name relevance: starts-with first, then contains
    std::sort(results.begin(), results.end(),
              [&q](const Command& a, const Command& b) {
        bool aStarts = a.name.toLower().startsWith(q);
        bool bStarts = b.name.toLower().startsWith(q);
        if (aStarts != bStarts) return aStarts;
        return a.name.toLower() < b.name.toLower();
    });
    return results;
}

QVector<CommandPalette::Command> CommandPalette::getByCategory(const QString& category) const
{
    QVector<Command> results;
    for (const auto& cmd : m_commands)
        if (cmd.category == category) results << cmd;
    return results;
}

QStringList CommandPalette::getCategories() const
{
    QStringList cats;
    for (const auto& cmd : m_commands)
        if (!cats.contains(cmd.category)) cats << cmd.category;
    cats.sort();
    return cats;
}

CommandPalette::Command CommandPalette::getCommand(const QString& id) const
{
    return m_commands.value(id);
}

bool CommandPalette::hasCommand(const QString& id) const
{
    return m_commands.contains(id);
}

void CommandPalette::setVisible(bool visible)
{
    if (m_visible == visible) return;
    m_visible = visible;
    emit visibilityChanged(visible);
}

void CommandPalette::onCommandExecuted(const QString& commandId)
{
    emit commandExecuted(commandId);
}

void CommandPalette::executeCommand(const QString& commandId)
{
    execute(commandId);
}

void CommandPalette::executeCommandWithArgs(const QString& commandId, const QJsonObject& args)
{
    m_lastCommandArgs = args;
    execute(commandId);
    emit commandExecutedWithArgs(commandId, args);
}

QVector<CommandPalette::Command> CommandPalette::searchCommands(const QString& query) const
{
    return search(query);
}

void CommandPalette::setFuzzySearch(bool enabled)
{
    m_fuzzySearch = enabled;
}

void CommandPalette::setMaxResults(int max)
{
    m_maxResults = qMax(1, max);
}

void CommandPalette::showPalette()
{
    setVisible(true);
    emit paletteShown();
}

void CommandPalette::hidePalette()
{
    setVisible(false);
    emit paletteHidden();
}

// ─── CommandPaletteModel ─────────────────────────────────────────────────

CommandPaletteModel::CommandPaletteModel(QObject* parent)
    : QObject(parent) {}

CommandPaletteModel::~CommandPaletteModel() {}

void CommandPaletteModel::setCommands(const QVector<CommandPalette::Command>& commands)
{
    m_commands = commands;
    applyFilter();
}

CommandPalette::Command CommandPaletteModel::getCommand(int row) const
{
    if (row >= 0 && row < m_filteredCommands.size())
        return m_filteredCommands[row];
    return CommandPalette::Command();
}

void CommandPaletteModel::setFilter(const QString& filter)
{
    m_filter = filter;
    applyFilter();
}

void CommandPaletteModel::setCategoryFilter(const QString& category)
{
    m_categoryFilter = category;
    applyFilter();
}

void CommandPaletteModel::sortBy(const QString& sortBy)
{
    m_sortBy = sortBy;
    applyFilter();
}

void CommandPaletteModel::applyFilter()
{
    m_filteredCommands.clear();
    for (const auto& cmd : m_commands) {
        if (!m_categoryFilter.isEmpty() && cmd.category != m_categoryFilter)
            continue;
        if (!m_filter.isEmpty() && !cmd.name.toLower().contains(m_filter.toLower()))
            continue;
        m_filteredCommands.append(cmd);
    }
    emit modelReset();
}

// ─── CommandPaletteDelegate ──────────────────────────────────────────────

CommandPaletteDelegate::CommandPaletteDelegate(QObject* parent)
    : QObject(parent) {}

CommandPaletteDelegate::~CommandPaletteDelegate() {}

void CommandPaletteDelegate::setModel(CommandPaletteModel* model)
{
    m_model = model;
}

void CommandPaletteDelegate::setRowHeight(int height)
{
    m_rowHeight = qMax(16, height);
}

void CommandPaletteDelegate::setIconSize(int size)
{
    m_iconSize = qMax(8, size);
}

void CommandPaletteDelegate::setCategoryVisibility(bool visible)
{
    m_categoryVisible = visible;
}

// ─── CommandPaletteSearch ────────────────────────────────────────────────

static CommandPaletteSearch* g_cmdSearchInstance = nullptr;

CommandPaletteSearch* CommandPaletteSearch::instance()
{
    if (!g_cmdSearchInstance)
        g_cmdSearchInstance = new CommandPaletteSearch();
    return g_cmdSearchInstance;
}

CommandPaletteSearch::CommandPaletteSearch(QObject* parent)
    : QObject(parent) {}

CommandPaletteSearch::~CommandPaletteSearch()
{
    g_cmdSearchInstance = nullptr;
}

void CommandPaletteSearch::setCommands(const QVector<CommandPalette::Command>& commands)
{
    m_commands = commands;
}

QVector<CommandPaletteSearch::SearchResult> CommandPaletteSearch::search(const QString& query) const
{
    return fuzzySearch(query);
}

QVector<CommandPaletteSearch::SearchResult> CommandPaletteSearch::fuzzySearch(const QString& query) const
{
    QVector<SearchResult> results;
    if (query.isEmpty()) return results;
    for (const auto& cmd : m_commands) {
        SearchResult r;
        r.command = cmd;
        r.score = calculateScore(cmd.name, query, r.matchedPositions);
        if (r.score >= m_threshold)
            results.append(r);
    }
    return results;
}

void CommandPaletteSearch::setSearchAlgorithm(const QString& algorithm)
{
    m_algorithm = algorithm;
}

void CommandPaletteSearch::setThreshold(float threshold)
{
    m_threshold = qBound(0.0f, threshold, 1.0f);
}

float CommandPaletteSearch::calculateScore(const QString& text, const QString& query, QVector<int>& matched) const
{
    matched.clear();
    QString lowerText = text.toLower();
    QString lowerQuery = query.toLower();

    if (lowerQuery.isEmpty()) return 0.0f;
    if (lowerText == lowerQuery) {
        for (int i = 0; i < query.size(); ++i) matched.append(i);
        return 1.0f;
    }

    // Simple substring match with character positions
    int queryIdx = 0;
    for (int i = 0; i < lowerText.size() && queryIdx < lowerQuery.size(); ++i) {
        if (lowerText[i] == lowerQuery[queryIdx]) {
            matched.append(i);
            ++queryIdx;
        }
    }

    if (queryIdx == lowerQuery.size()) {
        // Score based on match density (more compact = higher score)
        if (matched.size() >= 2) {
            int span = matched.last() - matched.first();
            return 1.0f - qMin(1.0f, (float)span / (float)lowerText.size());
        }
        return 0.5f;
    }

    matched.clear();
    return 0.0f;
}

} // namespace ks
