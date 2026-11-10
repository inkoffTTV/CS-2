#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BrutusProjectile.generated.h"

class ABrutusCharacter;
class APlayerState;
class UAbilitySystemComponent;
class UProjectileMovementComponent;
class USphereComponent;

// Base for ability projectiles (grenades, ...). The server spawns and simulates it; clients see a
// replicated copy. Teammates of the shooter (and the shooter) are not hit.
// On the server the first blocking hit stops the projectile and calls OnImpact(); the default
// OnImpact explodes, the default Explode destroys. Subclasses override both.
UCLASS(Abstract)
class BRUTUS_API ABrutusProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABrutusProjectile();

	// Server only: spawns Class at Location flying with world-space Velocity, owned by Shooter.
	static ABrutusProjectile* SpawnProjectile(TSubclassOf<ABrutusProjectile> Class, ABrutusCharacter* Shooter, const FVector& Location, const FVector& Velocity);

	template <class T>
	static T* SpawnProjectile(ABrutusCharacter* Shooter, const FVector& Location, const FVector& Velocity, TSubclassOf<T> Class = T::StaticClass())
	{
		return Cast<T>(SpawnProjectile(TSubclassOf<ABrutusProjectile>(Class.Get()), Shooter, Location, Velocity));
	}

	// Team of the shooter (255 = unknown). Replicated.
	uint8 GetTeamId() const { return TeamId; }

	// The hero that fired it, while it is alive (Instigator).
	ABrutusCharacter* GetShooter() const;

	// Server: ability system of the shooter's player state; stays valid after the shooter dies.
	UAbilitySystemComponent* GetSourceAbilitySystem() const;

	USphereComponent* GetCollision() const { return Collision; }
	UProjectileMovementComponent* GetProjectileMovement() const { return Movement; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostNetReceiveVelocity(const FVector& NewVelocity) override;

protected:
	virtual void BeginPlay() override;

	// Server: the projectile hit something and stopped.
	virtual void OnImpact(const FHitResult& Hit);

	// Server: detonate. Default destroys the projectile.
	virtual void Explode();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> Movement;

private:
	UFUNCTION()
	void HandleProjectileStop(const FHitResult& ImpactResult);

	UPROPERTY(Replicated)
	uint8 TeamId = 255;

	TWeakObjectPtr<APlayerState> SourcePlayerState;
};
