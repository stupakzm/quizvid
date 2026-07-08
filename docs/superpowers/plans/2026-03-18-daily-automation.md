# Daily Automation Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Automate daily Instagram Reel publishing by generating quiz questions with Gemini, rendering an MP4 with quizvid, and posting via the Instagram Graph API — all triggered by a GitHub Actions cron job.

**Architecture:** A Python orchestrator (`automate.py`) coordinates five focused modules: `categories.py` (data), `counters.py` (persistence), `gemini_client.py` (AI), `caption.py` (text), `video_renderer.py` (subprocess), and `instagram_client.py` (APIs). GitHub Actions runs the orchestrator daily at 14:00 UTC.

**Tech Stack:** Python 3.11, `google-generativeai`, `requests`, pytest, GitHub Actions, Instagram Graph API v21.0

---

## Chunk 1: Foundation — categories, counters, config

### Task 1: Create `requirements.txt` and `tests/` directory

**Files:**
- Create: `requirements.txt`
- Create: `tests/__init__.py`

- [ ] **Step 1: Write `requirements.txt`**

```
google-generativeai>=0.5.0
requests>=2.31.0
pytest>=7.4.0
```

- [ ] **Step 2: Create `tests/__init__.py`**

```python
```
(empty file)

- [ ] **Step 3: Install dependencies**

Run: `pip install -r requirements.txt`
Expected: All packages install without errors.

- [ ] **Step 4: Commit**

```bash
git add requirements.txt tests/__init__.py
git commit -m "chore: add Python dependencies and tests directory"
```

---

### Task 2: Implement `categories.py`

**Files:**
- Create: `categories.py`

- [ ] **Step 1: Write `categories.py`**

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
#
# NOTE: If you add a category with a new hour_utc value, also add a cron trigger
# for that hour in .github/workflows/daily.yml.
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

- [ ] **Step 2: Commit**

```bash
git add categories.py
git commit -m "feat: add categories single source of truth"
```

---

### Task 3: Test and implement `counters.py`

**Files:**
- Create: `tests/test_counters.py`
- Create: `counters.py`

- [ ] **Step 1: Write failing tests**

```python
# tests/test_counters.py
import json
import os
import tempfile
import pytest
from unittest.mock import patch

# We'll patch COUNTERS_FILE so tests don't touch the real file
import counters as counters_module


@pytest.fixture(autouse=True)
def tmp_counters_file(tmp_path):
    """Redirect counters file to a temp path for every test."""
    tmp_file = str(tmp_path / "counters.json")
    with patch.object(counters_module, "COUNTERS_FILE", tmp_file):
        yield tmp_file


def test_load_counters_missing_file():
    result = counters_module.load_counters()
    assert result == {}


def test_save_and_load_counters():
    data = {"Science": 3, "History": 1}
    counters_module.save_counters(data)
    assert counters_module.load_counters() == data


def test_get_post_number_initialises_active_categories():
    # All active categories should be initialised to 0
    from categories import CATEGORIES
    counters_module.get_post_number("Science")
    data = counters_module.load_counters()
    for cat in CATEGORIES:
        assert cat["name"] in data
        assert data[cat["name"]] == 0


def test_get_post_number_returns_existing_value():
    counters_module.save_counters({"Science": 5})
    assert counters_module.get_post_number("Science") == 5


def test_increment_increases_count():
    counters_module.save_counters({"Science": 2})
    result = counters_module.increment("Science")
    assert result == 3
    assert counters_module.load_counters()["Science"] == 3


def test_increment_does_not_delete_other_categories():
    counters_module.save_counters({"Science": 1, "OldCategory": 99})
    counters_module.increment("Science")
    data = counters_module.load_counters()
    assert data["OldCategory"] == 99  # preserved even if not in CATEGORIES


def test_increment_initialises_missing_category():
    # If counters.json exists but new category not yet in it
    counters_module.save_counters({"Science": 1})
    counters_module.increment("History")
    assert counters_module.load_counters()["History"] == 1
```

- [ ] **Step 2: Run tests to confirm they fail**

Run: `pytest tests/test_counters.py -v`
Expected: `ModuleNotFoundError: No module named 'counters'`

- [ ] **Step 3: Implement `counters.py`**

```python
# counters.py
import json
import os

from categories import CATEGORIES

COUNTERS_FILE = "counters.json"


def load_counters():
    if os.path.exists(COUNTERS_FILE):
        with open(COUNTERS_FILE) as f:
            return json.load(f)
    return {}


def save_counters(data):
    with open(COUNTERS_FILE, "w") as f:
        json.dump(data, f, indent=2)


def _ensure_active_categories(data):
    """Add any active categories missing from data. Never removes entries."""
    for cat in CATEGORIES:
        if cat["name"] not in data:
            data[cat["name"]] = 0
    return data


def get_post_number(category_name):
    """Return current post count for a category, initialising all active categories."""
    data = load_counters()
    data = _ensure_active_categories(data)
    save_counters(data)
    return data.get(category_name, 0)


def increment(category_name):
    """Increment post count for a category and return the new value."""
    data = load_counters()
    data = _ensure_active_categories(data)
    data[category_name] = data.get(category_name, 0) + 1
    save_counters(data)
    return data[category_name]
```

- [ ] **Step 4: Run tests to confirm they pass**

Run: `pytest tests/test_counters.py -v`
Expected: 7 tests PASSED

- [ ] **Step 5: Create initial `counters.json`**

```bash
python -c "import counters; counters.get_post_number('Science')"
```

Expected: `counters.json` created with all 7 categories set to 0.

- [ ] **Step 6: Commit**

```bash
git add counters.py tests/test_counters.py counters.json
git commit -m "feat: add counters module with auto-init and preservation logic"
```

---

### Task 4: Update `config.json`

**Files:**
- Modify: `config.json`

- [ ] **Step 1: Update `input.quiz_file` in `config.json`**

Change line:
```json
"quiz_file": "examples/sample_quiz.json"
```
to:
```json
"quiz_file": "examples/daily_quiz.json"
```

- [ ] **Step 2: Create placeholder `examples/daily_quiz.json`**

Copy `examples/sample_quiz.json` to `examples/daily_quiz.json` so the binary
can be tested before automation runs:

```bash
cp examples/sample_quiz.json examples/daily_quiz.json
```

- [ ] **Step 3: Commit**

```bash
git add config.json examples/daily_quiz.json
git commit -m "chore: point config to daily_quiz.json as default input"
```

---

## Chunk 2: Gemini client and caption builder

### Task 5: Test and implement `gemini_client.py`

**Files:**
- Create: `tests/test_gemini_client.py`
- Create: `gemini_client.py`

- [ ] **Step 1: Write failing tests**

```python
# tests/test_gemini_client.py
import json
import pytest
from unittest.mock import patch, MagicMock

import gemini_client


VALID_QUIZ = {
    "config": {"question_duration": 5, "reveal_duration": 2},
    "questions": [
        {"type": "standard", "question": "Q1?", "answers": ["A", "B", "C", "D"], "correct": [0]},
        {"type": "standard", "question": "Q2?", "answers": ["A", "B", "C"], "correct": [1]},
        {"type": "standard", "question": "Q3?", "answers": ["A", "B", "C", "D", "E"], "correct": [2]},
        {"type": "truefalse", "question": "Q4?", "answers": ["True", "False"], "correct": [1]},
        {"type": "multi", "question": "Q5?", "answers": ["A", "B", "C", "D"], "correct": [0, 2]},
    ],
}


def _make_mock_response(text):
    mock = MagicMock()
    mock.text = text
    return mock


def test_parse_valid_json():
    result = gemini_client._parse_and_validate(json.dumps(VALID_QUIZ))
    assert result["questions"][0]["type"] == "standard"
    assert len(result["questions"]) == 5


def test_parse_strips_markdown_fences():
    wrapped = f"```json\n{json.dumps(VALID_QUIZ)}\n```"
    result = gemini_client._parse_and_validate(wrapped)
    assert len(result["questions"]) == 5


def test_parse_rejects_wrong_question_count():
    bad = dict(VALID_QUIZ)
    bad["questions"] = VALID_QUIZ["questions"][:3]
    with pytest.raises(Exception):
        gemini_client._parse_and_validate(json.dumps(bad))


def test_parse_rejects_invalid_truefalse_answers():
    bad_questions = list(VALID_QUIZ["questions"])
    bad_questions[3] = {
        "type": "truefalse", "question": "Q?",
        "answers": ["Yes", "No"], "correct": [0]
    }
    bad = {**VALID_QUIZ, "questions": bad_questions}
    with pytest.raises(Exception):
        gemini_client._parse_and_validate(json.dumps(bad))


def test_parse_rejects_multi_with_too_few_correct():
    bad_questions = list(VALID_QUIZ["questions"])
    bad_questions[4] = {
        "type": "multi", "question": "Q?",
        "answers": ["A", "B", "C", "D"], "correct": [0]  # only 1 correct
    }
    bad = {**VALID_QUIZ, "questions": bad_questions}
    with pytest.raises(Exception):
        gemini_client._parse_and_validate(json.dumps(bad))


def test_parse_rejects_standard_with_no_answers():
    bad_questions = list(VALID_QUIZ["questions"])
    bad_questions[0] = {
        "type": "standard", "question": "Q?",
        "answers": ["A", "B"], "correct": [0]  # only 2 answers, min is 3
    }
    bad = {**VALID_QUIZ, "questions": bad_questions}
    with pytest.raises(Exception):
        gemini_client._parse_and_validate(json.dumps(bad))


def test_generate_quiz_calls_gemini_and_returns_data():
    category = {
        "name": "Science",
        "description": "General science topics.",
    }
    mock_model = MagicMock()
    mock_model.generate_content.return_value = _make_mock_response(json.dumps(VALID_QUIZ))

    with patch.dict("os.environ", {"GEMINI_API_KEY": "fake-key"}):
        with patch("gemini_client.genai") as mock_genai:
            mock_genai.GenerativeModel.return_value = mock_model
            result = gemini_client.generate_quiz(category)

    assert result["questions"][0]["type"] == "standard"
    prompt_arg = mock_model.generate_content.call_args[0][0]
    assert "Science" in prompt_arg
    assert "General science topics" in prompt_arg
```

- [ ] **Step 2: Run tests to confirm they fail**

Run: `pytest tests/test_gemini_client.py -v`
Expected: `ModuleNotFoundError: No module named 'gemini_client'`

- [ ] **Step 3: Implement `gemini_client.py`**

```python
# gemini_client.py
import json
import os

import google.generativeai as genai


QUIZ_SCHEMA_EXAMPLE = """{
  "config": {"question_duration": 5, "reveal_duration": 2},
  "questions": [
    {
      "type": "standard",
      "question": "Example question?",
      "answers": ["Answer A", "Answer B", "Answer C", "Answer D"],
      "correct": [1]
    }
  ]
}"""


def generate_quiz(category):
    """Call Gemini API and return a validated quiz dict for the given category."""
    genai.configure(api_key=os.environ["GEMINI_API_KEY"])
    model = genai.GenerativeModel("gemini-2.0-flash")

    prompt = (
        f"Generate a quiz JSON for the category: {category['name']}.\n\n"
        f"Category scope: {category['description']}\n\n"
        "Return ONLY valid JSON matching this schema exactly. "
        "No markdown, no explanation, no extra text.\n\n"
        f"Schema example:\n{QUIZ_SCHEMA_EXAMPLE}\n\n"
        "Rules:\n"
        "- Exactly 5 questions\n"
        "- Distribution: 3 standard, 1 truefalse, 1 multi (order may vary)\n"
        "- Factually accurate answers\n"
        "- Answer text max ~3 words\n"
        "- standard: 3-5 answers, exactly 1 correct index\n"
        '- truefalse: answers must be exactly ["True", "False"], '
        "correct is [0] or [1]\n"
        "- multi: 4-6 answers, 2-3 correct indices\n"
    )

    response = model.generate_content(prompt)
    return _parse_and_validate(response.text)


def _parse_and_validate(text):
    """Parse raw Gemini response text into a validated quiz dict."""
    text = text.strip()

    # Strip markdown code fences if present
    if text.startswith("```"):
        lines = text.split("\n")
        # Drop opening fence line and closing fence line
        lines = lines[1:]
        if lines and lines[-1].strip() == "```":
            lines = lines[:-1]
        text = "\n".join(lines)

    data = json.loads(text)

    if "config" not in data:
        raise ValueError("Missing 'config' key")
    if "questions" not in data:
        raise ValueError("Missing 'questions' key")
    if len(data["questions"]) != 5:
        raise ValueError(f"Expected 5 questions, got {len(data['questions'])}")

    for i, q in enumerate(data["questions"]):
        if q.get("type") not in ("standard", "truefalse", "multi"):
            raise ValueError(f"Question {i}: invalid type '{q.get('type')}'")
        if not isinstance(q.get("question"), str):
            raise ValueError(f"Question {i}: 'question' must be a string")
        if not isinstance(q.get("answers"), list):
            raise ValueError(f"Question {i}: 'answers' must be a list")
        if not isinstance(q.get("correct"), list):
            raise ValueError(f"Question {i}: 'correct' must be a list")

        if q["type"] == "truefalse":
            if q["answers"] != ["True", "False"]:
                raise ValueError(
                    f"Question {i}: truefalse answers must be ['True', 'False']"
                )
            if len(q["correct"]) != 1:
                raise ValueError(f"Question {i}: truefalse must have 1 correct")
        elif q["type"] == "standard":
            if len(q["answers"]) < 3:
                raise ValueError(
                    f"Question {i}: standard must have at least 3 answers"
                )
            if len(q["correct"]) != 1:
                raise ValueError(f"Question {i}: standard must have 1 correct")
        elif q["type"] == "multi":
            if not (4 <= len(q["answers"]) <= 6):
                raise ValueError(
                    f"Question {i}: multi must have 4-6 answers"
                )
            if not (2 <= len(q["correct"]) <= 3):
                raise ValueError(
                    f"Question {i}: multi must have 2-3 correct"
                )

    return data
```

- [ ] **Step 4: Run tests to confirm they pass**

Run: `pytest tests/test_gemini_client.py -v`
Expected: 7 tests PASSED

- [ ] **Step 5: Commit**

```bash
git add gemini_client.py tests/test_gemini_client.py
git commit -m "feat: add Gemini quiz generation client with validation"
```

---

### Task 6: Test and implement `caption.py`

**Files:**
- Create: `tests/test_caption.py`
- Create: `caption.py`

- [ ] **Step 1: Write failing tests**

```python
# tests/test_caption.py
import pytest
from unittest.mock import patch
from datetime import datetime

import caption as caption_module


SCIENCE_CATEGORY = {
    "name": "Science",
    "hashtags": ["#sciencefacts", "#didyouknow"],
}


def test_build_caption_contains_hook():
    result = caption_module.build_caption(SCIENCE_CATEGORY, post_number=3)
    # Should contain at least one hook from the pool
    found = any(hook in result for hook in caption_module.HOOK_POOL)
    assert found, f"No hook found in caption:\n{result}"


def test_build_caption_contains_category_and_number():
    result = caption_module.build_caption(SCIENCE_CATEGORY, post_number=7)
    assert "Science" in result
    assert "#7" in result


def test_build_caption_contains_required_hashtags():
    result = caption_module.build_caption(SCIENCE_CATEGORY, post_number=1)
    assert "#quiz" in result
    assert "#quiztime" in result
    assert "#trivia" in result
    assert "#sciencefacts" in result
    assert "#didyouknow" in result


def test_build_caption_does_not_contain_score_line():
    result = caption_module.build_caption(SCIENCE_CATEGORY, post_number=1)
    assert "Score:" not in result
    assert "0-5" not in result


def test_hook_rotates_by_day_of_year():
    # Two different days should potentially produce different hooks
    # Force day 0 → hook index 0, day 1 → hook index 1
    with patch("caption_module.datetime") as mock_dt:
        mock_dt.now.return_value.timetuple.return_value.tm_yday = 0
        result0 = caption_module.build_caption(SCIENCE_CATEGORY, 1)

        mock_dt.now.return_value.timetuple.return_value.tm_yday = 1
        result1 = caption_module.build_caption(SCIENCE_CATEGORY, 1)

    # Both should contain a hook; different days → different hooks
    assert result0 != result1 or len(caption_module.HOOK_POOL) == 1


def test_hook_pool_uses_len_not_magic_number():
    # Verify rotation uses dynamic pool length
    # Add a temp hook and confirm index still works
    original_pool = caption_module.HOOK_POOL[:]
    caption_module.HOOK_POOL.append("Test hook!")
    try:
        result = caption_module.build_caption(SCIENCE_CATEGORY, 1)
        assert isinstance(result, str)
    finally:
        caption_module.HOOK_POOL[:] = original_pool
```

- [ ] **Step 2: Run tests to confirm they fail**

Run: `pytest tests/test_caption.py -v`
Expected: `ModuleNotFoundError: No module named 'caption'`

- [ ] **Step 3: Implement `caption.py`**

```python
# caption.py
from datetime import datetime

HOOK_POOL = [
    "How many did you get right? 🧠",
    "What's your score? Comment below ⬇️",
    "Which was the hardest question? 👇",
    "Which one did you get wrong?",
    "Which one did you find easiest?",
    "Which one surprised you?",
    "Got all 5? Should questions be more difficult next time?",
    "How many did you know — be honest? 😅",
    "Which category do you want next week?",
]


def build_caption(category, post_number):
    """Build an Instagram caption for today's quiz post."""
    day_of_year = datetime.now().timetuple().tm_yday
    hook = HOOK_POOL[day_of_year % len(HOOK_POOL)]

    category_tag = f"#{category['name'].lower()}"
    extra_hashtags = " ".join(category["hashtags"])

    return (
        f"{hook}\n\n"
        f"Weekly {category['name']} Quiz #{post_number}\n\n"
        f"{category_tag} #quiz #quiztime #trivia {extra_hashtags}"
    )
```

- [ ] **Step 4: Fix import in test (patch path)**

In `tests/test_caption.py`, the patch path `"caption_module.datetime"` should be
`"caption.datetime"`. Fix it:

```python
with patch("caption.datetime") as mock_dt:
```

- [ ] **Step 5: Run tests to confirm they pass**

Run: `pytest tests/test_caption.py -v`
Expected: 6 tests PASSED

- [ ] **Step 6: Commit**

```bash
git add caption.py tests/test_caption.py
git commit -m "feat: add caption builder with rotating hook pool"
```

---

## Chunk 3: Video renderer and Instagram client

### Task 7: Test and implement `video_renderer.py`

**Files:**
- Create: `tests/test_video_renderer.py`
- Create: `video_renderer.py`

- [ ] **Step 1: Write failing tests**

```python
# tests/test_video_renderer.py
import pytest
from unittest.mock import patch, MagicMock
import subprocess

import video_renderer


def _make_proc(returncode=0, stdout="", stderr=""):
    mock = MagicMock()
    mock.returncode = returncode
    mock.stdout = stdout
    mock.stderr = stderr
    return mock


def test_compile_quizvid_success():
    with patch("video_renderer.subprocess.run", return_value=_make_proc(0)):
        assert video_renderer.compile_quizvid() is True


def test_compile_quizvid_failure_raises():
    with patch("video_renderer.subprocess.run", return_value=_make_proc(1, stderr="error")):
        with pytest.raises(RuntimeError, match="make failed"):
            video_renderer.compile_quizvid()


def test_render_video_success(tmp_path):
    video_file = tmp_path / "quiz_video.mp4"
    video_file.write_bytes(b"fake")

    with patch("video_renderer.subprocess.run", return_value=_make_proc(0)):
        with patch("video_renderer.OUTPUT_VIDEO", str(video_file)):
            result = video_renderer.render_video()
    assert result == str(video_file)


def test_render_video_process_failure_raises():
    with patch("video_renderer.subprocess.run", return_value=_make_proc(1, stderr="crash")):
        with pytest.raises(RuntimeError, match="quizvid failed"):
            video_renderer.render_video()


def test_render_video_missing_output_raises(tmp_path):
    with patch("video_renderer.subprocess.run", return_value=_make_proc(0)):
        with patch("video_renderer.OUTPUT_VIDEO", str(tmp_path / "missing.mp4")):
            with pytest.raises(RuntimeError, match="not found after render"):
                video_renderer.render_video()
```

- [ ] **Step 2: Run tests to confirm they fail**

Run: `pytest tests/test_video_renderer.py -v`
Expected: `ModuleNotFoundError: No module named 'video_renderer'`

- [ ] **Step 3: Implement `video_renderer.py`**

```python
# video_renderer.py
import os
import subprocess

OUTPUT_VIDEO = "quiz_video.mp4"


def compile_quizvid():
    """Run `make` to compile the quizvid binary. Raises RuntimeError on failure."""
    result = subprocess.run(["make"], capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(f"make failed:\n{result.stderr}")
    return True


def render_video(config_file="config.json"):
    """Run the quizvid binary to render the video. Returns path to output file."""
    result = subprocess.run(
        ["./bin/quizvid", config_file],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(f"quizvid failed:\n{result.stderr}")
    if not os.path.exists(OUTPUT_VIDEO):
        raise RuntimeError(f"{OUTPUT_VIDEO} not found after render")
    return OUTPUT_VIDEO
```

- [ ] **Step 4: Run tests to confirm they pass**

Run: `pytest tests/test_video_renderer.py -v`
Expected: 5 tests PASSED

- [ ] **Step 5: Commit**

```bash
git add video_renderer.py tests/test_video_renderer.py
git commit -m "feat: add video renderer subprocess wrapper"
```

---

### Task 8: Test and implement `instagram_client.py`

**Files:**
- Create: `tests/test_instagram_client.py`
- Create: `instagram_client.py`

- [ ] **Step 1: Write failing tests**

```python
# tests/test_instagram_client.py
import pytest
from unittest.mock import patch, MagicMock, mock_open, call
import requests

import instagram_client


ENV = {
    "GITHUB_TOKEN": "ghtoken",
    "GITHUB_REPOSITORY": "user/quizvid",
    "INSTAGRAM_USER_ID": "123456",
    "INSTAGRAM_ACCESS_TOKEN": "igtoken",
}


def _mock_response(status=200, json_data=None, raise_for_status=None):
    mock = MagicMock()
    mock.status_code = status
    mock.json.return_value = json_data or {}
    if raise_for_status:
        mock.raise_for_status.side_effect = raise_for_status
    else:
        mock.raise_for_status.return_value = None
    return mock


def test_upload_video_no_existing_release(tmp_path):
    video = tmp_path / "quiz_video.mp4"
    video.write_bytes(b"video")

    with patch.dict("os.environ", ENV):
        with patch("instagram_client.requests.get") as mock_get, \
             patch("instagram_client.requests.post") as mock_post, \
             patch("instagram_client.requests.delete") as mock_delete:

            # No existing release (404)
            mock_get.return_value = _mock_response(status=404)
            # Create release response
            mock_post.return_value = _mock_response(
                json_data={
                    "upload_url": "https://uploads.github.com/repos/user/quizvid/releases/1/assets{?name,label}",
                    "id": 1,
                }
            )
            # Asset upload response
            mock_post.side_effect = [
                mock_post.return_value,  # create release
                _mock_response(json_data={"browser_download_url": "https://example.com/video.mp4"}),
            ]

            url = instagram_client.upload_video_to_github(str(video))

    assert url == "https://example.com/video.mp4"
    mock_delete.assert_not_called()


def test_upload_video_deletes_existing_release(tmp_path):
    video = tmp_path / "quiz_video.mp4"
    video.write_bytes(b"video")

    with patch.dict("os.environ", ENV):
        with patch("instagram_client.requests.get") as mock_get, \
             patch("instagram_client.requests.post") as mock_post, \
             patch("instagram_client.requests.delete"):

            mock_get.return_value = _mock_response(json_data={"id": 42})
            mock_post.side_effect = [
                _mock_response(json_data={
                    "upload_url": "https://uploads.github.com/repos/user/quizvid/releases/2/assets{?name,label}",
                }),
                _mock_response(json_data={"browser_download_url": "https://example.com/v.mp4"}),
            ]
            instagram_client.upload_video_to_github(str(video))


def test_post_reel_success():
    with patch.dict("os.environ", ENV):
        with patch("instagram_client.requests.post") as mock_post, \
             patch("instagram_client.requests.get") as mock_get, \
             patch("instagram_client.time.sleep"):

            mock_post.side_effect = [
                _mock_response(json_data={"id": "container_99"}),   # create container
                _mock_response(json_data={"id": "post_777"}),        # publish
            ]
            mock_get.return_value = _mock_response(
                json_data={"status_code": "FINISHED"}
            )

            result = instagram_client.post_reel("https://example.com/video.mp4", "caption")

    assert result == "post_777"


def test_post_reel_timeout_raises():
    with patch.dict("os.environ", ENV):
        with patch("instagram_client.requests.post") as mock_post, \
             patch("instagram_client.requests.get") as mock_get, \
             patch("instagram_client.time.sleep"):

            mock_post.return_value = _mock_response(json_data={"id": "container_1"})
            mock_get.return_value = _mock_response(json_data={"status_code": "IN_PROGRESS"})

            with pytest.raises(TimeoutError, match="120s"):
                instagram_client.post_reel("https://example.com/v.mp4", "caption")


def test_post_reel_error_status_raises():
    with patch.dict("os.environ", ENV):
        with patch("instagram_client.requests.post") as mock_post, \
             patch("instagram_client.requests.get") as mock_get, \
             patch("instagram_client.time.sleep"):

            mock_post.return_value = _mock_response(json_data={"id": "container_2"})
            mock_get.return_value = _mock_response(json_data={"status_code": "ERROR"})

            with pytest.raises(RuntimeError, match="ERROR"):
                instagram_client.post_reel("https://example.com/v.mp4", "caption")
```

- [ ] **Step 2: Run tests to confirm they fail**

Run: `pytest tests/test_instagram_client.py -v`
Expected: `ModuleNotFoundError: No module named 'instagram_client'`

- [ ] **Step 3: Implement `instagram_client.py`**

```python
# instagram_client.py
import os
import time

import requests

GRAPH_API_VERSION = "v21.0"
GRAPH_BASE = f"https://graph.facebook.com/{GRAPH_API_VERSION}"
RELEASE_TAG = "daily-video"
POLL_INTERVAL_SECONDS = 5
POLL_MAX_ATTEMPTS = 24  # 24 × 5s = 120s


def upload_video_to_github(video_path):
    """
    Delete the existing daily-video release (if any), create a new one,
    upload the video as an asset, and return the public download URL.
    """
    token = os.environ["GITHUB_TOKEN"]
    repo = os.environ["GITHUB_REPOSITORY"]
    api_base = f"https://api.github.com/repos/{repo}"
    headers = {
        "Authorization": f"token {token}",
        "Accept": "application/vnd.github.v3+json",
    }

    # Delete existing release + tag if present
    r = requests.get(f"{api_base}/releases/tags/{RELEASE_TAG}", headers=headers)
    if r.status_code == 200:
        release_id = r.json()["id"]
        requests.delete(f"{api_base}/releases/{release_id}", headers=headers)
        requests.delete(f"{api_base}/git/refs/tags/{RELEASE_TAG}", headers=headers)

    # Create fresh release
    r = requests.post(
        f"{api_base}/releases",
        headers=headers,
        json={
            "tag_name": RELEASE_TAG,
            "name": RELEASE_TAG,
            "draft": False,
            "prerelease": False,
        },
    )
    r.raise_for_status()
    upload_url = r.json()["upload_url"].replace("{?name,label}", "")

    # Upload video asset
    with open(video_path, "rb") as f:
        r = requests.post(
            f"{upload_url}?name=quiz_video.mp4",
            headers={**headers, "Content-Type": "video/mp4"},
            data=f,
        )
    r.raise_for_status()
    return r.json()["browser_download_url"]


def post_reel(video_url, caption):
    """
    Create an Instagram Reels container, wait for it to be ready,
    publish it, and return the published media ID.
    """
    user_id = os.environ["INSTAGRAM_USER_ID"]
    access_token = os.environ["INSTAGRAM_ACCESS_TOKEN"]

    # Create container
    r = requests.post(
        f"{GRAPH_BASE}/{user_id}/media",
        data={
            "media_type": "REELS",
            "video_url": video_url,
            "caption": caption,
            "access_token": access_token,
        },
    )
    r.raise_for_status()
    container_id = r.json()["id"]

    # Poll until FINISHED
    for _ in range(POLL_MAX_ATTEMPTS):
        time.sleep(POLL_INTERVAL_SECONDS)
        r = requests.get(
            f"{GRAPH_BASE}/{container_id}",
            params={"fields": "status_code", "access_token": access_token},
        )
        r.raise_for_status()
        status = r.json().get("status_code")
        if status == "FINISHED":
            break
        if status == "ERROR":
            raise RuntimeError("Instagram container failed with status: ERROR")
    else:
        raise TimeoutError(
            f"Instagram container did not finish within "
            f"{POLL_MAX_ATTEMPTS * POLL_INTERVAL_SECONDS}s"
        )

    # Publish
    r = requests.post(
        f"{GRAPH_BASE}/{user_id}/media_publish",
        data={"creation_id": container_id, "access_token": access_token},
    )
    r.raise_for_status()
    return r.json()["id"]
```

- [ ] **Step 4: Run tests to confirm they pass**

Run: `pytest tests/test_instagram_client.py -v`
Expected: 5 tests PASSED

- [ ] **Step 5: Commit**

```bash
git add instagram_client.py tests/test_instagram_client.py
git commit -m "feat: add Instagram + GitHub Release posting client"
```

---

## Chunk 4: Orchestrator and GitHub Actions workflow

### Task 9: Test and implement `automate.py`

**Files:**
- Create: `tests/test_automate.py`
- Create: `automate.py`

- [ ] **Step 1: Write failing tests**

```python
# tests/test_automate.py
import json
import sys
import pytest
from unittest.mock import patch, MagicMock

import automate


SCIENCE_CATEGORY = {
    "name": "Science",
    "day": 0,
    "hour_utc": 14,
    "description": "General science.",
    "hashtags": ["#sciencefacts", "#didyouknow"],
}

VALID_QUIZ = {
    "config": {"question_duration": 5, "reveal_duration": 2},
    "questions": [
        {"type": "standard", "question": "Q?", "answers": ["A", "B", "C"], "correct": [0]},
        {"type": "standard", "question": "Q?", "answers": ["A", "B", "C"], "correct": [0]},
        {"type": "standard", "question": "Q?", "answers": ["A", "B", "C"], "correct": [0]},
        {"type": "truefalse", "question": "Q?", "answers": ["True", "False"], "correct": [1]},
        {"type": "multi", "question": "Q?", "answers": ["A", "B", "C", "D"], "correct": [0, 2]},
    ],
}


def test_no_category_today_exits_cleanly():
    with patch("automate.SCHEDULE", {}):
        with patch("automate.datetime_utcnow", return_value=MagicMock(weekday=lambda: 0)):
            with pytest.raises(SystemExit) as exc:
                automate.main()
    assert exc.value.code == 0


def test_full_run_increments_counter():
    mock_increment = MagicMock()
    with patch("automate.SCHEDULE", {0: SCIENCE_CATEGORY}), \
         patch("automate.datetime_utcnow", return_value=MagicMock(weekday=lambda: 0)), \
         patch("automate.generate_quiz", return_value=VALID_QUIZ), \
         patch("automate.compile_quizvid"), \
         patch("automate.render_video", return_value="quiz_video.mp4"), \
         patch("automate.get_post_number", return_value=0), \
         patch("automate.build_caption", return_value="caption"), \
         patch("automate.upload_video_to_github", return_value="https://example.com/v.mp4"), \
         patch("automate.post_reel", return_value="post_id"), \
         patch("automate.increment", mock_increment), \
         patch("builtins.open", MagicMock()):
        automate.main()
    mock_increment.assert_called_once_with("Science")


def test_gemini_retries_on_failure():
    call_count = {"n": 0}

    def flaky_generate(cat):
        call_count["n"] += 1
        if call_count["n"] == 1:
            raise ValueError("bad json")
        return VALID_QUIZ

    with patch("automate.SCHEDULE", {0: SCIENCE_CATEGORY}), \
         patch("automate.datetime_utcnow", return_value=MagicMock(weekday=lambda: 0)), \
         patch("automate.generate_quiz", side_effect=flaky_generate), \
         patch("automate.compile_quizvid"), \
         patch("automate.render_video", return_value="quiz_video.mp4"), \
         patch("automate.get_post_number", return_value=0), \
         patch("automate.build_caption", return_value="caption"), \
         patch("automate.upload_video_to_github", return_value="https://example.com/v.mp4"), \
         patch("automate.post_reel", return_value="post_id"), \
         patch("automate.increment"), \
         patch("builtins.open", MagicMock()):
        automate.main()
    assert call_count["n"] == 2


def test_gemini_two_failures_exits_with_error():
    with patch("automate.SCHEDULE", {0: SCIENCE_CATEGORY}), \
         patch("automate.datetime_utcnow", return_value=MagicMock(weekday=lambda: 0)), \
         patch("automate.generate_quiz", side_effect=ValueError("bad")):
        with pytest.raises(SystemExit) as exc:
            automate.main()
    assert exc.value.code == 1


def test_counter_not_incremented_if_post_fails():
    mock_increment = MagicMock()
    with patch("automate.SCHEDULE", {0: SCIENCE_CATEGORY}), \
         patch("automate.datetime_utcnow", return_value=MagicMock(weekday=lambda: 0)), \
         patch("automate.generate_quiz", return_value=VALID_QUIZ), \
         patch("automate.compile_quizvid"), \
         patch("automate.render_video", return_value="quiz_video.mp4"), \
         patch("automate.get_post_number", return_value=0), \
         patch("automate.build_caption", return_value="caption"), \
         patch("automate.upload_video_to_github", return_value="https://example.com/v.mp4"), \
         patch("automate.post_reel", side_effect=RuntimeError("API error")), \
         patch("automate.increment", mock_increment), \
         patch("builtins.open", MagicMock()):
        with pytest.raises(SystemExit):
            automate.main()
    mock_increment.assert_not_called()
```

- [ ] **Step 2: Run tests to confirm they fail**

Run: `pytest tests/test_automate.py -v`
Expected: `ModuleNotFoundError: No module named 'automate'`

- [ ] **Step 3: Implement `automate.py`**

```python
# automate.py
import json
import sys
from datetime import datetime

from categories import SCHEDULE
from counters import get_post_number, increment
from gemini_client import generate_quiz
from caption import build_caption
from video_renderer import compile_quizvid, render_video
from instagram_client import upload_video_to_github, post_reel

QUIZ_FILE = "examples/daily_quiz.json"


def datetime_utcnow():
    """Thin wrapper around datetime.utcnow() — patched in tests."""
    return datetime.utcnow()


def main():
    today = datetime_utcnow().weekday()  # 0=Mon, 6=Sun
    category = SCHEDULE.get(today)

    if category is None:
        print(f"No category scheduled for today (weekday {today}). Exiting.")
        sys.exit(0)

    print(f"Category: {category['name']}")

    # 1. Generate quiz (retry once on failure)
    print("Generating quiz questions...")
    quiz_data = None
    for attempt in range(2):
        try:
            quiz_data = generate_quiz(category)
            break
        except Exception as e:
            print(f"Attempt {attempt + 1} failed: {e}")
    if quiz_data is None:
        print("Failed to generate valid quiz JSON after 2 attempts.")
        sys.exit(1)

    # 2. Write quiz file
    with open(QUIZ_FILE, "w") as f:
        json.dump(quiz_data, f, indent=2)
    print(f"Quiz written to {QUIZ_FILE}")

    # 3. Compile quizvid
    print("Compiling quizvid...")
    compile_quizvid()

    # 4. Render video
    print("Rendering video...")
    video_path = render_video()
    print(f"Video rendered: {video_path}")

    # 5. Build caption
    post_number = get_post_number(category["name"]) + 1
    caption = build_caption(category, post_number)
    print(f"Caption:\n{caption}\n")

    # 6. Upload video
    print("Uploading video to GitHub Releases...")
    video_url = upload_video_to_github(video_path)
    print(f"Video URL: {video_url}")

    # 7. Post to Instagram
    print("Posting to Instagram...")
    post_id = post_reel(video_url, caption)
    print(f"Posted! Media ID: {post_id}")

    # 8. Update counter (only after successful post)
    increment(category["name"])
    print("Counter updated.")


if __name__ == "__main__":
    main()
```

- [ ] **Step 4: Run tests to confirm they pass**

Run: `pytest tests/test_automate.py -v`
Expected: 5 tests PASSED

- [ ] **Step 5: Run full test suite**

Run: `pytest tests/ -v`
Expected: All tests PASSED

- [ ] **Step 6: Commit**

```bash
git add automate.py tests/test_automate.py
git commit -m "feat: add main orchestrator automate.py"
```

---

### Task 10: Create `.github/workflows/daily.yml`

**Files:**
- Create: `.github/workflows/daily.yml`

- [ ] **Step 1: Create workflow directory**

```bash
mkdir -p .github/workflows
```

- [ ] **Step 2: Write `daily.yml`**

```yaml
# .github/workflows/daily.yml
#
# Runs daily at 14:00 UTC for all current categories.
#
# NOTE: If you add a category with a different hour_utc in categories.py,
# add a second cron trigger here for that hour.

name: Daily Quiz Post

on:
  schedule:
    - cron: '0 14 * * *'
  workflow_dispatch:  # allow manual trigger for testing

permissions:
  contents: write

jobs:
  post:
    runs-on: ubuntu-latest

    steps:
      - name: Checkout repo
        uses: actions/checkout@v4

      - name: Cache apt packages
        uses: actions/cache@v4
        with:
          path: /var/cache/apt/archives
          key: apt-ffmpeg-ft-jsonc-${{ runner.os }}

      - name: Install build dependencies
        run: |
          sudo apt-get update -qq
          sudo apt-get install -y \
            libavformat-dev libavcodec-dev libavutil-dev \
            libswscale-dev libswresample-dev \
            libfreetype6-dev libjson-c-dev

      - name: Cache Piper TTS binary
        id: cache-piper
        uses: actions/cache@v4
        with:
          path: /usr/local/bin/piper
          key: piper-2023.11.14-2-x86_64

      - name: Download Piper TTS
        if: steps.cache-piper.outputs.cache-hit != 'true'
        run: |
          curl -L https://github.com/rhasspy/piper/releases/download/2023.11.14-2/piper_linux_x86_64.tar.gz \
            | tar -xz -C /tmp
          sudo mv /tmp/piper/piper /usr/local/bin/piper
          sudo chmod +x /usr/local/bin/piper

      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: '3.11'

      - name: Install Python dependencies
        run: pip install -r requirements.txt

      - name: Compile quizvid
        run: make

      - name: Configure git identity
        run: |
          git config user.email "github-actions[bot]@users.noreply.github.com"
          git config user.name "github-actions[bot]"

      - name: Run automation
        env:
          GEMINI_API_KEY: ${{ secrets.GEMINI_API_KEY }}
          INSTAGRAM_USER_ID: ${{ secrets.INSTAGRAM_USER_ID }}
          INSTAGRAM_ACCESS_TOKEN: ${{ secrets.INSTAGRAM_ACCESS_TOKEN }}
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
          GITHUB_REPOSITORY: ${{ github.repository }}
        run: python automate.py

      - name: Commit updated counters
        run: |
          git diff --quiet counters.json || (
            git add counters.json &&
            git commit -m "chore: update post counters" &&
            git push
          )
```

- [ ] **Step 3: Commit**

```bash
git add .github/workflows/daily.yml
git commit -m "feat: add GitHub Actions daily workflow"
```

---

### Task 11: Final verification

- [ ] **Step 1: Run full test suite one last time**

Run: `pytest tests/ -v`
Expected: All tests PASSED, no warnings

- [ ] **Step 2: Verify `config.json` uses `daily_quiz.json`**

Run: `grep quiz_file config.json`
Expected: `"quiz_file": "examples/daily_quiz.json"`

- [ ] **Step 3: Verify `counters.json` has all 7 categories**

Run: `cat counters.json`
Expected: JSON with all 7 category names, all at 0 (or their current values)

- [ ] **Step 4: Dry-run the binary with the placeholder quiz**

Run: `./bin/quizvid`
Expected: Video renders successfully to `quiz_video.mp4`

- [ ] **Step 5: Push all commits to `main`**

Before enabling the workflow, push everything and confirm the Actions tab shows
the workflow listed (but not yet triggered — it only fires at 14:00 UTC or via
`workflow_dispatch`).

- [ ] **Step 6: Add GitHub Secrets**

In repo Settings → Secrets and variables → Actions, add:
- `GEMINI_API_KEY` — from https://aistudio.google.com/
- `INSTAGRAM_USER_ID` — from Meta Developer App
- `INSTAGRAM_ACCESS_TOKEN` — system user token from Meta

- [ ] **Step 7: Test with `workflow_dispatch`**

In the Actions tab, manually trigger the workflow to confirm the full pipeline
runs end-to-end.
