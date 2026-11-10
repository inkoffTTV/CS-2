#pragma once

#include "CoreMinimal.h"
#include "Vec3.h"

// Conversions between Unreal vectors and BrutusSim vectors (same units: cm, Z up).
inline bsim::Vec3 ToSim(const FVector& V)
{
	return bsim::Vec3{static_cast<float>(V.X), static_cast<float>(V.Y), static_cast<float>(V.Z)};
}

inline FVector ToUE(const bsim::Vec3& V)
{
	return FVector(V.X, V.Y, V.Z);
}
