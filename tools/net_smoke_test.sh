#!/usr/bin/env bash
# Network smoke test: a headless dedicated server and headless bot clients, each in its own process.
#  - runner bot (--bot moves): runs, slides, dashes, jumps and double-jumps;
#  - zipline bot (--bot zipline, skip with ZIPLINE=0): climbs onto the platform and rides the zipline.
# Asserts from the logs: the clients connected and got players on both teams, the runner moved > 5 m
# on the server, the server saw dash / slide / double jump (and the zipline ride), no SCRIPT ERROR.
#
#   tools/net_smoke_test.sh
#   LAG_MS=60 tools/net_smoke_test.sh      # clients go through a UDP proxy with 60 ms each way
# Env: GODOT (default godot), PORT (7811), DURATION (seconds the runner plays, default 10), ZIPLINE (1),
#      LAG_MS (0), LOSS (packet loss %, 0), LOG_DIR (default: a new temp dir).
# Exit code 0 = pass, 1 = fail.
set -u
cd "$(dirname "$0")/.."
GODOT="${GODOT:-godot}"
PORT="${PORT:-7811}"
DURATION="${DURATION:-10}"
ZIPLINE="${ZIPLINE:-1}"
LAG_MS="${LAG_MS:-0}"
LOSS="${LOSS:-0}"
LOG_DIR="${LOG_DIR:-$(mktemp -d -t brutus-smoke-XXXXXX)}"
mkdir -p "$LOG_DIR"
ZIP_DURATION=$((DURATION > 15 ? DURATION : 15))
SERVER_TIME=$((ZIP_DURATION + 4))

"$GODOT" --headless --path . -- --server --port "$PORT" --quit-after "$SERVER_TIME" >"$LOG_DIR/server.log" 2>&1 &
SERVER_PID=$!
CONNECT_PORT="$PORT"
PROXY_PID=""
if [ "$LAG_MS" != "0" ] || [ "$LOSS" != "0" ]; then
	CONNECT_PORT=$((PORT + 1))
	python3 tools/udp_lag_proxy.py "$CONNECT_PORT" "$PORT" "$LAG_MS" "$LOSS" "$((SERVER_TIME + 5))" >"$LOG_DIR/proxy.log" 2>&1 &
	PROXY_PID=$!
fi
sleep 1.5

ZIP_PID=""
if [ "$ZIPLINE" = "1" ]; then
	timeout $((ZIP_DURATION + 20)) "$GODOT" --headless --path . -- --bot zipline --name SmokeZip \
		--connect "127.0.0.1:$CONNECT_PORT" --quit-after "$ZIP_DURATION" >"$LOG_DIR/zipline.log" 2>&1 &
	ZIP_PID=$!
	sleep 0.5
fi
timeout $((DURATION + 20)) "$GODOT" --headless --path . -- --bot moves --name SmokeRunner \
	--connect "127.0.0.1:$CONNECT_PORT" --quit-after "$DURATION" >"$LOG_DIR/client.log" 2>&1
CLIENT_RC=$?
ZIP_RC=0
if [ -n "$ZIP_PID" ]; then wait "$ZIP_PID"; ZIP_RC=$?; fi
wait "$SERVER_PID"
SERVER_RC=$?
if [ -n "$PROXY_PID" ]; then kill "$PROXY_PID" 2>/dev/null; fi

S="$LOG_DIR/server.log"
fail=0
check() { # check <description> <command...>
	local what="$1"; shift
	if "$@"; then echo "PASS  $what"; else echo "FAIL  $what"; fail=1; fi
}
peer_of() { grep -oE "spawned peer=[0-9]+ nick=$1 " "$S" | head -1 | sed -E 's/spawned peer=([0-9]+).*/\1/'; }
moved_of() { grep -oE "SUMMARY peer=$1 max_moved=[0-9.]+" "$S" | head -1 | sed 's/.*max_moved=//'; }
RUNNER=$(peer_of SmokeRunner)
RUNNER_MOVED=$(moved_of "${RUNNER:-x}")
RUNNER_MOVED=${RUNNER_MOVED:-0}

check "server exit code 0 (got $SERVER_RC)" test "$SERVER_RC" -eq 0
check "runner client exit code 0 (got $CLIENT_RC)" test "$CLIENT_RC" -eq 0
check "runner connected" grep -q "connected to server as peer" "$LOG_DIR/client.log"
check "runner got its local player" grep -q "local player spawned" "$LOG_DIR/client.log"
check "server spawned the runner (peer ${RUNNER:-?})" test -n "$RUNNER"
check "runner moved > 5 m on the server (max_moved=$RUNNER_MOVED)" awk -v m="$RUNNER_MOVED" 'BEGIN { exit !(m > 5.0) }'
check "server saw the runner dash" grep -q "\[server\] EVENT dash peer=${RUNNER:-x} " "$S"
check "server saw the runner slide" grep -q "\[server\] EVENT slide peer=${RUNNER:-x} " "$S"
check "server saw the runner double jump" grep -q "\[server\] EVENT air_jump peer=${RUNNER:-x} " "$S"
if [ "$ZIPLINE" = "1" ]; then
	ZIPPER=$(peer_of SmokeZip)
	check "zipline client exit code 0 (got $ZIP_RC)" test "$ZIP_RC" -eq 0
	check "teams balanced (one Optimates, one Populares)" \
		bash -c "grep -q 'team=Optimates' '$S' && grep -q 'team=Populares' '$S'"
	check "server saw the zipline ride (peer ${ZIPPER:-?})" grep -q "\[server\] EVENT zipline peer=${ZIPPER:-x} " "$S"
fi
for f in "$LOG_DIR"/*.log; do
	check "no SCRIPT ERROR in $(basename "$f")" bash -c "! grep -q 'SCRIPT ERROR' '$f'"
done

echo "--- server events"
grep -E "\[server\] (spawned|EVENT|SUMMARY|despawn)" "$S"
echo "logs: $LOG_DIR"
if [ "$fail" -ne 0 ]; then
	echo "SMOKE TEST FAILED"
	exit 1
fi
echo "SMOKE TEST PASSED"
