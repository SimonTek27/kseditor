#pragma once
#include "../EngineModule.h"
#include <QString>
#include <QDir>

namespace ks::engine::assets {

class Packaging : public EngineModule {
public:
    static Packaging& instance(){ static Packaging s; return s; }
    QString moduleName() const override { return "Packaging"; }
    QString moduleId() const override { return "ks.packaging"; }
    bool initialize() override { m_initialized=true; return true; }
    void shutdown() override { m_initialized=false; }

    bool cook(const QString& projectDir, const QString& outDir){
        QDir d(projectDir);
        if (!d.exists()) return false;
        QDir().mkpath(outDir);
        return true;
    }
    bool hotReload(const QString& assetPath){ Q_UNUSED(assetPath); return true; }
    QString runtimePath(const QString& asset) const { return asset; }
};

} // namespace ks::engine::assets
