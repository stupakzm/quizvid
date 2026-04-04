# instagram_client.py
import base64
import os
import time

import requests

GRAPH_API_VERSION = "v21.0"
GRAPH_BASE = f"https://graph.facebook.com/{GRAPH_API_VERSION}"
VIDEO_BRANCH = "video-release"
POLL_INTERVAL_SECONDS = 5
POLL_MAX_ATTEMPTS = 24  # 24 × 5s = 120s


def upload_video_to_github(video_path):
    """
    Push the video to the video-release branch via the Git Data API and
    return a raw.githubusercontent.com URL — a direct, non-redirect,
    permanent URL that Instagram's async media ingestion can reliably fetch.

    GitHub release asset URLs redirect to expiring S3 presigned URLs (~5 min).
    raw.githubusercontent.com serves content directly with no redirect.
    Using the commit SHA in the URL ensures no stale CDN cache issues.
    """
    token = os.environ["GITHUB_TOKEN"]
    repo = os.environ["GITHUB_REPOSITORY"]
    api_base = f"https://api.github.com/repos/{repo}"
    headers = {
        "Authorization": f"token {token}",
        "Accept": "application/vnd.github.v3+json",
    }

    # 1. Create a git blob for the video
    with open(video_path, "rb") as f:
        content_b64 = base64.b64encode(f.read()).decode()
    r = requests.post(f"{api_base}/git/blobs", headers=headers, json={
        "content": content_b64,
        "encoding": "base64",
    })
    r.raise_for_status()
    blob_sha = r.json()["sha"]

    # 2. Create a tree containing only the video file
    r = requests.post(f"{api_base}/git/trees", headers=headers, json={
        "tree": [{"path": "quiz_video.mp4", "mode": "100644", "type": "blob", "sha": blob_sha}],
    })
    r.raise_for_status()
    tree_sha = r.json()["sha"]

    # 3. Create an orphan commit (no parent — keeps the branch history clean)
    r = requests.post(f"{api_base}/git/commits", headers=headers, json={
        "message": "Daily video",
        "tree": tree_sha,
        "parents": [],
    })
    r.raise_for_status()
    commit_sha = r.json()["sha"]

    # 4. Force-update (or create) the video-release branch
    r = requests.get(f"{api_base}/git/ref/heads/{VIDEO_BRANCH}", headers=headers)
    if r.status_code == 200:
        r = requests.patch(f"{api_base}/git/refs/heads/{VIDEO_BRANCH}", headers=headers, json={
            "sha": commit_sha,
            "force": True,
        })
    else:
        r = requests.post(f"{api_base}/git/refs", headers=headers, json={
            "ref": f"refs/heads/{VIDEO_BRANCH}",
            "sha": commit_sha,
        })
    r.raise_for_status()

    # Use commit SHA (not branch name) to bypass CDN caching of stale content
    download_url = f"https://raw.githubusercontent.com/{repo}/{commit_sha}/quiz_video.mp4"
    print(f"Video URL: {download_url}")
    return download_url


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
    if not r.ok:
        raise RuntimeError(f"Instagram container creation failed {r.status_code}: {r.text}")
    container_id = r.json()["id"]

    # Poll until FINISHED
    for _ in range(POLL_MAX_ATTEMPTS):
        time.sleep(POLL_INTERVAL_SECONDS)
        r = requests.get(
            f"{GRAPH_BASE}/{container_id}",
            params={"fields": "status_code,status", "access_token": access_token},
        )
        r.raise_for_status()
        data = r.json()
        status = data.get("status_code")
        if status == "FINISHED":
            break
        if status == "ERROR":
            detail = data.get("status", "no detail")
            raise RuntimeError(f"Instagram container failed with status: ERROR — {detail}")
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
