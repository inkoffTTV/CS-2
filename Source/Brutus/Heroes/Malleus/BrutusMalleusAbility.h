#pragma once

#include "CoreMinimal.h"
#include "Abilities/BrutusGameplayAbility.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Engine/NetSerialization.h"
#include "BrutusMalleusAbility.generated.h"

class ABrutusHero_Malleus;
class APawn;

// Where the owning client aimed and when (its estimate of server time), sent to the server as
// GAS target data. The server validates it like a weapon shot and uses the time for lag compensation.
USTRUCT()
struct BRUTUS_API FBrutusMalleusAimTargetData : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	// Hero's eyes (GetPawnViewLocation) on the client.
	UPROPERTY()
	FVector_NetQuantize10 Origin = FVector::ZeroVector;

	// From the eyes towards the camera crosshair's aim point.
	UPROPERTY()
	FVector_NetQuantizeNormal Direction = FVector::ForwardVector;

	// GameState->GetServerWorldTimeSeconds() on the client when it activated.
	UPROPERTY()
	double ClientServerTime = 0.0;

	virtual UScriptStruct* GetScriptStruct() const override { return FBrutusMalleusAimTargetData::StaticStruct(); }

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FBrutusMalleusAimTargetData> : public TStructOpsTypeTraitsBase2<FBrutusMalleusAimTargetData>
{
	enum
	{
		WithNetSerializer = true,
	};
};

// Base of Malleus's abilities: small helpers plus the aimed-ability flow used by the hammer and the chain.
//
// Aimed flow (call StartAim() from ActivateAbility after CommitAbility):
//  - the locally controlled machine builds the aim, sends it to the server (when it is a client) and
//    calls OnAimReady(bLocal = true) for cosmetics; it then ends its instance without replicating the end;
//  - the server calls OnAimReady(bAuthority = true) once with the validated aim (from the client's target
//    data, or directly on a listen-server host) and ends the ability. Gameplay effects happen only there,
//    so a high-ping activation (prediction + confirmation) hits once.
UCLASS(Abstract)
class BRUTUS_API UBrutusMalleusAbility : public UBrutusGameplayAbility
{
	GENERATED_BODY()

public:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// Server shots further than this from the hero's eyes are rejected (same as the weapon).
	static constexpr float MaxAimOriginError = 200.f;
	// The server gives up waiting for the client's aim after this long.
	static constexpr float ServerAimWaitSeconds = 1.f;

	ABrutusHero_Malleus* GetMalleus() const;
	bool IsAuthority() const;
	// Ends this activation. Clients pass bReplicate = false so their local end never cancels the server's work.
	void EndSelf(bool bWasCancelled);

	void StartAim();
	// bAuthority: apply gameplay (damage, pulls). bLocal: draw cosmetics. A listen-server host gets both at once.
	virtual void OnAimReady(const FBrutusMalleusAimTargetData& Aim, bool bAuthority, bool bLocal) {}

	// Server time the aim's targets are rewound to.
	double GetRewindTime(const FBrutusMalleusAimTargetData& Aim) const;

	// Draws only where someone can see it (not on a dedicated server).
	bool CanDrawDebug() const;

private:
	static bool BuildLocalAim(const APawn& Pawn, FBrutusMalleusAimTargetData& Out);
	bool ValidateServerAim(const FBrutusMalleusAimTargetData& Aim) const;
	void OnServerTargetData(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ApplicationTag);

	UFUNCTION()
	void OnServerAimTimeout();

	FDelegateHandle TargetDataDelegateHandle;
};
