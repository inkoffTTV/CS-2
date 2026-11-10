#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BrutusHitscanWeaponComponent.generated.h"

// Greybox hitscan rifle. Client aims and shows the tracer at once; the server rewinds
// hitboxes to what the shooter saw and applies damage through GAS.
UCLASS(ClassGroup = (Brutus), meta = (BlueprintSpawnableComponent))
class BRUTUS_API UBrutusHitscanWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBrutusHitscanWeaponComponent();

	// Starting values for tuning.
	UPROPERTY(EditAnywhere, Category = "Weapon")
	float FireRate = 10.f; // shots per second

	UPROPERTY(EditAnywhere, Category = "Weapon")
	float Damage = 20.f;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	float HeadMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	float MaxRange = 10000.f;

	void StartFire();
	void StopFire();

	// World time of the last confirmed hit and whether it was a headshot (owning client, for the HUD).
	double GetLastHitMarkerTime() const { return LastHitMarkerTime; }
	bool WasLastHitHead() const { return bLastHitHead; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void FireOnce();
	bool ComputeShot(FVector& OutStart, FVector& OutDir) const;
	void DrawTracer(const FVector& Start, const FVector& End, const FColor& Color) const;

	UFUNCTION(Server, Reliable)
	void Server_Fire(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, double ClientServerTime);

	UFUNCTION(Client, Unreliable)
	void Client_ConfirmHit(bool bHead);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_Tracer(FVector_NetQuantize Start, FVector_NetQuantize End);

	bool bTriggerHeld = false;
	double NextLocalFireTime = 0.0;
	double LastServerFireTime = -1000.0;
	double LastHitMarkerTime = -1000.0;
	bool bLastHitHead = false;
};
