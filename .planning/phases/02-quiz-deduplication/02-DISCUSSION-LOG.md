# Phase 2: Quiz Deduplication - Discussion Log

**Date:** 2026-04-01
**Areas discussed:** Duplicate definition, Tracking file design, Persistence mechanism, Regeneration behavior

---

## Area 1: What counts as a duplicate

**Q:** What should make two quizzes count as "the same"?
**Options:** Same category+date | Same question texts (content hash) | Full quiz JSON match
**Selected:** Same question texts (content hash)

**Q:** What should be hashed — just the questions array, or the full quiz JSON?
**Options:** Questions array only | Full quiz JSON
**Selected:** Questions array only

---

## Area 2: Tracking file design

**Q:** What should `posted_quizzes.json` store per entry?
**Options:** Hash + metadata | Hash only | Full quiz JSON per entry
**Selected:** Hash + metadata — `{"hash": "...", "category": "...", "date": "..."}`

---

## Area 3: Persistence mechanism

**Q:** How should `posted_quizzes.json` persist across pipeline runs?
**Options:** Committed to repo like counters.json | GitHub Actions artifact
**Selected:** Committed to repo like counters.json

---

## Area 4: Regeneration behavior

**Q:** How many retries if a duplicate is detected?
**Options:** 3 retries | 1 retry | Unlimited
**Selected:** 3 retries — with Gemini prompt augmented to include previously used question texts

**Q:** What to include in the retry prompt?
**Options:** Previously used question texts | Generic "avoid repeating" note
**Selected:** Previously used question texts (pass the actual question texts from the duplicate)

**Q:** If all 3 retries still produce duplicates — what should the pipeline do?
**Options:** Fail the run | Post anyway
**Selected:** Fail the run with a clear error message
