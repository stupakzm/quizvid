# dedup.py
import json
import os
from datetime import datetime, timezone

from rapidfuzz import fuzz

POSTED_QUIZZES_FILE = "posted_quizzes.json"
SIMILARITY_THRESHOLD = 75  # WRatio score (0–100); catches rephrasings, not unrelated topics
OVERLAP_THRESHOLD = 3      # number of similar questions before flagged as duplicate


def load_posted_quizzes():
    """Load list of posted quiz records. Returns [] if file missing."""
    if os.path.exists(POSTED_QUIZZES_FILE):
        with open(POSTED_QUIZZES_FILE) as f:
            return json.load(f)
    return []


def save_posted_quizzes(data):
    with open(POSTED_QUIZZES_FILE, "w") as f:
        json.dump(data, f, indent=2)


def _questions_are_similar(q1: str, q2: str) -> bool:
    return fuzz.WRatio(q1, q2) >= SIMILARITY_THRESHOLD


def is_duplicate(quiz_data, category_name=None):
    """
    Check if the quiz overlaps with previously posted quizzes.
    A quiz is considered a duplicate if OVERLAP_THRESHOLD or more of its
    questions are similar (fuzzy match) to any question in any past quiz.
    Pass category_name to restrict comparison to the same category only.
    """
    posted = load_posted_quizzes()
    all_past_questions = [
        q for entry in posted
        if (category_name is None or entry.get("category") == category_name)
        for q in entry.get("questions", [])
    ]

    current_questions = get_question_texts(quiz_data)
    overlap = sum(
        1
        for cq in current_questions
        if any(_questions_are_similar(cq, pq) for pq in all_past_questions)
    )
    return overlap >= OVERLAP_THRESHOLD


def record_quiz(quiz_data, category_name):
    """Append quiz record after successful post."""
    posted = load_posted_quizzes()
    posted.append({
        "questions": get_question_texts(quiz_data),
        "category": category_name,
        "date": datetime.now(timezone.utc).strftime("%Y-%m-%d"),
    })
    save_posted_quizzes(posted)


def get_question_texts(quiz_data):
    """Extract question text strings for avoid_questions retry prompt."""
    return [q["question"] for q in quiz_data["questions"]]


def get_past_questions(category_name=None):
    """Return all recorded question texts, optionally filtered by category."""
    posted = load_posted_quizzes()
    return [
        q
        for entry in posted
        if (category_name is None or entry.get("category") == category_name)
        for q in entry.get("questions", [])
    ]
