#include "Heroes/Clavus/BrutusHero_Clavus.h"

#include "Combat/BrutusHitscanWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Heroes/Clavus/BrutusClavusRamAbility.h"
#include "Heroes/Clavus/BrutusClavusShieldAbility.h"
#include "Heroes/Clavus/BrutusClavusStickyGrenadeAbility.h"
#include "UObject/ConstructorHelpers.h"

ABrutusHero_Clavus::ABrutusHero_Clavus(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Capsule stays at the base 42 / 96.
	HeroMaxHealth = 275.f;
	PlaceholderMeshPath = FSoftObjectPath(TEXT("/Game/Characters/Clavus/SM_Hero_Clavus_Posed.SM_Hero_Clavus_Posed"));

	ShoulderPad = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShoulderPad"));
	// Same parent as the body cylinder, so network smoothing moves both together.
	ShoulderPad->SetupAttachment(GetMesh());
	ShoulderPad->ComponentTags.Add(GreyboxTag);
	ShoulderPad->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShoulderPad->SetGenerateOverlapEvents(false);
	ShoulderPad->SetCanEverAffectNavigation(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		ShoulderPad->SetStaticMesh(Cube.Object);
	}
	FitShoulderPad();
}

TArray<TSubclassOf<UBrutusGameplayAbility>> ABrutusHero_Clavus::GetHeroAbilities() const
{
	TArray<TSubclassOf<UBrutusGameplayAbility>> Abilities;
	Abilities.Add(TSubclassOf<UBrutusGameplayAbility>(UBrutusClavusStickyGrenadeAbility::StaticClass()));
	Abilities.Add(TSubclassOf<UBrutusGameplayAbility>(UBrutusClavusShieldAbility::StaticClass()));
	Abilities.Add(TSubclassOf<UBrutusGameplayAbility>(UBrutusClavusRamAbility::StaticClass()));
	return Abilities;
}

void ABrutusHero_Clavus::ConfigureWeapon(UBrutusHitscanWeaponComponent& InWeapon)
{
	// Nailgun: fast, accurate, light hits.
	InWeapon.FireRate = 12.f;
	InWeapon.Damage = 14.f;
	InWeapon.Pellets = 1;
	InWeapon.SpreadDegrees = 1.2f;
	InWeapon.HeadMultiplier = 1.5f;
}

void ABrutusHero_Clavus::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	FitShoulderPad();
}

void ABrutusHero_Clavus::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	FitShoulderPad();
}

void ABrutusHero_Clavus::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	FitShoulderPad();
}

void ABrutusHero_Clavus::FitShoulderPad()
{
	if (!ShoulderPad)
	{
		return;
	}
	// Engine cube is 100 cm per side, centred on its pivot. Pad: 50 long, 30 wide, 22 high.
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float Radius = Capsule->GetUnscaledCapsuleRadius();
	const float HalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	ShoulderPad->SetRelativeScale3D(FVector(0.5f, 0.3f, 0.22f));
	// +Y is the hero's right. Like the body cylinder, cancel the skeletal mesh's offset from the capsule
	// so the pad sits relative to the capsule centre (the mesh has no rotation or scale here).
	const float ShoulderZ = HalfHeight - 30.f;
	ShoulderPad->SetRelativeLocation(FVector(0.f, Radius + 2.f, ShoulderZ - GetMesh()->GetRelativeLocation().Z));
}
