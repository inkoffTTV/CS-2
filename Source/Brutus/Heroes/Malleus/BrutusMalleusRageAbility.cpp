#include "Heroes/Malleus/BrutusMalleusRageAbility.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Heroes/Malleus/BrutusHero_Malleus.h"
#include "Heroes/Malleus/BrutusMalleusEffects.h"
#include "Heroes/Malleus/BrutusMalleusTags.h"

UBrutusMalleusRageAbility::UBrutusMalleusRageAbility()
{
	InputId = EBrutusAbilityInput::Ultimate;
	CooldownSeconds = 75.f;
	CooldownTag = BrutusMalleusTags::Cooldown_Rage;
}

void UBrutusMalleusRageAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ABrutusHero_Malleus* Hero = GetMalleus();
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!Hero || !ASC || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (IsAuthority())
	{
		// Effects only on the server: the replicated attributes reach the clients.
		const FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		RageEffectHandle = ASC->ApplyGameplayEffectToSelf(GetDefault<UBrutusMalleusRageEffect>(), 1.f, Context);

		const FGameplayEffectSpecHandle HealSpec = ASC->MakeOutgoingSpec(UBrutusMalleusHealEffect::StaticClass(), 1.f, Context);
		if (HealSpec.IsValid())
		{
			HealSpec.Data->SetSetByCallerMagnitude(UBrutusMalleusHealEffect::HealName, Heal);
			ASC->ApplyGameplayEffectSpecToSelf(*HealSpec.Data.Get());
		}
	}

	// Server: authoritative size. Owning client: predicted, so the owner's movement grows at the same moment as the server's.
	RagingHero = Hero;
	Hero->SetRageActive(true);

	UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, UBrutusMalleusRageEffect::DurationSeconds);
	Wait->OnFinish.AddDynamic(this, &UBrutusMalleusRageAbility::OnRageFinished);
	Wait->ReadyForActivation();
}

void UBrutusMalleusRageAbility::OnRageFinished()
{
	if (IsActive())
	{
		EndSelf(false);
	}
}

void UBrutusMalleusRageAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsActive())
	{
		// Also on cancel (death, hero change): the buff and the size never outlive the ability.
		if (RageEffectHandle.IsValid())
		{
			if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
			{
				ASC->RemoveActiveGameplayEffect(RageEffectHandle);
			}
			RageEffectHandle = FActiveGameplayEffectHandle();
		}
		if (ABrutusHero_Malleus* Hero = RagingHero.Get())
		{
			Hero->SetRageActive(false);
		}
		RagingHero.Reset();
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
