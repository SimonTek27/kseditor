# Mouse input (native UI)

## UiInput
| API | Role |
|-----|------|
| `inject(MouseEvent)` | generic event |
| `setPosition` / `setButton` / `addWheel` | platform helpers |
| `isHovering(Rect)` | hit-test |
| `state().leftReleased()` | click detection |
| `registerTarget` + `onClick` | optional callbacks |

## Platform hook
```cpp
// Win32 WM_MOUSEMOVE / WM_LBUTTONDOWN / UP / WM_MOUSEWHEEL
loop.ui().handleMouseMove((float)x, (float)y);
loop.ui().handleMouseButton(MouseButton::Left, true, x, y);
loop.ui().handleMouseWheel(delta / 120.f, x, y);

// Or:
MouseEvent e;
e.type = MouseEventType::Down;
e.button = MouseButton::Left;
e.x = x; e.y = y;
loop.ui().handleMouse(e);
```

## Frame order
```
beginFrame (clear click flags, clear targets)
  build overlays (hover + click while drawing)
endFrame (hover enter/leave, click callbacks)
```

## Modal panels
Device / Multiplayer / Menu return `true` from `handleMouse` → consume event (no drive input).
