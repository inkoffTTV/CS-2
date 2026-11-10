#include "Character/BrutusHitboxComponent.h"

#include "Components/CapsuleComponent.h"
#include "Core/BrutusSimConvert.h"
#include "GameFramework/Character.h"

std::vector<bsim::Capsule> UBrutusHitboxComponent::BuildWorldCapsules() const
{
	std::vector<bsim::Capsule> Shapes;
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return Shapes;
	}
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const FVector Center = Capsule->GetComponentLocation();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float Height = Capsule->GetScaledCapsuleHalfHeight() * 2.f;
	const float Bottom = Center.Z - Height * 0.5f;

	const FVector BodyA(Center.X, Center.Y, Bottom + Radius);
	const FVector BodyB(Center.X, Center.Y, FMath::Max(Bottom + Radius, Bottom + Height * BodyTopFraction - Radius));
	const FVector Head(Center.X, Center.Y, Bottom + Height * HeadCenterFraction);

	Shapes.push_back(bsim::Capsule{ToSim(BodyA), ToSim(BodyB), Radius, bsim::HitZone::Body});
	Shapes.push_back(bsim::Capsule{ToSim(Head), ToSim(Head), HeadRadius, bsim::HitZone::Head});
	return Shapes;
}
