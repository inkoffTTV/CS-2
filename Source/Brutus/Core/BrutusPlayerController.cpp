#include "Core/BrutusPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Engine/LocalPlayer.h"

void ABrutusPlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	BuildInput();
}

void ABrutusPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(MappingContext, 0);
		}
	}
}

const UInputAction* ABrutusPlayerController::GetAction(EBrutusInput Input) const
{
	const int32 Index = static_cast<int32>(Input);
	return Actions.IsValidIndex(Index) ? Actions[Index].Get() : nullptr;
}

void ABrutusPlayerController::BuildInput()
{
	if (MappingContext)
	{
		return;
	}

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Brutus"));

	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		Actions.Add(Action);
		return Action;
	};

	// Order must match EBrutusInput.
	UInputAction* Move = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	UInputAction* Look = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	UInputAction* Jump = MakeAction(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	UInputAction* Dash = MakeAction(TEXT("IA_Dash"), EInputActionValueType::Boolean);
	UInputAction* Slide = MakeAction(TEXT("IA_Slide"), EInputActionValueType::Boolean);
	UInputAction* Fire = MakeAction(TEXT("IA_Fire"), EInputActionValueType::Boolean);
	UInputAction* Interact = MakeAction(TEXT("IA_Interact"), EInputActionValueType::Boolean);
	check(Actions.Num() == static_cast<int32>(EBrutusInput::Count));

	// Move: X = right, Y = forward.
	auto Swizzle = [this]() { return NewObject<UInputModifierSwizzleAxis>(MappingContext); };
	auto Negate = [this]() { return NewObject<UInputModifierNegate>(MappingContext); };

	MappingContext->MapKey(Move, EKeys::W).Modifiers.Add(Swizzle());
	{
		FEnhancedActionKeyMapping& S = MappingContext->MapKey(Move, EKeys::S);
		S.Modifiers.Add(Swizzle());
		S.Modifiers.Add(Negate());
	}
	MappingContext->MapKey(Move, EKeys::A).Modifiers.Add(Negate());
	MappingContext->MapKey(Move, EKeys::D);

	MappingContext->MapKey(Look, EKeys::Mouse2D);
	MappingContext->MapKey(Jump, EKeys::SpaceBar);
	MappingContext->MapKey(Dash, EKeys::LeftShift);
	MappingContext->MapKey(Slide, EKeys::LeftControl);
	MappingContext->MapKey(Slide, EKeys::C);
	MappingContext->MapKey(Fire, EKeys::LeftMouseButton);
	MappingContext->MapKey(Interact, EKeys::E);
}
