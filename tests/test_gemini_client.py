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

    with patch.dict("os.environ", {"GEMINI_API_KEY": "fake-key"}):
        with patch("gemini_client.genai") as mock_genai:
            mock_client = MagicMock()
            mock_genai.Client.return_value = mock_client
            mock_client.models.generate_content.return_value = _make_mock_response(json.dumps(VALID_QUIZ))
            result = gemini_client.generate_quiz(category)

    assert result["questions"][0]["type"] == "standard"
    prompt_arg = mock_client.models.generate_content.call_args[1]["contents"]
    assert "Science" in prompt_arg
    assert "General science topics" in prompt_arg
