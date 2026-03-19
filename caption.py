# caption.py
from datetime import datetime

HOOK_POOL = [
    "How many did you get right? 🧠",
    "What's your score? Comment below ⬇️",
    "Which was the hardest question? 👇",
    "Which one did you get wrong?",
    "Which one did you find easiest?",
    "Which one surprised you?",
    "Got all 5? Should questions be more difficult next time?",
    "How many did you know — be honest? 😅",
    "Which category do you want next week?",
]


def build_caption(category, post_number):
    """Build an Instagram caption for today's quiz post."""
    day_of_year = datetime.now().timetuple().tm_yday
    hook = HOOK_POOL[day_of_year % len(HOOK_POOL)]

    category_tag = f"#{category['name'].lower()}"
    extra_hashtags = " ".join(category["hashtags"])

    return (
        f"{hook}\n\n"
        f"Weekly {category['name']} Quiz #{post_number}\n\n"
        f"{category_tag} #quiz #quiztime #trivia {extra_hashtags}"
    )
