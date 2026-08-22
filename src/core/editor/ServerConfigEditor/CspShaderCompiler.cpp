#include "CspShaderCompiler.h"

#include <QProcess>
#include <QTemporaryFile>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// ── CspShaderCompiler ─────────────────────────────────────────────────────

QStringList CspShaderCompiler::findAvailableCompilers()
{
    QStringList compilers;
    QStringList candidates = {"glslangValidator", "glslc", "shaderc"};
    for (const auto& name : candidates) {
        QProcess proc;
        proc.start(name, {"--version"});
        if (proc.waitForFinished(3000) && proc.exitCode() == 0)
            compilers << name;
    }
    return compilers;
}

ShaderCompileResult CspShaderCompiler::compileGLSL(const QString& source, bool isFragment,
                                                     const QString& entryPoint)
{
    ShaderCompileResult result;

    if (!validateSource(source, &result.errors))
        return result;

    result.parsedErrors = parseCompileErrors(result.errors, source);
    return compileGLSLToSPIRV(source, isFragment, entryPoint);
}

ShaderCompileResult CspShaderCompiler::compileGLSLToSPIRV(const QString& source, bool isFragment,
                                                            const QString& entryPoint)
{
    ShaderCompileResult result;

    QTemporaryFile tmpFile(QDir::temp().filePath("ks_shader_XXXXXX.glsl"));
    if (!tmpFile.open()) {
        QString msg = "Failed to create temp file";
        result.errors << msg;
        return result;
    }
    tmpFile.write(source.toUtf8());
    tmpFile.flush();
    QString tmpPath = tmpFile.fileName();
    tmpFile.close();

    QString stage = isFragment ? "frag" : "vert";
    QString outPath = tmpPath + ".spv";

    // Try glslangValidator first
    QProcess proc;
    QStringList args = {"-V", tmpPath, "-o", outPath, "--entry-point", entryPoint,
                        "-S", stage};
    proc.start("glslangValidator", args);
    if (proc.waitForFinished(30000) && proc.exitCode() == 0) {
        result.compilerUsed = "glslangValidator";
    } else {
        // Fallback to glslc
        QStringList args2 = {"-fshader-stage=" + stage, "-o", outPath, tmpPath,
                             "-entry-point=" + entryPoint};
        proc.start("glslc", args2);
        if (proc.waitForFinished(30000) && proc.exitCode() == 0) {
            result.compilerUsed = "glslc";
        } else {
            QString stderrOutput = proc.readAllStandardError();
            result.errors << stderrOutput;
            result.errors << "No working shader compiler found (tried glslangValidator, glslc)";
            result.parsedErrors = parseCompilerOutput(stderrOutput);
            QFile::remove(outPath);
            return result;
        }
    }

    QFile spvFile(outPath);
    if (!spvFile.open(QIODevice::ReadOnly)) {
        result.errors << "Failed to read compiled SPIR-V";
        return result;
    }
    result.spirv = spvFile.readAll();
    spvFile.close();
    QFile::remove(outPath);

    result.success = !result.spirv.isEmpty();
    return result;
}

// Parse structured errors from compiler stderr output (glslangValidator/glslc format)
// Format examples:
//   ERROR: <filename>:<line>:<column>: <message>
//   WARNING: <filename>:<line>:<column>: <message>
QList<ShaderCompileError> CspShaderCompiler::parseCompilerOutput(const QString& stderrOutput)
{
    QList<ShaderCompileError> results;
    QStringList lines = stderrOutput.split('\n', Qt::SkipEmptyParts);
    static QRegularExpression re(
        R"(^(ERROR|WARNING)\s*:\s*(?:\S+)?:?(\d+):(\d+)?\s*:\s*(.*)$)",
        QRegularExpression::CaseInsensitiveOption
    );

    for (const auto& line : lines) {
        auto m = re.match(line.trimmed());
        ShaderCompileError err;
        err.rawText = line;
        if (m.hasMatch()) {
            err.severity = m.captured(1).toLower();
            err.line = m.captured(2).toInt();
            err.column = m.captured(3).isEmpty() ? -1 : m.captured(3).toInt();
            err.message = m.captured(4).trimmed();
        } else {
            err.severity = "error";
            err.line = -1;
            err.column = -1;
            err.message = line.trimmed();
        }
        results.append(err);
    }
    return results;
}

// Parse our own internal validation errors with line info.
// Matches strings like "error on line 42: message" or plain messages.
QList<ShaderCompileError> CspShaderCompiler::parseCompileErrors(const QStringList& rawErrors, const QString& /*source*/)
{
    QList<ShaderCompileError> results;
    static QRegularExpression lineRe(
        R"(line\s+(\d+)\s*[,:]?\s*(.*))",
        QRegularExpression::CaseInsensitiveOption
    );

    for (const auto& raw : rawErrors) {
        // Try glslangValidator/glslc format first
        static QRegularExpression compilerRe(
            R"(^(ERROR|WARNING)\s*:\s*(?:\S+)?:?(\d+):(\d+)?\s*:\s*(.*)$)",
            QRegularExpression::CaseInsensitiveOption
        );
        auto m = compilerRe.match(raw.trimmed());
        if (m.hasMatch()) {
            ShaderCompileError err;
            err.severity = m.captured(1).toLower();
            err.line = m.captured(2).toInt();
            err.column = m.captured(3).isEmpty() ? -1 : m.captured(3).toInt();
            err.message = m.captured(4).trimmed();
            err.rawText = raw;
            results.append(err);
            continue;
        }

        // Try "line N: ..." format
        auto lm = lineRe.match(raw);
        if (lm.hasMatch()) {
            ShaderCompileError err;
            err.severity = "error";
            err.line = lm.captured(1).toInt();
            err.message = lm.captured(2).trimmed();
            err.rawText = raw;
            results.append(err);
            continue;
        }

        // Plain message, no line info
        ShaderCompileError err;
        err.severity = "error";
        err.line = -1;
        err.column = -1;
        err.message = raw;
        err.rawText = raw;
        results.append(err);
    }
    return results;
}

bool CspShaderCompiler::validateSource(const QString& source, QStringList* errors)
{
    if (source.isEmpty()) {
        if (errors) *errors << "Shader source is empty";
        return false;
    }
    if (source.length() > 500000) {
        if (errors) *errors << "Shader source exceeds 500KB limit";
        return false;
    }
    for (const QChar& c : source) {
        if (c.unicode() < 32 && c != '\n' && c != '\r' && c != '\t') {
            if (errors) *errors << "Invalid character in shader source";
            return false;
        }
    }
    return true;
}

ShaderProfile CspShaderCompiler::profileShader(const QString& source)
{
    return estimateComplexity(source);
}

ShaderProfile CspShaderCompiler::estimateComplexity(const QString& source)
{
    ShaderProfile profile;
    if (source.isEmpty()) return profile;

    QStringList lines = source.split('\n');
    QString stripped;
    for (const auto& l : lines) {
        QString s = l.trimmed();
        if (s.isEmpty() || s.startsWith("//") || s.startsWith("#"))
            continue;
        stripped += s + "\n";
    }

    // Remove block comments
    QString clean;
    int depth = 0;
    for (int i = 0; i < stripped.length(); ++i) {
        if (stripped.mid(i, 2) == "/*") { depth++; i++; continue; }
        if (stripped.mid(i, 2) == "*/") { depth--; i++; continue; }
        if (depth == 0) clean += stripped[i];
    }

    // Count texture samples
    QRegularExpression texRe(R"(\btexture\s*\()");
    profile.textureSamples = clean.count(texRe);
    profile.textureSamples += clean.count("texture2D");
    profile.textureSamples += clean.count("texture3D");
    profile.textureSamples += clean.count("textureCube");
    profile.textureSamples += clean.count("sampler2D");

    // Count uniforms
    QRegularExpression uniformRe(R"(^\s*uniform\s)", QRegularExpression::MultilineOption);
    profile.uniformCount = clean.count(uniformRe);

    // Count inputs/outputs
    QRegularExpression inRe(R"(^\s*(layout\s*\([^)]*\)\s*)?in\s+)", QRegularExpression::MultilineOption);
    QRegularExpression outRe(R"(^\s*(layout\s*\([^)]*\)\s*)?out\s+)", QRegularExpression::MultilineOption);
    profile.inputCount = clean.count(inRe);
    profile.outputCount = clean.count(outRe);

    // Count branches
    profile.branchCount = clean.count("if(") + clean.count("if ") +
                          clean.count("else") + clean.count("switch");

    // Count loops
    profile.loopCount = clean.count("for(") + clean.count("for ") +
                        clean.count("while(") + clean.count("while ");

    // Count function calls
    QRegularExpression funcRe(R"(\b(\w+)\s*\()");
    auto it = funcRe.globalMatch(clean);
    QStringList keywords = {"if", "while", "for", "switch", "return", "case", "break",
                            "continue", "discard", "texture", "texture2D", "texture3D",
                            "textureCube", "sizeof", "layout", "in", "out", "uniform"};
    while (it.hasNext()) {
        auto m = it.next();
        if (!keywords.contains(m.captured(1)))
            profile.functionCalls++;
    }

    // Check for discard
    profile.usesDiscard = clean.contains("discard");

    // Check for derivatives
    profile.usesDerivatives = clean.contains("dFdx") || clean.contains("dFdy") ||
                              clean.contains("fwidth");

    // Check for feedback
    profile.usesFeedback = clean.contains("gl_FragColor") ||
                           clean.contains("imageStore");

    // Estimate instructions (rough: count operators + function calls)
    int operators = clean.count('+') + clean.count('-') + clean.count('*') +
                    clean.count('/') + clean.count('=');
    profile.estimatedInstructions = operators + profile.functionCalls * 5 +
                                    profile.textureSamples * 10 +
                                    profile.branchCount * 3 + profile.loopCount * 8;

    // Estimated complexity (0.0-1.0)
    float score = 0.0f;
    score += qMin(1.0f, profile.textureSamples / 20.0f) * 0.25f;
    score += qMin(1.0f, profile.branchCount / 15.0f) * 0.15f;
    score += qMin(1.0f, profile.loopCount / 5.0f) * 0.15f;
    score += qMin(1.0f, profile.uniformCount / 20.0f) * 0.10f;
    score += qMin(1.0f, profile.estimatedInstructions / 500.0f) * 0.20f;
    score += profile.usesDerivatives ? 0.05f : 0.0f;
    score += profile.usesDiscard ? 0.05f : 0.0f;
    score += profile.usesFeedback ? 0.05f : 0.0f;
    profile.estimatedComplexity = qMin(1.0f, score);

    return profile;
}

bool CspShaderCompiler::isGLSLVersionSupported(const QString& source)
{
    QString version = detectGLSLVersion(source);
    if (version.isEmpty()) return true;
    int ver = version.toInt();
    return ver >= 120 && ver <= 460;
}

QString CspShaderCompiler::detectGLSLVersion(const QString& source)
{
    static QRegularExpression re(R"(#version\s+(\d+))");
    auto m = re.match(source);
    return m.hasMatch() ? m.captured(1) : QString();
}

QStringList CspShaderCompiler::getRequiredExtensions(const QString& source)
{
    QStringList extensions;
    static QRegularExpression re(R"(#extension\s+(\S+)\s*:\s*(\S+))");
    auto it = re.globalMatch(source);
    while (it.hasNext()) {
        auto m = it.next();
        extensions << m.captured(1);
    }
    return extensions;
}

QString ShaderProfile::summary() const
{
    return QString("Shader: %1\n"
                   "  Estimated instructions: %2\n"
                   "  Texture samples: %3\n"
                   "  Uniforms: %4 | Inputs: %5 | Outputs: %6\n"
                   "  Branches: %7 | Loops: %8 | Function calls: %9\n"
                   "  Uses discard: %10 | Derivatives: %11\n"
                   "  Complexity: %12%")
        .arg(name)
        .arg(estimatedInstructions)
        .arg(textureSamples)
        .arg(uniformCount).arg(inputCount).arg(outputCount)
        .arg(branchCount).arg(loopCount).arg(functionCalls)
        .arg(usesDiscard ? "yes" : "no")
        .arg(usesDerivatives ? "yes" : "no")
        .arg(estimatedComplexity * 100.0, 0, 'f', 0);
}

// ── CspShaderManager ──────────────────────────────────────────────────────

bool CspShaderManager::loadFromConfig(const QString& extConfigPath)
{
    QFile file(extConfigPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    shaderSlots.clear();
    QString currentSection;
    CspShaderSlot currentSlot;

    QTextStream ts(&file);
    while (!ts.atEnd()) {
        QString line = ts.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(';')) continue;

        if (line.startsWith('[') && line.endsWith(']')) {
            if (!currentSlot.name.isEmpty() && !currentSlot.type.isEmpty())
                shaderSlots.append(currentSlot);
            currentSlot = CspShaderSlot();
            currentSlot.name = line.mid(1, line.length() - 2);

            if (currentSlot.name.startsWith("SHADER_REPLACEMENT_"))
                currentSlot.type = "SHADER_REPLACEMENT";
            else if (currentSlot.name.startsWith("SHADER_ADJUSTMENT_"))
                currentSlot.type = "SHADER_ADJUSTMENT";
            continue;
        }

        int eq = line.indexOf('=');
        if (eq < 0) continue;
        QString key = line.left(eq).trimmed().toUpper();
        QString val = line.mid(eq + 1).trimmed();

        if (key == "SOURCE" || key == "FILE") {
            currentSlot.sourcePath = val;
            QFile sf(val.startsWith('/') ? val : QFileInfo(extConfigPath).dir().filePath(val));
            if (sf.open(QIODevice::ReadOnly | QIODevice::Text))
                currentSlot.source = QString::fromUtf8(sf.readAll());
        } else if (key == "TYPE" || key == "STAGE") {
            currentSlot.type = val;
        }
    }
    if (!currentSlot.name.isEmpty() && !currentSlot.type.isEmpty())
        shaderSlots.append(currentSlot);

    return !shaderSlots.isEmpty();
}

bool CspShaderManager::compileAll()
{
    compiledCount = 0;
    failedCount = 0;
    errors.clear();

    for (int i = 0; i < shaderSlots.size(); ++i) {
        auto result = compileSlotGLSL(shaderSlots[i].name);
        if (result.success) {
            shaderSlots[i].spirv = result.spirv;
            shaderSlots[i].needsRecompile = false;
            compiledCount++;
        } else {
            failedCount++;
            for (const auto& e : result.errors)
                errors << QString("[%1] %2").arg(shaderSlots[i].name).arg(e);
            // Propagate parsed errors with slot name prefix
            for (const auto& pe : result.parsedErrors) {
                QString slotPrefix = QString("[%1] ").arg(shaderSlots[i].name);
                errors << slotPrefix + pe.message;
                errors << slotPrefix + pe.rawText;
            }
        }
    }
    return failedCount == 0;
}

bool CspShaderManager::compileSlot(const QString& slotName)
{
    for (auto& slot : shaderSlots) {
        if (slot.name == slotName) {
            auto result = compileSlotGLSL(slotName);
            if (result.success) {
                slot.spirv = result.spirv;
                slot.needsRecompile = false;
                return true;
            }
            for (const auto& e : result.errors)
                errors << QString("[%1] %2").arg(slotName).arg(e);
            return false;
        }
    }
    errors << QString("Slot '%1' not found").arg(slotName);
    return false;
}

ShaderCompileResult CspShaderManager::compileSlotGLSL(const QString& slotName)
{
    for (const auto& slot : shaderSlots) {
        if (slot.name == slotName) {
            bool isFragment = slot.type.contains("frag", Qt::CaseInsensitive) ||
                              slot.type.contains("pixel", Qt::CaseInsensitive);
            return CspShaderCompiler::compileGLSL(slot.source, isFragment);
        }
    }
    ShaderCompileResult r;
    r.errors << "Slot not found";
    return r;
}

bool CspShaderManager::saveCompiled(const QString& outputDir)
{
    QDir dir(outputDir);
    if (!dir.exists()) dir.mkpath(".");

    for (const auto& slot : shaderSlots) {
        if (slot.spirv.isEmpty()) continue;
        QString outPath = dir.filePath(slot.name + ".spv");
        QFile f(outPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(slot.spirv);
            f.close();
        }
    }
    return true;
}
