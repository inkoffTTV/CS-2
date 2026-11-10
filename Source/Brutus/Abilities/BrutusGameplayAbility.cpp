#include "Abilities/BrutusGameplayAbility.h"

#include "Abilities/BrutusCooldownEffect.h"
#include "Abilities/BrutusGameplayTags.h"
#include "Brutus.h"
#include "Character/BrutusCharacter.h"

UBrutusGameplayAbility::UBrutusGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	CooldownGameplayEffectClass = UBrutusCooldownEffect::StaticClass();

	ActivationBlockedTags.AddTag(BrutusTags::State_Stunned);
	ActivationBlockedTags.AddTag(BrutusTags::State_Dead);
}

ABrutusCharacter* UBrutusGameplayAbility::GetBrutusCharacter() const
{
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	return Info ? Cast<ABrutusCharacter>(Info->AvatarActor.Get()) : nullptr;
}

const FGameplayTagContainer* UBrutusGameplayAbility::GetCooldownTags() const
{
	CooldownTagsScratch.Reset();
	if (const FGameplayTagContainer* ParentTags = Super::GetCooldownTags())
	{
		CooldownTagsScratch.AppendTags(*ParentTags);
	}
	const FGameplayTag Tag = GetCooldownTag();
	if (Tag.IsValid())
	{
		CooldownTagsScratch.AddTag(Tag);
	}
	return &CooldownTagsScratch;
}

void UBrutusGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UGameplayEffect* CooldownEffect = GetCooldownGameplayEffect();
	if (CooldownSeconds <= 0.f || !CooldownEffect)
	{
		return;
	}
	const FGameplayTag Tag = GetCooldownTag();
	if (!Tag.IsValid())
	{
		UE_LOG(LogBrutus, Warning, TEXT("%s has a cooldown but no cooldown tag; the cooldown will not block it."), *GetNameSafe(GetClass()));
	}
	const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CooldownEffect->GetClass(), GetAbilityLevel(Handle, ActorInfo));
	if (!Spec.IsValid())
	{
		return;
	}
	if (Tag.IsValid())
	{
		Spec.Data->DynamicGrantedTags.AddTag(Tag);
	}
	Spec.Data->SetSetByCallerMagnitude(UBrutusCooldownEffect::DurationName, CooldownSeconds);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
}
