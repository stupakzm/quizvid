# Phase 1: CI Compatibility - Research

**Researched:** 2026-04-01
**Domain:** GitHub Actions workflow modernization, Node.js runtime compatibility
**Confidence:** HIGH

## Summary

The QuizVid daily automation pipeline runs via GitHub Actions cron job (.github/workflows/daily.yml), currently using three deprecated Node.js 20 actions: `actions/checkout@v4`, `actions/cache@v4`, and `actions/setup-python@v5`. GitHub has announced a hard deadline of **June 2, 2026** for migrating all actions to Node.js 24-compatible versions. Node.js 20 reaches end-of-life in April 2026 and will be removed from runners in fall 2026.

The fix is straightforward: upgrade three actions to their Node.js 24 compatible versions. This is a non-breaking change for the project — the new action versions maintain backward compatibility with the existing workflow structure and Python 3.11 runtime.

**Primary recommendation:** Upgrade actions/checkout to v5 (or v6), actions/cache to v5, and actions/setup-python to v6 in the workflow file. This eliminates deprecation warnings and ensures the pipeline will continue to run after June 2, 2026.

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| CI-01 | Pipeline uses Node.js 24-compatible GitHub Actions versions (no deprecation warnings) | All three actions identified; verified Node.js 24 compatibility versions available; upgrade path documented |

## Standard Stack

### GitHub Actions for CI

| Action | Current | Required | Purpose | Node.js 24 Compatible |
|--------|---------|----------|---------|----------------------|
| actions/checkout | v4 | v5+ | Clone repository | v5.0.0+, v6.0.2+ (current: v6.0.2) |
| actions/cache | v4 | v5+ | Cache build dependencies (Piper TTS, apt packages) | v5.0.4+ (current: v5.0.4) |
| actions/setup-python | v5 | v6+ | Install Python 3.11 runtime | v6.2.0+ (current: v6.2.0) |

**Runner requirement:** Minimum runner v2.327.1 to support Node.js 24 actions.

**Installation:** Update .github/workflows/daily.yml workflow file:
```yaml
- uses: actions/checkout@v6
- uses: actions/cache@v5
- uses: actions/setup-python@v6
```

## Architecture Patterns

### Current Workflow Structure

The daily.yml workflow is a single-job pipeline:

```yaml
name: Daily Quiz Post
on:
  schedule:
    - cron: '0 14 * * *'  # 14:00 UTC daily
  workflow_dispatch:

jobs:
  post:
    runs-on: ubuntu-latest
    steps:
      # 1. Checkout
      # 2. Install dependencies (apt, Piper TTS)
      # 3. Set up Python
      # 4. Install Python packages
      # 5. Compile C binary
      # 6. Configure git
      # 7. Run automation
      # 8. Commit and push
```

**Pattern:** The workflow is dependency-ordered (checkout → cache → setup-python → build → run → commit). Node.js 24 migration requires only action version bumps; step order and logic remain unchanged.

### Recommended Update Strategy

**File to modify:** `.github/workflows/daily.yml` (three lines)

**Change scope:**
- Line 24: `uses: actions/checkout@v4` → `uses: actions/checkout@v6`
- Line 27: `uses: actions/cache@v4` → `uses: actions/cache@v5`
- Line 60: `uses: actions/setup-python@v5` → `uses: actions/setup-python@v6`

**Testing approach:** 
1. Merge the workflow change to main
2. Trigger with `workflow_dispatch` to test immediately
3. Verify no Node.js 20 deprecation warnings in the action log

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| GitHub Actions caching | Custom caching logic in shell scripts | actions/cache@v5 | Built-in cache service, atomic operations, correct key invalidation |
| Python environment setup | Manual apt/pyenv/pip commands | actions/setup-python@v6 | Handles PATH, virtual env, pip cache, multiple Python versions |
| Repository checkout | Manual git clone in script | actions/checkout@v6 | Handles SSH keys, sparse checkouts, fetch depth, LFS, submodules |
| Node.js runtime updates | Custom runner configuration | GitHub's managed action updates | GitHub tests compatibility; custom solutions miss deprecation deadlines |

**Key insight:** Attempting to work around the Node.js 24 migration with custom shell steps is technically possible but creates maintenance debt — GitHub's official actions are tested against Node.js 24 and will receive security updates.

## Common Pitfalls

### Pitfall 1: Upgrading v4 → v5 without checking runner compatibility
**What goes wrong:** Action fails with "Runner version v2.327.1+ required" error.
**Why it happens:** Node.js 24 actions require a minimum runner version that the user's self-hosted runners may not meet.
**How to avoid:** Verify the GitHub Actions runner is at least v2.327.1. For GitHub-hosted runners (ubuntu-latest), this is automatic. Self-hosted runners may need updating.
**Warning signs:** Job fails with "Node.js 24 not available" or runner version mismatch error.

### Pitfall 2: Only upgrading one action, leaving others on Node.js 20
**What goes wrong:** Workflow still shows Node.js 20 deprecation warnings because not all actions are upgraded.
**Why it happens:** Each action version is independent; skipping one leaves that deprecation.
**How to avoid:** Update checkout, cache, AND setup-python together as a set.
**Warning signs:** Partial warning list in job logs (e.g., "checkout@v4 deprecated" but not cache).

### Pitfall 3: Missing the April 2026 EOL → June 2026 forced migration timeline
**What goes wrong:** Workflow fails in June when GitHub forces Node.js 24 and old actions no longer run.
**Why it happens:** Treating the deadline as optional or assuming graceful degradation.
**How to avoid:** Update now (April 2026 present); deadline is hard (June 2, 2026).
**Warning signs:** No action needed; this is a blocking deadline.

### Pitfall 4: Assuming action patch versions (v5.0.0 → v5.0.4) handle incompatible Node.js transitions
**What goes wrong:** Action behavior or API changes between major versions cause unexpected failures.
**Why it happens:** v5 and v6 may differ in cache key naming, default Python environments, or checkout behavior.
**How to avoid:** Read release notes for v5→v6 or v4→v5 migration guides if upgrading beyond one major version.
**Warning signs:** Cache misses, Python version mismatches, or checkout failures post-upgrade.

## Code Examples

### Current Workflow (Node.js 20 — deprecated)

```yaml
# Source: /home/stupakzm/projects/quizvid/.github/workflows/daily.yml (current state)
steps:
  - name: Checkout repo
    uses: actions/checkout@v4

  - name: Cache apt packages
    uses: actions/cache@v4
    with:
      path: /var/cache/apt/archives
      key: apt-ffmpeg-ft-jsonc-${{ runner.os }}

  - name: Set up Python
    uses: actions/setup-python@v5
    with:
      python-version: '3.11'
```

### Upgraded Workflow (Node.js 24 — compatible)

```yaml
# Target state: Node.js 24 compatible
steps:
  - name: Checkout repo
    uses: actions/checkout@v6

  - name: Cache apt packages
    uses: actions/cache@v5
    with:
      path: /var/cache/apt/archives
      key: apt-ffmpeg-ft-jsonc-${{ runner.os }}

  - name: Set up Python
    uses: actions/setup-python@v6
    with:
      python-version: '3.11'
```

**Changes:**
- actions/checkout@v4 → @v6 (v5 also works; v6 is newer)
- actions/cache@v4 → @v5 (only breaking version for Node.js 24)
- actions/setup-python@v5 → @v6 (v5 is Node.js 20; v6+ required for Node.js 24)

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| actions/checkout@v4 | actions/checkout@v6 | June 2, 2026 (hard deadline) | Node.js 20 deprecated Sept 2025; Node.js 24 forced in June 2026 |
| actions/cache@v4 | actions/cache@v5 | June 2, 2026 (hard deadline) | New cache service v2 API required for Node.js 24 |
| actions/setup-python@v5 | actions/setup-python@v6 | June 2, 2026 (hard deadline) | v5 is Node.js 20 only; v6+ required for Node.js 24 |

**Deprecated/outdated:**
- **actions/checkout@v4:** Node.js 20 only, deprecated as of September 2025. Will fail on runners after June 2, 2026.
- **actions/cache@v4:** Node.js 20 only, deprecated. v5 uses new cache service API.
- **actions/setup-python@v5:** Node.js 20 only, deprecated. v6 is the first major version with Node.js 24 support.

## Open Questions

None identified. The GitHub Actions deprecation path is official, documented, and verified through multiple authoritative sources. Action version compatibility is clear, and the upgrade is low-risk.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| GitHub Actions runner | Workflow execution | ✓ | ubuntu-latest (automatic) | — |
| Runner Node.js support | actions/checkout, actions/cache, actions/setup-python | ✓ (v2.327.1+) | v2.+ (auto-updated) | Manual runner update (self-hosted only) |
| Python 3.11 | Python step | ✓ (via setup-python) | 3.11.x | — |

**Missing dependencies:** None. All dependencies are provided by GitHub-hosted runners.

**Note:** Self-hosted runners must be at v2.327.1+ to support Node.js 24 actions. No other environment setup required.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | GitHub Actions workflow validation |
| Config file | .github/workflows/daily.yml |
| Quick run command | `workflow_dispatch` button or `gh workflow run daily.yml` |
| Full suite command | N/A (single workflow, single job) |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| CI-01 | Workflow completes without Node.js 20 deprecation warnings | Workflow log inspection | Manual: trigger `workflow_dispatch`, inspect logs for "Node.js 20 actions are deprecated" | ✅ .github/workflows/daily.yml exists |
| CI-01 | All actions reference Node.js 24-compatible versions | Code inspection | `grep -E 'actions/(checkout\|cache\|setup-python)@' .github/workflows/daily.yml` | ✅ .github/workflows/daily.yml exists |

### Sampling Rate
- **Per task commit:** Trigger workflow manually after merge to verify no deprecation warnings appear
- **Per wave merge:** Full workflow run (includes all steps: checkout, cache, Python setup, build, automation, commit)
- **Phase gate:** At least one successful workflow run with no Node.js 20 warnings before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `.github/workflows/daily.yml` already exists — no new test infrastructure needed
- [ ] Workflow validation: Run `workflow_dispatch` after changes to confirm no Node.js 20 deprecation warnings in job logs

*(Existing workflow infrastructure covers all phase requirements. No test files or fixtures needed.)*

## Sources

### Primary (HIGH confidence)
- [GitHub Blog: Deprecation of Node 20 on GitHub Actions runners](https://github.blog/changelog/2025-09-19-deprecation-of-node-20-on-github-actions-runners/) - Official GitHub announcement of Node.js 20 deprecation timeline (April 2026 EOL, June 2, 2026 forced migration)
- [actions/checkout releases](https://github.com/actions/checkout/releases) - Verified v5.0.0+ and v6.0.2+ support Node.js 24 (minimum runner v2.327.1)
- [actions/cache releases](https://github.com/actions/cache/releases) - Verified v5.0.4+ is the Node.js 24 compatible version (released March 18, 2025)
- [actions/setup-python releases](https://github.com/actions/setup-python/releases) - Verified v6.2.0+ with Node.js 24 compatibility (v6.0.0+ required, current v6.2.0 released January 22, 2025)

### Secondary (MEDIUM confidence)
- [GitHub Actions Node.js 20 Deprecation guides](https://devactivity.com/insights/boosting-software-engineering-efficiency-navigating-nodejs-deprecation-in-github-actions/) - Timeline confirmation and remediation options
- Community issues ([processing/p5.js#8674](https://github.com/processing/p5.js/issues/8674), [adafruit/circuitpython#10888](https://github.com/adafruit/circuitpython/issues/10888)) - Confirmation of upgrade path and version numbers from real projects

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Official release pages and GitHub Blog provide exact version numbers and compatibility dates
- Architecture: HIGH - Workflow structure is simple and verified by inspection
- Pitfalls: MEDIUM - Based on common action upgrade issues; no project-specific gotchas identified
- Deprecation timeline: HIGH - Official GitHub announcement with specific dates

**Research date:** 2026-04-01
**Valid until:** 2026-06-02 (hard deadline for Node.js 24 migration)
**Note:** Recommend completing this phase well before June 2, 2026 to avoid any surprises on the forced migration date.
