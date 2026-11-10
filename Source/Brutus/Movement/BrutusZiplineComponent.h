#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Zipline.h"
#include "BrutusZiplineComponent.generated.h"

class UStaticMeshComponent;

// A straight zipline cable between two local points. Registers itself with the zipline registry
// and draws its own cable mesh, so it can live on any actor (placed zipline, greybox arena).
UCLASS(ClassGroup = (Brutus), meta = (BlueprintSpawnableComponent))
class BRUTUS_API UBrutusZiplineComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Zipline", meta = (MakeEditWidget = true))
	FVector StartLocal = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Zipline", meta = (MakeEditWidget = true))
	FVector EndLocal = FVector(2000.f, 0.f, 0.f);

	bsim::ZiplineSegment GetWorldSegment() const;

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

private:
	void UpdateCableMesh();

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> CableMesh;
};
