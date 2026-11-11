#!/usr/bin/env bash
# Combat screenshot: a listen-server host under Xvfb plays the shooter bot (Optimates) while three
# headless bots join: a target (Populares), a runner (Optimates, keeps the teams even) and an enemy
# shooter (Populares) that fires back. The host saves the picture at a moment with a kill in the
# feed and a hit marker on the crosshair (--shot-view combat).
#   tools/combat_screenshot.sh [OUT.png]      # default docs/screenshots/combat.png
# Env: GODOT (default godot), PORT (7795), DELAY (seconds after the host spawned, default 14).
set -u
cd "$(dirname "$0")/.."
GODOT="${GODOT:-godot}"
PORT="${PORT:-7795}"
DELAY="${DELAY:-16}"
OUT="${1:-docs/screenshots/combat.png}"
LOG_DIR="$(mktemp -d -t brutus-shot-XXXXXX)"
bot() { # bot <scenario> <nick> <start delay>
	(sleep "$3"; timeout 120 "$GODOT" --headless --path . -- --bot "$1" --name "$2" --connect "127.0.0.1:$PORT" \
		--quit-after 90 >"$LOG_DIR/$2.log" 2>&1) &
}
bot target Gaius 5
bot moves Cassius 8
bot shooter Marcus 16
timeout 180 xvfb-run -a "$GODOT" --path . --rendering-driver opengl3 --rendering-method gl_compatibility -- \
	--host --port "$PORT" --bot shooter --name Brutus --screenshot "$OUT" --shot-view combat --shot-delay "$DELAY" \
	>"$LOG_DIR/host.log" 2>&1
RC=$?
pkill -f -- "--connect 127.0.0.1:$PORT" 2>/dev/null
wait
grep -E "\[screenshot\]|KILL|SCRIPT ERROR" "$LOG_DIR/host.log"
echo "logs: $LOG_DIR"
exit "$RC"
