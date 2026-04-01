# Phase 2: Quiz Deduplication - Context

**Gathered:** 2026-04-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Add duplicate detection to the daily quiz pipeline so the same quiz content is never posted twice. This covers:
- Recording each successfully posted quiz to a persistent tracking file (`posted_quizzes.json`)
- Detecting duplicates before posting and regenerating with AI guidance if one is found

Out of scope: per-question deduplication, quiz history UI, browsing past quizzes.

</domain>

<decisions>
## Implementation Decisions

### Duplicate Definition
- **D-01:** Two quizzes are considered duplicates if the SHA-256 hash of their **questions array** matches. The full quiz JSON is NOT hashed — only the list of question objects (texts + answers). Metadata fields (category, date, etc.) are excluded from the hash.

### Tracking File Design
- **D-02:** `posted_quizzes.json` stores one entry per successfully posted quiz with the structure:
  ```json
  {"hash": "<sha256>", "category": "Geography", "date": "2026-04-01"}
  ```
  Human-readable for debugging; hash enables the dedup check.

### Persistence Mechanism
- **D-03:** `posted_quizzes.json` is committed to the repo after each successful post, alongside `counters.json`. Uses the existing "Commit updated files" workflow step — no new infrastructure.

### Regeneration Behavior
- **D-04:** On duplicate detection, retry quiz generation up to **3 times**.
- **D-05:** Each retry prompt includes the **question texts** from the duplicate quiz so Gemini knows specifically what to avoid repeating.
- **D-06:** If all 3 retries still produce duplicates, **fail the run** with a clear error message. Do not post a duplicate.

### Claude's Discretion
- Hash algorithm: SHA-256 (standard, no dependency needed — Python `hashlib`)
- Where in `automate.py` to insert the check: after step 1 (generate quiz), before step 2 (write quiz file)
- How to serialize the questions array for hashing: `json.dumps(questions, sort_keys=True, ensure_ascii=False)`
- Module structure: new `dedup.py` module (like `counters.py`) or inline in `automate.py`
- How to pass prior question texts to Gemini retry: as an additional context string in the `generate_quiz()` call

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Core Pipeline
- `automate.py` — Pipeline entry point; dedup check inserts after quiz generation (step 1), recording inserts after successful post (step 9)
- `counters.py` — Existing persistence pattern to follow: load JSON → mutate → save → commit
- `counters.json` — Example of committed tracking file structure

### Quiz Generation
- `gemini_client.py` — `generate_quiz(category, model=)` signature; understand how to pass additional context for retry prompts

### Tests
- `tests/test_automate.py` — Existing automate test patterns; new dedup tests should follow the same mocking style

</canonical_refs>

<specifics>
## Specific Ideas

- Questions array hashing: use `json.dumps(quiz_data["questions"], sort_keys=True, ensure_ascii=False).encode()` → `hashlib.sha256(...).hexdigest()`
- Retry prompt addition: pass a `avoid_questions` parameter to `generate_quiz()` containing the list of question texts from the duplicate, e.g. `["What is the capital of France?", ...]`
- `posted_quizzes.json` initial state: `[]` (empty array) — created on first successful post if it doesn't exist

</specifics>

<deferred>
## Deferred Ideas

- None identified during discussion.

</deferred>

---

*Phase: 02-quiz-deduplication*
*Context gathered: 2026-04-01 via discuss-phase*
