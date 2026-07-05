#include "WizardSystem.h"
#include <QDebug>

namespace ks {

// ─── WizardPage ───────────────────────────────────────────────────────────────

WizardPage::~WizardPage() = default;

bool WizardPage::isComplete() const { return m_complete; }
void WizardPage::enter()  { emit entered(); }
void WizardPage::exit()   { emit exited(); }

QJsonObject WizardPage::getData() const { return m_data; }
void WizardPage::setData(const QJsonObject& data)
{
    m_data = data;
    emit dataChanged(data);
}

void WizardPage::setComplete(bool complete)
{
    if (m_complete == complete) return;
    m_complete = complete;
    emit completeChanged(complete);
}

// ─── Wizard ───────────────────────────────────────────────────────────────────

Wizard::~Wizard() = default;

void Wizard::setTitle(const QString& t) { m_title = t; }

void Wizard::addPage(WizardPage* page)
{
    if (!page) return;
    m_pages.append(page);
    connect(page, &WizardPage::completeChanged, this, [this](){
        emit canGoNextChanged(canGoNext());
    });
}

void Wizard::removePage(const QString& pageId)
{
    m_pages.removeIf([&](WizardPage* p){ return p->getId() == pageId; });
}

WizardPage* Wizard::currentPage() const
{
    return (m_currentIndex >= 0 && m_currentIndex < m_pages.size())
           ? m_pages[m_currentIndex] : nullptr;
}

WizardPage* Wizard::getPage(const QString& id) const
{
    for (auto* p : m_pages) if (p->getId() == id) return p;
    return nullptr;
}

int Wizard::pageCount() const { return m_pages.size(); }

bool Wizard::canGoNext() const
{
    auto* p = currentPage();
    return p && p->isComplete() && m_currentIndex < m_pages.size() - 1;
}

bool Wizard::canGoBack() const { return m_currentIndex > 0; }
bool Wizard::isLastPage() const { return m_currentIndex == m_pages.size() - 1; }
bool Wizard::isFirstPage() const { return m_currentIndex == 0; }

void Wizard::goNext()
{
    if (!canGoNext()) return;
    if (auto* p = currentPage()) p->exit();
    ++m_currentIndex;
    if (auto* p = currentPage()) p->enter();
    emit pageChanged(m_currentIndex);
}

void Wizard::goBack()
{
    if (!canGoBack()) return;
    if (auto* p = currentPage()) p->exit();
    --m_currentIndex;
    if (auto* p = currentPage()) p->enter();
    emit pageChanged(m_currentIndex);
}

void Wizard::goToPage(const QString& pageId)
{
    for (int i = 0; i < m_pages.size(); ++i) {
        if (m_pages[i]->getId() == pageId) {
            if (auto* p = currentPage()) p->exit();
            m_currentIndex = i;
            if (auto* p = currentPage()) p->enter();
            emit pageChanged(m_currentIndex);
            return;
        }
    }
}

void Wizard::start()
{
    m_currentIndex = 0;
    if (auto* p = currentPage()) p->enter();
    emit started();
    emit pageChanged(0);
}

void Wizard::finish()
{
    // Collect all page data
    QJsonObject combined;
    for (auto* p : m_pages) {
        QJsonObject d = p->getData();
        for (auto it = d.begin(); it != d.end(); ++it)
            combined[it.key()] = it.value();
    }
    emit finished(combined);
}

void Wizard::cancel()
{
    emit cancelled();
}

QJsonObject Wizard::collectData() const
{
    QJsonObject result;
    for (auto* p : m_pages) {
        QJsonObject d = p->getData();
        for (auto it = d.begin(); it != d.end(); ++it)
            result[it.key()] = it.value();
    }
    return result;
}

// ─── WizardFactory ───────────────────────────────────────────────────────────

WizardFactory::WizardFactory(QObject* parent) : QObject(parent) {}
WizardFactory::~WizardFactory() = default;

void WizardFactory::registerWizard(const QString& id, Wizard* (*createFunc)()) { m_creators[id] = createFunc; emit wizardRegistered(id); }
void WizardFactory::unregisterWizard(const QString& wizardId) { m_creators.remove(wizardId); emit wizardUnregistered(wizardId); }

Wizard* WizardFactory::createWizard(const QString& wizardId) const
{
    if (m_creators.contains(wizardId)) return m_creators[wizardId]();
    return nullptr;
}

QVector<QString> WizardFactory::getAvailableWizards() const { return m_creators.keys().toVector(); }
bool WizardFactory::hasWizard(const QString& wizardId) const { return m_creators.contains(wizardId); }

// ─── WizardManager ───────────────────────────────────────────────────────────

WizardManager::WizardManager(QObject* parent) : QObject(parent) {}
WizardManager::~WizardManager() = default;

void WizardManager::registerWizard(const QString& wizardId, Wizard* wizard) { m_wizards[wizardId] = wizard; }
void WizardManager::unregisterWizard(const QString& wizardId) { m_wizards.remove(wizardId); }
Wizard* WizardManager::getWizard(const QString& wizardId) const { return m_wizards.value(wizardId, nullptr); }

bool WizardManager::showWizard(const QString& wizardId)
{
    Wizard* w = getWizard(wizardId);
    if (!w) return false;
    showWizard(w);
    return true;
}

void WizardManager::showWizard(Wizard* wizard)
{
    m_activeWizard = wizard;
    emit wizardShown(wizard);
}

void WizardManager::closeWizard()
{
    m_activeWizard = nullptr;
    emit wizardClosed();
}

// ─── ProjectCreationWizard ───────────────────────────────────────────────────

ProjectCreationWizard::ProjectCreationWizard(QObject* parent) : Wizard(parent) {}
ProjectCreationWizard::~ProjectCreationWizard() = default;

void ProjectCreationWizard::setTemplates(const QVector<QString>& templates) { m_templates = templates; }
void ProjectCreationWizard::setRecentProjects(const QVector<QString>& recent) { m_recent = recent; }

// ─── ImportWizard ────────────────────────────────────────────────────────────

ImportWizard::ImportWizard(QObject* parent) : Wizard(parent) {}
ImportWizard::~ImportWizard() = default;

// ─── ExportWizard ────────────────────────────────────────────────────────────

ExportWizard::ExportWizard(QObject* parent) : Wizard(parent) {}
ExportWizard::~ExportWizard() = default;

void ExportWizard::setInputFile(const QString& path) { m_inputFile = path; }
void ExportWizard::setAvailableFormats(const QVector<QString>& formats) { m_formats = formats; }

} // namespace ks
