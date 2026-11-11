extends RefCounted
const T := preload("res://tests/t.gd")


func test_first_player_is_optimates() -> void:
	var none: Array[int] = []
	T.eq(BTeams.pick_team(none), BTeams.OPTIMATES)


func test_balances_to_smaller_side() -> void:
	var one: Array[int] = [BTeams.OPTIMATES]
	T.eq(BTeams.pick_team(one), BTeams.POPULARES)
	var even: Array[int] = [BTeams.OPTIMATES, BTeams.POPULARES]
	T.eq(BTeams.pick_team(even), BTeams.OPTIMATES)
	var more_pop: Array[int] = [BTeams.POPULARES, BTeams.POPULARES, BTeams.OPTIMATES]
	T.eq(BTeams.pick_team(more_pop), BTeams.OPTIMATES)


func test_names_and_colors() -> void:
	T.eq(BTeams.team_name(0), "Optimates")
	T.eq(BTeams.team_name(1), "Populares")
	T.eq(BTeams.team_color(0), Color(0.42, 0.36, 1.0))
	T.eq(BTeams.team_color(1), Color(1.0, 0.45, 0.1))


func test_spawn_index_rotates() -> void:
	T.eq(BTeams.spawn_index(0, 2), 0)
	T.eq(BTeams.spawn_index(1, 2), 1)
	T.eq(BTeams.spawn_index(2, 2), 0)
	T.eq(BTeams.spawn_index(0, 0), -1)
