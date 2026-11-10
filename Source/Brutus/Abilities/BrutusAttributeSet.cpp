#include "Abilities/BrutusAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UBrutusAttributeSet::UBrutusAttributeSet()
{
	// Starting value for tuning.
	InitMaxHealth(250.f);
	InitHealth(250.f);
	InitMoveSpeedMultiplier(1.f);
	InitDamageTakenMultiplier(1.f);
}

void UBrutusAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UBrutusAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBrutusAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBrutusAttributeSet, MoveSpeedMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBrutusAttributeSet, DamageTakenMultiplier, COND_None, REPNOTIFY_Always);
}

void UBrutusAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetMoveSpeedMultiplierAttribute() || Attribute == GetDamageTakenMultiplierAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UBrutusAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute != GetHealthAttribute())
	{
		return;
	}
	SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	if (GetHealth() <= 0.f && !bOutOfHealth)
	{
		bOutOfHealth = true;
		OnOutOfHealth.Broadcast(Data.EffectSpec.GetContext().GetOriginalInstigator());
	}
}

void UBrutusAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBrutusAttributeSet, Health, OldValue);
}

void UBrutusAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBrutusAttributeSet, MaxHealth, OldValue);
}

void UBrutusAttributeSet::OnRep_MoveSpeedMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBrutusAttributeSet, MoveSpeedMultiplier, OldValue);
}

void UBrutusAttributeSet::OnRep_DamageTakenMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBrutusAttributeSet, DamageTakenMultiplier, OldValue);
}
