# video_renderer.py
import os
import subprocess

OUTPUT_VIDEO = "quiz_video.mp4"


def compile_quizvid():
    """Run `make` to compile the quizvid binary. Raises RuntimeError on failure."""
    result = subprocess.run(["make"], capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(f"make failed:\n{result.stderr}")
    return True


def render_video(config_file="config.json"):
    """Run the quizvid binary to render the video. Returns path to output file."""
    result = subprocess.run(
        ["./bin/quizvid", config_file],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(f"quizvid failed:\n{result.stderr}")
    if not os.path.exists(OUTPUT_VIDEO):
        raise RuntimeError(f"{OUTPUT_VIDEO} not found after render")
    return OUTPUT_VIDEO
