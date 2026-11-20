#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WaterQueryLibrary.generated.h"

class APhysicsVolume;

/**
 * Answers "is this point under water, and how deep?" for any system: breath, the underwater camera, weapon range.
 *
 * Water is plain engine Physics Volumes with "Water Volume" ticked, the same thing the character movement component
 * uses to switch into swimming. The surface is the top of the volume's bounds, so a flooding system can later raise
 * the water by moving or scaling the volume and everything here follows.
 *
 * Unity comparison: a static utility class, but a UBlueprintFunctionLibrary also exposes each function to Blueprints.
 */
UCLASS()
class ENDLESSOCEAN_API UWaterQueryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Depth of Location below the water surface, in cm. 0 if the point isn't in any water volume.
	 * Where water volumes overlap, the deepest result wins.
	 */
	UFUNCTION(BlueprintPure, Category="Water", meta=(WorldContext="WorldContextObject"))
	static float GetWaterDepthAt(const UObject* WorldContextObject, FVector Location, APhysicsVolume*& OutWaterVolume);

	/** Convenience overload when the volume isn't needed. */
	static float GetWaterDepthAt(const UObject* WorldContextObject, const FVector& Location);

	/** True if Location is inside a water volume. */
	UFUNCTION(BlueprintPure, Category="Water", meta=(WorldContext="WorldContextObject"))
	static bool IsUnderwater(const UObject* WorldContextObject, FVector Location);

	/** World Z of the top of a water volume (its surface). */
	UFUNCTION(BlueprintPure, Category="Water")
	static float GetWaterSurfaceZ(const APhysicsVolume* WaterVolume);
};
