extends RefCounted
const T := preload("res://tests/t.gd")


func test_falloff_center() -> void:
	T.approx(BAreaDamage.radial_falloff(0.0, 3.5, 0.3), 1.0)


func test_falloff_linear_to_min_fraction() -> void:
	T.approx(BAreaDamage.radial_falloff(1.75, 3.5, 0.3), 0.65)
	T.approx(BAreaDamage.radial_falloff(3.5, 3.5, 0.3), 0.3)


func test_falloff_zero_beyond_radius() -> void:
	T.eq(BAreaDamage.radial_falloff(3.5001, 3.5, 0.3), 0.0)
	T.eq(BAreaDamage.radial_falloff(10.0, 3.5, 0.3), 0.0)


func test_falloff_degenerate() -> void:
	T.approx(BAreaDamage.radial_falloff(-0.05, 3.5, 0.3), 1.0)
	T.eq(BAreaDamage.radial_falloff(0.0, 0.0, 0.3), 0.0)
