#include "Movement/BrutusZiplineRegistry.h"

#include "Core/BrutusSimConvert.h"
#include "Movement/BrutusZiplineComponent.h"

void UBrutusZiplineRegistry::Register(UBrutusZiplineComponent* Zipline)
{
	Ziplines.AddUnique(Zipline);
}

void UBrutusZiplineRegistry::Unregister(UBrutusZiplineComponent* Zipline)
{
	Ziplines.Remove(Zipline);
}

UBrutusZiplineComponent* UBrutusZiplineRegistry::FindNearest(const FVector& Pos, float MaxDist) const
{
	UBrutusZiplineComponent* Best = nullptr;
	float BestDist = MaxDist;
	for (const TWeakObjectPtr<UBrutusZiplineComponent>& Weak : Ziplines)
	{
		UBrutusZiplineComponent* Zipline = Weak.Get();
		if (!Zipline)
		{
			continue;
		}
		const float Dist = bsim::DistanceTo(Zipline->GetWorldSegment(), ToSim(Pos));
		if (Dist <= BestDist)
		{
			BestDist = Dist;
			Best = Zipline;
		}
	}
	return Best;
}
