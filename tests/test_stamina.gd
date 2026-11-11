extends RefCounted
const T := preload("res://tests/t.gd")


func test_full_stamina_allows_exactly_max_charges() -> void:
	var cfg := BStaminaConfig.new()
	var s := BStaminaState.new()
	T.is_true(BStamina.try_consume(s, cfg))
	T.is_true(BStamina.try_consume(s, cfg))
	T.is_true(BStamina.try_consume(s, cfg))
	T.is_false(BStamina.try_consume(s, cfg))
	T.approx(s.charges, 0.0)


func test_regen_one_charge_per_interval() -> void:
	var cfg := BStaminaConfig.new()
	var s := BStaminaState.new(0.0)
	BStamina.regen(s, cfg, 2.5)
	T.approx(s.charges, 1.0)


func test_partial_regen_not_enough_to_spend() -> void:
	var cfg := BStaminaConfig.new()
	var s := BStaminaState.new(0.0)
	BStamina.regen(s, cfg, 1.25)
	T.approx(s.charges, 0.5)
	T.is_false(BStamina.try_consume(s, cfg))
	T.approx(s.charges, 0.5)


func test_regen_clamps_at_max() -> void:
	var cfg := BStaminaConfig.new()
	var s := BStaminaState.new(0.0)
	BStamina.regen(s, cfg, 100.0)
	T.approx(s.charges, 3.0)


func test_multi_charge_cost_fails_without_side_effects() -> void:
	var cfg := BStaminaConfig.new()
	var s := BStaminaState.new(1.9)
	T.is_false(BStamina.try_consume(s, cfg, 2))
	T.approx(s.charges, 1.9)
