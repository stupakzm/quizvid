# automate.py
import json
import os
import shutil
import sys
import time
from datetime import datetime


def _load_env_local(path=".env.local"):
    """Load key=value pairs from .env.local into os.environ (local dev only)."""
    if not os.path.exists(path):
        return
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, _, value = line.partition("=")
            os.environ.setdefault(key.strip(), value.strip())


_load_env_local()

from categories import SCHEDULE
from counters import get_post_number, increment
from dedup import is_duplicate, record_quiz, get_question_texts, get_past_questions
from gemini_client import generate_quiz, FALLBACK_MODELS
from caption import build_caption
from video_renderer import compile_quizvid, render_video
from instagram_client import upload_video_to_github, post_reel

QUIZ_FILE = "examples/daily_quiz.json"
OUTPUTS_DIR = "outputs"


def datetime_utcnow():
    """Thin wrapper around datetime.utcnow() — patched in tests."""
    return datetime.utcnow()


def save_outputs(video_path, quiz_data):
    """Save a timestamped copy of the video and quiz JSON to outputs/."""
    os.makedirs(OUTPUTS_DIR, exist_ok=True)
    ts = datetime_utcnow().strftime("%Y-%m-%d_%H-%M-%S")
    out_video = os.path.join(OUTPUTS_DIR, f"{ts}.mp4")
    out_json = os.path.join(OUTPUTS_DIR, f"{ts}.json")
    shutil.copy2(video_path, out_video)
    with open(out_json, "w") as f:
        json.dump(quiz_data, f, indent=2)
    print(f"Saved video:  {out_video}")
    print(f"Saved quiz:   {out_json}")
    return out_video, out_json


def main():
    dry_run = "--dry-run" in sys.argv

    today = datetime_utcnow().weekday()  # 0=Mon, 6=Sun
    category = SCHEDULE.get(today)

    if category is None:
        print(f"No category scheduled for today (weekday {today}). Exiting.")
        sys.exit(0)

    print(f"Category: {category['name']}")
    if dry_run:
        print("[DRY RUN] Instagram upload/post will be skipped.")

    # 1. Generate quiz (try primary model twice, then fallbacks once each)
    print("Generating quiz questions...")
    past_questions = get_past_questions(category["name"])
    if past_questions:
        print(f"Avoiding {len(past_questions)} past questions for {category['name']}.")

    quiz_data = None
    attempts = [FALLBACK_MODELS[0], FALLBACK_MODELS[0]] + FALLBACK_MODELS[1:]
    for i, model in enumerate(attempts):
        try:
            quiz_data = generate_quiz(category, model=model, avoid_questions=past_questions or None)
            break
        except Exception as e:
            print(f"Attempt {i + 1} ({model}) failed: {e}")
            if i < len(attempts) - 1:
                print("Waiting 15s before retry...")
                time.sleep(15)
    if quiz_data is None:
        print(f"Failed to generate valid quiz JSON after {len(attempts)} attempts.")
        sys.exit(1)

    # 1b. Dedup check — regenerate if duplicate (per D-04, D-05, D-06)
    MAX_DEDUP_RETRIES = 5
    accumulated_avoid = list(past_questions)
    for retry in range(MAX_DEDUP_RETRIES):
        if not is_duplicate(quiz_data, category_name=category["name"]):
            break
        print(f"Duplicate detected (retry {retry + 1}/{MAX_DEDUP_RETRIES}), regenerating...")
        accumulated_avoid += get_question_texts(quiz_data)
        dedup_model = FALLBACK_MODELS[2]  # escalate to gemini-2.5-flash for retries
        quiz_data = generate_quiz(category, model=dedup_model, avoid_questions=accumulated_avoid)
    else:
        if is_duplicate(quiz_data, category_name=category["name"]):
            print("All dedup retries exhausted — quiz is still a duplicate. Aborting.")
            sys.exit(1)

    # 2. Write quiz file
    with open(QUIZ_FILE, "w") as f:
        json.dump(quiz_data, f, indent=2)
    print(f"Quiz written to {QUIZ_FILE}")

    # 2b. Inject preview into config.json (per D-02)
    post_number = get_post_number(category["name"]) + 1
    with open("config.json") as f:
        render_config = json.load(f)
    render_config["preview"] = {
        "category": category["name"],
        "counter": post_number,
    }
    with open("config.json", "w") as f:
        json.dump(render_config, f, indent=2)
    print(f"Preview injected: {category['name']} #{post_number}")

    # 3. Compile quizvid
    print("Compiling quizvid...")
    compile_quizvid()

    # 4. Render video
    print("Rendering video...")
    video_path = render_video()
    print(f"Video rendered: {video_path}")

    # 5. Save outputs (always — useful for local review)
    save_outputs(video_path, quiz_data)

    # 6. Build caption
    caption = build_caption(category, post_number)
    print(f"Caption:\n{caption}\n")

    if dry_run:
        print("[DRY RUN] Skipping GitHub upload and Instagram post.")
        print("Done. Review the video in outputs/")
        return

    # 7. Upload video
    print("Uploading video to GitHub Releases...")
    video_url = upload_video_to_github(video_path)
    print(f"Video URL: {video_url}")

    # 8. Post to Instagram
    print("Posting to Instagram...")
    try:
        post_id = post_reel(video_url, caption)
    except Exception as e:
        print(f"Failed to post to Instagram: {e}")
        sys.exit(1)
    print(f"Posted! Media ID: {post_id}")

    # 9. Update counter (only after successful post)
    increment(category["name"])
    print("Counter updated.")

    # 10. Record quiz for dedup tracking (only after successful post)
    record_quiz(quiz_data, category["name"])
    print("Quiz recorded for dedup tracking.")


if __name__ == "__main__":
    main()
