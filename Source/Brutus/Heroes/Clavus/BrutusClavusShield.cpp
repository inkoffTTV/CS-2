#include "Heroes/Clavus/BrutusClavusShield.h"

#include "Character/BrutusCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABrutusClavusShield::ABrutusClavusShield()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(false); // never moves after spawning

	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	// X is the facing direction: thickness along X, width along Y, height along Z.
	Collision->InitBoxExtent(FVector(Thickness * 0.5f, Width * 0.5f, Height * 0.5f));
	// WorldStatic: the lag-compensation occlusion trace and the grenade's sight trace query that type.
	Collision->SetCollisionObjectType(ECC_WorldStatic);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // do not yank cameras behind it
	Collision->SetCanEverAffectNavigation(false);
	RootComponent = Collision;

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Collision);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetGenerateOverlapEvents(false);
	Visual->SetCanEverAffectNavigation(false);
	// Engine cube is 100 cm per side, centred on its pivot.
	Visual->SetRelativeScale3D(FVector(Thickness / 100.f, Width / 100.f, Height / 100.f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Visual->SetStaticMesh(Cube.Object);
	}
}

ABrutusClavusShield* ABrutusClavusShield::SpawnShield(ABrutusCharacter* OwnerHero, const FVector& FloorLocation, float Yaw)
{
	UWorld* World = OwnerHero ? OwnerHero->GetWorld() : nullptr;
	if (!World || !OwnerHero->HasAuthority())
	{
		return nullptr;
	}
	const FTransform Transform(FRotator(0.f, Yaw, 0.f), FloorLocation + FVector(0.f, 0.f, Height * 0.5f));
	ABrutusClavusShield* Shield = World->SpawnActorDeferred<ABrutusClavusShield>(ABrutusClavusShield::StaticClass(), Transform, OwnerHero, OwnerHero, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Shield)
	{
		return nullptr;
	}
	Shield->TeamId = OwnerHero->GetTeamId();
	Shield->FinishSpawning(Transform);
	return Shield;
}

void ABrutusClavusShield::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		Health = MaxHealth;
		SetLifeSpan(LifetimeSeconds);
	}
}

void ABrutusClavusShield::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABrutusClavusShield, Health);
	DOREPLIFETIME(ABrutusClavusShield, TeamId);
}

float ABrutusClavusShield::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || DamageAmount <= 0.f)
	{
		return 0.f;
	}
	const ABrutusCharacter* Attacker = Cast<ABrutusCharacter>(DamageCauser);
	if (!Attacker && EventInstigator)
	{
		Attacker = Cast<ABrutusCharacter>(EventInstigator->GetPawn());
	}
	if (Attacker && TeamId != 255 && Attacker->GetTeamId() == TeamId)
	{
		return 0.f; // friendly fire is off: allies' shots are stopped but do not hurt it
	}
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	ApplyShieldDamage(Applied);
	return Applied;
}

void ABrutusClavusShield::ApplyShieldDamage(float Amount)
{
	if (!HasAuthority() || Amount <= 0.f || Health <= 0.f || IsActorBeingDestroyed())
	{
		return;
	}
	Health = FMath::Max(0.f, Health - Amount);
	if (Health <= 0.f)
	{
		Destroy();
	}
}
