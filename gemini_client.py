# gemini_client.py
import json
import os

from google import genai


QUIZ_SCHEMA_EXAMPLE = """{
  "config": {"question_duration": 5, "reveal_duration": 2},
  "questions": [
    {
      "type": "standard",
      "question": "Example question?",
      "answers": ["Answer A", "Answer B", "Answer C", "Answer D"],
      "correct": [1]
    }
  ]
}"""


def generate_quiz(category):
    """Call Gemini API and return a validated quiz dict for the given category."""
    client = genai.Client(api_key=os.environ["GEMINI_API_KEY"])

    prompt = (
        f"Generate a quiz JSON for the category: {category['name']}.\n\n"
        f"Category scope: {category['description']}\n\n"
        "Return ONLY valid JSON matching this schema exactly. "
        "No markdown, no explanation, no extra text.\n\n"
        f"Schema example:\n{QUIZ_SCHEMA_EXAMPLE}\n\n"
        "Rules:\n"
        "- Exactly 5 questions\n"
        "- Distribution: 3 standard, 1 truefalse, 1 multi (order may vary)\n"
        "- Factually accurate answers\n"
        "- Answer text max ~3 words\n"
        "- standard: 3-5 answers, exactly 1 correct index\n"
        '- truefalse: answers must be exactly ["True", "False"], '
        "correct is [0] or [1]\n"
        "- multi: 4-6 answers, 2-3 correct indices\n"
    )

    response = client.models.generate_content(model="gemini-2.5-flash-lite", contents=prompt)
    return _parse_and_validate(response.text)


def _parse_and_validate(text):
    """Parse raw Gemini response text into a validated quiz dict."""
    text = text.strip()

    # Strip markdown code fences if present
    if text.startswith("```"):
        lines = text.split("\n")
        lines = lines[1:]
        if lines and lines[-1].strip() == "```":
            lines = lines[:-1]
        text = "\n".join(lines)

    data = json.loads(text)

    if "config" not in data:
        raise ValueError("Missing 'config' key")
    if "questions" not in data:
        raise ValueError("Missing 'questions' key")
    if len(data["questions"]) != 5:
        raise ValueError(f"Expected 5 questions, got {len(data['questions'])}")

    for i, q in enumerate(data["questions"]):
        if q.get("type") not in ("standard", "truefalse", "multi"):
            raise ValueError(f"Question {i}: invalid type '{q.get('type')}'")
        if not isinstance(q.get("question"), str):
            raise ValueError(f"Question {i}: 'question' must be a string")
        if not isinstance(q.get("answers"), list):
            raise ValueError(f"Question {i}: 'answers' must be a list")
        if not isinstance(q.get("correct"), list):
            raise ValueError(f"Question {i}: 'correct' must be a list")

        if q["type"] == "truefalse":
            if q["answers"] != ["True", "False"]:
                raise ValueError(
                    f"Question {i}: truefalse answers must be ['True', 'False']"
                )
            if len(q["correct"]) != 1:
                raise ValueError(f"Question {i}: truefalse must have 1 correct")
        elif q["type"] == "standard":
            if len(q["answers"]) < 3:
                raise ValueError(
                    f"Question {i}: standard must have at least 3 answers"
                )
            if len(q["correct"]) != 1:
                raise ValueError(f"Question {i}: standard must have 1 correct")
        elif q["type"] == "multi":
            if not (4 <= len(q["answers"]) <= 6):
                raise ValueError(
                    f"Question {i}: multi must have 4-6 answers"
                )
            if not (2 <= len(q["correct"]) <= 3):
                raise ValueError(
                    f"Question {i}: multi must have 2-3 correct"
                )

    return data
