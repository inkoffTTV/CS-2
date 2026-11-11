class_name BBallistics
extends RefCounted


## Initial velocity V such that a point launched from `from` under constant acceleration (0, gravity, 0)
## is at `to` after flight_time: to = from + V*t + 0.5*g*t^2. Gravity is signed (Godot: negative Y).
## flight_time <= 0 returns the zero vector.
static func launch_velocity_to_reach(from: Vector3, to: Vector3, gravity: float, flight_time: float) -> Vector3:
	if flight_time <= 0.0:
		return Vector3.ZERO
	return (to - from) / flight_time - Vector3(0.0, 0.5 * gravity * flight_time, 0.0)
