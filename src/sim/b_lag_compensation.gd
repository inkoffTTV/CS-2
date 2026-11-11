class_name BLagCompensation
extends RefCounted
## Server-side hitbox histories of every hero (keyed by peer id) and rewound ray traces.
## A hero that was down (dead) gets a teleport mark on its first record after coming back, and
## rewinding into an earlier life misses.

const HISTORY_SECONDS: float = 3.0

var _histories: Dictionary[int, BHitboxHistory] = {}
var _gap: Dictionary[int, bool] = {}
var _life_start: Dictionary[int, float] = {}


func record(peer: int, time: float, shapes: Array[BCapsule]) -> void:
	var h: BHitboxHistory = _histories.get(peer)
	if h == null:
		h = BHitboxHistory.new(HISTORY_SECONDS)
		_histories[peer] = h
		_life_start[peer] = time
	var teleported: bool = _gap.get(peer, false)
	if teleported:
		_life_start[peer] = time
	h.record(time, shapes, teleported)
	_gap[peer] = false


## The hero is down (dead / not yet respawned): no records, and the next one is a teleport.
func mark_gap(peer: int) -> void:
	if _histories.has(peer):
		_gap[peer] = true


func prune(now: float) -> void:
	for h: BHitboxHistory in _histories.values():
		h.prune(now)


func forget(peer: int) -> void:
	_histories.erase(peer)
	_gap.erase(peer)
	_life_start.erase(peer)


func peers() -> Array[int]:
	return _histories.keys()


## Pose of peer at time (Array[BCapsule]) or null: unknown, before its current life, or down now.
func sample(peer: int, time: float) -> Variant:
	var h: BHitboxHistory = _histories.get(peer)
	if h == null or _gap.get(peer, false) or time < _life_start.get(peer, INF):
		return null
	return h.sample_at(time)


## Nearest hit of ray among candidates rewound to time; null on a miss.
func trace(ray: BRay, max_distance: float, time: float, candidates: Array[int]) -> BRewindHit:
	var best: BRewindHit = null
	for peer in candidates:
		var pose: Variant = sample(peer, time)
		if pose == null:
			continue
		var shapes: Array[BCapsule] = []
		shapes.assign(pose)
		var hit: BRayHit = BRaycast.raycast_capsules(ray, shapes, max_distance)
		if hit != null and (best == null or hit.distance < best.hit.distance):
			best = BRewindHit.new(peer, hit, ray.origin + ray.dir * hit.distance)
	return best
