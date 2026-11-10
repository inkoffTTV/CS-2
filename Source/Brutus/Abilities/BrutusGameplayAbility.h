#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "BrutusGameplayAbility.generated.h"

class ABrutusCharacter;

// Ability input slots. The value is the GAS InputID the ability spec is granted with.
UENUM(BlueprintType)
enum class EBrutusAbilityInput : uint8
{
	None = 0,
	Ability1, // Q
	Ability2, // F
	Ultimate, // R
	AltFire,  // right mouse button
};

// Base for every hero ability: locally predicted, instanced per actor, blocked while stunned or dead.
//
// Declaring a hero ability (in its constructor):
//   InputId = EBrutusAbilityInput::Ability1;
//   CooldownSeconds = 12.f;
//   CooldownTag = BrutusClavusTags::Cooldown_StickyGrenade; // native tag "Cooldown.Clavus.StickyGrenade"
// (or override GetCooldownTag()). The cooldown is one UBrutusCooldownEffect whose duration is
// CooldownSeconds and which grants the ability's cooldown tag; CommitAbility() applies it and
// CanActivateAbility() checks it. Without a valid cooldown tag the cooldown cannot block activation.
UCLASS(Abstract)
class BRUTUS_API UBrutusGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBrutusGameplayAbility();

	// Key the ability is bound to when granted.
	UPROPERTY(EditDefaultsOnly, Category = "Brutus")
	EBrutusAbilityInput InputId = EBrutusAbilityInput::None;

	// 0 = no cooldown.
	UPROPERTY(EditDefaultsOnly, Category = "Brutus")
	float CooldownSeconds = 0.f;

	// Unique per ability, under the Cooldown root (e.g. "Cooldown.Malleus.Chain").
	UPROPERTY(EditDefaultsOnly, Category = "Brutus")
	FGameplayTag CooldownTag;

	virtual FGameplayTag GetCooldownTag() const { return CooldownTag; }

	// The hero this ability runs on (the avatar), or null.
	ABrutusCharacter* GetBrutusCharacter() const;

	// UGameplayAbility
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

private:
	// Scratch container returned by GetCooldownTags (the engine API returns a pointer).
	mutable FGameplayTagContainer CooldownTagsScratch;
};
