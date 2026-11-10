#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// Native gameplay tags shared by the ability framework.
// Per-ability cooldown tags live next to each hero's abilities, declared the same way, e.g.
//   .h:   BRUTUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Clavus_StickyGrenade);
//   .cpp: UE_DEFINE_GAMEPLAY_TAG(Cooldown_Clavus_StickyGrenade, "Cooldown.Clavus.StickyGrenade");
namespace BrutusTags
{
// Owner cannot move, jump, dash, fire or activate abilities.
BRUTUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Stunned);
// Owner is dead; abilities are blocked.
BRUTUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
// Gameplay event sent when an ability or weapon hits something.
BRUTUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit);
// Root of every per-ability cooldown tag ("Cooldown.<Hero>.<Ability>").
BRUTUS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown);
}
