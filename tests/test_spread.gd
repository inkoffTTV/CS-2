extends RefCounted
const T := preload("res://tests/t.gd")


func _angle_deg(a: Vector3, b: Vector3) -> float:
	return rad_to_deg(acos(clampf(a.dot(b), -1.0, 1.0)))


func _same(a: Array[Vector3], b: Array[Vector3]) -> bool:
	if a.size() != b.size():
		return false
	for i in a.size():
		if a[i] != b[i]:
			return false
	return true


func test_pellets_normalized_and_in_cone() -> void:
	var fwds: Array[Vector3] = [Vector3(1, 0, 0), Vector3(1, 2, 3).normalized(), Vector3(0, 1, 0), Vector3(0, -1, 0)]
	for fwd in fwds:
		var dirs := BSpread.pellet_directions(fwd, 5.0, 100, 1234)
		T.eq(dirs.size(), 100)
		for d in dirs:
			T.approx(d.length(), 1.0, 1e-4)
			T.is_true(_angle_deg(fwd, d) <= 5.0 + 1e-2, "angle")


func test_pellets_spread_out() -> void:
	var dirs := BSpread.pellet_directions(Vector3(1, 0, 0), 5.0, 100, 7)
	var mx := 0.0
	for d in dirs:
		mx = maxf(mx, _angle_deg(Vector3(1, 0, 0), d))
	T.is_true(mx > 3.5, "max angle")


func test_same_seed_same_pellets() -> void:
	var a := BSpread.pellet_directions(Vector3(0, 0, -1), 5.0, 100, 42)
	var b := BSpread.pellet_directions(Vector3(0, 0, -1), 5.0, 100, 42)
	var c := BSpread.pellet_directions(Vector3(0, 0, -1), 5.0, 100, 43)
	T.is_true(_same(a, b))
	T.is_false(_same(a, c))


func test_single_pellet_no_spread_is_forward() -> void:
	var f := Vector3(1, 2, 3).normalized()
	var dirs := BSpread.pellet_directions(f, 0.0, 1, 99)
	T.eq(dirs.size(), 1)
	T.eq(dirs[0], f)


func test_non_positive_count_is_empty() -> void:
	T.eq(BSpread.pellet_directions(Vector3(1, 0, 0), 5.0, 0, 1).size(), 0)
	T.eq(BSpread.pellet_directions(Vector3(1, 0, 0), 5.0, -3, 1).size(), 0)
