# Phase 3: Category Preview Frame - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-02
**Phase:** 03-category-preview-frame
**Areas discussed:** Implementation approach, Data passing, Visual design, Counter text format

---

## Implementation Approach

| Option | Description | Selected |
|--------|-------------|----------|
| C renderer | Add pre-loop block in main.c; reuses existing text/video primitives | ✓ |
| Python FFmpeg post-process | Generate 1-frame mp4 via FFmpeg, concat with quiz video | |

**User's choice:** C renderer
**Notes:** User selected the recommended option. Keeps rendering self-contained in the binary.

---

## Data Passing

| Option | Description | Selected |
|--------|-------------|----------|
| Inject into config.json | Python writes `preview` section before render_video() | ✓ |
| Separate preview.json | Python writes a standalone file; C reads if present | |
| CLI argument | Pass --category/--counter flags to binary | |

**User's choice:** Inject into config.json
**Notes:** User confirmed after clarification about the schedule structure (categories.py / SCHEDULE dict) and whether C could read counters.json directly. Conclusion: Python already has both values at render time — simpler to inject than have C re-read them.

---

## Visual Design

| Option | Description | Selected |
|--------|-------------|----------|
| Solid color from scheme | Same background as quiz questions | ✓ |
| Brand accent color | Distinct "title card" color | |
| Text only | No decorative elements beyond text | ✓ |
| Rounded rect card | Text in a rounded-rect panel | |
| Layout: stacked centered | Category name center, counter below — Claude decides specifics | ✓ |

**User's choice:** Solid scheme background, text only, stacked layout
**Notes:** User said "preview should be theme text in middle and under counter" — category name centered vertically/horizontally, counter text below it. Font sizes and spacing left to Claude's discretion.

---

## Counter Text Format

| Option | Description | Selected |
|--------|-------------|----------|
| #12 | Hash + number only | ✓ |
| Post #12 | Label + hash + number | |
| Science #12 | Both on one line | |

**User's choice:** `#12`
**Notes:** Short, Instagram-native. Category name on line above, `#12` below.

---

## Claude's Discretion

- Font sizes (suggested ~120–140px category, ~80px counter)
- Vertical spacing between text lines
- Whether `preview` becomes a struct in `AppConfig` or standalone fields
- Title case vs ALL CAPS for category name

## Deferred Ideas

None — discussion stayed within phase scope.
