# QuizVid Daily Automation — Design Spec

**Date:** 2026-03-18
**Status:** In Review

---

## Overview

Fully automate daily Instagram Reel publishing for quizvid:
- AI generates a quiz JSON matching the existing schema
- The C program renders it into an MP4
- The video is posted to Instagram with a structured caption
- Runs daily via GitHub Actions for free

---

## Architecture

```
GitHub Actions (daily cron, per-category schedule)
        │
        ▼
   automate.py
   ├── 1. Determine today's category (day-of-week → category map)
   │      If no category is assigned to today → exit cleanly (no post)
   ├── 2. Call Gemini API → validated quiz JSON (5 questions)
   ├── 3. Write JSON to examples/daily_quiz.json
   ├── 4. Compile quizvid (make)
   ├── 5. Run ./bin/quizvid → quiz_video.mp4
   ├── 6. Build caption (hook + info + hashtags)
   ├── 7. Upload video as GitHub Release asset → public URL
   ├── 8. Post to Instagram Graph API (container → publish)
   └── 9. Increment counters.json (write file only — git handled by workflow)
        │
        ▼
   Workflow step: git add counters.json && git commit && git push
```

---

## File Structure

New files added to repo:

```
quizvid/
├── automate.py                          # main orchestrator script
├── categories.py                        # single source of truth for all categories
├── counters.json                        # post count per category (never shrinks)
├── .github/
│   └── workflows/
│       └── daily.yml                   # cron workflow
└── examples/
    └── daily_quiz.json                 # generated each run (overwritten)
```

`config.json` `input.quiz_file` is permanently updated to `examples/daily_quiz.json`
and committed. This is a **one-time manual prerequisite** that must be done and
pushed to `main` before the workflow is enabled. This becomes the new default.
For custom local runs, pass a different config as a CLI argument:
`./bin/quizvid my_config.json`.

All required assets are already committed to the repo:
- Voice model: `assets/voices/en_US-libritts_r-medium.onnx`
- Background audio: `assets/audio/ticking_converted.wav`
- Font: `assets/fonts/Roboto-Bold.ttf`

No asset download steps are needed in the workflow.

---

## Category System

### Design principles

- **Single source of truth:** All category data lives in `categories.py`.
  `automate.py`, caption generation, and the cron workflow all derive their
  behaviour from it — no magic numbers or hardcoded category names anywhere else.
- **Resilient to removal:** If a category is deleted from `categories.py`, its day
  simply posts nothing. The program never crashes on missing days.
- **`counters.json` never shrinks:** When a new category is added, its counter is
  auto-initialised to 0 on first run. When a category is suspended or removed,
  its counter entry is preserved so historical counts are not lost if it returns.
- **Adding a category requires exactly:** a name, a day-of-week (0=Mon … 6=Sun),
  a post time (UTC hour), a description, and two hashtags. The program handles
  everything else automatically.

### `categories.py` structure

```python
# categories.py
# ─────────────────────────────────────────────────────────────────────────────
# SINGLE SOURCE OF TRUTH for all quiz categories.
#
# To ADD a category:
#   1. Append an entry to CATEGORIES below.
#      Required fields: name, day (0=Mon … 6=Sun), hour_utc, description, hashtags
#   2. counters.json will be updated automatically on the next run.
#   That's it. No other files need editing.
#
# To REMOVE or SUSPEND a category:
#   1. Delete or comment out its entry below.
#   2. Its day will simply post nothing. Its counter is preserved in counters.json.
#
# Places that read CATEGORIES (for your awareness — no manual edits needed):
#   - automate.py        : selects today's category, builds prompt, builds caption
#   - daily.yml          : cron schedule is derived from category hour_utc values
#   - counters.json      : auto-updated at runtime (never manually edit)
# ─────────────────────────────────────────────────────────────────────────────

CATEGORIES = [
    {
        "name": "Science",
        "day": 0,          # Monday
        "hour_utc": 14,
        "description": (
            "General science topics: physics, chemistry, biology, astronomy, "
            "earth science, human body, inventions, Nobel Prize discoveries, "
            "space exploration, natural phenomena."
        ),
        "hashtags": ["#sciencefacts", "#didyouknow"],
    },
    {
        "name": "History",
        "day": 1,          # Tuesday
        "hour_utc": 14,
        "description": (
            "World history: ancient civilisations, wars, empires, historical figures, "
            "revolutions, treaties, dates of major events, dynasties, explorers, "
            "and turning points in human history."
        ),
        "hashtags": ["#historyfacts", "#historybuff"],
    },
    {
        "name": "Geography",
        "day": 2,          # Wednesday
        "hour_utc": 14,
        "description": (
            "World geography: capitals, countries, continents, oceans, mountains, "
            "rivers, flags, populations, borders, landmarks, time zones, "
            "and geographic records (largest, smallest, highest, deepest)."
        ),
        "hashtags": ["#geographyquiz", "#aroundtheworld"],
    },
    {
        "name": "Sports",
        "day": 3,          # Thursday
        "hour_utc": 14,
        "description": (
            "Sports facts and records: Olympic history, world championships, "
            "famous athletes, team records, iconic moments, rules of sports, "
            "FIFA/NBA/NFL/tennis/F1 history, sports firsts and milestones."
        ),
        "hashtags": ["#sportsfacts", "#sportstrivia"],
    },
    {
        "name": "Movies",
        "day": 4,          # Friday
        "hour_utc": 14,
        "description": (
            "Cinema: Oscar winners, box office records, famous directors and actors, "
            "iconic quotes, sequel/prequel facts, production trivia, movie genres, "
            "film history, studio records, and cult classics."
        ),
        "hashtags": ["#moviefacts", "#cinephile"],
    },
    {
        "name": "Trends",
        "day": 5,          # Saturday
        "hour_utc": 14,
        "description": (
            "Recent events in media and culture (use your most recent knowledge): "
            "award show results (Oscars, Grammys, Emmys, BAFTAs, VMAs), "
            "viral social media creators and their milestones (subscribers, followers), "
            "trending shows and movies (streaming records, cancellations, renewals), "
            "tech product launches (phones, AI tools, apps, consoles), "
            "sports records broken recently, celebrity viral moments, "
            "internet and meme culture, viral challenges, cultural moments."
        ),
        "hashtags": ["#trending", "#viral"],
    },
    {
        "name": "Tech",
        "day": 6,          # Sunday
        "hour_utc": 14,
        "description": (
            "Technology: computer science fundamentals, programming languages, "
            "hardware history, internet history, famous tech companies and founders, "
            "software milestones, cybersecurity basics, AI/ML concepts, "
            "gadgets, gaming history, and tech innovations."
        ),
        "hashtags": ["#techfacts", "#technology"],
    },
]

# Derived lookup: day-of-week (0–6) → category entry (or None if day unassigned)
SCHEDULE = {cat["day"]: cat for cat in CATEGORIES}
```

### Behaviour when days are unassigned

`automate.py` calls `SCHEDULE.get(today_weekday)`. If `None` is returned, the
script logs "No category scheduled for today" and exits with code 0 (clean exit,
no failure in GitHub Actions). The workflow's git commit step is skipped.

---

## AI Question Generation

**Model:** `gemini-2.0-flash` (Google free tier, 1500 req/day)

**Prompt contract:**
- Return ONLY valid JSON, no markdown, no explanation
- Exactly 5 questions
- Distribution: 3 `standard`, 1 `truefalse`, 1 `multi` (order may vary)
- The remaining 5th slot (beyond the required 3+1+1) may be any type
- Factually accurate answers
- Answer text max ~3 words (fits on screen)
- `standard`: 3–5 answers, 1 correct index (variable count is valid)
- `truefalse`: answers = `["True", "False"]`, correct = `[0]` or `[1]`
- `multi`: 4–6 answers, 2–3 correct indices

The category `description` field from `categories.py` is injected verbatim into
the prompt to guide topic selection:

```
Generate a quiz JSON for the category: {category_name}.

Category scope: {category_description}

Return ONLY valid JSON matching this schema exactly: ...
```

**Validation:** Parse JSON, check schema structure and field types.
Retry once on failure before aborting the run.

**Schema:**
```json
{
  "config": { "question_duration": 5, "reveal_duration": 2 },
  "questions": [
    {
      "type": "standard",
      "question": "...",
      "answers": ["...", "...", "...", "..."],
      "correct": [1]
    }
  ]
}
```

Note: `config.question_duration` and `config.reveal_duration` in the quiz JSON are
included because the C program expects them in the schema. Actual render timing is
controlled by `config.json` timing/animation blocks. These values are fixed
constants in the Gemini prompt (5 and 2).

---

## Caption Structure

```
{engagement_hook}

Weekly {Category} Quiz #{post_number}

#{category} #quiz #quiztime #trivia #{relevant1} #{relevant2}
```

`post_number` is the per-category counter from `counters.json` (e.g. Science post #3),
incremented after each successful publish for that category.

### Engagement Hook Pool

Rotates by `day_of_year % len(HOOK_POOL)` — shifts independently of the category
rotation so all hook×category combinations occur over time. The pool size is never
hardcoded; the modulo always uses the actual list length.

1. How many did you get right? 🧠
2. What's your score? Comment below ⬇️
3. Which was the hardest question? 👇
4. Which one did you get wrong?
5. Which one did you find easiest?
6. Which one surprised you?
7. Got all 5? Should questions be more difficult next time?
8. How many did you know — be honest? 😅
9. Which category do you want next week?

### Per-Category Hashtags

Hashtags come from the `hashtags` field in `categories.py`. No separate hashtag
table exists — adding a category automatically includes its hashtags in captions.

---

## counters.json

```json
{
  "Science": 3,
  "History": 1,
  "Geography": 0,
  "Sports": 0,
  "Movies": 0,
  "Trends": 0,
  "Tech": 0
}
```

**Rules:**
- On each run, `automate.py` reads the file, ensures every active category name in
  `CATEGORIES` has an entry (initialised to 0 if missing), then increments the
  current category's count after a successful publish.
- Entries are **never deleted** from the file, even if a category is removed from
  `categories.py`. This preserves historical counts if the category returns.
- The file is written to disk by `automate.py`; git commit is done by the workflow.

---

## Instagram Posting Flow

### One-time manual setup
1. Convert Instagram account to Creator or Business
2. Create Meta Developer App at developers.facebook.com
3. Obtain a system user access token (never expires — recommended over 60-day token)
4. Store in GitHub Secrets: `INSTAGRAM_USER_ID`, `INSTAGRAM_ACCESS_TOKEN`
5. **Repo must be public** so GitHub Release asset URLs are accessible without auth
   (Instagram Graph API requires a public, non-redirecting video URL)

### GitHub Release asset strategy
- Fixed release tag: `daily-video`
- `automate.py` deletes the existing `daily-video` release + tag (if any), creates a
  new one, and uploads `quiz_video.mp4` as the asset
- The resulting public asset URL is passed to the Instagram container creation call
- `GH_TOKEN` = built-in `secrets.GITHUB_TOKEN` with `permissions: contents: write`
  declared in the workflow YAML

### Automated posting (per run)
1. Delete + recreate GitHub Release `daily-video`, upload `quiz_video.mp4` → public URL
2. `POST /v21.0/{user-id}/media` with `video_url`, `caption`, `media_type=REELS`
3. Poll container status every 5s until `FINISHED` (timeout 120s)
4. `POST /v21.0/{user-id}/media_publish` with `creation_id`

API version is defined as a constant `GRAPH_API_VERSION = "v21.0"` in `automate.py`
and should be kept up to date when Meta releases new versions.

---

## GitHub Actions Workflow

**Schedule:** `cron: '0 14 * * *'` (14:00 UTC daily)

The single daily trigger covers all current categories since they all share
`hour_utc: 14`. If a category is later added with a different `hour_utc`, a second
cron trigger must be added to `daily.yml` for that hour. This is the one manual
step that cannot be fully automated from `categories.py` alone — it is noted in the
`categories.py` file header as a reminder.

**Permissions block (top-level in workflow YAML):**
```yaml
permissions:
  contents: write
```

**Steps:**
1. Checkout repo (default `GITHUB_TOKEN` with contents:write)
2. Cache apt packages (key: `apt-ffmpeg-ft-jsonc`)
3. Install build deps: `libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libswresample-dev libfreetype6-dev libjson-c-dev`
4. Download & cache Piper TTS binary:
   - Version: `2023.11.14-2`
   - URL: `https://github.com/rhasspy/piper/releases/download/2023.11.14-2/piper_linux_x86_64.tar.gz`
   - Extract to `/usr/local/bin/piper`
   - Cache key: `piper-2023.11.14-2-x86_64`
5. `make` — compile quizvid
6. Configure git identity:
   ```
   git config user.email "github-actions[bot]@users.noreply.github.com"
   git config user.name "github-actions[bot]"
   ```
7. `python automate.py`
   (reads `GEMINI_API_KEY`, `INSTAGRAM_USER_ID`, `INSTAGRAM_ACCESS_TOKEN`,
   `GITHUB_TOKEN` from env)
   `automate.py` writes the updated `counters.json` to disk but does NOT run git.
   If no category is scheduled today, exits cleanly — skip step 8.
8. `git add counters.json && git commit -m "chore: update counters" && git push`
   (runs in the workflow YAML after automate.py exits successfully with a post made)

**GitHub Secrets required:**

| Secret | Purpose |
|--------|---------|
| `GEMINI_API_KEY` | Google AI Studio (free) |
| `INSTAGRAM_USER_ID` | Meta Developer App |
| `INSTAGRAM_ACCESS_TOKEN` | System user token |

`GITHUB_TOKEN` is provided automatically by Actions — no manual secret needed.

**Runtime estimate:** ~3–5 min/run. Well within free tier limits.

---

## Adding a New Category — Checklist

When you decide to add a new category, here is everything to do:

1. **`categories.py`** — append a new entry with `name`, `day`, `hour_utc`,
   `description`, and `hashtags`. This is the only required code change.
2. **`daily.yml`** — if the new category uses a different `hour_utc` than existing
   ones, add a new cron trigger for that hour. (If same hour, nothing to do.)
3. **`counters.json`** — no action needed. The counter is auto-created on first run.

That's it. Gemini prompt, caption, hashtags, hook rotation, and counter tracking
are all driven from `categories.py` automatically.

---

## Error Handling

| Failure point | Behaviour |
|---------------|-----------|
| No category scheduled today | Exit cleanly (code 0), skip post |
| Gemini returns invalid JSON | Retry once, then fail the run |
| `make` fails | Fail immediately with error output |
| Video render fails | Fail immediately |
| GitHub Release upload fails | Fail immediately |
| Instagram container not FINISHED in 120s | Fail with timeout error |
| Instagram publish fails | Fail, do not increment counter |
| Git push fails | Log warning, don't block (counter drift acceptable) |
