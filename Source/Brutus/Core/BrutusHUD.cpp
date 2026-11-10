#include "Core/BrutusHUD.h"

#include "Abilities/BrutusAttributeSet.h"
#include "Character/BrutusCharacter.h"
#include "Core/BrutusPlayerState.h"
#include "Core/BrutusTeams.h"
#include "Character/BrutusMovementComponent.h"
#include "Combat/BrutusHitscanWeaponComponent.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"

void ABrutusHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	DrawCrosshair();

	int32 Row = 0;
	if (const APlayerState* OwnPS = GetOwningPlayerController() ? GetOwningPlayerController()->PlayerState : nullptr)
	{
		DrawTextRow(FString::Printf(TEXT("Ping %.0f ms"), OwnPS->GetPingInMilliseconds()), Row++);
	}
	if (const ABrutusPlayerState* PS = GetOwningPlayerController() ? GetOwningPlayerController()->GetPlayerState<ABrutusPlayerState>() : nullptr)
	{
		const UBrutusAttributeSet* Attr = PS->GetAttributes();
		DrawTextRow(FString::Printf(TEXT("Health %.0f / %.0f"), Attr->GetHealth(), Attr->GetMaxHealth()), Row++);
		DrawTextRow(BrutusTeams::GetTeamName(PS->GetTeamId()), Row++, BrutusTeams::GetTeamColor(PS->GetTeamId()));

		// Debug: enemies that exist on this client are exactly the ones the server lets us see.
		int32 VisibleEnemies = 0;
		for (TActorIterator<ABrutusCharacter> It(GetWorld()); It; ++It)
		{
			VisibleEnemies += (It->GetTeamId() != 255 && It->GetTeamId() != PS->GetTeamId()) ? 1 : 0;
		}
		DrawTextRow(FString::Printf(TEXT("Visible enemies %d"), VisibleEnemies), Row++, FLinearColor(0.6f, 0.8f, 1.f));
	}
	if (const ABrutusCharacter* Hero = Cast<ABrutusCharacter>(GetOwningPawn()))
	{
		if (const UBrutusMovementComponent* Move = Hero->GetBrutusMovement())
		{
			DrawTextRow(FString::Printf(TEXT("Stamina %.1f / %d"), Move->GetStaminaCharges(), Move->GetMaxStaminaCharges()), Row++);
		}
		// Hit marker: an X around the crosshair for 0.2 s after a confirmed hit (red on headshot).
		if (const UBrutusHitscanWeaponComponent* Weapon = Hero->GetWeapon())
		{
			if (GetWorld()->GetTimeSeconds() - Weapon->GetLastHitMarkerTime() < 0.2)
			{
				const float CX = Canvas->ClipX * 0.5f;
				const float CY = Canvas->ClipY * 0.5f;
				const FLinearColor Color = Weapon->WasLastHitHead() ? FLinearColor::Red : FLinearColor::White;
				DrawLine(CX - 14.f, CY - 14.f, CX - 6.f, CY - 6.f, Color, 2.f);
				DrawLine(CX + 14.f, CY - 14.f, CX + 6.f, CY - 6.f, Color, 2.f);
				DrawLine(CX - 14.f, CY + 14.f, CX - 6.f, CY + 6.f, Color, 2.f);
				DrawLine(CX + 14.f, CY + 14.f, CX + 6.f, CY + 6.f, Color, 2.f);
			}
		}
	}
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
