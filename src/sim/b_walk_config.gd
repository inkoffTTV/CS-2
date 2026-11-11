class_name BWalkConfig
extends RefCounted
## Ground/air locomotion tuning (m, m/s, m/s^2, s). Values follow the Unreal M1 hero (cm / 100).

var walk_speed: float = 6.5
var crouch_speed: float = 3.0
var ground_accel: float = 50.0  # toward the wished velocity while on the ground
var braking: float = 40.0  # slowing down with no input, or when faster than the speed cap
var air_control: float = 0.4  # fraction of ground_accel available in the air
var jump_velocity: float = 6.5
var gravity: float = 12.74  # 9.8 * 1.3, as GravityScale 1.3 in Unreal
var dash_duration: float = 0.2
var air_dash_exit_factor: float = 1.25  # horizontal speed cap (x walk_speed) when a dash ends in the air
