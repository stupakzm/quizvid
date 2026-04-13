# tests/test_dedup.py
import json
import pytest

import dedup


SAMPLE_QUIZ = {
    "config": {"question_duration": 5, "reveal_duration": 2},
    "questions": [
        {"type": "standard", "question": "What is H2O?", "answers": ["Water", "Oxygen", "Hydrogen"], "correct": [0]},
        {"type": "standard", "question": "Speed of light?", "answers": ["300k km/s", "150k km/s", "600k km/s"], "correct": [0]},
        {"type": "standard", "question": "Largest planet?", "answers": ["Jupiter", "Saturn", "Earth"], "correct": [0]},
        {"type": "truefalse", "question": "Earth is flat?", "answers": ["True", "False"], "correct": [1]},
        {"type": "multi", "question": "Noble gases?", "answers": ["Argon", "Helium", "Nitrogen", "Oxygen"], "correct": [0, 1]},
    ],
}

DIFFERENT_QUIZ = {
    "config": {"question_duration": 5, "reveal_duration": 2},
    "questions": [
        {"type": "standard", "question": "Capital of France?", "answers": ["Paris", "London", "Berlin"], "correct": [0]},
        {"type": "standard", "question": "Sun color?", "answers": ["Yellow", "White", "Orange"], "correct": [1]},
        {"type": "standard", "question": "Smallest country?", "answers": ["Vatican", "Monaco", "Malta"], "correct": [0]},
        {"type": "truefalse", "question": "Moon is a planet?", "answers": ["True", "False"], "correct": [1]},
        {"type": "multi", "question": "Primary colors?", "answers": ["Red", "Blue", "Green", "Yellow"], "correct": [0, 1]},
    ],
}

# Same questions as SAMPLE_QUIZ but slightly rephrased
REPHRASED_QUIZ = {
    "config": {"question_duration": 5, "reveal_duration": 2},
    "questions": [
        {"type": "standard", "question": "What is the chemical formula H2O?", "answers": ["Water", "Oxygen", "Hydrogen"], "correct": [0]},
        {"type": "standard", "question": "What is the speed of light?", "answers": ["300k km/s", "150k km/s", "600k km/s"], "correct": [0]},
        {"type": "standard", "question": "Which is the largest planet?", "answers": ["Jupiter", "Saturn", "Earth"], "correct": [0]},
        {"type": "truefalse", "question": "Is the Earth flat?", "answers": ["True", "False"], "correct": [1]},
        {"type": "multi", "question": "Which are noble gases?", "answers": ["Argon", "Helium", "Nitrogen", "Oxygen"], "correct": [0, 1]},
    ],
}


def _make_posted_file(tmp_path, quizzes):
    """Write a posted_quizzes.json with the given list of quiz_data dicts."""
    f = tmp_path / "posted_quizzes.json"
    records = [
        {"questions": dedup.get_question_texts(q), "category": "Test", "date": "2026-04-01"}
        for q in quizzes
    ]
    f.write_text(json.dumps(records))
    return f


# --- load / save ---

def test_load_posted_quizzes_returns_empty_when_file_missing(tmp_path, monkeypatch):
    monkeypatch.setattr(dedup, "POSTED_QUIZZES_FILE", str(tmp_path / "nonexistent.json"))
    assert dedup.load_posted_quizzes() == []


def test_load_posted_quizzes_returns_list_when_file_exists(tmp_path, monkeypatch):
    f = tmp_path / "posted_quizzes.json"
    records = [{"questions": ["Q1"], "category": "Science", "date": "2026-04-01"}]
    f.write_text(json.dumps(records))
    monkeypatch.setattr(dedup, "POSTED_QUIZZES_FILE", str(f))
    assert dedup.load_posted_quizzes() == records


# --- is_duplicate ---

def test_is_duplicate_returns_false_when_no_history(tmp_path, monkeypatch):
    monkeypatch.setattr(dedup, "POSTED_QUIZZES_FILE", str(tmp_path / "posted_quizzes.json"))
    assert dedup.is_duplicate(SAMPLE_QUIZ) is False


def test_is_duplicate_returns_true_for_exact_repeat(tmp_path, monkeypatch):
    f = _make_posted_file(tmp_path, [SAMPLE_QUIZ])
    monkeypatch.setattr(dedup, "POSTED_QUIZZES_FILE", str(f))
    assert dedup.is_duplicate(SAMPLE_QUIZ) is True


def test_is_duplicate_returns_false_for_completely_different_quiz(tmp_path, monkeypatch):
    f = _make_posted_file(tmp_path, [SAMPLE_QUIZ])
    monkeypatch.setattr(dedup, "POSTED_QUIZZES_FILE", str(f))
    assert dedup.is_duplicate(DIFFERENT_QUIZ) is False


def test_is_duplicate_catches_rephrased_questions(tmp_path, monkeypatch):
    f = _make_posted_file(tmp_path, [SAMPLE_QUIZ])
    monkeypatch.setattr(dedup, "POSTED_QUIZZES_FILE", str(f))
    assert dedup.is_duplicate(REPHRASED_QUIZ) is True


def test_is_duplicate_respects_overlap_threshold(tmp_path, monkeypatch):
    """Quiz sharing fewer questions than OVERLAP_THRESHOLD should not be flagged."""
    monkeypatch.setattr(dedup, "POSTED_QUIZZES_FILE", str(tmp_path / "posted_quizzes.json"))
    monkeypatch.setattr(dedup, "OVERLAP_THRESHOLD", 4)

    # Only 2 questions overlap with SAMPLE_QUIZ history
    partial_overlap_quiz = {
        "questions": [
            {"question": "What is H2O?"},          # matches
            {"question": "Speed of light?"},        # matches
            {"question": "Capital of France?"},     # new
            {"question": "Smallest country?"},      # new
            {"question": "Primary colors?"},        # new
        ]
    }
    f = _make_posted_file(tmp_path, [SAMPLE_QUIZ])
    monkeypatch.setattr(dedup, "POSTED_QUIZZES_FILE", str(f))
    assert dedup.is_duplicate(partial_overlap_quiz) is False


# --- record_quiz ---

def test_record_quiz_creates_file_if_missing(tmp_path, monkeypatch):
    f = tmp_path / "posted_quizzes.json"
    monkeypatch.setattr(dedup, "POSTED_QUIZZES_FILE", str(f))
    dedup.record_quiz(SAMPLE_QUIZ, "Science")
    assert f.exists()
    data = json.loads(f.read_text())
    assert len(data) == 1
    assert data[0]["questions"] == dedup.get_question_texts(SAMPLE_QUIZ)
    assert data[0]["category"] == "Science"
    assert "date" in data[0]


def test_record_quiz_appends_to_existing_file(tmp_path, monkeypatch):
    f = _make_posted_file(tmp_path, [DIFFERENT_QUIZ])
    monkeypatch.setattr(dedup, "POSTED_QUIZZES_FILE", str(f))
    dedup.record_quiz(SAMPLE_QUIZ, "Science")
    data = json.loads(f.read_text())
    assert len(data) == 2
    assert data[1]["questions"] == dedup.get_question_texts(SAMPLE_QUIZ)


# --- get_question_texts ---

def test_get_question_texts_returns_list_of_strings():
    texts = dedup.get_question_texts(SAMPLE_QUIZ)
    assert isinstance(texts, list)
    assert len(texts) == 5
    assert texts[0] == "What is H2O?"
    assert all(isinstance(t, str) for t in texts)
