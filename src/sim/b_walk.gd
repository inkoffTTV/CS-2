class_name BWalk
extends RefCounted
## Walking / air-control rules (pure functions). Metres, Y up, forward -Z.


## Horizontal unit forward vector for a yaw (radians, rotation about +Y). yaw = 0 faces -Z.
static func facing_from_yaw(yaw: float) -> Vector3:
	return Vector3(-sin(yaw), 0.0, -cos(yaw))


## World-space wish direction from a move stick (x = right, y = forward), length clamped to 1.
static func wish_dir(move: Vector2, yaw: float) -> Vector3:
	var forward: Vector3 = facing_from_yaw(yaw)
	var right := Vector3(cos(yaw), 0.0, -sin(yaw))
	var dir: Vector3 = right * move.x + forward * move.y
	return dir.limit_length(1.0)


## One step of horizontal velocity. On the ground it accelerates toward wish * max_speed (or brakes
## to zero without input); in the air only air_control * ground_accel is available and the current
## speed is never reduced by input (momentum from dashes, slides and ziplines is kept).
static func step_horizontal(vel_h: Vector3, wish: Vector3, dt: float, grounded: bool, max_speed: float,
		cfg: BWalkConfig) -> Vector3:
	var v := Vector3(vel_h.x, 0.0, vel_h.z)
	if grounded:
		var target: Vector3 = wish * max_speed
		var rate: float = cfg.ground_accel
		if wish.length_squared() < 1e-6 or v.length() > max_speed + 1e-3:
			rate = cfg.braking
		return v.move_toward(target, rate * dt)
	if wish.length_squared() < 1e-6:
		return v
	var cap: float = maxf(max_speed, v.length())
	var accelerated: Vector3 = v + wish * (cfg.ground_accel * cfg.air_control * dt)
	return accelerated.limit_length(cap)


## Velocity after an airborne dash runs out: horizontal speed capped once so it cannot be carried
## as a launch; vertical speed kept.
static func end_air_dash(velocity: Vector3, cfg: BWalkConfig) -> Vector3:
	var h := Vector3(velocity.x, 0.0, velocity.z).limit_length(cfg.walk_speed * cfg.air_dash_exit_factor)
	return Vector3(h.x, velocity.y, h.z)
