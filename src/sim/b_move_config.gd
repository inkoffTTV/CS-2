class_name BMoveConfig
extends RefCounted
## Starting tuning values for hero movement (m, m/s, m/s^2).

var dash_speed: float = 15.0
var air_jumps: int = 1
var slide_min_start_speed: float = 5.0
var slide_start_boost: float = 2.5
var slide_decel: float = 4.0
var slide_max_speed: float = 18.0
var slide_exit_speed: float = 3.0
var gravity: float = 9.8  # magnitude, applied along -Y
