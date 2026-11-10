#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BrutusZiplineRegistry.generated.h"

class UBrutusZiplineComponent;

// All ziplines in the world, for movement queries on both server and clients.
UCLASS()
class BRUTUS_API UBrutusZiplineRegistry : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void Register(UBrutusZiplineComponent* Zipline);
	void Unregister(UBrutusZiplineComponent* Zipline);

	// Nearest cable within MaxDist of Pos, or nullptr.
	UBrutusZiplineComponent* FindNearest(const FVector& Pos, float MaxDist) const;

private:
	TArray<TWeakObjectPtr<UBrutusZiplineComponent>> Ziplines;
};
