class_name BCombatRules
extends RefCounted
## Health, death and shot timing rules (pure functions).

const MAX_HEALTH: float = 250.0
const RESPAWN_SECONDS: float = 5.0


## Damage of one hit on a zone.
static func zone_damage(base: float, zone: BCapsule.Zone, head_multiplier: float) -> float:
	return base * head_multiplier if zone == BCapsule.Zone.HEAD else base


## Dead now: the last life event is a death (respawn_tick < 0 or older than death_tick).
static func is_dead(death_tick: int, respawn_tick: int) -> bool:
	return death_tick >= 0 and respawn_tick <= death_tick


## Dead at tick, looking at the latest death and respawn only.
static func is_dead_at(tick: int, death_tick: int, respawn_tick: int) -> bool:
	if death_tick < 0 or tick < death_tick:
		return false
	return respawn_tick <= death_tick or tick < respawn_tick


static func respawn_due(now_tick: int, death_tick: int, respawn_tick: int, tickrate: int) -> bool:
	return is_dead(death_tick, respawn_tick) and now_tick >= death_tick + roundi(RESPAWN_SECONDS * tickrate)


## Seconds until respawn (>= 0) for a dead player, 0 when alive.
static func respawn_in(now_tick: int, death_tick: int, respawn_tick: int, tickrate: int) -> float:
	if not is_dead(death_tick, respawn_tick):
		return 0.0
	return maxf(0.0, float(death_tick + roundi(RESPAWN_SECONDS * tickrate) - now_tick) / float(tickrate))


## Server time of what a client shows for other players when it fires: the client's synced clock
## (tick + tick_factor) minus one tick of interpolation (it blends ticks N-1 and N) minus the
## one-way trip the server state took to arrive (rtt / 2).
static func view_time(tick: int, tick_factor: float, ticktime: float, rtt: float) -> float:
	return (float(tick) - 1.0 + clampf(tick_factor, 0.0, 1.0)) * ticktime - maxf(0.0, rtt) * 0.5
