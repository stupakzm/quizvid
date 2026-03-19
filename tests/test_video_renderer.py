# tests/test_video_renderer.py
import pytest
from unittest.mock import patch, MagicMock
import subprocess

import video_renderer


def _make_proc(returncode=0, stdout="", stderr=""):
    mock = MagicMock()
    mock.returncode = returncode
    mock.stdout = stdout
    mock.stderr = stderr
    return mock


def test_compile_quizvid_success():
    with patch("video_renderer.subprocess.run", return_value=_make_proc(0)):
        assert video_renderer.compile_quizvid() is True


def test_compile_quizvid_failure_raises():
    with patch("video_renderer.subprocess.run", return_value=_make_proc(1, stderr="error")):
        with pytest.raises(RuntimeError, match="make failed"):
            video_renderer.compile_quizvid()


def test_render_video_success(tmp_path):
    video_file = tmp_path / "quiz_video.mp4"
    video_file.write_bytes(b"fake")

    with patch("video_renderer.subprocess.run", return_value=_make_proc(0)):
        with patch("video_renderer.OUTPUT_VIDEO", str(video_file)):
            result = video_renderer.render_video()
    assert result == str(video_file)


def test_render_video_process_failure_raises():
    with patch("video_renderer.subprocess.run", return_value=_make_proc(1, stderr="crash")):
        with pytest.raises(RuntimeError, match="quizvid failed"):
            video_renderer.render_video()


def test_render_video_missing_output_raises(tmp_path):
    with patch("video_renderer.subprocess.run", return_value=_make_proc(0)):
        with patch("video_renderer.OUTPUT_VIDEO", str(tmp_path / "missing.mp4")):
            with pytest.raises(RuntimeError, match="not found after render"):
                video_renderer.render_video()
