#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TeamVision.h"
#include "BrutusTeamVisionSubsystem.generated.h"

// Server only: which enemies each team can see. Heroes an enemy team cannot see are not
// replicated to that team at all (see ABrutusCharacter::IsNetRelevantFor), so wallhacks
// have nothing to show.
UCLASS()
class BRUTUS_API UBrutusTeamVisionSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	// Starting values for tuning.
	static constexpr float SightRadius = 3000.f;
	static constexpr float UpdateInterval = 0.1f;
	static constexpr float LookaheadSeconds = 0.1f;

	UBrutusTeamVisionSubsystem();

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	bool IsVisibleToTeam(const AActor* Target, uint8 Team) const;

private:
	void UpdateVision();

	bsim::TeamVision Vision;
	float TimeSinceUpdate = 0.f;
};
