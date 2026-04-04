# tests/test_instagram_client.py
import pytest
from unittest.mock import patch, MagicMock, mock_open, call
import requests

import instagram_client


ENV = {
    "GITHUB_TOKEN": "ghtoken",
    "GITHUB_REPOSITORY": "user/quizvid",
    "INSTAGRAM_USER_ID": "123456",
    "INSTAGRAM_ACCESS_TOKEN": "igtoken",
}


def _mock_response(status=200, json_data=None, raise_for_status=None):
    mock = MagicMock()
    mock.status_code = status
    mock.json.return_value = json_data or {}
    if raise_for_status:
        mock.raise_for_status.side_effect = raise_for_status
    else:
        mock.raise_for_status.return_value = None
    return mock


def _make_upload_mocks(branch_exists=False):
    """Return (mock_get, mock_post, mock_patch) side_effects for upload_video_to_github."""
    blob_resp = _mock_response(json_data={"sha": "blobsha123"})
    tree_resp = _mock_response(json_data={"sha": "treesha456"})
    commit_resp = _mock_response(json_data={"sha": "commitsha789"})
    post_side_effects = [blob_resp, tree_resp, commit_resp]

    # GET /git/ref/heads/video-release — exists or not
    get_resp = _mock_response(status=200 if branch_exists else 404)

    if branch_exists:
        patch_resp = _mock_response(json_data={"ref": "refs/heads/video-release"})
        return get_resp, post_side_effects, patch_resp
    else:
        create_ref_resp = _mock_response(json_data={"ref": "refs/heads/video-release"})
        post_side_effects.append(create_ref_resp)
        return get_resp, post_side_effects, None


def test_upload_video_no_existing_branch(tmp_path):
    video = tmp_path / "quiz_video.mp4"
    video.write_bytes(b"video")

    get_resp, post_side_effects, _ = _make_upload_mocks(branch_exists=False)

    with patch.dict("os.environ", ENV):
        with patch("instagram_client.requests.get", return_value=get_resp), \
             patch("instagram_client.requests.post", side_effect=post_side_effects), \
             patch("instagram_client.requests.patch") as mock_patch:

            url = instagram_client.upload_video_to_github(str(video))

    assert url == "https://raw.githubusercontent.com/user/quizvid/commitsha789/quiz_video.mp4"
    mock_patch.assert_not_called()


def test_upload_video_force_updates_existing_branch(tmp_path):
    video = tmp_path / "quiz_video.mp4"
    video.write_bytes(b"video")

    get_resp, post_side_effects, patch_resp = _make_upload_mocks(branch_exists=True)

    with patch.dict("os.environ", ENV):
        with patch("instagram_client.requests.get", return_value=get_resp), \
             patch("instagram_client.requests.post", side_effect=post_side_effects), \
             patch("instagram_client.requests.patch", return_value=patch_resp) as mock_patch:

            url = instagram_client.upload_video_to_github(str(video))

    assert url == "https://raw.githubusercontent.com/user/quizvid/commitsha789/quiz_video.mp4"
    mock_patch.assert_called_once()


def test_post_reel_success():
    with patch.dict("os.environ", ENV):
        with patch("instagram_client.requests.post") as mock_post, \
             patch("instagram_client.requests.get") as mock_get, \
             patch("instagram_client.time.sleep"):

            mock_post.side_effect = [
                _mock_response(json_data={"id": "container_99"}),   # create container
                _mock_response(json_data={"id": "post_777"}),        # publish
            ]
            mock_get.return_value = _mock_response(
                json_data={"status_code": "FINISHED"}
            )

            result = instagram_client.post_reel("https://example.com/video.mp4", "caption")

    assert result == "post_777"


def test_post_reel_timeout_raises():
    with patch.dict("os.environ", ENV):
        with patch("instagram_client.requests.post") as mock_post, \
             patch("instagram_client.requests.get") as mock_get, \
             patch("instagram_client.time.sleep"):

            mock_post.return_value = _mock_response(json_data={"id": "container_1"})
            mock_get.return_value = _mock_response(json_data={"status_code": "IN_PROGRESS"})

            with pytest.raises(TimeoutError, match="120s"):
                instagram_client.post_reel("https://example.com/v.mp4", "caption")


def test_post_reel_error_status_raises():
    with patch.dict("os.environ", ENV):
        with patch("instagram_client.requests.post") as mock_post, \
             patch("instagram_client.requests.get") as mock_get, \
             patch("instagram_client.time.sleep"):

            mock_post.return_value = _mock_response(json_data={"id": "container_2"})
            mock_get.return_value = _mock_response(json_data={"status_code": "ERROR"})

            with pytest.raises(RuntimeError, match="ERROR"):
                instagram_client.post_reel("https://example.com/v.mp4", "caption")
