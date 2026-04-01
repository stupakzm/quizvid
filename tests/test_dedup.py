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


def test_compute_quiz_hash_returns_hex_string():
    h = dedup.compute_quiz_hash(SAMPLE_QUIZ)
    assert isinstance(h, str)
    assert len(h) == 64  # SHA-256 hex digest is 64 chars


def test_compute_quiz_hash_same_questions_same_hash():
    h1 = dedup.compute_quiz_hash(SAMPLE_QUIZ)
    h2 = dedup.compute_quiz_hash(SAMPLE_QUIZ)
    assert h1 == h2


def test_compute_quiz_hash_ignores_config_metadata():
    quiz_with_diff_config = {
        "config": {"question_duration": 999, "reveal_duration": 999},
        "questions": SAMPLE_QUIZ["questions"],
    }
    h1 = dedup.compute_quiz_hash(SAMPLE_QUIZ)
    h2 = dedup.compute_quiz_hash(quiz_with_diff_config)
    assert h1 == h2


def test_compute_quiz_hash_differs_when_question_changes():
    h1 = dedup.compute_quiz_hash(SAMPLE_QUIZ)
    h2 = dedup.compute_quiz_hash(DIFFERENT_QUIZ)
    assert h1 != h2


def test_load_posted_quizzes_returns_empty_when_file_missing(tmp_path, monkeypatch):
    monkeypatch.setattr(dedup, "POSTED_QUIZZES_FILE", str(tmp_path / "nonexistent.json"))
    result = dedup.load_posted_quizzes()
    assert result == []


def test_load_posted_quizzes_returns_list_when_file_exists(tmp_path, monkeypatch):
    f = tmp_path / "posted_quizzes.json"
    records = [{"hash": "abc123", "category": "Science", "date": "2026-04-01"}]
    f.write_text(json.dumps(records))
    monkeypatch.setattr(dedup, "POSTED_QUIZZES_FILE", str(f))
    result = dedup.load_posted_quizzes()
    assert result == records


def test_is_duplicate_returns_false_when_no_matches(tmp_path, monkeypatch):
    monkeypatch.setattr(dedup, "POSTED_QUIZZES_FILE", str(tmp_path / "posted_quizzes.json"))
    assert dedup.is_duplicate(SAMPLE_QUIZ) is False


def test_is_duplicate_returns_true_when_hash_matches(tmp_path, monkeypatch):
    f = tmp_path / "posted_quizzes.json"
    h = dedup.compute_quiz_hash(SAMPLE_QUIZ)
    f.write_text(json.dumps([{"hash": h, "category": "Science", "date": "2026-04-01"}]))
    monkeypatch.setattr(dedup, "POSTED_QUIZZES_FILE", str(f))
    assert dedup.is_duplicate(SAMPLE_QUIZ) is True


def test_is_duplicate_returns_false_for_different_quiz(tmp_path, monkeypatch):
    f = tmp_path / "posted_quizzes.json"
    h = dedup.compute_quiz_hash(SAMPLE_QUIZ)
    f.write_text(json.dumps([{"hash": h, "category": "Science", "date": "2026-04-01"}]))
    monkeypatch.setattr(dedup, "POSTED_QUIZZES_FILE", str(f))
    assert dedup.is_duplicate(DIFFERENT_QUIZ) is False


def test_record_quiz_creates_file_if_missing(tmp_path, monkeypatch):
    f = tmp_path / "posted_quizzes.json"
    monkeypatch.setattr(dedup, "POSTED_QUIZZES_FILE", str(f))
    dedup.record_quiz(SAMPLE_QUIZ, "Science")
    assert f.exists()
    data = json.loads(f.read_text())
    assert len(data) == 1
    assert data[0]["hash"] == dedup.compute_quiz_hash(SAMPLE_QUIZ)
    assert data[0]["category"] == "Science"
    assert "date" in data[0]


def test_record_quiz_appends_to_existing_file(tmp_path, monkeypatch):
    f = tmp_path / "posted_quizzes.json"
    existing = [{"hash": "oldhash", "category": "History", "date": "2026-03-01"}]
    f.write_text(json.dumps(existing))
    monkeypatch.setattr(dedup, "POSTED_QUIZZES_FILE", str(f))
    dedup.record_quiz(SAMPLE_QUIZ, "Science")
    data = json.loads(f.read_text())
    assert len(data) == 2
    assert data[0]["hash"] == "oldhash"
    assert data[1]["hash"] == dedup.compute_quiz_hash(SAMPLE_QUIZ)


def test_get_question_texts_returns_list_of_strings():
    texts = dedup.get_question_texts(SAMPLE_QUIZ)
    assert isinstance(texts, list)
    assert len(texts) == 5
    assert texts[0] == "What is H2O?"
    assert all(isinstance(t, str) for t in texts)
