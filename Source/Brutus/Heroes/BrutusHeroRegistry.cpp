#include "Heroes/BrutusHeroRegistry.h"

#include "Brutus.h"
#include "Character/BrutusCharacter.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

namespace BrutusHeroRegistry
{
bool IsValidHero(EBrutusHero Hero)
{
	// The enum's reflection also lists the generated EBrutusHero_MAX entry; that is not a hero.
	const UEnum* Enum = StaticEnum<EBrutusHero>();
	const int32 Index = Enum->GetIndexByValue(static_cast<int64>(Hero));
	return Index != INDEX_NONE && !Enum->GetNameStringByIndex(Index).EndsWith(TEXT("_MAX"));
}

FString GetHeroName(EBrutusHero Hero)
{
	return StaticEnum<EBrutusHero>()->GetNameStringByValue(static_cast<int64>(Hero));
}

bool ParseHeroName(const FString& Name, EBrutusHero& OutHero)
{
	const UEnum* Enum = StaticEnum<EBrutusHero>();
	for (int32 Index = 0; Index < Enum->NumEnums(); ++Index)
	{
		const EBrutusHero Hero = static_cast<EBrutusHero>(Enum->GetValueByIndex(Index));
		if (IsValidHero(Hero) && Enum->GetNameStringByIndex(Index).Equals(Name, ESearchCase::IgnoreCase))
		{
			OutHero = Hero;
			return true;
		}
	}
	return false;
}

TSubclassOf<ABrutusCharacter> GetHeroClass(EBrutusHero Hero)
{
	if (IsValidHero(Hero))
	{
		const FString Path = FString::Printf(TEXT("/Script/Brutus.BrutusHero_%s"), *GetHeroName(Hero));
		UClass* Class = FindObject<UClass>(nullptr, *Path);
		if (Class && Class->IsChildOf(ABrutusCharacter::StaticClass()))
		{
			return Class;
		}
		UE_LOG(LogBrutus, Warning, TEXT("Hero class %s not found, using ABrutusCharacter"), *Path);
	}
	return ABrutusCharacter::StaticClass();
}
}
