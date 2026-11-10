#pragma once

#include "CoreMinimal.h"
#include "Combat/BrutusProjectile.h"
#include "BrutusClavusStickyGrenade.generated.h"

class ABrutusCharacter;
class UStaticMeshComponent;

// Clavus's sticky grenade. Sticks to the first thing it hits (attaches to a hero), explodes
// FuseAfterStickSeconds later, or FuseUnstuckSeconds after the throw if it never hits anything.
// The explosion damages enemies of the thrower's team within Radius that the blast point can see
// (WorldStatic line of sight to their centre), with linear falloff down to MinFalloff at the edge.
// If the hero it is stuck to is destroyed (death, hero switch), it detaches and explodes where it is
// when the fuse runs out. Damage comes from the thrower's player-state ability system, so it still
// counts after the thrower died.
UCLASS()
class BRUTUS_API ABrutusClavusStickyGrenade : public ABrutusProjectile
{
	GENERATED_BODY()

public:
	ABrutusClavusStickyGrenade();

	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float FuseAfterStickSeconds = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float FuseUnstuckSeconds = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float Damage = 90.f;

	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float Radius = 350.f;

	// Damage fraction at the edge of the radius.
	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float MinFalloff = 0.3f;

	bool IsStuck() const { return bStuck; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnImpact(const FHitResult& Hit) override;
	virtual void Explode() override;

private:
	// Server.
	void StickTo(const FHitResult& Hit);
	void HandleFuse();
	void DetachFromCarrier();
	void ApplyExplosionDamage(const FVector& Center) const;

	// Every machine: stop simulating the flight and stop colliding.
	void StopLocalFlight();

	UFUNCTION()
	void OnRep_Stuck();

	UFUNCTION()
	void HandleCarrierDestroyed(AActor* DestroyedActor);

	// Cosmetic blast marker.
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_Exploded(FVector_NetQuantize Center);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Visual;

	UPROPERTY(ReplicatedUsing = OnRep_Stuck)
	bool bStuck = false;

	bool bExploded = false;

	// Server: hero the grenade is attached to.
	TWeakObjectPtr<ABrutusCharacter> Carrier;

	FTimerHandle FuseTimer;
};
