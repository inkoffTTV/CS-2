#include "Abilities/BrutusGameplayTags.h"

namespace BrutusTags
{
UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Stunned, "State.Stunned", "Cannot move, jump, dash, fire or use abilities.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "Hero is dead.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Hit, "Event.Hit", "An ability or weapon hit something.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown, "Cooldown", "Root of per-ability cooldown tags.");
}
