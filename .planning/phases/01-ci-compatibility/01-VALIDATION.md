---
phase: 1
slug: ci-compatibility
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-01
---

# Phase 1 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | GitHub Actions workflow logs (manual inspection) |
| **Config file** | `.github/workflows/*.yml` |
| **Quick run command** | `gh workflow run <workflow> && gh run watch` |
| **Full suite command** | `gh run list --limit 1 && gh run view --log` |
| **Estimated runtime** | ~60 seconds |

---

## Sampling Rate

- **After every task commit:** Inspect workflow YAML for correct action version strings
- **After every plan wave:** Trigger workflow manually, verify no deprecation warnings in log
- **Before `/gsd:verify-work`:** Full workflow run must be green with no Node.js deprecation warnings
- **Max feedback latency:** 60 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 1-01-01 | 01 | 1 | CI-01 | grep | `grep -r "actions/checkout@v6" .github/workflows/` | ✅ | ⬜ pending |
| 1-01-02 | 01 | 1 | CI-01 | grep | `grep -r "actions/cache@v5" .github/workflows/` | ✅ | ⬜ pending |
| 1-01-03 | 01 | 1 | CI-01 | grep | `grep -r "actions/setup-python@v6" .github/workflows/` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

*Existing infrastructure covers all phase requirements — no test framework installation needed.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| No Node.js deprecation warnings in CI log | CI-01 | Requires live workflow run on GitHub Actions | Trigger workflow via `gh workflow run`, view logs with `gh run view --log`, confirm no "Node.js 20 actions are deprecated" lines |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
