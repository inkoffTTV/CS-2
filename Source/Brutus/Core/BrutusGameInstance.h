#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "BrutusGameInstance.generated.h"

UCLASS()
class BRUTUS_API UBrutusGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
};
