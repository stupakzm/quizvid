# automate.py
import json
import sys
from datetime import datetime

from categories import SCHEDULE
from counters import get_post_number, increment
from gemini_client import generate_quiz
from caption import build_caption
from video_renderer import compile_quizvid, render_video
from instagram_client import upload_video_to_github, post_reel

QUIZ_FILE = "examples/daily_quiz.json"


def datetime_utcnow():
    """Thin wrapper around datetime.utcnow() — patched in tests."""
    return datetime.utcnow()


def main():
    today = datetime_utcnow().weekday()  # 0=Mon, 6=Sun
    category = SCHEDULE.get(today)

    if category is None:
        print(f"No category scheduled for today (weekday {today}). Exiting.")
        sys.exit(0)

    print(f"Category: {category['name']}")

    # 1. Generate quiz (retry once on failure)
    print("Generating quiz questions...")
    quiz_data = None
    for attempt in range(2):
        try:
            quiz_data = generate_quiz(category)
            break
        except Exception as e:
            print(f"Attempt {attempt + 1} failed: {e}")
    if quiz_data is None:
        print("Failed to generate valid quiz JSON after 2 attempts.")
        sys.exit(1)

    # 2. Write quiz file
    with open(QUIZ_FILE, "w") as f:
        json.dump(quiz_data, f, indent=2)
    print(f"Quiz written to {QUIZ_FILE}")

    # 3. Compile quizvid
    print("Compiling quizvid...")
    compile_quizvid()

    # 4. Render video
    print("Rendering video...")
    video_path = render_video()
    print(f"Video rendered: {video_path}")

    # 5. Build caption
    post_number = get_post_number(category["name"]) + 1
    caption = build_caption(category, post_number)
    print(f"Caption:\n{caption}\n")

    # 6. Upload video
    print("Uploading video to GitHub Releases...")
    video_url = upload_video_to_github(video_path)
    print(f"Video URL: {video_url}")

    # 7. Post to Instagram
    print("Posting to Instagram...")
    try:
        post_id = post_reel(video_url, caption)
    except Exception as e:
        print(f"Failed to post to Instagram: {e}")
        sys.exit(1)
    print(f"Posted! Media ID: {post_id}")

    # 8. Update counter (only after successful post)
    increment(category["name"])
    print("Counter updated.")


if __name__ == "__main__":
    main()
