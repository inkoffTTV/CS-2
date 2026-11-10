#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BrutusHUD.generated.h"

// Canvas HUD for the greybox milestone: crosshair and debug readouts, no UMG assets.
UCLASS()
class BRUTUS_API ABrutusHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawCrosshair();
	void DrawTextRow(const FString& Text, int32 Row, const FLinearColor& Color = FLinearColor::White);
};
