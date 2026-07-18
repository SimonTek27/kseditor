#include "FontCreatorQmlBridge.h"
#include "core/sys/LogManager.h"
#include <QWidget>
#include <QJsonObject>

namespace ks {

FontCreatorEditorModule::FontCreatorEditorModule(QWidget* parent)
    : EditorModule(parent)
{
}

bool FontCreatorEditorModule::initialize()
{
    LOG_INFO("FontCreatorEditorModule", "Initialized");
    return true;
}

void FontCreatorEditorModule::shutdown()
{
}

void FontCreatorEditorModule::importFile(const QString& filePath)
{
    Q_UNUSED(filePath);
}

void FontCreatorEditorModule::exportFile(const QString& filePath)
{
    Q_UNUSED(filePath);
}

QJsonObject FontCreatorEditorModule::serializeProject() const
{
    return QJsonObject();
}

void FontCreatorEditorModule::deserializeProject(const QJsonObject& data)
{
    Q_UNUSED(data);
}

} // namespace ks