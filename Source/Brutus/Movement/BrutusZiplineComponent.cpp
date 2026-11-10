#include "Movement/BrutusZiplineComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Core/BrutusSimConvert.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Movement/BrutusZiplineRegistry.h"

bsim::ZiplineSegment UBrutusZiplineComponent::GetWorldSegment() const
{
	const FTransform& T = GetComponentTransform();
	return bsim::ZiplineSegment{ToSim(T.TransformPosition(StartLocal)), ToSim(T.TransformPosition(EndLocal))};
}

void UBrutusZiplineComponent::OnRegister()
{
	Super::OnRegister();
	if (UWorld* World = GetWorld())
	{
		if (UBrutusZiplineRegistry* Registry = World->GetSubsystem<UBrutusZiplineRegistry>())
		{
			Registry->Register(this);
		}
	}
	UpdateCableMesh();
}

void UBrutusZiplineComponent::OnUnregister()
{
	if (UWorld* World = GetWorld())
	{
		if (UBrutusZiplineRegistry* Registry = World->GetSubsystem<UBrutusZiplineRegistry>())
		{
			Registry->Unregister(this);
		}
	}
	if (CableMesh)
	{
		CableMesh->DestroyComponent();
		CableMesh = nullptr;
	}
	Super::OnUnregister();
}

void UBrutusZiplineComponent::UpdateCableMesh()
{
	if (!GetOwner())
	{
		return;
	}
	if (!CableMesh)
	{
		CableMesh = NewObject<UStaticMeshComponent>(GetOwner(), NAME_None, RF_Transient);
		CableMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
		CableMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CableMesh->SetupAttachment(this);
		CableMesh->RegisterComponent();
	}

	// Engine cylinder: 100 cm tall along Z, 100 cm diameter. Stretch it between the two points.
	const FVector Mid = (StartLocal + EndLocal) * 0.5f;
	const FVector Dir = EndLocal - StartLocal;
	const float Length = Dir.Size();
	const FRotator Rot = FRotationMatrix::MakeFromZ(Dir.GetSafeNormal()).Rotator();
	CableMesh->SetRelativeLocationAndRotation(Mid, Rot);
	CableMesh->SetRelativeScale3D(FVector(0.06f, 0.06f, FMath::Max(Length, 1.f) / 100.f));
}
