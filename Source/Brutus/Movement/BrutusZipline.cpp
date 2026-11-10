#include "Movement/BrutusZipline.h"

#include "Movement/BrutusZiplineComponent.h"

ABrutusZipline::ABrutusZipline()
{
	Zipline = CreateDefaultSubobject<UBrutusZiplineComponent>(TEXT("Zipline"));
	RootComponent = Zipline;
	bReplicates = false;
}
