// Stub implementations no longer needed — real implementations of KN5Parser
// and FBXExporter are compiled directly into kseditor_lib (see CMakeLists.txt
// lines 180-182). Previously, these stubs overrode the real implementations
// at link time due to MSVC /FORCE:MULTIPLE, making KN5/FBX non-functional.
//
// If you add new plugin-only symbols that genuinely have no host-side
// implementation, add them here with a clear#ifndef guard.
