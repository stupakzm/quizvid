# Phase 3: Category Preview Frame - Context

**Gathered:** 2026-04-02
**Status:** Ready for planning

<domain>
## Phase Boundary

Add a 1-frame opening scene to every rendered video that displays the current category name and per-category post counter. The frame is inserted before the question loop in the C renderer. The rest of the quiz video plays unchanged after this frame.

Out of scope: preview frame duration > 1 frame, multiple preview styles, any UI or manual override.

</domain>

<decisions>
## Implementation Decisions

### Implementation Approach
- **D-01:** Preview frame is rendered inside the C binary (`src/main.c`) — a pre-loop block renders exactly 1 frame before the question loop begins. Reuses existing `text_render_centered_alpha` and `video_draw_rounded_rect_alpha` primitives. No extra subprocess or Python dependency.

### Data Passing
- **D-02:** Python injects a `preview` section into `config.json` before calling `render_video()`. Structure:
  ```json
  "preview": {
    "category": "Science",
    "counter": 12
  }
  ```
  C reads `config->preview.category` and `config->preview.counter`. This uses the existing config loading path — no new files.

### Visual Design
- **D-03:** Background: solid color from the active color scheme (same background as quiz questions — no distinct intro color).
- **D-04:** Layout: category name text centered horizontally and vertically in the frame; counter text centered horizontally, directly below the category name. Claude's discretion on spacing and font sizing (category name should be prominent — approx 2× question font size; counter smaller below it).
- **D-05:** No decorative elements beyond text — clean, minimal.

### Counter Text Format
- **D-06:** Counter displays as `#12` (just the number with a hash, e.g. `#1`, `#12`). Category name on the line above, counter on the line below.

  Example frame (1080×1920):
  ```
  (vertical center)
      SCIENCE
        #12
  ```

### Claude's Discretion
- Exact font sizes (suggested: ~120–140px for category name, ~80px for counter)
- Vertical spacing between the two text lines
- Whether to add a `preview` struct to `AppConfig` or handle it as standalone fields in config parsing
- Where in `config.c` to parse the new `preview` fields

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### C Renderer
- `src/main.c` — Pipeline entry point; preview frame block inserts before the question loop (`for (int q = 0; q < quiz.num_questions; q++)`)
- `include/text.h` — `text_render_centered_alpha`, `text_render_wrapped_centered` — use these for rendering category and counter text
- `include/video.h` — `video_fill_rgb_color`, `video_draw_rect` — use for background fill
- `include/config.h` — `AppConfig` struct; new `preview` fields (category string + counter int) extend this
- `src/config.c` — JSON parsing logic to extend for `preview` section

### Python Pipeline
- `automate.py` — `post_number` and `category["name"]` are both available before `render_video()` is called (lines ~118–120); inject `preview` into config.json here
- `video_renderer.py` — `render_video(config_file)` invocation; no changes needed unless config path changes
- `config.json` — Template config file; `preview` section is written at runtime, not stored permanently

### Tests
- `tests/test_automate.py` — Existing pipeline test patterns; new preview injection step should be covered

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `text_render_centered_alpha` (`include/text.h`): renders horizontally centered text with alpha — directly usable for both category name and counter lines
- `video_fill_rgb_color` (`include/video.h`): fills the RGB buffer with a solid color — use for background
- `video_draw_rounded_rect_alpha` (`include/video.h`): available if a card/panel background is ever wanted (not needed for this phase — text only)
- `config.c` / `AppConfig` (`include/config.h`): existing JSON parsing pattern to extend for `preview` fields

### Established Patterns
- Config-driven rendering: all layout/visual parameters come from `config.json` via `AppConfig`
- Color scheme: active colors set by `config_apply()` at startup — preview frame should use the same `g_colors` globals as the rest of the render
- Frame render loop: each question renders N frames via `quiz_render_frame()` + `muxer_write_video_frame()` — preview frame follows the same write pattern

### Integration Points
- `main.c` pre-question-loop: insert preview frame render + `muxer_write_video_frame(muxer, rgb_buffer)` before `for (int q = 0; q < quiz.num_questions; q++)`
- `automate.py` step 3.5 (between "Write quiz file" and "Compile quizvid"): inject `preview` into config.json
- `AppConfig` struct: add `char *preview_category` and `int preview_counter` fields

</code_context>

<specifics>
## Specific Ideas

- Counter rendered as `#12` — just hash + number, no label
- Category name in ALL CAPS or title case (Claude's discretion — consistent with how quiz questions render text)
- Both lines vertically centered as a group (not independently) so they feel like one unit

</specifics>

<deferred>
## Deferred Ideas

- None — discussion stayed within phase scope.

</deferred>

---

*Phase: 03-category-preview-frame*
*Context gathered: 2026-04-02*
