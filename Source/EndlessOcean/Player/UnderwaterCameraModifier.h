#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "Engine/Scene.h"
#include "UnderwaterCameraModifier.generated.h"

/**
 * Blends an underwater look (tint, desaturation, vignette, short-range blur) onto the camera whenever the camera
 * itself is inside a water volume, fading in and out as you dive and surface.
 *
 * Done as a camera modifier rather than a post-process volume in every flooded room: it follows the water
 * automatically (including water that rises later) and stays next to the other camera effects. It's always on,
 * unlike head-bob, so it's separate from USurvivorCameraModifier.
 *
 * Make a Blueprint subclass to tune UnderwaterSettings; tick a setting's checkbox to make it apply.
 */
UCLASS(Blueprintable, BlueprintType, PrioritizeCategories=(Underwater))
class ENDLESSOCEAN_API UUnderwaterCameraModifier : public UCameraModifier
{
	GENERATED_BODY()

public:

	UUnderwaterCameraModifier();

	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

	/** How far into the underwater look we are (0 = above water, 1 = fully under). */
	UFUNCTION(BlueprintPure, Category="Underwater")
	float GetUnderwaterBlend() const { return UnderwaterBlend; }

protected:

	virtual void ModifyPostProcess(float DeltaTime, float& PostProcessBlendWeight, FPostProcessSettings& PostProcessSettings) override;

	/** Post-process settings applied underwater. Only the ticked (overridden) settings have any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Underwater")
	FPostProcessSettings UnderwaterSettings;

	/** How quickly the look fades in and out when the camera crosses the surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Underwater", meta=(ClampMin=0.5, ClampMax=30))
	float BlendSpeed = 8.0f;

private:

	float UnderwaterBlend = 0.0f;
};
