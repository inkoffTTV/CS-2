#include "Heroes/Malleus/BrutusMalleusGroundSlamAbility.h"

#include "AbilitySystemComponent.h"
#include "Abilities/BrutusDamageEffect.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AreaDamage.h"
#include "Character/BrutusCharacter.h"
#include "Character/BrutusHitboxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/BrutusSimConvert.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Heroes/Malleus/BrutusMalleusEffects.h"
#include "Heroes/Malleus/BrutusMalleusTags.h"
#include "Overlap.h"

namespace
{
// Extra time a client waits for the server to end the ability before ending its own copy.
constexpr float ClientEndGraceSeconds = 1.f;
}

UBrutusMalleusGroundSlamAbility::UBrutusMalleusGroundSlamAbility()
{
	InputId = EBrutusAbilityInput::Ability1;
	CooldownSeconds = 14.f;
	CooldownTag = BrutusMalleusTags::Cooldown_GroundSlam;
}

void UBrutusMalleusGroundSlamAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ABrutusCharacter* Hero = GetBrutusCharacter();
	if (!Hero || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Runs on the predicting client and on the server, so the owner's movement does not wait for a round trip.
	// The launch puts the hero in falling mode (also off a zipline or out of a slide).
	Hero->LaunchCharacter(FVector(0.f, 0.f, LaunchZ), false, true);

	// Landing is judged by the server only; the client just draws the impact and waits for the server's end.
	BoundHero = Hero;
	Hero->LandedDelegate.AddUniqueDynamic(this, &UBrutusMalleusGroundSlamAbility::OnLanded);

	UAbilityTask_WaitDelay* Timeout = UAbilityTask_WaitDelay::WaitDelay(this, IsAuthority() ? MaxAirTime : MaxAirTime + ClientEndGraceSeconds);
	Timeout->OnFinish.AddDynamic(this, &UBrutusMalleusGroundSlamAbility::OnAirTimeout);
	Timeout->ReadyForActivation();
}

void UBrutusMalleusGroundSlamAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UnbindLanded();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UBrutusMalleusGroundSlamAbility::UnbindLanded()
{
	if (ABrutusCharacter* Hero = BoundHero.Get())
	{
		Hero->LandedDelegate.RemoveDynamic(this, &UBrutusMalleusGroundSlamAbility::OnLanded);
	}
	BoundHero.Reset();
}

void UBrutusMalleusGroundSlamAbility::OnLanded(const FHitResult& Hit)
{
	ABrutusCharacter* Hero = BoundHero.Get();
	if (!IsActive() || !Hero)
	{
		return;
	}
	UnbindLanded(); // one slam per activation

	if (CanDrawDebug())
	{
		const FVector Feet = Hero->GetActorLocation() - FVector(0.f, 0.f, Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		DrawDebugCircle(GetWorld(), Feet + FVector(0.f, 0.f, 5.f), Radius, 48, FColor::Orange, false, 0.6f, 0, 4.f, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
	}
	if (IsAuthority())
	{
		Slam(*Hero);
		EndSelf(false);
	}
}

void UBrutusMalleusGroundSlamAbility::OnAirTimeout()
{
	if (IsActive())
	{
		EndSelf(true); // never landed (or the server's end never reached this client)
	}
}

void UBrutusMalleusGroundSlamAbility::Slam(ABrutusCharacter& Hero)
{
	UWorld* World = GetWorld();
	UAbilitySystemComponent* SourceASC = Hero.GetAbilitySystemComponent();
	if (!World || !SourceASC)
	{
		return;
	}
	// Impact point: the feet. Current poses: this is the server's own landing, not a client's shot.
	const FVector Center = Hero.GetActorLocation() - FVector(0.f, 0.f, Hero.GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	const uint8 Team = Hero.GetTeamId();

	struct FSlamHit
	{
		TWeakObjectPtr<ABrutusCharacter> Target;
		float Fraction = 0.f;
	};
	TArray<FSlamHit> Hits; // collected first: damage can kill (and destroy) a hero
	for (TActorIterator<ABrutusCharacter> It(World); It; ++It)
	{
		ABrutusCharacter* Target = *It;
		if (Target == &Hero || Target->GetTeamId() == Team || !Target->GetAbilitySystemComponent())
		{
			continue;
		}
		const UBrutusHitboxComponent* Hitbox = Target->FindComponentByClass<UBrutusHitboxComponent>();
		const std::vector<bsim::Capsule> Shapes = Hitbox ? Hitbox->BuildWorldCapsules() : std::vector<bsim::Capsule>();
		if (Shapes.empty() || !bsim::SphereOverlapsCapsules(ToSim(Center), Radius, Shapes))
		{
			continue;
		}
		// Distance to the body's surface (0 when the centre is inside it), so big heroes are not favoured.
		const bsim::Capsule& Body = Shapes.front();
		const FVector A = ToUE(Body.A);
		const FVector B = ToUE(Body.B);
		const float SurfaceDistance = FMath::Max(0.f, static_cast<float>(FMath::PointDistToSegment(Center, A, B)) - Body.Radius);

		// No slamming through walls.
		FCollisionQueryParams Params(SCENE_QUERY_STAT(BrutusMalleusSlam), false, &Hero);
		Params.AddIgnoredActor(Target);
		if (World->LineTraceTestByObjectType(Center + FVector(0.f, 0.f, 50.f), (A + B) * 0.5f, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			continue;
		}
		FSlamHit& NewHit = Hits.AddDefaulted_GetRef();
		NewHit.Target = Target;
		NewHit.Fraction = bsim::RadialFalloff(FMath::Min(SurfaceDistance, Radius), Radius, MinFalloff);
	}

	for (const FSlamHit& SlamHit : Hits)
	{
		ABrutusCharacter* Target = SlamHit.Target.Get();
		UAbilitySystemComponent* TargetASC = Target ? Target->GetAbilitySystemComponent() : nullptr;
		if (!TargetASC || SlamHit.Fraction <= 0.f)
		{
			continue;
		}
		const FGameplayEffectSpecHandle Slow = SourceASC->MakeOutgoingSpec(UBrutusMalleusSlowEffect::StaticClass(), 1.f, SourceASC->MakeEffectContext());
		if (Slow.IsValid())
		{
			SourceASC->ApplyGameplayEffectSpecToTarget(*Slow.Data.Get(), TargetASC);
		}
		UBrutusDamageEffect::Apply(SourceASC, TargetASC, Damage * SlamHit.Fraction);
	}
}
