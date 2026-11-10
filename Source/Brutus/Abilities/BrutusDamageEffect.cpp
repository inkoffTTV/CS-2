#include "Abilities/BrutusDamageEffect.h"

#include "Abilities/BrutusAttributeSet.h"
#include "AbilitySystemComponent.h"

const FName UBrutusDamageEffect::DamageName(TEXT("Damage"));

UBrutusDamageEffect::UBrutusDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = DamageName;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UBrutusAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(Modifier);
}

void UBrutusDamageEffect::Apply(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, float Amount)
{
	if (!Source || !Target || Amount <= 0.f || !Source->IsOwnerActorAuthoritative())
	{
		return;
	}
	const FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(UBrutusDamageEffect::StaticClass(), 1.f, Source->MakeEffectContext());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(DamageName, -Amount);
		Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target);
	}
}
