#include "Core/BrutusGameInstance.h"

#include "AbilitySystemGlobals.h"

void UBrutusGameInstance::Init()
{
	Super::Init();

	// Custom target data (Malleus aim) is serialized through the ability system's script-struct cache,
	// which InitGlobalData builds. Newer engines may already have done it; never do it twice.
	UAbilitySystemGlobals& Globals = UAbilitySystemGlobals::Get();
	if (!Globals.IsAbilitySystemGlobalsInitialized())
	{
		Globals.InitGlobalData();
	}
}
