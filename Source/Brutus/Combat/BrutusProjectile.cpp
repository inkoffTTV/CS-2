#include "Combat/BrutusProjectile.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Character/BrutusCharacter.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"

ABrutusProjectile::ABrutusProjectile()
{
	bReplicates = true;
	SetReplicatingMovement(true);
	InitialLifeSpan = 10.f;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(8.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Collision->SetCanEverAffectNavigation(false);
	RootComponent = Collision;

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->SetUpdatedComponent(Collision);
	Movement->InitialSpeed = 0.f; // the spawn helper sets the world-space velocity directly
	Movement->MaxSpeed = 0.f;     // no limit
	Movement->bInitialVelocityInLocalSpace = false;
	Movement->bRotationFollowsVelocity = true;
	Movement->bShouldBounce = false;
	Movement->ProjectileGravityScale = 1.f;
}

ABrutusProjectile* ABrutusProjectile::SpawnProjectile(TSubclassOf<ABrutusProjectile> Class, ABrutusCharacter* Shooter, const FVector& Location, const FVector& Velocity)
{
	UWorld* World = Shooter ? Shooter->GetWorld() : nullptr;
	if (!Class || !World || !Shooter->HasAuthority())
	{
		return nullptr;
	}
	const FTransform Transform(Velocity.IsNearlyZero() ? Shooter->GetActorRotation() : Velocity.Rotation(), Location);
	ABrutusProjectile* Projectile = World->SpawnActorDeferred<ABrutusProjectile>(Class, Transform, Shooter, Shooter, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		return nullptr;
	}
	Projectile->TeamId = Shooter->GetTeamId();
	Projectile->SourcePlayerState = Shooter->GetPlayerState();
	Projectile->Movement->Velocity = Velocity;
	Projectile->FinishSpawning(Transform);
	return Projectile;
}

ABrutusCharacter* ABrutusProjectile::GetShooter() const
{
	return Cast<ABrutusCharacter>(GetInstigator());
}

UAbilitySystemComponent* ABrutusProjectile::GetSourceAbilitySystem() const
{
	const IAbilitySystemInterface* Source = Cast<IAbilitySystemInterface>(SourcePlayerState.Get());
	return Source ? Source->GetAbilitySystemComponent() : nullptr;
}

void ABrutusProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABrutusProjectile, TeamId);
}

void ABrutusProjectile::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	// Clients simulate the flight between updates with the replicated velocity.
	if (Movement)
	{
		Movement->Velocity = NewVelocity;
	}
}

void ABrutusProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Friendly fire is off: the shooter and its teammates do not stop the projectile.
	// Done on clients too (TeamId and Instigator arrive with the initial replication) so their copy agrees.
	for (TActorIterator<ABrutusCharacter> It(GetWorld()); It; ++It)
	{
		if (*It == GetInstigator() || (TeamId != 255 && It->GetTeamId() == TeamId))
		{
			Collision->IgnoreActorWhenMoving(*It, true);
		}
	}

	if (HasAuthority())
	{
		Movement->OnProjectileStop.AddDynamic(this, &ABrutusProjectile::HandleProjectileStop);
	}
}

void ABrutusProjectile::HandleProjectileStop(const FHitResult& ImpactResult)
{
	if (HasAuthority())
	{
		OnImpact(ImpactResult);
	}
}

void ABrutusProjectile::OnImpact(const FHitResult& Hit)
{
	Explode();
}

void ABrutusProjectile::Explode()
{
	if (HasAuthority())
	{
		Destroy();
	}
}
