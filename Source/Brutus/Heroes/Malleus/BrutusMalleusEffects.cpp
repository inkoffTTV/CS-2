#include "Heroes/Malleus/BrutusMalleusEffects.h"

#include "Abilities/BrutusAttributeSet.h"

const FName UBrutusMalleusHealEffect::HealName(TEXT("Heal"));

UBrutusMalleusSlowEffect::UBrutusMalleusSlowEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(DurationSeconds));

	FGameplayModifierInfo Slow;
	Slow.Attribute = UBrutusAttributeSet::GetMoveSpeedMultiplierAttribute();
	Slow.ModifierOp = EGameplayModOp::Multiplicitive;
	Slow.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(SpeedMultiplier));
	Modifiers.Add(Slow);
}

UBrutusMalleusRageEffect::UBrutusMalleusRageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(DurationSeconds));

	FGameplayModifierInfo Armor;
	Armor.Attribute = UBrutusAttributeSet::GetDamageTakenMultiplierAttribute();
	Armor.ModifierOp = EGameplayModOp::Multiplicitive;
	Armor.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(DamageTakenMultiplier));
	Modifiers.Add(Armor);
}

UBrutusMalleusHealEffect::UBrutusMalleusHealEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = HealName;

	FGameplayModifierInfo Heal;
	Heal.Attribute = UBrutusAttributeSet::GetHealthAttribute();
	Heal.ModifierOp = EGameplayModOp::Additive;
	Heal.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(Heal);
}
