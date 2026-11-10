#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "BrutusAttributeSet.generated.h"

#define BRUTUS_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_MULTICAST_DELEGATE_OneParam(FBrutusOutOfHealth, AActor* /*Instigator*/);

UCLASS()
class BRUTUS_API UBrutusAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UBrutusAttributeSet();

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Health")
	FGameplayAttributeData Health;
	BRUTUS_ATTRIBUTE_ACCESSORS(UBrutusAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Health")
	FGameplayAttributeData MaxHealth;
	BRUTUS_ATTRIBUTE_ACCESSORS(UBrutusAttributeSet, MaxHealth)

	// Server only: fires once when Health reaches 0. ResetOutOfHealth re-arms it after respawn.
	mutable FBrutusOutOfHealth OnOutOfHealth;
	void ResetOutOfHealth() { bOutOfHealth = false; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

private:
	bool bOutOfHealth = false;
};
