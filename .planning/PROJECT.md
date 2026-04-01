# QuizVid

## What This Is

QuizVid is a fully automated daily quiz video generator and Instagram publisher. It generates trivia quiz content via Gemini AI, renders a 1080x1920 vertical video with TTS audio (Piper) and animated captions using a custom C/FFmpeg renderer, uploads it to GitHub Releases, and publishes it as an Instagram Reel — all triggered by a GitHub Actions cron job.

## Core Value

Every day, a new quiz Reel is published to Instagram with zero manual intervention.

## Current Milestone: v1.0 — Automation Polish

**Goal:** Harden the daily automation pipeline with CI hygiene, deduplication, and a richer video opening frame.

**Target features:**
- Update GitHub Actions to Node.js 24-compatible versions (checkout, cache, setup-python)
- Track successfully posted quizzes to avoid repeating the same quiz later
- Add a 1-frame opening scene to each video displaying today's category name and post counter

## Requirements

### Validated

- ✓ Quiz generation via Gemini AI with model fallbacks — v0 (existing)
- ✓ Video rendering (1080×1920, H.264/AAC, Piper TTS, captions) — v0 (existing)
- ✓ Instagram Reels posting via Graph API — v0 (existing)
- ✓ GitHub Actions daily cron automation — v0 (existing)
- ✓ Post counter tracking (counters.json) — v0 (existing)
- ✓ Caption generation with hashtags — v0 (existing)
- ✓ Video uploaded to GitHub Releases for public URL hosting — v0 (existing)

### Active

- [ ] CI-01: GitHub Actions uses Node.js 24-compatible action versions
- [ ] DEDUP-01: System records each successfully posted quiz to avoid repeats
- [ ] DEDUP-02: Pipeline skips (or regenerates) if today's quiz was already posted
- [ ] PREVIEW-01: Video opens with a 1-frame scene showing category name and counter

### Out of Scope

- Web dashboard — no UI needed, fully headless automation
- Multiple categories per day — one post per day is the design
- Video editing features — fixed format, not configurable per-post
- Manual quiz override — quiz is always AI-generated

## Context

- **Video renderer**: Custom C binary (`bin/quizvid`) using libavformat/libavcodec/libfreetype. Renders from JSON quiz spec + config.json.
- **Quiz generation**: Python (`gemini_client.py`) with Gemini 2.0 Flash, fallbacks to Flash-8B and 1.5 Pro.
- **Automation entry point**: `automate.py` — generates quiz → renders video → uploads to GitHub Releases → posts to Instagram.
- **Counter storage**: `counters.json` tracks per-category post numbers.
- **Video hosting**: GitHub Release asset (`daily-video` tag) — URL resolved to CDN before passing to Instagram.
- **Instagram auth**: Long-lived token (60-day expiry), Business account linked to Facebook Page.

## Constraints

- **Stack**: Python 3.11 + C (FFmpeg libs) — no new runtimes
- **CI**: GitHub Actions — must stay within free tier limits
- **Video format**: 1080×1920, H.264, AAC, 30fps — Instagram Reels spec, non-negotiable
- **Dependencies**: Piper TTS binary + voice models cached in Actions, libav* installed via apt

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| GitHub Releases for video hosting | Free, public URL, no extra service | ✓ Good |
| Resolve redirect URL before Instagram | Instagram doesn't follow GitHub redirects | ✓ Good |
| C/FFmpeg renderer vs Python/moviepy | Performance + codec control needed | ✓ Good |
| Long-lived token (manual refresh) | No server-side OAuth refresh needed | — Pending |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd:transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd:complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-04-01 after milestone v1.0 initialized*
