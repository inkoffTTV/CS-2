extends RefCounted
const T := preload("res://tests/t.gd")


func test_zone_damage() -> void:
	T.approx(BCombatRules.zone_damage(20.0, BCapsule.Zone.BODY, 1.5), 20.0)
	T.approx(BCombatRules.zone_damage(20.0, BCapsule.Zone.HEAD, 1.5), 30.0)


func test_life_windows() -> void:
	T.is_false(BCombatRules.is_dead(-1, -1), "never died")
	T.is_true(BCombatRules.is_dead(100, -1), "died once")
	T.is_false(BCombatRules.is_dead(100, 250), "respawned")
	T.is_true(BCombatRules.is_dead(400, 250), "died again")
	T.is_false(BCombatRules.is_dead_at(99, 100, -1))
	T.is_true(BCombatRules.is_dead_at(100, 100, -1))
	T.is_true(BCombatRules.is_dead_at(249, 100, 250))
	T.is_false(BCombatRules.is_dead_at(250, 100, 250))


func test_respawn_timing() -> void:
	T.is_false(BCombatRules.respawn_due(249, 100, -1, 30))
	T.is_true(BCombatRules.respawn_due(250, 100, -1, 30))
	T.is_false(BCombatRules.respawn_due(500, 100, 250, 30), "already respawned")
	T.approx(BCombatRules.respawn_in(130, 100, -1, 30), 4.0)
	T.approx(BCombatRules.respawn_in(130, 100, 120, 30), 0.0)


func test_view_time() -> void:
	# Tick 300 at 30 tps, halfway to the next tick, 100 ms round trip: shows tick 299.5 minus 50 ms.
	T.approx(BCombatRules.view_time(300, 0.5, 1.0 / 30.0, 0.1), 299.5 / 30.0 - 0.05)
	T.approx(BCombatRules.view_time(300, 0.0, 1.0 / 30.0, -1.0), 299.0 / 30.0, 1e-5, "negative rtt ignored")
