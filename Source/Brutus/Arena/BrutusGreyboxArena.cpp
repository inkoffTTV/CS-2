#include "Arena/BrutusGreyboxArena.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Movement/BrutusZiplineComponent.h"
#include "UObject/ConstructorHelpers.h"

ABrutusGreyboxArena::ABrutusGreyboxArena()
{
	bReplicates = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	CubeMesh = Cube.Object;

	// Floor 60 x 60 m.
	AddBox(TEXT("Floor"), FVector(0, 0, -50), FVector(6000, 6000, 100));

	// Perimeter, 4 m high.
	AddBox(TEXT("WallNorth"), FVector(3000, 0, 200), FVector(100, 6000, 400));
	AddBox(TEXT("WallSouth"), FVector(-3000, 0, 200), FVector(100, 6000, 400));
	AddBox(TEXT("WallEast"), FVector(0, 3000, 200), FVector(6000, 100, 400));
	AddBox(TEXT("WallWest"), FVector(0, -3000, 200), FVector(6000, 100, 400));

	// Centre wall with two 6 m passages (at Y = -900 and Y = +900): blocks sight between halves.
	AddBox(TEXT("MidWallA"), FVector(0, -2100, 200), FVector(50, 1800, 400));
	AddBox(TEXT("MidWallB"), FVector(0, 0, 200), FVector(50, 1200, 400));
	AddBox(TEXT("MidWallC"), FVector(0, 2100, 200), FVector(50, 1800, 400));

	// Cover 2 x 2 x 1.5 m.
	const FVector CoverSize(200, 200, 150);
	const FVector CoverSpots[] = {
		{-1500, -1500, 75}, {-1500, 1500, 75}, {1500, -1500, 75}, {1500, 1500, 75},
		{-2200, 0, 75}, {2200, 0, 75}, {-800, 2200, 75}, {800, -2200, 75}};
	int32 Index = 0;
	for (const FVector& Spot : CoverSpots)
	{
		AddBox(*FString::Printf(TEXT("Cover%d"), Index++), Spot, CoverSize);
	}

	// Slide ramps: 20 degrees free-standing; 30 degrees leads up to the zipline platform.
	const float Ramp20Rise = 1500.f * FMath::Sin(FMath::DegreesToRadians(20.f));
	AddBox(TEXT("Ramp20"), FVector(-1500, -2600, Ramp20Rise * 0.5f), FVector(1500, 400, 20), FRotator(20.f, 0.f, 0.f));

	const float Ramp30Length = 600.f / FMath::Sin(FMath::DegreesToRadians(30.f));
	const float Ramp30Run = Ramp30Length * FMath::Cos(FMath::DegreesToRadians(30.f));
	AddBox(TEXT("Ramp30"), FVector(-2200.f + Ramp30Run * 0.5f, 2500, 300), FVector(Ramp30Length, 400, 20), FRotator(-30.f, 0.f, 0.f));

	// Platform 6 m high in the team-0 corner, zipline across the arena over the centre wall.
	AddBox(TEXT("Platform"), FVector(-2500, 2500, 300), FVector(600, 600, 600));

	Zipline = CreateDefaultSubobject<UBrutusZiplineComponent>(TEXT("Zipline"));
	Zipline->SetupAttachment(RootComponent);
	Zipline->StartLocal = FVector(-2500, 2500, 850);
	Zipline->EndLocal = FVector(2400, -2400, 450);
}

void ABrutusGreyboxArena::AddBox(const TCHAR* Name, const FVector& Center, const FVector& SizeCm, const FRotator& Rotation)
{
	UStaticMeshComponent* Box = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Box->SetupAttachment(RootComponent);
	Box->SetMobility(EComponentMobility::Static);
	Box->SetStaticMesh(CubeMesh);
	Box->SetRelativeLocationAndRotation(Center, Rotation);
	Box->SetRelativeScale3D(SizeCm / 100.f); // engine cube is 100 cm
	Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName); // WorldStatic: blocks shots and sight
}
