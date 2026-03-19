# instagram_client.py
import os
import time

import requests

GRAPH_API_VERSION = "v21.0"
GRAPH_BASE = f"https://graph.facebook.com/{GRAPH_API_VERSION}"
RELEASE_TAG = "daily-video"
POLL_INTERVAL_SECONDS = 5
POLL_MAX_ATTEMPTS = 24  # 24 × 5s = 120s


def upload_video_to_github(video_path):
    """
    Delete the existing daily-video release (if any), create a new one,
    upload the video as an asset, and return the public download URL.
    """
    token = os.environ["GITHUB_TOKEN"]
    repo = os.environ["GITHUB_REPOSITORY"]
    api_base = f"https://api.github.com/repos/{repo}"
    headers = {
        "Authorization": f"token {token}",
        "Accept": "application/vnd.github.v3+json",
    }

    # Delete existing release + tag if present
    r = requests.get(f"{api_base}/releases/tags/{RELEASE_TAG}", headers=headers)
    if r.status_code == 200:
        release_id = r.json()["id"]
        requests.delete(f"{api_base}/releases/{release_id}", headers=headers)
        requests.delete(f"{api_base}/git/refs/tags/{RELEASE_TAG}", headers=headers)

    # Create fresh release
    r = requests.post(
        f"{api_base}/releases",
        headers=headers,
        json={
            "tag_name": RELEASE_TAG,
            "name": RELEASE_TAG,
            "draft": False,
            "prerelease": False,
        },
    )
    r.raise_for_status()
    upload_url = r.json()["upload_url"].replace("{?name,label}", "")

    # Upload video asset
    with open(video_path, "rb") as f:
        r = requests.post(
            f"{upload_url}?name=quiz_video.mp4",
            headers={**headers, "Content-Type": "video/mp4"},
            data=f,
        )
    r.raise_for_status()
    return r.json()["browser_download_url"]


def post_reel(video_url, caption):
    """
    Create an Instagram Reels container, wait for it to be ready,
    publish it, and return the published media ID.
    """
    user_id = os.environ["INSTAGRAM_USER_ID"]
    access_token = os.environ["INSTAGRAM_ACCESS_TOKEN"]

    # Create container
    r = requests.post(
        f"{GRAPH_BASE}/{user_id}/media",
        data={
            "media_type": "REELS",
            "video_url": video_url,
            "caption": caption,
            "access_token": access_token,
        },
    )
    r.raise_for_status()
    container_id = r.json()["id"]

    # Poll until FINISHED
    for _ in range(POLL_MAX_ATTEMPTS):
        time.sleep(POLL_INTERVAL_SECONDS)
        r = requests.get(
            f"{GRAPH_BASE}/{container_id}",
            params={"fields": "status_code", "access_token": access_token},
        )
        r.raise_for_status()
        status = r.json().get("status_code")
        if status == "FINISHED":
            break
        if status == "ERROR":
            raise RuntimeError("Instagram container failed with status: ERROR")
    else:
        raise TimeoutError(
            f"Instagram container did not finish within "
            f"{POLL_MAX_ATTEMPTS * POLL_INTERVAL_SECONDS}s"
        )

    # Publish
    r = requests.post(
        f"{GRAPH_BASE}/{user_id}/media_publish",
        data={"creation_id": container_id, "access_token": access_token},
    )
    r.raise_for_status()
    return r.json()["id"]
