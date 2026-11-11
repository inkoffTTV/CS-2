class_name BMoveRules
extends RefCounted
## Movement rules (pure functions). Metres, Y up.


static func _with_speed(direction: Vector3, speed: float) -> Vector3:
	return BMath.safe_normalized(direction) * speed


## Horizontal dash velocity along wish_dir, or along facing when there is no input.
static func compute_dash_velocity(wish_dir: Vector3, facing: Vector3, config: BMoveConfig) -> Vector3:
	var dir: Vector3 = BMath.safe_normalized(BMath.flatten(wish_dir))
	if dir.length_squared() < 1e-6:
		dir = BMath.safe_normalized(BMath.flatten(facing))
	return dir * config.dash_speed


static func try_air_jump(state: BAirJumpState, config: BMoveConfig) -> bool:
	if state.used >= config.air_jumps:
		return false
	state.used += 1
	return true


static func reset_air_jumps(state: BAirJumpState) -> void:
	state.used = 0


static func can_start_slide(horizontal_speed: float, grounded: bool, config: BMoveConfig) -> bool:
	return grounded and horizontal_speed >= config.slide_min_start_speed


## Velocity right after entering a slide: boost along horizontal travel, clamped to slide_max_speed.
static func start_slide_velocity(velocity: Vector3, config: BMoveConfig) -> Vector3:
	var boosted: Vector3 = velocity + BMath.safe_normalized(BMath.flatten(velocity)) * config.slide_start_boost
	var speed: float = boosted.length()
	return _with_speed(boosted, config.slide_max_speed) if speed > config.slide_max_speed else boosted


## One slide step: velocity is turned into the floor plane (keeping its speed), gains gravity along the
## downhill direction, loses slide_decel, and is clamped to [0, slide_max_speed].
static func step_slide(velocity: Vector3, floor_normal: Vector3, dt: float, config: BMoveConfig) -> Vector3:
	var n: Vector3 = BMath.safe_normalized(floor_normal)
	var speed: float = velocity.length()
	var v: Vector3 = _with_speed(velocity - n * velocity.dot(n), speed)
	var down := Vector3(0.0, -1.0, 0.0)
	var downhill: Vector3 = down - n * down.dot(n)
	v = v + downhill * (config.gravity * dt)
	var new_speed: float = clampf(v.length() - config.slide_decel * dt, 0.0, config.slide_max_speed)
	return _with_speed(v, new_speed)


static func should_exit_slide(velocity: Vector3, config: BMoveConfig) -> bool:
	return velocity.length() < config.slide_exit_speed
