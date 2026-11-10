#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BrutusZipline.generated.h"

class UBrutusZiplineComponent;

// Placeable zipline. Static level geometry: exists on server and clients, not replicated.
UCLASS()
class BRUTUS_API ABrutusZipline : public AActor
{
	GENERATED_BODY()

public:
	ABrutusZipline();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBrutusZiplineComponent> Zipline;
};
