class_name BTokenBucket
extends RefCounted
## Rate limiter: refills at rate tokens/s up to capacity; each action takes one token.
## Starts full, so a burst of up to capacity actions passes at once.

var rate: float = 1.0
var capacity: float = 1.0
var tokens: float = 1.0

var _last: float = -INF


func _init(p_rate: float = 1.0, p_capacity: float = 1.0) -> void:
	rate = maxf(0.0, p_rate)
	capacity = maxf(1.0, p_capacity)
	tokens = capacity


## Takes one token at time now (seconds, monotonic). False when empty or now is not finite.
## Time going backwards adds nothing.
func try_take(now: float) -> bool:
	if not is_finite(now):
		return false
	if is_finite(_last) and now > _last:
		tokens = minf(capacity, tokens + (now - _last) * rate)
	_last = maxf(_last, now)
	if tokens >= 1.0:
		tokens -= 1.0
		return true
	return false
