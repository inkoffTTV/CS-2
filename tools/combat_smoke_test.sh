#!/usr/bin/env bash
# Combat smoke test: a headless dedicated server and two headless bot clients on different teams.
#  - shooter bot (--bot shooter, joins first -> Optimates): walks to the +Z passage, strafes, fires
#    at the nearest visible enemy;
#  - target bot (--bot target, joins second -> Populares): runs back and forth on a lane in view.
# Asserts from the server log: shots received, hits on the moving target, at least one kill and a
# respawn, few rejected shots, no SCRIPT ERROR anywhere. Prints the average lag-compensation rewind
# of the hits and how many of them would have missed without rewinding; with LAG_MS the average
# rewind must be close to the round trip (2 x LAG_MS), without lag it must stay small.
#
#   tools/combat_smoke_test.sh
#   LAG_MS=80 LOSS=3 tools/combat_smoke_test.sh   # clients through the UDP lag proxy
# Env: GODOT (default godot), PORT (7831), DURATION (seconds the bots play, default 25), MIN_HITS (20),
#      LAG_MS (0), LOSS (packet loss %, 0), LOG_DIR (default: a new temp dir).
# Exit code 0 = pass, 1 = fail.
set -u
cd "$(dirname "$0")/.."
GODOT="${GODOT:-godot}"
PORT="${PORT:-7831}"
DURATION="${DURATION:-25}"
MIN_HITS="${MIN_HITS:-20}"
LAG_MS="${LAG_MS:-0}"
LOSS="${LOSS:-0}"
LOG_DIR="${LOG_DIR:-$(mktemp -d -t brutus-combat-XXXXXX)}"
mkdir -p "$LOG_DIR"
SERVER_TIME=$((DURATION + 6))

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
timeout $((DURATION + 25)) "$GODOT" --headless --path . -- --bot shooter --name SmokeShooter \
	--connect "127.0.0.1:$CONNECT_PORT" --quit-after "$((DURATION + 2))" >"$LOG_DIR/shooter.log" 2>&1 &
SHOOTER_PID=$!
sleep 2
timeout $((DURATION + 25)) "$GODOT" --headless --path . -- --bot target --name SmokeTarget \
	--connect "127.0.0.1:$CONNECT_PORT" --quit-after "$DURATION" >"$LOG_DIR/target.log" 2>&1
TARGET_RC=$?
wait "$SHOOTER_PID"
SHOOTER_RC=$?
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
count() { grep -cE "$1" "$S"; }
SHOOTER=$(peer_of SmokeShooter)
TARGET=$(peer_of SmokeTarget)
SH="${SHOOTER:-x}"
TG="${TARGET:-x}"
MISSES=$(count "\] SHOT peer=$SH ")
HITS=$(count "\] HIT shooter=$SH target=$TG ")
HEAD_HITS=$(count "\] HIT shooter=$SH target=$TG zone=head")
REJECTS=$(count "\] REJECT peer=$SH ")
SHOTS=$((MISSES + HITS + REJECTS))
KILLS=$(count "\] KILL killer=$SH victim=$TG ")
RESPAWNS=$(count "\] RESPAWN peer=$TG ")
read -r AVG_REWIND MIN_REWIND MAX_REWIND UNREWOUND_MISS < <(grep -E "\] HIT shooter=$SH target=$TG " "$S" |
	sed -E 's/.*rewind_ms=([0-9.]+).*unrewound=([a-z]+).*/\1 \2/' |
	awk '{ s += $1; n++; if (n == 1 || $1 < lo) lo = $1; if ($1 > hi) hi = $1; if ($2 == "miss") m++ }
		END { if (n == 0) print "0 0 0 0"; else printf "%.1f %.1f %.1f %d\n", s / n, lo, hi, m }')
RTT=$((2 * LAG_MS))

check "server exit code 0 (got $SERVER_RC)" test "$SERVER_RC" -eq 0
check "shooter client exit code 0 (got $SHOOTER_RC)" test "$SHOOTER_RC" -eq 0
check "target client exit code 0 (got $TARGET_RC)" test "$TARGET_RC" -eq 0
check "server spawned shooter (peer ${SHOOTER:-?}) and target (peer ${TARGET:-?})" test -n "$SHOOTER" -a -n "$TARGET"
check "shooter and target on different teams" bash -c "grep -q 'nick=SmokeShooter team=Optimates' '$S' && grep -q 'nick=SmokeTarget team=Populares' '$S'"
check "server received shots ($SHOTS) >= 30" test "$SHOTS" -ge 30
check "hits on the moving target ($HITS, $HEAD_HITS head) >= $MIN_HITS" test "$HITS" -ge "$MIN_HITS"
check "at least one kill ($KILLS)" test "$KILLS" -ge 1
check "target respawned ($RESPAWNS)" test "$RESPAWNS" -ge 1
check "rejected shots <= 5% ($REJECTS of $SHOTS)" test $((REJECTS * 20)) -le "$SHOTS"
if [ "$LAG_MS" != "0" ]; then
	check "average rewind ${AVG_REWIND} ms ~ RTT ${RTT} ms (within [0.8 x RTT, RTT + 100])" \
		awk -v a="$AVG_REWIND" -v r="$RTT" 'BEGIN { exit !(a >= 0.8 * r && a <= r + 100) }'
else
	check "average rewind ${AVG_REWIND} ms < 80 ms without lag" awk -v a="$AVG_REWIND" 'BEGIN { exit !(a < 80) }'
fi
for f in "$LOG_DIR"/*.log; do
	check "no SCRIPT ERROR in $(basename "$f")" bash -c "! grep -q 'SCRIPT ERROR' '$f'"
done

echo "--- summary"
echo "lag ${LAG_MS} ms one way, loss ${LOSS}%: shots=$SHOTS hits=$HITS head=$HEAD_HITS misses=$MISSES rejects=$REJECTS kills=$KILLS respawns=$RESPAWNS"
echo "rewind ms: avg=$AVG_REWIND min=$MIN_REWIND max=$MAX_REWIND; hits that would miss without rewind: $UNREWOUND_MISS of $HITS"
grep -E "\[server\] (spawned|KILL|RESPAWN|despawn)" "$S"
grep -E "\[server\] REJECT" "$S" | sort | uniq -c | head -5
echo "logs: $LOG_DIR"
if [ "$fail" -ne 0 ]; then
	echo "COMBAT SMOKE TEST FAILED"
	exit 1
fi
echo "COMBAT SMOKE TEST PASSED"
