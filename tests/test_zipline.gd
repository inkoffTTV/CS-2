extends RefCounted
const T := preload("res://tests/t.gd")


func _seg() -> BZiplineSegment:
	return BZiplineSegment.new(Vector3.ZERO, Vector3(10, 0, 0))


func test_project_t_clamped() -> void:
	var seg := _seg()
	T.approx(BZipline.project_t(seg, Vector3(2.5, 0, -0.5)), 0.25)
	T.approx(BZipline.project_t(seg, Vector3(-1, 0, 0)), 0.0)
	T.approx(BZipline.project_t(seg, Vector3(20, 0, 0)), 1.0)


func test_point_at_and_distance_to() -> void:
	var seg := _seg()
	T.approx(BZipline.point_at(seg, 0.5).x, 5.0)
	T.approx(BZipline.distance_to(seg, Vector3(2.5, 0, -0.5)), 0.5)


func test_advance_t_moves_and_clamps() -> void:
	var seg := _seg()
	T.approx(BZipline.advance_t(seg, 0.9, 1, 18.0, 0.1), 1.0)
	T.approx(BZipline.advance_t(seg, 0.5, -1, 10.0, 0.1), 0.4)


func test_choose_direction_follows_facing() -> void:
	var seg := _seg()
	T.eq(BZipline.choose_direction(seg, Vector3(-1, 0, 0)), -1)
	T.eq(BZipline.choose_direction(seg, Vector3(1, 0, -0.2)), 1)


func test_degenerate_segment_no_nan() -> void:
	var p := Vector3(0.05, 0.05, -0.05)
	var seg := BZiplineSegment.new(p, p)
	var t := BZipline.project_t(seg, Vector3(1, 0, 0))
	T.is_false(is_nan(t))
	T.approx(t, 0.0)
	T.approx(BZipline.advance_t(seg, 0.0, 1, 10.0, 0.1), 1.0)
	T.approx(BZipline.advance_t(seg, 1.0, -1, 10.0, 0.1), 0.0)


func test_defaults_in_metres() -> void:
	var cfg := BZiplineConfig.new()
	T.approx(cfg.speed, 18.0)
	T.approx(cfg.attach_radius, 2.0)
	T.approx(cfg.hang_offset_y, -1.2)
