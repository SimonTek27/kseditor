#pragma once
#include <string>
namespace ks { namespace engine { namespace sys {
class UndoStack {
public:
    static UndoStack& instance() { static UndoStack s; return s; }
    void clear() {}
    bool canUndo() const { return false; }
    bool canRedo() const { return false; }
};
}}} // namespace
