extends RefCounted
const T := preload("res://tests/t.gd")


func _observer() -> BVisionActor:
	return BVisionActor.new(1, 0, Vector3.ZERO, 20.0, true, false)


func _enemy(x: float, hidden: bool = false) -> BVisionActor:
	return BVisionActor.new(2, 1, Vector3(x, 0, 0), 20.0, true, hidden)


func _both(x: float, hidden: bool = false) -> Array[BVisionActor]:
	var a: Array[BVisionActor] = [_observer(), _enemy(x, hidden)]
	return a


func _clear(_o: BVisionActor, _t: BVisionActor) -> bool:
	return true


func _blocked(_o: BVisionActor, _t: BVisionActor) -> bool:
	return false


func test_enemy_in_sight_visible() -> void:
	var v := BTeamVision.new()
	v.update(0.0, _both(15.0), _clear)
	T.is_true(v.is_visible_to_team(2, 0))


func test_enemy_behind_wall_not_visible() -> void:
	var v := BTeamVision.new()
	v.update(0.0, _both(15.0), _blocked)
	T.is_false(v.is_visible_to_team(2, 0))


func test_short_grace_period() -> void:
	var v := BTeamVision.new()
	v.update(0.0, _both(15.0), _clear)
	v.update(0.15, _both(15.0), _blocked)
	T.is_true(v.is_visible_to_team(2, 0))
	v.update(0.3, _both(15.0), _blocked)
	T.is_false(v.is_visible_to_team(2, 0))


func test_hidden_revealed_only_close() -> void:
	var v := BTeamVision.new()
	v.update(0.0, _both(15.0, true), _clear)
	T.is_false(v.is_visible_to_team(2, 0))
	v.update(0.1, _both(5.0, true), _clear)
	T.is_true(v.is_visible_to_team(2, 0))


func test_hidden_no_grace() -> void:
	var v := BTeamVision.new()
	v.update(0.0, _both(5.0, true), _clear)
	v.update(0.1, _both(15.0, true), _clear)
	T.is_false(v.is_visible_to_team(2, 0))


func test_beyond_sight_radius_not_visible() -> void:
	var v := BTeamVision.new()
	v.update(0.0, _both(25.0), _clear)
	T.is_false(v.is_visible_to_team(2, 0))


func test_own_team_always_visible_unknown_never() -> void:
	var v := BTeamVision.new()
	v.update(0.0, _both(25.0), _blocked)
	T.is_true(v.is_visible_to_team(1, 0))
	T.is_true(v.is_visible_to_team(2, 1))
	T.is_false(v.is_visible_to_team(99, 0))


func test_missing_id_dropped_no_inherited_grace() -> void:
	var v := BTeamVision.new()
	v.update(0.0, _both(15.0), _clear)
	T.is_true(v.is_visible_to_team(2, 0))
	var only: Array[BVisionActor] = [_observer()]
	v.update(0.05, only, _clear)
	T.is_false(v.is_visible_to_team(2, 0))
	v.update(0.1, _both(15.0), _blocked)
	T.is_false(v.is_visible_to_team(2, 0))


func test_defaults_in_metres() -> void:
	var c := BVisionConfig.new()
	T.approx(c.hidden_reveal_radius, 6.0)
	T.approx(c.grace_seconds, 0.2)
