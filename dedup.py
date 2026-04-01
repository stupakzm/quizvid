# dedup.py
import hashlib
import json
import os
from datetime import datetime

POSTED_QUIZZES_FILE = "posted_quizzes.json"


def load_posted_quizzes():
    """Load list of posted quiz records. Returns [] if file missing."""
    if os.path.exists(POSTED_QUIZZES_FILE):
        with open(POSTED_QUIZZES_FILE) as f:
            return json.load(f)
    return []


def save_posted_quizzes(data):
    with open(POSTED_QUIZZES_FILE, "w") as f:
        json.dump(data, f, indent=2)


def compute_quiz_hash(quiz_data):
    """SHA-256 hash of the questions array only (per D-01)."""
    payload = json.dumps(quiz_data["questions"], sort_keys=True, ensure_ascii=False).encode()
    return hashlib.sha256(payload).hexdigest()


def is_duplicate(quiz_data):
    """Check if quiz questions hash matches any previously posted quiz."""
    current_hash = compute_quiz_hash(quiz_data)
    posted = load_posted_quizzes()
    return any(entry["hash"] == current_hash for entry in posted)


def record_quiz(quiz_data, category_name):
    """Append quiz record after successful post (per D-02)."""
    posted = load_posted_quizzes()
    posted.append({
        "hash": compute_quiz_hash(quiz_data),
        "category": category_name,
        "date": datetime.utcnow().strftime("%Y-%m-%d"),
    })
    save_posted_quizzes(posted)


def get_question_texts(quiz_data):
    """Extract question text strings for avoid_questions retry prompt."""
    return [q["question"] for q in quiz_data["questions"]]
