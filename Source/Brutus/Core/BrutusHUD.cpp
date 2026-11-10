#include "Core/BrutusHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void ABrutusHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	DrawCrosshair();
}

void ABrutusHUD::DrawCrosshair()
{
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;
	const float Size = 8.f;
	DrawLine(CX - Size, CY, CX + Size, CY, FLinearColor::White, 2.f);
	DrawLine(CX, CY - Size, CX, CY + Size, FLinearColor::White, 2.f);
}

void ABrutusHUD::DrawTextRow(const FString& Text, int32 Row, const FLinearColor& Color)
{
	DrawText(Text, Color, 24.f, 24.f + Row * 20.f, GEngine->GetMediumFont());
}
