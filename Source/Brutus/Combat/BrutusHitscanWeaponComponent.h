#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BrutusHitscanWeaponComponent.generated.h"

// Greybox hitscan weapon (rifle, nailgun, shotgun). Client aims and shows the tracers at once;
// the server rewinds hitboxes to what the shooter saw and applies damage through GAS.
// Each shot fires Pellets rays spread within SpreadDegrees; the client picks a seed per shot and both
// sides rebuild the same pellet directions with bsim::PelletDirections.
UCLASS(ClassGroup = (Brutus), meta = (BlueprintSpawnableComponent))
class BRUTUS_API UBrutusHitscanWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBrutusHitscanWeaponComponent();

	// Starting values for tuning.
	UPROPERTY(EditAnywhere, Category = "Weapon")
	float FireRate = 10.f; // shots per second

	// Per pellet.
	UPROPERTY(EditAnywhere, Category = "Weapon")
	float Damage = 20.f;

	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = "1", ClampMax = "32"))
	int32 Pellets = 1;

	// Half-angle of the spread cone. 0 with one pellet = perfectly accurate.
	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = "0", ClampMax = "45"))
	float SpreadDegrees = 0.f;

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
	TArray<FVector> BuildPelletDirections(const FVector& Dir, uint32 Seed) const;
	int32 GetClampedPellets() const { return FMath::Clamp(Pellets, 1, 32); }
	void DrawTracer(const FVector& Start, const FVector& End, const FColor& Color) const;
	bool TraceAimable(const FVector& From, const FVector& To, FHitResult& OutHit) const;

	UFUNCTION(Server, Reliable)
	void Server_Fire(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, double ClientServerTime, uint32 Seed);

	UFUNCTION(Client, Unreliable)
	void Client_ConfirmHit(bool bHead);

	// Other clients rebuild the pellets from the seed and trace them locally (cosmetic only).
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_Tracer(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, uint32 Seed);

	bool bTriggerHeld = false;
	FRandomStream SeedStream;
	double NextLocalFireTime = 0.0;
	// Server rate limit: token bucket that tolerates RPC bursts after packet loss.
	static constexpr float MaxShotBurst = 3.f;
	float ServerShotTokens = MaxShotBurst;
	double LastServerTokenTime = -1.0;
	double LastHitMarkerTime = -1000.0;
	bool bLastHitHead = false;
};
