#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "BrutusDamageExecution.generated.h"

// Health -= SetByCaller "Damage" (positive) x the target's DamageTakenMultiplier.
UCLASS()
class BRUTUS_API UBrutusDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UBrutusDamageExecution();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
