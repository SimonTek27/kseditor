#include "NativeRenderer.h"

// Ui types live under simulator; include when building ksimulator.
// For engine-only builds this TU can still compile if UI headers are on the path.
#if __has_include("../../simulator/ui/UiGpuPass.h")
#include "../../simulator/ui/UiGpuPass.h"
#include "../../simulator/ui/UiRenderer.h"
#define KS_HAS_UI_GPU 1
#else
#define KS_HAS_UI_GPU 0
#endif

namespace ks {
namespace sim {

void NativeRenderer::drawUi(const ui::UiRenderer& ui) {
#if KS_HAS_UI_GPU
    if (!m_ok) return;
    if (!m_uiPass) {
        m_uiPass = std::make_shared<ui::UiGpuPass>();
        m_uiPass->initialize(ui.font());
    }
    m_uiPass->uploadFrame(ui);
    m_uiPass->draw();
    if (m_uiPass->indexCount() > 0)
        ++m_uiDraws;
#else
    (void)ui;
#endif
}

} // namespace sim
} // namespace ks
