#include "Abilities/BrutusStunEffect.h"

#include "Abilities/BrutusGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

const FName UBrutusStunEffect::DurationName(TEXT("Duration"));

UBrutusStunEffect::UBrutusStunEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = DurationName;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
}

void UBrutusStunEffect::Apply(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, float Seconds)
{
	if (!Source || !Target || Seconds <= 0.f || !Source->IsOwnerActorAuthoritative())
	{
		return;
	}
	const FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(UBrutusStunEffect::StaticClass(), 1.f, Source->MakeEffectContext());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(DurationName, Seconds);
		Spec.Data->DynamicGrantedTags.AddTag(BrutusTags::State_Stunned);
		Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target);
	}
}

bool UBrutusStunEffect::IsActorStunned(const AActor* Actor)
{
	return IsStunned(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor));
}

bool UBrutusStunEffect::IsStunned(const UAbilitySystemComponent* ASC)
{
	return ASC && ASC->HasMatchingGameplayTag(BrutusTags::State_Stunned);
}
