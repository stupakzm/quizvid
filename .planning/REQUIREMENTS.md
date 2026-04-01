# Requirements: QuizVid

**Defined:** 2026-04-01
**Core Value:** Every day, a new quiz Reel is published to Instagram with zero manual intervention.

## v1.0 Requirements

### CI

- [x] **CI-01**: Pipeline uses Node.js 24-compatible GitHub Actions versions (no deprecation warnings)

### Deduplication

- [ ] **DEDUP-01**: System records each successfully posted quiz to a persistent tracking file
- [ ] **DEDUP-02**: Pipeline detects a previously posted quiz and regenerates a new one before posting

### Preview Frame

- [ ] **PREVIEW-01**: Video opens with a 1-frame scene displaying the category name and per-category post counter

## Future Requirements

*(None identified -- scope is intentionally narrow for this milestone)*

## Out of Scope

| Feature | Reason |
|---------|--------|
| Per-question deduplication | Quiz content varies enough by generation; topic-level dedup is sufficient |
| Automatic token refresh | Long-lived token (60 days) is acceptable; manual refresh is a known process |
| Preview frame duration > 1 frame | 1 frame keeps video length unchanged; longer intros are a separate feature |
| Multiple preview styles | Single fixed layout for simplicity |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| CI-01 | Phase 1 | Complete |
| DEDUP-01 | Phase 2 | Pending |
| DEDUP-02 | Phase 2 | Pending |
| PREVIEW-01 | Phase 3 | Pending |

**Coverage:**
- v1.0 requirements: 4 total
- Mapped to phases: 4
- Unmapped: 0

---
*Requirements defined: 2026-04-01*
*Last updated: 2026-04-01 after roadmap creation*
