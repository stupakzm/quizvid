---
phase: 02-quiz-deduplication
plan: 01
subsystem: pipeline
tags: [dedup, hashing, sha256, json-tracking, gemini, retry-logic]

# Dependency graph
requires:
  - phase: 01-ci-compatibility
    provides: Stable GitHub Actions workflow for daily automation
provides:
  - SHA-256 quiz hashing and duplicate detection via dedup.py
  - Retry-with-avoidance logic for duplicate quizzes in automate.py
  - posted_quizzes.json tracking committed to repo after each run
affects:
  - 03-opening-frame

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Load/mutate/save JSON persistence pattern (like counters.py) for posted_quizzes.json"
    - "SHA-256 hashing of normalized questions array for content deduplication"
    - "Retry loop with avoid_questions context passed to Gemini on duplicate detection"

key-files:
  created:
    - dedup.py
    - tests/test_dedup.py
  modified:
    - gemini_client.py
    - automate.py
    - tests/test_automate.py
    - .github/workflows/daily.yml

key-decisions:
  - "Hash only the questions array (not full quiz JSON) for dedup — config/metadata excluded per D-01"
  - "SHA-256 via Python hashlib — no new dependencies, deterministic with sort_keys=True"
  - "Retry up to 3 times on duplicate; pass avoid_questions texts to Gemini per D-04 D-05"
  - "Fail pipeline (sys.exit(1)) if all retries produce duplicates per D-06"
  - "Record quiz only after successful Instagram post (after counter increment) per D-03"

patterns-established:
  - "Dedup module follows counters.py pattern: POSTED_QUIZZES_FILE constant, load/save JSON helpers"
  - "Automate tests mock is_duplicate, record_quiz, shutil.copy2 for full isolation"
  - "TDD: RED commit (failing tests) then GREEN commit (implementation)"

requirements-completed: [DEDUP-01, DEDUP-02]

# Metrics
duration: 35min
completed: 2026-04-01
---

# Phase 2 Plan 1: Quiz Deduplication Summary

**SHA-256 hash-based quiz dedup module with 3-retry loop, avoid_questions Gemini context, and posted_quizzes.json tracking committed after each successful post**

## Performance

- **Duration:** ~35 min
- **Started:** 2026-04-01T20:45:00Z
- **Completed:** 2026-04-01T21:17:00Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments
- New `dedup.py` module with 6 functions: compute_quiz_hash, is_duplicate, record_quiz, get_question_texts, load_posted_quizzes, save_posted_quizzes
- 12 unit tests in `test_dedup.py` covering all behaviors with tmp_path isolation
- Pipeline updated in `automate.py` with dedup check + 3-retry loop using avoid_questions
- `gemini_client.py` now accepts optional avoid_questions parameter for guided regeneration
- `.github/workflows/daily.yml` commits `posted_quizzes.json` alongside `counters.json`
- All 51 tests pass with zero failures

## Task Commits

Each task was committed atomically:

1. **Task 1: Create dedup module with tests (RED)** - `2eb0691` (test)
2. **Task 1: Create dedup module with tests (GREEN)** - `b0f15fc` (feat)
3. **Task 2: Integrate dedup into pipeline and update tests** - `a416131` (feat)

_Note: Task 1 used TDD — separate RED (failing tests) and GREEN (implementation) commits._

## Files Created/Modified
- `dedup.py` - Quiz hashing, duplicate detection, recording, question text extraction
- `tests/test_dedup.py` - 12 unit tests covering all dedup module behaviors
- `gemini_client.py` - Added optional avoid_questions parameter to generate_quiz
- `automate.py` - Dedup check + retry loop after generation; record_quiz after successful post
- `tests/test_automate.py` - Added 4 dedup-specific tests; updated existing tests to mock dedup functions
- `.github/workflows/daily.yml` - Added posted_quizzes.json to git add step

## Decisions Made
- Hash only the questions array (not full quiz JSON) — per D-01, metadata excluded so same questions = same hash regardless of config changes
- Use json.dumps with sort_keys=True, ensure_ascii=False for deterministic serialization
- MAX_DEDUP_RETRIES = 3 — fail hard if all retries still duplicate (per D-06)
- avoid_questions passed via get_question_texts so Gemini knows exactly what to avoid (per D-05)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed pre-existing test failures in test_automate.py**
- **Found during:** Task 2 (test verification)
- **Issue:** Existing tests test_full_run_increments_counter, test_gemini_retries_on_failure, test_counter_not_incremented_if_post_fails were calling shutil.copy2 on a mock video path which hung/failed. Additionally, after dedup imports were added to automate.py, these tests needed is_duplicate and record_quiz mocks for isolation.
- **Fix:** Added `patch("automate.is_duplicate", return_value=False)`, `patch("automate.record_quiz")`, and `patch("automate.shutil.copy2")` to each full-run test
- **Files modified:** tests/test_automate.py
- **Verification:** All 51 tests pass (0 failures)
- **Committed in:** a416131 (Task 2 commit)

---

**Total deviations:** 1 auto-fixed (Rule 1 bug — pre-existing test isolation issue)
**Impact on plan:** Required to achieve "no regressions" success criteria. Added 3 mock patches to 3 existing tests.

## Issues Encountered
- Discovered tests test_full_run_increments_counter, test_gemini_retries_on_failure, and test_counter_not_incremented_if_post_fails were already failing before this plan (shutil.copy2 called on mock paths). Fixed by adding proper mocks.

## User Setup Required
None - no external service configuration required. The posted_quizzes.json file will be created automatically on first successful post.

## Next Phase Readiness
- Dedup infrastructure is complete; pipeline will never post duplicate quizzes
- Phase 3 (opening frame) can proceed — no dedup blockers
- posted_quizzes.json accumulates automatically in the repo after each run

---
*Phase: 02-quiz-deduplication*
*Completed: 2026-04-01*
