#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BrutusCooldownEffect.generated.h"

// Shared cooldown for every UBrutusGameplayAbility. Duration comes from the SetByCaller value
// DurationName; the ability adds its own cooldown tag to the spec's dynamic granted tags.
UCLASS()
class BRUTUS_API UBrutusCooldownEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBrutusCooldownEffect();

	static const FName DurationName;
};
