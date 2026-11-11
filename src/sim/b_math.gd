class_name BMath
extends RefCounted
## Small vector helpers. Godot units: metres, Y up.


## Zero vector for (near-)zero input instead of NaN.
static func safe_normalized(v: Vector3) -> Vector3:
	var len: float = v.length()
	return v / len if len > 1e-6 else Vector3.ZERO


## Drops the vertical (Y) component.
static func flatten(v: Vector3) -> Vector3:
	return Vector3(v.x, 0.0, v.z)
