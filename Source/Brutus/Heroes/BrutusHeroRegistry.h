#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "BrutusHeroRegistry.generated.h"

class ABrutusCharacter;

UENUM(BlueprintType)
enum class EBrutusHero : uint8
{
	Clavus,
	Malleus,
};

// Maps a hero to its pawn class.
//
// Registration is by naming convention, nothing to call: the pawn class of hero <Name> is the native
// class "/Script/Brutus.BrutusHero_<Name>", i.e. declare `class ABrutusHero_Clavus : public ABrutusCharacter`
// (any folder of the Brutus module, e.g. Heroes/Clavus/BrutusHero_Clavus.h). Native classes of this module
// are always loaded, so the lookup cannot miss once the class exists. Until then the hero falls back to
// the plain ABrutusCharacter (with a warning in the log).
namespace BrutusHeroRegistry
{
BRUTUS_API TSubclassOf<ABrutusCharacter> GetHeroClass(EBrutusHero Hero);

// "Clavus" / "Malleus" (the enum entry name).
BRUTUS_API FString GetHeroName(EBrutusHero Hero);

// Case-insensitive parse of a hero name. False if no hero has that name.
BRUTUS_API bool ParseHeroName(const FString& Name, EBrutusHero& OutHero);

BRUTUS_API bool IsValidHero(EBrutusHero Hero);
}
