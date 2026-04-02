---
phase: 03-category-preview-frame
plan: "01"
subsystem: video-renderer + automation-pipeline
tags: [c-renderer, config-parsing, preview-frame, python-pipeline, tests]
dependency_graph:
  requires: []
  provides: [PREVIEW-01]
  affects: [include/config.h, src/config.c, src/main.c, automate.py, tests/test_automate.py]
tech_stack:
  added: []
  patterns: [json-c preview parsing, toupper inline uppercase, mock_open for json-load tests]
key_files:
  created: []
  modified:
    - include/config.h
    - src/config.c
    - src/main.c
    - automate.py
    - tests/test_automate.py
decisions:
  - Preview frame renders exactly 1 frame before question loop, guarded by NULL check on preview_category
  - A/V sync maintained by writing 1 frame of silence alongside the preview video frame
  - post_number computed once before config.json write and reused for caption building
  - Existing tests updated to use mock_open(read_data="{}") instead of MagicMock() to handle json.load in preview injection step
metrics:
  duration: 2min
  completed_date: "2026-04-02"
  tasks_completed: 2
  files_modified: 5
---

# Phase 03 Plan 01: Category Preview Frame Summary

**One-liner:** Category preview frame rendered in C before question loop, with Python injecting category name and post counter into config.json at runtime.

## What Was Built

### C Renderer (include/config.h, src/config.c, src/main.c)

- Added `preview_category` (char*) and `preview_counter` (int) fields to `AppConfig` struct
- `config_get_default()` initializes them to `NULL` and `0` (no preview by default)
- `config_load()` parses a `"preview"` JSON object with `category` and `counter` keys
- `config_free()` frees `preview_category` if set
- `src/main.c` renders exactly 1 preview frame before the question loop when `preview_category != NULL`:
  - Background filled with `active_colors.background`
  - Category name uppercased via `toupper()`, rendered centered at y=900, font size 130
  - Counter formatted as `#N`, rendered centered at y=1020, font size 80
  - Video frame written to muxer; 1 frame of silence written for A/V sync
  - `global_frame` incremented and log line printed

### Python Pipeline (automate.py, tests/test_automate.py)

- `post_number` computation moved from step 6 to step 2b (before `compile_quizvid()`)
- config.json read, augmented with `"preview": {"category": ..., "counter": ...}`, and written back
- Duplicate `post_number` calculation removed from step 6; reuses pre-computed value
- New test `test_preview_injected_into_config` verifies config.json is written with correct preview keys and values using a `tmp_path`-backed tracking open function
- All existing tests updated: `patch("builtins.open", MagicMock())` replaced with `patch("builtins.open", _mock_open_json())` using `mock_open(read_data="{}")` to handle the new `json.load(f)` call in preview injection

## Verification

- `make clean && make` compiled without errors or warnings
- `python3 -m pytest tests/test_automate.py -x -v` — 10/10 tests passed

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Existing tests broke due to json.load on MagicMock**

- **Found during:** Task 2 verification
- **Issue:** The new preview injection step calls `json.load(f)` on the opened config.json. Existing tests used `patch("builtins.open", MagicMock())` — `json.load` internally calls `fp.read()` which returned a MagicMock (not a string), causing `TypeError: the JSON object must be str, bytes or bytearray, not MagicMock`.
- **Fix:** Added `_mock_open_json()` helper using `mock_open(read_data="{}")` and replaced all 6 `patch("builtins.open", MagicMock())` calls in existing tests. No behavior change — existing tests still verify the same behavior, the mock just now returns valid JSON from reads.
- **Files modified:** tests/test_automate.py
- **Commit:** 331f003 (included in Task 2 commit)

## Known Stubs

None — all data paths are fully wired. The preview section is injected from live runtime data (category name from SCHEDULE, counter from counters.json via get_post_number).

## Self-Check: PASSED
