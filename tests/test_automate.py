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
         patch("automate.is_duplicate", return_value=False), \
         patch("automate.record_quiz"), \
         patch("automate.compile_quizvid"), \
         patch("automate.render_video", return_value="quiz_video.mp4"), \
         patch("automate.shutil.copy2"), \
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

    def flaky_generate(cat, model=None, avoid_questions=None):
        call_count["n"] += 1
        if call_count["n"] == 1:
            raise ValueError("bad json")
        return VALID_QUIZ

    with patch("automate.SCHEDULE", {0: SCIENCE_CATEGORY}), \
         patch("automate.datetime_utcnow", return_value=MagicMock(weekday=lambda: 0)), \
         patch("automate.generate_quiz", side_effect=flaky_generate), \
         patch("automate.is_duplicate", return_value=False), \
         patch("automate.record_quiz"), \
         patch("automate.compile_quizvid"), \
         patch("automate.render_video", return_value="quiz_video.mp4"), \
         patch("automate.shutil.copy2"), \
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
         patch("automate.is_duplicate", return_value=False), \
         patch("automate.record_quiz"), \
         patch("automate.compile_quizvid"), \
         patch("automate.render_video", return_value="quiz_video.mp4"), \
         patch("automate.shutil.copy2"), \
         patch("automate.get_post_number", return_value=0), \
         patch("automate.build_caption", return_value="caption"), \
         patch("automate.upload_video_to_github", return_value="https://example.com/v.mp4"), \
         patch("automate.post_reel", side_effect=RuntimeError("API error")), \
         patch("automate.increment", mock_increment), \
         patch("builtins.open", MagicMock()):
        with pytest.raises(SystemExit):
            automate.main()
    mock_increment.assert_not_called()


def test_duplicate_quiz_triggers_regeneration():
    """Mock is_duplicate returning True once then False — generate_quiz called twice."""
    is_dup_calls = {"n": 0}

    def is_dup_side_effect(quiz):
        is_dup_calls["n"] += 1
        return is_dup_calls["n"] == 1  # True first time, False second

    mock_generate = MagicMock(return_value=VALID_QUIZ)
    with patch("automate.SCHEDULE", {0: SCIENCE_CATEGORY}), \
         patch("automate.datetime_utcnow", return_value=MagicMock(weekday=lambda: 0)), \
         patch("automate.generate_quiz", mock_generate), \
         patch("automate.is_duplicate", side_effect=is_dup_side_effect), \
         patch("automate.get_question_texts", return_value=["Q1", "Q2"]), \
         patch("automate.record_quiz"), \
         patch("automate.compile_quizvid"), \
         patch("automate.render_video", return_value="quiz_video.mp4"), \
         patch("automate.shutil.copy2"), \
         patch("automate.get_post_number", return_value=0), \
         patch("automate.build_caption", return_value="caption"), \
         patch("automate.upload_video_to_github", return_value="https://example.com/v.mp4"), \
         patch("automate.post_reel", return_value="post_id"), \
         patch("automate.increment"), \
         patch("builtins.open", MagicMock()):
        automate.main()
    # Called once for initial generation + once for dedup retry
    assert mock_generate.call_count == 2
    # Second call must include avoid_questions
    _, kwargs = mock_generate.call_args
    assert "avoid_questions" in kwargs
    assert kwargs["avoid_questions"] == ["Q1", "Q2"]


def test_all_dedup_retries_exhausted_exits():
    """Mock is_duplicate always returning True — pipeline exits with code 1."""
    with patch("automate.SCHEDULE", {0: SCIENCE_CATEGORY}), \
         patch("automate.datetime_utcnow", return_value=MagicMock(weekday=lambda: 0)), \
         patch("automate.generate_quiz", return_value=VALID_QUIZ), \
         patch("automate.is_duplicate", return_value=True), \
         patch("automate.get_question_texts", return_value=["Q1", "Q2"]):
        with pytest.raises(SystemExit) as exc:
            automate.main()
    assert exc.value.code == 1


def test_record_quiz_called_after_successful_post():
    """record_quiz is called once with quiz_data and category name after successful post."""
    mock_record = MagicMock()
    with patch("automate.SCHEDULE", {0: SCIENCE_CATEGORY}), \
         patch("automate.datetime_utcnow", return_value=MagicMock(weekday=lambda: 0)), \
         patch("automate.generate_quiz", return_value=VALID_QUIZ), \
         patch("automate.is_duplicate", return_value=False), \
         patch("automate.record_quiz", mock_record), \
         patch("automate.compile_quizvid"), \
         patch("automate.render_video", return_value="quiz_video.mp4"), \
         patch("automate.shutil.copy2"), \
         patch("automate.get_post_number", return_value=0), \
         patch("automate.build_caption", return_value="caption"), \
         patch("automate.upload_video_to_github", return_value="https://example.com/v.mp4"), \
         patch("automate.post_reel", return_value="post_id"), \
         patch("automate.increment"), \
         patch("builtins.open", MagicMock()):
        automate.main()
    mock_record.assert_called_once_with(VALID_QUIZ, "Science")


def test_record_quiz_not_called_if_post_fails():
    """record_quiz is NOT called when post_reel raises an exception."""
    mock_record = MagicMock()
    with patch("automate.SCHEDULE", {0: SCIENCE_CATEGORY}), \
         patch("automate.datetime_utcnow", return_value=MagicMock(weekday=lambda: 0)), \
         patch("automate.generate_quiz", return_value=VALID_QUIZ), \
         patch("automate.is_duplicate", return_value=False), \
         patch("automate.record_quiz", mock_record), \
         patch("automate.compile_quizvid"), \
         patch("automate.render_video", return_value="quiz_video.mp4"), \
         patch("automate.shutil.copy2"), \
         patch("automate.get_post_number", return_value=0), \
         patch("automate.build_caption", return_value="caption"), \
         patch("automate.upload_video_to_github", return_value="https://example.com/v.mp4"), \
         patch("automate.post_reel", side_effect=RuntimeError("API error")), \
         patch("automate.increment"), \
         patch("builtins.open", MagicMock()):
        with pytest.raises(SystemExit):
            automate.main()
    mock_record.assert_not_called()
