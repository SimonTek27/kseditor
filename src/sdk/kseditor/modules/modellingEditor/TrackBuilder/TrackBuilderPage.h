#pragma once
// ============================================================================
// TrackBuilderPage.h
// Top-level QWidget that combines:
//   - TrackViewport   (3D OpenGL view, centre)
//   - TrackBuilderWidget (tool panel, right)
//   - Toolbar strip    (top)
// This is what gets added as a page in ksEditor's ribbon/stacked widget.
// ============================================================================

#include "TrackBuilderModule.h"
#include "TrackBuilderWidget.h"
#include "TrackViewport.h"
#include <QWidget>
#include <QSplitter>
#include <QToolBar>
#include <QLabel>
#include <QActionGroup>

namespace ks { namespace track {

class TrackBuilderPage : public QWidget
{
    Q_OBJECT
public:
    explicit TrackBuilderPage(QWidget* parent = nullptr);
    ~TrackBuilderPage() override;

    TrackBuilderModule* module()   { return m_module; }
    TrackViewport*      viewport() { return m_viewport; }

private slots:
    void onToolChanged(QAction* action);
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onExport();

private:
    QToolBar*            m_toolbar  = nullptr;
    TrackBuilderWidget*  m_panel    = nullptr;
    TrackViewport*       m_viewport = nullptr;
    TrackBuilderModule*  m_module   = nullptr;

    QAction* makeToolAction(QActionGroup* grp, const QString& icon,
                             const QString& tip, const QString& mode,
                             bool checked=false);
};

}} // namespace ks::track
