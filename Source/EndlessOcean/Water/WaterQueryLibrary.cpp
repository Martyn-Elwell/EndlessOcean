#include "WaterQueryLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PhysicsVolume.h"

float UWaterQueryLibrary::GetWaterDepthAt(const UObject* WorldContextObject, FVector Location, APhysicsVolume*& OutWaterVolume)
{
	OutWaterVolume = nullptr;

	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return 0.0f;
	}

	float BestDepth = 0.0f;

	// The world keeps a list of every placed physics volume (the "default" volume, which fills all space, is not in it).
	// Levels have a handful of these, so a linear scan is cheaper than any spatial query.
	for (auto It = World->GetNonDefaultPhysicsVolumeIterator(); It; ++It)
	{
		APhysicsVolume* Volume = It->Get();
		if (!Volume || !Volume->bWaterVolume || !Volume->EncompassesPoint(Location))
		{
			continue;
		}

		const float Depth = GetWaterSurfaceZ(Volume) - Location.Z;
		if (Depth > BestDepth)
		{
			BestDepth = Depth;
			OutWaterVolume = Volume;
		}
	}

	return BestDepth;
}

float UWaterQueryLibrary::GetWaterDepthAt(const UObject* WorldContextObject, const FVector& Location)
{
	APhysicsVolume* Ignored = nullptr;
	return GetWaterDepthAt(WorldContextObject, Location, Ignored);
}

bool UWaterQueryLibrary::IsUnderwater(const UObject* WorldContextObject, FVector Location)
{
	return GetWaterDepthAt(WorldContextObject, Location) > 0.0f;
}

float UWaterQueryLibrary::GetWaterSurfaceZ(const APhysicsVolume* WaterVolume)
{
	// Brush volumes are axis-aligned boxes in practice; the top of the bounding box is the surface.
	return WaterVolume ? WaterVolume->GetComponentsBoundingBox(true).Max.Z : 0.0f;
}
