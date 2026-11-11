class_name BWeaponConfig
extends RefCounted
## Hitscan weapon tuning (m, s). Generic defaults; heroes configure their own later.

var fire_rate: float = 10.0  # shots per second while the trigger is held
var damage: float = 20.0
var head_multiplier: float = 1.5
var max_range: float = 100.0
var max_rewind: float = 0.25  # lag compensation cap, seconds
var max_origin_error: float = 2.0  # shot origin vs the shooter's eyes on the server
var burst_seconds: float = 0.5  # server rate limit tolerates this much fire arriving at once


func interval() -> float:
	return 1.0 / maxf(0.01, fire_rate)


## Server token bucket capacity: one shot plus burst_seconds worth of fire.
func burst() -> float:
	return 1.0 + fire_rate * burst_seconds
