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


def test_upload_video_no_existing_release(tmp_path):
    video = tmp_path / "quiz_video.mp4"
    video.write_bytes(b"video")

    with patch.dict("os.environ", ENV):
        with patch("instagram_client.requests.get") as mock_get, \
             patch("instagram_client.requests.post") as mock_post, \
             patch("instagram_client.requests.delete") as mock_delete:

            # No existing release (404)
            mock_get.return_value = _mock_response(status=404)
            # Create release response, then asset upload response
            mock_post.side_effect = [
                _mock_response(
                    json_data={
                        "upload_url": "https://uploads.github.com/repos/user/quizvid/releases/1/assets{?name,label}",
                        "id": 1,
                    }
                ),
                _mock_response(json_data={"browser_download_url": "https://example.com/video.mp4"}),
            ]

            url = instagram_client.upload_video_to_github(str(video))

    assert url == "https://example.com/video.mp4"
    mock_delete.assert_not_called()


def test_upload_video_deletes_existing_release(tmp_path):
    video = tmp_path / "quiz_video.mp4"
    video.write_bytes(b"video")

    with patch.dict("os.environ", ENV):
        with patch("instagram_client.requests.get") as mock_get, \
             patch("instagram_client.requests.post") as mock_post, \
             patch("instagram_client.requests.delete"):

            mock_get.return_value = _mock_response(json_data={"id": 42})
            mock_post.side_effect = [
                _mock_response(json_data={
                    "upload_url": "https://uploads.github.com/repos/user/quizvid/releases/2/assets{?name,label}",
                }),
                _mock_response(json_data={"browser_download_url": "https://example.com/v.mp4"}),
            ]
            instagram_client.upload_video_to_github(str(video))


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
