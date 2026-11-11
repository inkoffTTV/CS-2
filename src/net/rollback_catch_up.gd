class_name BRollbackCatchUp
extends Node
## netfox runs the rollback loop once per tick loop and, unless new remote input or state arrived,
## only resimulates the last tick. When a frame takes longer than a tick (fps < tickrate) the tick
## loop advances several ticks at once and locally-owned players (the listen-server host, a client's
## own prediction) would only move one tick per frame. This asks the rollback loop to resimulate
## every tick advanced in the current tick loop.

var _loop_first_tick: int = -1


func _ready() -> void:
	NetworkTime.before_tick_loop.connect(_on_before_tick_loop)
	NetworkRollback.before_loop.connect(_on_before_rollback)


func _on_before_tick_loop() -> void:
	_loop_first_tick = NetworkTime.tick


func _on_before_rollback() -> void:
	if _loop_first_tick >= 0 and _loop_first_tick < NetworkTime.tick:
		NetworkRollback.notify_resimulation_start(_loop_first_tick)
