class_name BHealth
extends Node
## Server-authoritative health and life state of a hero. Changed only on the server and replicated
## to every peer by the MultiplayerSynchronizer child (also to peers that join later). Not rollback
## state: the hero's rollback tick reads death_tick / respawn_tick to freeze a dead hero and to
## teleport it to respawn_xform on the respawn tick.

signal changed

var max_health: float = BCombatRules.MAX_HEALTH
var health: float = BCombatRules.MAX_HEALTH:
	set(v):
		if v != health:
			health = v
			changed.emit()
var death_tick: int = -1:
	set(v):
		death_tick = v
		changed.emit()
var respawn_tick: int = -1:
	set(v):
		respawn_tick = v
		changed.emit()
var respawn_xform: Transform3D = Transform3D.IDENTITY

var last_attacker: int = 0  # server only


func is_dead() -> bool:
	return BCombatRules.is_dead(death_tick, respawn_tick)


func is_dead_at(tick: int) -> bool:
	return BCombatRules.is_dead_at(tick, death_tick, respawn_tick)


## Seconds until the respawn while dead.
func respawn_in() -> float:
	return BCombatRules.respawn_in(NetworkTime.tick, death_tick, respawn_tick, NetworkTime.tickrate)


## Server: takes damage. Returns true when this hit killed.
func apply_damage(amount: float, attacker: int) -> bool:
	if is_dead() or amount <= 0.0:
		return false
	last_attacker = attacker
	health = maxf(0.0, health - amount)
	if health > 0.0:
		return false
	death_tick = NetworkTime.tick
	return true


## Server: back to full health at xform from the current tick on.
func respawn(xform: Transform3D) -> void:
	respawn_xform = xform
	health = max_health
	respawn_tick = NetworkTime.tick
