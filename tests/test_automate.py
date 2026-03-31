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

    def flaky_generate(cat, model=None):
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
         patch("automate.time.sleep"), \
         patch("builtins.open", MagicMock()):
        automate.main()
    assert call_count["n"] == 2


def test_gemini_two_failures_exits_with_error():
    with patch("automate.SCHEDULE", {0: SCIENCE_CATEGORY}), \
         patch("automate.datetime_utcnow", return_value=MagicMock(weekday=lambda: 0)), \
         patch("automate.generate_quiz", side_effect=ValueError("bad")), \
         patch("automate.time.sleep"):
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
