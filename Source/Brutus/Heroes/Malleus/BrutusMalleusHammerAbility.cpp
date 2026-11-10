#include "Heroes/Malleus/BrutusMalleusHammerAbility.h"

#include "Abilities/BrutusDamageEffect.h"
#include "Character/BrutusCharacter.h"
#include "Character/BrutusHitboxComponent.h"
#include "Combat/BrutusLagCompensationSubsystem.h"
#include "Core/BrutusSimConvert.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Heroes/Malleus/BrutusMalleusTags.h"
#include "Overlap.h"

UBrutusMalleusHammerAbility::UBrutusMalleusHammerAbility()
{
	InputId = EBrutusAbilityInput::AltFire;
	CooldownSeconds = 0.9f;
	CooldownTag = BrutusMalleusTags::Cooldown_Hammer;
}

void UBrutusMalleusHammerAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	StartAim();
}

void UBrutusMalleusHammerAbility::OnAimReady(const FBrutusMalleusAimTargetData& Aim, bool bAuthority, bool bLocal)
{
	ABrutusCharacter* Hero = GetBrutusCharacter();
	UWorld* World = GetWorld();
	if (!Hero || !World)
	{
		return;
	}

	// "In front" is horizontal: looking up or down does not lift the swing.
	FVector Forward = FVector(Aim.Direction.X, Aim.Direction.Y, 0.f).GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		Forward = Hero->GetActorForwardVector().GetSafeNormal2D();
	}
	const FVector Center = FVector(Aim.Origin) + Forward * ReachForward;

	if (bLocal && CanDrawDebug())
	{
		DrawDebugSphere(World, Center, SphereRadius, 16, FColor::Orange, false, 0.25f);
	}
	if (!bAuthority)
	{
		return;
	}

	UBrutusLagCompensationSubsystem* LagComp = World->GetSubsystem<UBrutusLagCompensationSubsystem>();
	UAbilitySystemComponent* SourceASC = Hero->GetAbilitySystemComponent();
	if (!LagComp || !SourceASC)
	{
		return;
	}
	const double RewindTime = GetRewindTime(Aim);
	const uint8 Team = Hero->GetTeamId();

	// Collected first: damage can kill (and destroy) a hero.
	TArray<TWeakObjectPtr<ABrutusCharacter>> Victims;
	for (TActorIterator<ABrutusCharacter> It(World); It; ++It)
	{
		ABrutusCharacter* Target = *It;
		if (Target == Hero || Target->GetTeamId() == Team || !Target->GetAbilitySystemComponent())
		{
			continue; // self, teammates (no friendly fire), heroes without a player
		}
		// The pose the swinging client saw; a hero spawned too recently for that has only its current pose.
		std::optional<std::vector<bsim::Capsule>> Shapes = LagComp->SampleHitboxes(Target, RewindTime);
		if (!Shapes)
		{
			const UBrutusHitboxComponent* Hitbox = Target->FindComponentByClass<UBrutusHitboxComponent>();
			if (!Hitbox)
			{
				continue;
			}
			Shapes = Hitbox->BuildWorldCapsules();
		}
		if (Shapes->empty() || !bsim::SphereOverlapsCapsules(ToSim(Center), SphereRadius, *Shapes))
		{
			continue;
		}
		// No hitting through walls: the swing must reach the target's body from the eyes.
		const bsim::Capsule& Body = Shapes->front();
		const FVector BodyCenter = (ToUE(Body.A) + ToUE(Body.B)) * 0.5f;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(BrutusMalleusHammer), false, Hero);
		Params.AddIgnoredActor(Target);
		if (World->LineTraceTestByObjectType(Aim.Origin, BodyCenter, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			continue;
		}
		Victims.Add(Target);
	}

	for (const TWeakObjectPtr<ABrutusCharacter>& Victim : Victims)
	{
		if (ABrutusCharacter* Target = Victim.Get())
		{
			UBrutusDamageEffect::Apply(SourceASC, Target->GetAbilitySystemComponent(), Damage);
		}
	}
}
