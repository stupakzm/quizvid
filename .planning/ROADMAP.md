# Roadmap: QuizVid

## Overview

Harden the daily automation pipeline across three areas: fix CI compatibility (Node.js 24), prevent duplicate quiz posts via tracking and detection, and add a category preview frame to the video opening. Each phase delivers an independently verifiable improvement to the pipeline.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: CI Compatibility** - Update GitHub Actions to Node.js 24-compatible versions (completed 2026-04-01)
- [x] **Phase 2: Quiz Deduplication** - Track posted quizzes and regenerate on duplicates (completed 2026-04-01)
- [ ] **Phase 3: Category Preview Frame** - Add opening scene with category name and post counter

## Phase Details

### Phase 1: CI Compatibility
**Goal**: Daily pipeline runs without Node.js deprecation warnings
**Depends on**: Nothing (first phase)
**Requirements**: CI-01
**Success Criteria** (what must be TRUE):
  1. GitHub Actions workflow completes without any Node.js deprecation warnings in the log
  2. All action versions (checkout, cache, setup-python) reference Node.js 24-compatible releases
**Plans**: 1 plan

Plans:
- [x] 01-01-PLAN.md — Upgrade all GitHub Actions to Node.js 24-compatible versions

### Phase 2: Quiz Deduplication
**Goal**: Pipeline never posts the same quiz twice
**Depends on**: Phase 1
**Requirements**: DEDUP-01, DEDUP-02
**Success Criteria** (what must be TRUE):
  1. After a successful post, the quiz is recorded in a persistent tracking file (posted_quizzes.json)
  2. If the generated quiz matches a previously posted quiz, the pipeline regenerates a new one automatically
  3. The tracking file persists across pipeline runs (committed to repo or stored as artifact)
**Plans**: 1 plan

Plans:
- [x] 02-01-PLAN.md — Add dedup module, integrate into pipeline with retry logic, update workflow commit step

### Phase 3: Category Preview Frame
**Goal**: Every video opens with a branded category scene before the quiz begins
**Depends on**: Phase 1
**Requirements**: PREVIEW-01
**Success Criteria** (what must be TRUE):
  1. The rendered video starts with a frame displaying the current category name
  2. The opening frame also displays the per-category post counter (e.g., "Science #12")
  3. The rest of the quiz video plays unchanged after the opening frame
**Plans**: 1 plan

Plans:
- [ ] 03-01-PLAN.md — Add preview frame rendering to C binary and preview injection to Python pipeline

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. CI Compatibility | 1/1 | Complete   | 2026-04-01 |
| 2. Quiz Deduplication | 1/1 | Complete   | 2026-04-01 |
| 3. Category Preview Frame | 0/1 | Not started | - |
