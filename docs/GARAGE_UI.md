# Garage screen design (ksim)

Independent cinematic racing UI — no third-party branding.

## Layout

```
┌─ red rail ─────────────────────────────────────────────────────┐
│  KSIM · GARAGE                    [Vehicle name]               │
│  ───────────────────────────────────────────────────────────── │
│  TABS:  OVERVIEW │ TYRES │ SUSPENSION │ AERO │ BRAKES │ GEARS  │
│                                                                │
│  ┌─ list (left 42%) ──┐   ┌─ detail / bars (right) ──────────┐│
│  │ ▶ parameter name   │   │  value  [========----]  unit     ││
│  │   parameter name   │   │  short description               ││
│  │   ...              │   │  optional schematic slot         ││
│  └────────────────────┘   └──────────────────────────────────┘│
│                                                                │
│  Footer: ↑↓ select   ←→ adjust   TAB category   ESC back       │
└────────────────────────────────────────────────────────────────┘
```

## Interaction
| Input | Action |
|-------|--------|
| ↑ / ↓ | Select row |
| ← / → or − / + | Adjust value |
| Tab / Q·E | Previous / next category |
| Enter | Apply (optional commit) |
| Esc | Close garage → menu |

## Categories
| Tab | Parameters |
|-----|------------|
| Overview | Summary + fuel / ballast / TC / ABS |
| Tyres | FL/FR/RL/RR pressure (bar) |
| Suspension | Ride height F/R, spring F/R |
| Aero | Front / rear wing |
| Brakes | Bias, optional duct |
| Gears | Diff preload (extend ratios later) |

## Visual language (same as main menu)
- Dark scrim, red accent rail, large section title
- Selected row: soft plate + 4px accent bar
- Values as monospace + horizontal bar (normalized min–max)
- Branding: **ksim** only
