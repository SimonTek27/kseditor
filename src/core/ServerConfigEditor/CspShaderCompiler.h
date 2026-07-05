#pragma once

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <QRegularExpression>

// Structured shader compile error with source location
struct ShaderCompileError {
    int line = -1;          // 1-based source line
    int column = -1;        // column offset (if available)
    QString message;        // human-readable error text
    QString severity;       // "error" or "warning"
    QString rawText;        // original line from compiler output
};

struct ShaderCompileResult {
    bool success = false;
    QByteArray spirv;
    QStringList errors;           // raw error strings (kept for backwards compat)
    QStringList warnings;         // raw warning strings
    QList<ShaderCompileError> parsedErrors;  // structured errors with line numbers
    QString compilerUsed;
};

struct ShaderProfile {
    QString name;
    int estimatedInstructions = 0;
    int textureSamples = 0;
    int uniformCount = 0;
    int inputCount = 0;
    int outputCount = 0;
    int branchCount = 0;
    int loopCount = 0;
    int functionCalls = 0;
    bool usesDiscard = false;
    bool usesDerivatives = false;
    bool usesFeedback = false;
    float estimatedComplexity = 0.0f;

    QString summary() const;
};

class CspShaderCompiler {
public:
    static QStringList findAvailableCompilers();
    static ShaderCompileResult compileGLSL(const QString& source, bool isFragment,
                                            const QString& entryPoint = "main");
    static ShaderCompileResult compileGLSLToSPIRV(const QString& source, bool isFragment,
                                                    const QString& entryPoint = "main");
    static bool validateSource(const QString& source, QStringList* errors = nullptr);

    // Structured error parsing
    static QList<ShaderCompileError> parseCompileErrors(const QStringList& rawErrors, const QString& source);
    static QList<ShaderCompileError> parseCompilerOutput(const QString& stderrOutput);

    static ShaderProfile profileShader(const QString& source);
    static ShaderProfile estimateComplexity(const QString& source);

    static bool isGLSLVersionSupported(const QString& source);
    static QString detectGLSLVersion(const QString& source);
    static QStringList getRequiredExtensions(const QString& source);
};

struct CspShaderSlot {
    QString name;
    QString type;
    QString sourcePath;
    QString compiledPath;
    QString source;
    QByteArray spirv;
    bool needsRecompile = false;
};

class CspShaderManager {
public:
    bool loadFromConfig(const QString& extConfigPath);
    bool compileAll();
    bool compileSlot(const QString& slotName);
    ShaderCompileResult compileSlotGLSL(const QString& slotName);
    bool saveCompiled(const QString& outputDir);

    QList<CspShaderSlot> shaderSlots;
    QStringList errors;
    int compiledCount = 0;
    int failedCount = 0;
};
