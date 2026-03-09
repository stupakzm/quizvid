# QuizVid

Generate quiz videos with AI voiceover for social media (Instagram, TikTok, YouTube Shorts).

## Features

- **Multiple question types**: Standard (2-6 answers), True/False, Multiple correct answers
- **AI voiceover**: Local neural TTS using Piper
- **Smart timing**: Auto-adjusts to audio length with configurable reveal duration
- **Background audio**: Ambient sounds with volume ducking during speech
- **Animations**: Smooth fade-in effects for questions and answers
- **Colorblind-friendly**: Multiple color schemes including accessible gold/purple
- **Professional output**: 1080×1920 MP4 with H.264 video + AAC audio

## Dependencies

### Required Packages
```bash
sudo apt update
sudo apt install -y \
  build-essential \
  gcc \
  make \
  pkg-config \
  libavcodec-dev \
  libavformat-dev \
  libavutil-dev \
  libswscale-dev \
  libswresample-dev \
  libfreetype6-dev \
  libjson-c-dev \
  ffmpeg
```

### Piper TTS

Download and install Piper:
```bash
cd ~/Downloads
wget https://github.com/rhasspy/piper/releases/download/v1.2.0/piper_amd64.tar.gz
tar xzf piper_amd64.tar.gz
cd piper

# Install to system
sudo cp lib*.so* /usr/local/lib/
sudo mkdir -p /usr/local/share/espeak-ng-data
sudo cp -r espeak-ng-data/* /usr/local/share/espeak-ng-data/
sudo cp piper /usr/local/bin/
sudo ldconfig

# Create symlink for espeak data
sudo ln -s /usr/local/share/espeak-ng-data /usr/share/espeak-ng-data

# Verify installation
piper --version
```

### Voice Model

Download at least one voice model:
```bash
mkdir -p assets/voices
cd assets/voices

# Download English US voice (Lessac - female, clear)
wget https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/en/en_US/lessac/medium/en_US-lessac-medium.onnx
wget https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/en/en_US/lessac/medium/en_US-lessac-medium.onnx.json
```

**More voices:** https://huggingface.co/rhasspy/piper-voices

## Setup

1. **Clone repository:**
```bash
git clone https://github.com/stupakzm/quizvid.git
cd quizvid
```

2. **Create required directories:**
```bash
mkdir -p assets/fonts assets/voices assets/audio
mkdir -p build bin examples
```

3. **Download font:**
```bash
cd assets/fonts
wget https://github.com/google/fonts/raw/main/apache/roboto/static/Roboto-Bold.ttf
cd ../..
```

4. **Build:**
```bash
make
```

## Usage

### Basic Example

1. **Create a quiz file** (`examples/my_quiz.json`):
```json
{
  "config": {
    "question_duration": 5,
    "reveal_duration": 2
  },
  "questions": [
    {
      "type": "standard",
      "question": "What is 2 + 2?",
      "answers": ["3", "4", "5", "6"],
      "correct": [1]
    },
    {
      "type": "truefalse",
      "question": "The Earth is flat.",
      "answers": ["True", "False"],
      "correct": [1]
    },
    {
      "type": "multi",
      "question": "Which are primary colors?",
      "answers": ["Red", "Green", "Blue", "Purple"],
      "correct": [0, 1, 2]
    }
  ]
}
```

2. **Configure** (`config.json`):
```json
{
  "video": {
    "width": 1080,
    "height": 1920,
    "fps": 30
  },
  "audio": {
    "enabled": true,
    "engine": "piper",
    "voice_model": "assets/voices/en_US-lessac-medium.onnx",
    "speed": 1.0,
    "sample_rate": 22050,
    "background": {
      "enabled": false,
      "file": "assets/audio/ticking.wav",
      "volume_with_voice": 0.15,
      "volume_without_voice": 0.35
    }
  },
  "timing": {
    "reveal_duration": 2.0,
    "transition_duration": 0.3,
    "think_duration": 2.0
  },
  "appearance": {
    "color_scheme": "colorblind",
    "font_path": "assets/fonts/Roboto-Bold.ttf"
  },
  "input": {
    "quiz_file": "examples/my_quiz.json"
  },
  "output": {
    "file": "quiz_video.mp4"
  }
}
```

3. **Generate video:**
```bash
make run
```

4. **Preview:**
```bash
ffplay quiz_video.mp4
```

### Makefile Commands
```bash
make          # Build project
make run      # Build and run
make test     # Build, run, and play video
make clean    # Remove build artifacts
make rebuild  # Clean, build, run, and play
```

## Configuration

### Question Types

- **`standard`**: 2-6 answers, single correct (most common)
- **`truefalse`**: Always True/False (auto-generated answers)
- **`multi`**: 2-6 answers, multiple correct (shows "Multiple correct" hint)

### Color Schemes

- **`colorblind`**: Gold and purple (accessible, default)
- **`grayscale`**: Minimalistic black/white/gray
- **`default`**: Blue and green (original)

### Timing System

Video duration auto-adjusts based on:
1. **Animation time**: Time for all elements to fade in (~1.7s)
2. **Audio duration**: Voiceover length
3. **Think time**: Pause after answers visible (configurable)
4. **Reveal duration**: Show correct answer (configurable)

**Formula:** `total_time = MAX(audio, animation) + think_time + reveal_time`

### Background Audio

Optional ambient sounds (e.g., ticking clock):

1. **Convert to mono 22050 Hz:**
```bash
ffmpeg -i input.wav -ar 22050 -ac 1 assets/audio/ticking.wav
```

2. **Enable in config.json:**
```json
"background": {
  "enabled": true,
  "file": "assets/audio/ticking.wav",
  "volume_with_voice": 0.15,
  "volume_without_voice": 0.35
}
```

**Behavior:**
- Quiet during voiceover
- Louder during animations/think time  
- Silent during reveal

## Project Structure
```
quizvid/
├── src/              # C source files
├── include/          # Header files
├── build/            # Compiled objects
├── bin/              # Executable
├── assets/
│   ├── fonts/        # Font files (Roboto-Bold.ttf)
│   ├── voices/       # Piper TTS voice models
│   └── audio/        # Background audio files
├── examples/         # Sample quiz JSON files
├── config.json       # Main configuration
├── Makefile          # Build system
└── README.md
```

## Architecture

- **video.c**: Video encoding (H.264), RGB→YUV conversion, timer bar
- **audio.c**: TTS generation (Piper), WAV loading, audio mixing
- **text.c**: FreeType text rendering with alpha blending
- **quiz.c**: Question parsing, timing calculation, frame rendering
- **colors.c**: Color scheme system
- **config.c**: JSON configuration parsing
- **muxer.c**: Audio/video synchronization and MP4 output
- **main.c**: Pipeline orchestration

## Troubleshooting

**Piper not found:**
```bash
which piper
piper --version
```

**Audio artifacts/distortion:**
- Ensure background audio is 22050 Hz mono
- Use: `ffmpeg -i input.wav -ar 22050 -ac 1 output.wav`

**Video ends early:**
- Check timing calculations in console output
- Verify audio generation succeeded for all questions

**Compile errors:**
- Ensure all dev packages installed: `sudo apt install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev libfreetype6-dev libjson-c-dev`

## License

MIT License - See LICENSE file for details

## Credits

- **TTS**: [Piper](https://github.com/rhasspy/piper) by Rhasspy
- **Font**: Roboto by Google Fonts
- **Libraries**: FFmpeg, FreeType, json-c
