class_name BStamina
extends RefCounted
## Shared stamina pool for dash and air jumps. Charges are fractional while regenerating.


## Spends cost charges only if at least cost are available; otherwise leaves the state untouched.
static func try_consume(state: BStaminaState, _config: BStaminaConfig, cost: int = 1) -> bool:
	if cost <= 0 or state.charges < float(cost):
		return false
	state.charges = maxf(0.0, state.charges - float(cost))
	return true


## Regenerates charges over delta_seconds, clamped to max_charges.
static func regen(state: BStaminaState, config: BStaminaConfig, delta_seconds: float) -> void:
	if delta_seconds <= 0.0 or config.regen_seconds_per_charge <= 0.0:
		return
	state.charges = minf(float(config.max_charges), state.charges + delta_seconds / config.regen_seconds_per_charge)
