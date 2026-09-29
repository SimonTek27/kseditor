#pragma once

// Offline KN5 -> NativeMesh (.nmsh) baker.
//
// This is the Qt-based half of the "bake, don't parse live" pipeline
// documented in src/simulator/NativeRenderer.h: SimulatorApp (Qt-free) never
// links KN5Parser or touches QString; instead, this editor-side tool reads a
// .kn5 through the existing, unmodified KN5Parser and writes out a simple
// binary format (NativeRenderer::loadMeshFromFile()'s "NMSH" format) that the
// native runtime can load with nothing but <fstream>.
//
// Deliberately does not modify KN5Parser.h/.cpp/KN5Types.h — those are
// shared with the live editor content-loading path, and changing parsing
// behavior there without a way to compile-test risks breaking real AC
// content loading in the editor. This file only reads what KN5Parser already
// exposes.
//
// KNOWN LIMITATION (inherited from KN5Parser, not introduced here):
// KN5Parser.cpp reads mesh.nodeIndex but never parses or exposes a KN5
// node/scene-graph — there is no per-node parent transform available
// anywhere in KN5File. Only the single file-level kn5.worldMatrix is applied
// here, uniformly, to every mesh. Meshes authored directly in that space
// bake correctly; meshes that depend on additional node-local offsets from
// the (unparsed) KN5 node tree will not be positioned correctly until
// KN5Parser itself is extended to parse that tree.
//
// Skinned meshes are skipped (NativeRenderer has no bone/skinning support
// yet); only static geometry is baked.

#include <QString>
#include <string>

namespace ks::tools {

struct BakeResult {
    bool success = false;
    int meshesWritten = 0;
    int meshesSkipped = 0; // skinned, or empty vertex data
    QString error;
};

// Writes one <outputDir>/<sanitized mesh name>.nmsh per static mesh in the
// .kn5, plus <outputDir>/manifest.txt (one mesh name per line, matching the
// names NativeRenderer::loadMeshFromFile() should be called with).
BakeResult bakeKN5ToNativeMeshes(const QString& kn5Path, const std::string& outputDir);

} // namespace ks::tools
