#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HitboxHistory.h"
#include "BrutusHitboxComponent.generated.h"

// Describes the hero's hit shapes for lag compensation. Greybox: body capsule + head sphere
// derived from the collision capsule. Real heroes will build these from their physics asset.
UCLASS(ClassGroup = (Brutus), meta = (BlueprintSpawnableComponent))
class BRUTUS_API UBrutusHitboxComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	std::vector<bsim::Capsule> BuildWorldCapsules() const;

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	float BodyTopFraction = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	float HeadCenterFraction = 0.9f;

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	float HeadRadius = 15.f;
};
