#include "Heroes/Clavus/BrutusClavusStickyGrenade.h"

#include "Abilities/BrutusDamageEffect.h"
#include "AbilitySystemComponent.h"
#include "AreaDamage.h"
#include "Character/BrutusCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ABrutusClavusStickyGrenade::ABrutusClavusStickyGrenade()
{
	GetCollision()->InitSphereRadius(10.f);

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(GetCollision());
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetGenerateOverlapEvents(false);
	Visual->SetCanEverAffectNavigation(false);
	Visual->SetRelativeScale3D(FVector(0.2f)); // engine sphere is 100 cm across
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		Visual->SetStaticMesh(Sphere.Object);
	}
}

void ABrutusClavusStickyGrenade::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABrutusClavusStickyGrenade, bStuck);
}

void ABrutusClavusStickyGrenade::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		// Never hit anything: explode in the air.
		GetWorldTimerManager().SetTimer(FuseTimer, this, &ABrutusClavusStickyGrenade::HandleFuse, FuseUnstuckSeconds, false);
	}
}

void ABrutusClavusStickyGrenade::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(FuseTimer);
	if (ABrutusCharacter* Hero = Carrier.Get())
	{
		Hero->OnDestroyed.RemoveDynamic(this, &ABrutusClavusStickyGrenade::HandleCarrierDestroyed);
	}
	Carrier.Reset();
	Super::EndPlay(EndPlayReason);
}

void ABrutusClavusStickyGrenade::OnImpact(const FHitResult& Hit)
{
	// Do not call Super: the base explodes on impact.
	if (!bStuck && !bExploded)
	{
		StickTo(Hit);
	}
}

void ABrutusClavusStickyGrenade::StickTo(const FHitResult& Hit)
{
	bStuck = true;
	StopLocalFlight();

	// Only heroes carry the grenade; walls, floors and shields do not move, so it just stays put.
	ABrutusCharacter* Hero = Cast<ABrutusCharacter>(Hit.GetActor());
	if (IsValid(Hero) && !Hero->IsActorBeingDestroyed())
	{
		Carrier = Hero;
		AttachToActor(Hero, FAttachmentTransformRules::KeepWorldTransform);
		Hero->OnDestroyed.AddDynamic(this, &ABrutusClavusStickyGrenade::HandleCarrierDestroyed);
	}

	// Replaces the in-flight fuse.
	GetWorldTimerManager().SetTimer(FuseTimer, this, &ABrutusClavusStickyGrenade::HandleFuse, FuseAfterStickSeconds, false);
	ForceNetUpdate();
}

void ABrutusClavusStickyGrenade::OnRep_Stuck()
{
	if (bStuck)
	{
		StopLocalFlight();
	}
}

void ABrutusClavusStickyGrenade::StopLocalFlight()
{
	if (UProjectileMovementComponent* Move = GetProjectileMovement())
	{
		Move->StopMovementImmediately();
		Move->Deactivate();
	}
	// Attached to a hero it must not block anyone's movement or shots.
	GetCollision()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABrutusClavusStickyGrenade::HandleCarrierDestroyed(AActor* DestroyedActor)
{
	// The carrier died (or switched hero): stay where it was and let the fuse run out there.
	DetachFromCarrier();
}

void ABrutusClavusStickyGrenade::DetachFromCarrier()
{
	if (ABrutusCharacter* Hero = Carrier.Get())
	{
		Hero->OnDestroyed.RemoveDynamic(this, &ABrutusClavusStickyGrenade::HandleCarrierDestroyed);
	}
	Carrier.Reset();
	if (GetAttachParentActor())
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		ForceNetUpdate();
	}
}

void ABrutusClavusStickyGrenade::HandleFuse()
{
	Explode();
}

void ABrutusClavusStickyGrenade::Explode()
{
	if (!HasAuthority() || bExploded)
	{
		return;
	}
	bExploded = true;
	GetWorldTimerManager().ClearTimer(FuseTimer);

	// Detach first: the blast may kill the carrier, whose destruction must not call back into us.
	DetachFromCarrier();
	const FVector Center = GetActorLocation();
	Multicast_Exploded(Center);
	ApplyExplosionDamage(Center);

	Super::Explode(); // destroys
}

void ABrutusClavusStickyGrenade::ApplyExplosionDamage(const FVector& Center) const
{
	UWorld* World = GetWorld();
	UAbilitySystemComponent* SourceASC = GetSourceAbilitySystem();
	if (!World || !SourceASC)
	{
		return;
	}

	// Pick every victim before applying damage: a lethal hit destroys that hero immediately.
	struct FVictim
	{
		TWeakObjectPtr<ABrutusCharacter> Hero;
		float Amount = 0.f;
	};
	TArray<FVictim> Victims;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ClavusGrenadeSight), false, this);
	for (TActorIterator<ABrutusCharacter> It(World); It; ++It)
	{
		ABrutusCharacter* Hero = *It;
		if (!IsValid(Hero) || Hero == GetInstigator())
		{
			continue;
		}
		const uint8 HeroTeam = Hero->GetTeamId();
		if (HeroTeam == 255 || (GetTeamId() != 255 && HeroTeam == GetTeamId()))
		{
			continue; // no team yet, or the thrower's team (friendly fire is off)
		}
		const FVector Target = Hero->GetActorLocation();
		const float Distance = FVector::Dist(Center, Target);
		if (Distance > Radius)
		{
			continue;
		}
		FHitResult Blocker;
		if (World->LineTraceSingleByObjectType(Blocker, Center, Target, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			continue; // behind a wall or a shield
		}
		const float Amount = Damage * bsim::RadialFalloff(Distance, Radius, MinFalloff);
		if (Amount > 0.f)
		{
			FVictim& Victim = Victims.AddDefaulted_GetRef();
			Victim.Hero = Hero;
			Victim.Amount = Amount;
		}
	}

	for (const FVictim& Victim : Victims)
	{
		ABrutusCharacter* Hero = Victim.Hero.Get();
		if (IsValid(Hero))
		{
			UBrutusDamageEffect::Apply(SourceASC, Hero->GetAbilitySystemComponent(), Victim.Amount);
		}
	}
}

void ABrutusClavusStickyGrenade::Multicast_Exploded_Implementation(FVector_NetQuantize Center)
{
	if (GetWorld()->GetNetMode() != NM_DedicatedServer)
	{
		DrawDebugSphere(GetWorld(), Center, Radius, 16, FColor::Orange, false, 0.4f);
	}
}
