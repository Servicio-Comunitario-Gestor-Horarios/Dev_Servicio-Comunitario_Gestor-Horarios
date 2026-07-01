#!/usr/bin/env bash
set -euo pipefail

# --- defaults ---
OUTPUT="timelapse-$(date +%F).mp4"
RESOLUTION="1920x1080"
FPS=60
SPD=0.5
START_DATE=""
STOP_DATE=""

# --- help ---
usage() {
  cat <<EOF
Usage: $(basename "$0") [options]

Generate a Gource timelapse video from git history.

Options:
  -o, --output PATH       Output MP4 path (default: timelapse-YYYY-MM-DD.mp4)
  -r, --viewport WxH      Viewport size (default: 1920x1080)
  -f, --fps N             Framerate (default: 60)
  -s, --seconds-per-day N Speed — seconds per day in video (default: 0.5)
  --start-date DATE       Git log start (e.g. "2024-01-01")
  --stop-date DATE        Git log end (e.g. "2024-12-31")
  -h, --help              Show this help and exit
EOF
  exit 0
}

# --- dependency validation ---
check_deps() {
  local missing=0
  if ! command -v gource &>/dev/null; then
    echo "ERROR: gource not found."
    echo "  Install: sudo apt install gource    # Debian/Ubuntu"
    echo "  Install: sudo pacman -S gource      # Arch/EndeavourOS"
    echo "  Install: brew install gource        # macOS"
    ((missing++))
  fi
  if ! command -v ffmpeg &>/dev/null; then
    echo "ERROR: ffmpeg not found."
    echo "  Install: sudo apt install ffmpeg    # Debian/Ubuntu"
    echo "  Install: sudo pacman -S ffmpeg      # Arch/EndeavourOS"
    echo "  Install: brew install ffmpeg        # macOS"
    ((missing++))
  fi
  if ((missing > 0)); then
    exit 1
  fi
}

# --- repo validation ---
check_repo() {
  if ! git rev-parse --is-inside-work-tree &>/dev/null; then
    echo "ERROR: Not inside a git repository."
    exit 1
  fi
  if ! git log --oneline --max-count=1 &>/dev/null 2>&1; then
    echo "ERROR: No commits found in this repository."
    exit 1
  fi
}

# --- parse args ---
while [[ $# -gt 0 ]]; do
  case "$1" in
    -o|--output) OUTPUT="$2"; shift 2 ;;
    -r|--resolution|--viewport) RESOLUTION="$2"; shift 2 ;;
    -f|--fps) FPS="$2"; shift 2 ;;
    -s|--seconds-per-day) SPD="$2"; shift 2 ;;
    --start-date) START_DATE="$2"; shift 2 ;;
    --stop-date) STOP_DATE="$2"; shift 2 ;;
    -h|--help) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
done

check_deps
check_repo

# --- build gource args ---
GOURCE_ARGS=(
  --output-framerate "$FPS"
  --seconds-per-day "$SPD"
  --viewport "$RESOLUTION"
  --highlight-users
  --multi-sampling
  --stop-at-end
  --hide "mouse,progress"
)

if [[ -n "$START_DATE" ]]; then
  GOURCE_ARGS+=(--start-date "$START_DATE")
fi
if [[ -n "$STOP_DATE" ]]; then
  GOURCE_ARGS+=(--stop-date "$STOP_DATE")
fi

echo "Generating timelapse..."
echo "  Output:       $OUTPUT"
echo "  Resolution:   $RESOLUTION"
echo "  FPS:          $FPS"
echo "  Speed:        ${SPD}s/day"
[[ -n "$START_DATE" ]] && echo "  Start date:   $START_DATE"
[[ -n "$STOP_DATE" ]] && echo "  Stop date:    $STOP_DATE"

gource "${GOURCE_ARGS[@]}" -o - \
  | ffmpeg -y -r "$FPS" -f image2pipe -vcodec ppm -i - \
    -vcodec libx264 -preset medium -pix_fmt yuv420p -crf 23 \
    "$OUTPUT"

echo "Done: $OUTPUT"
