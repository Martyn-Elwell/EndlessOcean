#include "UnderwaterCameraModifier.h"

#include "WaterQueryLibrary.h"

UUnderwaterCameraModifier::UUnderwaterCameraModifier()
{
	// Murky green-blue water: tinted, washed out, dark at the edges, and blurry beyond a few metres.
	// Each bOverride_ flag is the "checkbox" next to a setting in the Details panel.
	UnderwaterSettings.bOverride_SceneColorTint = true;
	UnderwaterSettings.SceneColorTint = FLinearColor(0.45f, 0.75f, 0.8f);

	UnderwaterSettings.bOverride_ColorSaturation = true;
	UnderwaterSettings.ColorSaturation = FVector4(0.6f, 0.6f, 0.6f, 1.0f);

	UnderwaterSettings.bOverride_VignetteIntensity = true;
	UnderwaterSettings.VignetteIntensity = 0.9f;

	UnderwaterSettings.bOverride_DepthOfFieldFocalDistance = true;
	UnderwaterSettings.DepthOfFieldFocalDistance = 150.0f;
	UnderwaterSettings.bOverride_DepthOfFieldFstop = true;
	UnderwaterSettings.DepthOfFieldFstop = 1.2f;
}

bool UUnderwaterCameraModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	// Decide from the final camera position (after head-bob etc.), then let the base class run, which calls
	// ModifyPostProcess below.
	const float Target = UWaterQueryLibrary::IsUnderwater(this, InOutPOV.Location) ? 1.0f : 0.0f;
	UnderwaterBlend = FMath::FInterpTo(UnderwaterBlend, Target, DeltaTime, BlendSpeed);

	return Super::ModifyCamera(DeltaTime, InOutPOV);
}

void UUnderwaterCameraModifier::ModifyPostProcess(float DeltaTime, float& PostProcessBlendWeight, FPostProcessSettings& PostProcessSettings)
{
	PostProcessBlendWeight = UnderwaterBlend * Alpha;
	if (PostProcessBlendWeight > 0.0f)
	{
		PostProcessSettings = UnderwaterSettings;
	}
}
