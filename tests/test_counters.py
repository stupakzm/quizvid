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
