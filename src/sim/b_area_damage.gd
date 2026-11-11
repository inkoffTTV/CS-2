class_name BAreaDamage
extends RefCounted


## Damage multiplier for a radial blast: 1 at the center, linear down to min_fraction at radius, 0 beyond.
static func radial_falloff(distance: float, radius: float, min_fraction: float) -> float:
	if radius <= 0.0 or distance > radius:
		return 0.0
	if distance <= 0.0:
		return 1.0
	return 1.0 + (min_fraction - 1.0) * (distance / radius)
