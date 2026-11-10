#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HitboxHistory.h"
#include "BrutusLagCompensationSubsystem.generated.h"

class ABrutusCharacter;

struct FBrutusRewindHit
{
	TWeakObjectPtr<ABrutusCharacter> Target;
	float Distance = 0.f;
	bsim::HitZone Zone = bsim::HitZone::Body;
	FVector ImpactPoint = FVector::ZeroVector;
};

// Server only: records every hero's hitboxes each tick (3 s of history) and traces shots
// against the poses the shooter actually saw.
UCLASS()
class BRUTUS_API UBrutusLagCompensationSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static constexpr double HistorySeconds = 3.0;
	static constexpr double MaxShotRewind = 0.25;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	double GetServerTime() const;

	TOptional<FBrutusRewindHit> RewindTrace(const FVector& Start, const FVector& Dir, float MaxRange, double ShotServerTime, const AActor* Shooter) const;

private:
	void OnHeroTeleported(ABrutusCharacter* Hero);

	TMap<TWeakObjectPtr<ABrutusCharacter>, bsim::HitboxHistory> Histories;
	TSet<TWeakObjectPtr<ABrutusCharacter>> PendingTeleports;
};
