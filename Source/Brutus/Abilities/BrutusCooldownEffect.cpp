#include "Abilities/BrutusCooldownEffect.h"

const FName UBrutusCooldownEffect::DurationName(TEXT("Cooldown"));

UBrutusCooldownEffect::UBrutusCooldownEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = DurationName;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
}
