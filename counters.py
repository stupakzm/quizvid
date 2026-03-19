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
