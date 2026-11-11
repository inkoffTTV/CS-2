extends RefCounted
const T := preload("res://tests/t.gd")


func test_starts_full_then_empties() -> void:
	var b := BTokenBucket.new(10.0, 3.0)
	T.is_true(b.try_take(0.0))
	T.is_true(b.try_take(0.0))
	T.is_true(b.try_take(0.0))
	T.is_false(b.try_take(0.0), "4th shot in the same instant")


func test_refills_at_rate() -> void:
	var b := BTokenBucket.new(10.0, 1.0)
	T.is_true(b.try_take(0.0))
	T.is_false(b.try_take(0.05), "half an interval later")
	T.is_true(b.try_take(0.1), "one interval later")


func test_steady_fire_at_rate_passes() -> void:
	var cfg := BWeaponConfig.new()
	var b := BTokenBucket.new(cfg.fire_rate, cfg.burst())
	var ok: int = 0
	for i in 100:
		if b.try_take(float(i) * cfg.interval()):
			ok += 1
	T.eq(ok, 100)


func test_burst_after_hold_passes() -> void:
	# Shots fired at 10/s but delivered in clumps (a lost packet holds the reliable channel for 0.4 s).
	var cfg := BWeaponConfig.new()
	var b := BTokenBucket.new(cfg.fire_rate, cfg.burst())
	var ok: int = 0
	var sent: int = 0
	for clump in 10:
		var t: float = 0.4 * float(clump + 1)
		for i in 4:
			sent += 1
			if b.try_take(t):
				ok += 1
	T.eq(ok, sent)


func test_faster_than_rate_is_limited() -> void:
	var cfg := BWeaponConfig.new()
	var b := BTokenBucket.new(cfg.fire_rate, cfg.burst())
	var ok: int = 0
	for i in 100:  # 100 shots in 2 s = 50/s
		if b.try_take(float(i) * 0.02):
			ok += 1
	T.is_true(ok <= int(cfg.burst()) + 21, "accepted %d" % ok)
	T.is_true(ok >= 20, "accepted %d" % ok)


func test_non_finite_and_backwards_time() -> void:
	var b := BTokenBucket.new(10.0, 1.0)
	T.is_false(b.try_take(NAN))
	T.is_true(b.try_take(5.0))
	T.is_false(b.try_take(1.0), "time going back adds no tokens")
	T.is_false(b.try_take(INF))
