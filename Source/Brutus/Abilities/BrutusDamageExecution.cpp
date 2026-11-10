#include "Abilities/BrutusDamageExecution.h"

#include "Abilities/BrutusAttributeSet.h"
#include "Abilities/BrutusDamageEffect.h"

namespace
{
struct FBrutusDamageCaptures
{
	FGameplayEffectAttributeCaptureDefinition DamageTakenMultiplier;

	FBrutusDamageCaptures()
		: DamageTakenMultiplier(UBrutusAttributeSet::GetDamageTakenMultiplierAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
	{
	}
};

const FBrutusDamageCaptures& DamageCaptures()
{
	static const FBrutusDamageCaptures Captures;
	return Captures;
}
}

UBrutusDamageExecution::UBrutusDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageCaptures().DamageTakenMultiplier);
}

void UBrutusDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float Multiplier = 1.f;
	if (!ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageCaptures().DamageTakenMultiplier, EvalParams, Multiplier))
	{
		Multiplier = 1.f;
	}

	const float Damage = Spec.GetSetByCallerMagnitude(UBrutusDamageEffect::DamageName, false, 0.f);
	const float FinalDamage = FMath::Max(0.f, Damage) * FMath::Max(0.f, Multiplier);
	if (FinalDamage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UBrutusAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, -FinalDamage));
	}
}
