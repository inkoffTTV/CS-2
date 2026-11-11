class_name BHitboxHistory
extends RefCounted
## Time-stamped hitbox poses of one character, used to rewind shots (lag compensation).

var _max_age: float
var _times: Array[float] = []
var _shapes: Array[Array] = []  # each: Array[BCapsule]
var _teleported: Array[bool] = []


func _init(max_age_seconds: float = 3.0) -> void:
	_max_age = max_age_seconds


static func _copy_shapes(src: Array) -> Array[BCapsule]:
	var out: Array[BCapsule] = []
	for c in src:
		out.append((c as BCapsule).copy())
	return out


## Appends a snapshot. Times must increase; older or equal times are ignored.
## teleported marks a discontinuity (respawn, teleport): no interpolation across it.
func record(time: float, shapes: Array[BCapsule], teleported: bool = false) -> void:
	if not _times.is_empty() and time <= _times[_times.size() - 1]:
		return
	_times.append(time)
	_shapes.append(_copy_shapes(shapes))
	_teleported.append(teleported)


## Pose at time (an Array[BCapsule], fresh copies). Before the oldest snapshot or non-finite: null.
## After the newest: newest.
func sample_at(time: float) -> Variant:
	if _times.is_empty() or not is_finite(time) or time < _times[0]:
		return null
	var last: int = _times.size() - 1
	if time >= _times[last]:
		return _copy_shapes(_shapes[last])

	# First snapshot strictly after time (upper bound).
	var lo: int = 0
	var hi: int = _times.size()
	while lo < hi:
		var mid: int = (lo + hi) / 2
		if time < _times[mid]:
			hi = mid
		else:
			lo = mid + 1
	if lo == 0 or lo >= _times.size():
		return null
	var i1: int = lo
	var i0: int = lo - 1
	var s0: Array = _shapes[i0]
	var s1: Array = _shapes[i1]
	if _teleported[i1] or s0.size() != s1.size():
		return _copy_shapes(s0)

	var alpha: float = (time - _times[i0]) / (_times[i1] - _times[i0])
	var out: Array[BCapsule] = _copy_shapes(s0)
	for i in out.size():
		var c0: BCapsule = s0[i]
		var c1: BCapsule = s1[i]
		out[i].a = c0.a.lerp(c1.a, alpha)
		out[i].b = c0.b.lerp(c1.b, alpha)
		out[i].radius = c0.radius + (c1.radius - c0.radius) * alpha
	return out


## Drops snapshots older than now - max_age.
func prune(now: float) -> void:
	var cutoff: float = now - _max_age
	while not _times.is_empty() and _times[0] < cutoff:
		_times.pop_front()
		_shapes.pop_front()
		_teleported.pop_front()


func num() -> int:
	return _times.size()
