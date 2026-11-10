#pragma once

#include "CoreMinimal.h"

// The two sides of BRUTUS (docs/design.md, "Стороны").
namespace BrutusTeams
{
	constexpr uint8 Optimates = 0;
	constexpr uint8 Populares = 1;

	inline const TCHAR* GetTeamName(uint8 TeamId)
	{
		switch (TeamId)
		{
		case Optimates: return TEXT("Optimates");
		case Populares: return TEXT("Populares");
		default: return TEXT("None");
		}
	}

	// Team colours: Optimates cool purple-blue, Populares warm fire-orange.
	inline FLinearColor GetTeamColor(uint8 TeamId)
	{
		switch (TeamId)
		{
		case Optimates: return FLinearColor(0.42f, 0.36f, 1.f);
		case Populares: return FLinearColor(1.f, 0.45f, 0.1f);
		default: return FLinearColor::White;
		}
	}
}
