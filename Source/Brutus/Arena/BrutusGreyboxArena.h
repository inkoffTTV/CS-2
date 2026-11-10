#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BrutusGreyboxArena.generated.h"

class UStaticMeshComponent;
class UBrutusZiplineComponent;

// Milestone 1 test arena built from engine cubes: floor, walls, cover, slide ramps,
// a platform and a zipline. Drop one at the world origin of an otherwise empty map.
UCLASS()
class BRUTUS_API ABrutusGreyboxArena : public AActor
{
	GENERATED_BODY()

public:
	ABrutusGreyboxArena();

private:
	void AddBox(const TCHAR* Name, const FVector& Center, const FVector& SizeCm, const FRotator& Rotation = FRotator::ZeroRotator);

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBrutusZiplineComponent> Zipline;
};
