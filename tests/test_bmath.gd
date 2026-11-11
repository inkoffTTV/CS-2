extends RefCounted
const T := preload("res://tests/t.gd")


func test_vec_basic_math() -> void:
	T.approx(Vector3(3, 4, 0).length(), 5.0)
	T.vec(BMath.safe_normalized(Vector3.ZERO), Vector3.ZERO)
	T.vec(BMath.safe_normalized(Vector3(0, 0, 1e-9)), Vector3.ZERO)
	T.approx(Vector3.ZERO.lerp(Vector3(10, 0, 0), 0.25).x, 2.5)
	T.approx(BMath.flatten(Vector3(1, 2, 3)).y, 0.0)
	T.approx(BMath.flatten(Vector3(1, 2, 3)).z, 3.0)
	T.approx(Vector3(1, 0, 0).dot(Vector3(0, 1, 0)), 0.0)
