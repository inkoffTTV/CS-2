#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BrutusClavusShield.generated.h"

class ABrutusCharacter;
class AController;
class UBoxComponent;
class UStaticMeshComponent;

// Clavus's deployable shield: a wall Width x Height x Thickness standing on the floor.
// Its collision object type is WorldStatic, so it blocks hitscan shots (the aim trace and the
// lag-compensation occlusion trace), projectiles, grenade line of sight and movement - for both teams.
// Enemy shots wear it down (TakeDamage / ApplyShieldDamage); it disappears at 0 HP or after LifetimeSeconds.
// Spawned only by the server (SpawnShield); clients see a replicated copy.
UCLASS()
class BRUTUS_API ABrutusClavusShield : public AActor
{
	GENERATED_BODY()

public:
	ABrutusClavusShield();

	static constexpr float Width = 300.f;
	static constexpr float Height = 200.f;
	static constexpr float Thickness = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float MaxHealth = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "Clavus")
	float LifetimeSeconds = 8.f;

	// Server: spawns a shield whose bottom centre is at FloorLocation, facing Yaw (its face is
	// perpendicular to that direction), owned by Owner's team.
	static ABrutusClavusShield* SpawnShield(ABrutusCharacter* OwnerHero, const FVector& FloorLocation, float Yaw);

	// Server: removes Amount health; destroys the shield at 0.
	void ApplyShieldDamage(float Amount);

	float GetHealth() const { return Health; }
	uint8 GetTeamId() const { return TeamId; }

	// Server: generic damage entry point (hitscan weapons call it on WorldStatic actors that stop a shot).
	// Damage from the shield's own team is ignored (friendly fire is off).
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Visual;

	UPROPERTY(Replicated)
	float Health = 0.f;

	UPROPERTY(Replicated)
	uint8 TeamId = 255;
};
