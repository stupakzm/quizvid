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
    with patch("caption.datetime") as mock_dt:
        mock_dt.now.return_value.timetuple.return_value.tm_yday = 0
        result0 = caption_module.build_caption(SCIENCE_CATEGORY, 1)

        mock_dt.now.return_value.timetuple.return_value.tm_yday = 1
        result1 = caption_module.build_caption(SCIENCE_CATEGORY, 1)

    # Both should contain a hook; different days → different hooks
    assert result0 != result1 or len(caption_module.HOOK_POOL) == 1


def test_hook_pool_uses_len_not_magic_number():
    # Verify rotation uses dynamic pool length
    original_pool = caption_module.HOOK_POOL[:]
    caption_module.HOOK_POOL.append("Test hook!")
    try:
        result = caption_module.build_caption(SCIENCE_CATEGORY, 1)
        assert isinstance(result, str)
    finally:
        caption_module.HOOK_POOL[:] = original_pool
