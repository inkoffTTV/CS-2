#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BrutusPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;

UENUM()
enum class EBrutusInput : uint8
{
	Move,
	Look,
	Jump,
	Dash,
	Slide,
	Fire,
	Interact,
	Count UMETA(Hidden)
};

// Builds Enhanced Input actions and key mappings at runtime, so the project needs no input assets.
UCLASS()
class BRUTUS_API ABrutusPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	const UInputAction* GetAction(EBrutusInput Input) const;

private:
	void BuildInput();

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> Actions;
};
