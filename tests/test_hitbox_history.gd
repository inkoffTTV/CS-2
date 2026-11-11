extends RefCounted
const T := preload("res://tests/t.gd")


func _at(x: float) -> Array[BCapsule]:
	var a: Array[BCapsule] = [BCapsule.new(Vector3(x, 0, 0), Vector3(x, 1, 0), 0.3, BCapsule.Zone.BODY)]
	return a


func test_sample_interpolates() -> void:
	var h := BHitboxHistory.new()
	h.record(0.0, _at(0.0))
	h.record(1.0, _at(1.0))
	var s: Variant = h.sample_at(0.5)
	if T.not_null(s):
		T.approx(s[0].a.x, 0.5)
		T.approx(s[0].b.y, 1.0)


func test_sample_before_is_miss_after_is_latest() -> void:
	var h := BHitboxHistory.new()
	h.record(0.0, _at(0.0))
	h.record(1.0, _at(1.0))
	T.is_null(h.sample_at(-0.1))
	var s: Variant = h.sample_at(2.0)
	if T.not_null(s):
		T.approx(s[0].a.x, 1.0)


func test_teleport_not_interpolated() -> void:
	var h := BHitboxHistory.new()
	h.record(0.0, _at(0.0))
	h.record(1.0, _at(50.0), true)
	var before: Variant = h.sample_at(0.99)
	if T.not_null(before):
		T.approx(before[0].a.x, 0.0)
	var after: Variant = h.sample_at(1.0)
	if T.not_null(after):
		T.approx(after[0].a.x, 50.0)


func test_prune_drops_old() -> void:
	var h := BHitboxHistory.new(3.0)
	for t in range(0, 6):
		h.record(float(t), _at(float(t) / 100.0))
	h.prune(5.0)
	T.eq(h.num(), 4)
	T.is_null(h.sample_at(1.5))


func test_out_of_order_ignored() -> void:
	var h := BHitboxHistory.new()
	h.record(1.0, _at(0.0))
	h.record(0.5, _at(0.1))
	T.eq(h.num(), 1)


func test_different_shape_counts_fall_back_to_earlier() -> void:
	var h := BHitboxHistory.new()
	h.record(0.0, _at(0.0))
	var two := _at(1.0)
	two.append(BCapsule.new(Vector3(1, 1.5, 0), Vector3(1, 1.5, 0), 0.15, BCapsule.Zone.HEAD))
	h.record(1.0, two)
	var s: Variant = h.sample_at(0.5)
	if T.not_null(s):
		T.eq(s.size(), 1)
		T.approx(s[0].a.x, 0.0)


func test_non_finite_time_is_miss() -> void:
	var h := BHitboxHistory.new()
	h.record(0.0, _at(0.0))
	h.record(1.0, _at(1.0))
	T.is_null(h.sample_at(NAN))
	T.is_null(h.sample_at(INF))
	T.is_null(h.sample_at(-INF))


func test_sample_is_a_copy() -> void:
	var h := BHitboxHistory.new()
	h.record(0.0, _at(0.0))
	var s: Variant = h.sample_at(0.0)
	if T.not_null(s):
		s[0].a.x = 99.0
	var s2: Variant = h.sample_at(0.0)
	if T.not_null(s2):
		T.approx(s2[0].a.x, 0.0)
