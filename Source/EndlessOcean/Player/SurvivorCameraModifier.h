#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "SurvivorCameraModifier.generated.h"

class ACharacter;
class UStaminaComponent;

/**
 * Makes the first-person camera feel heavy and vulnerable: head-bob, idle and exhausted breathing,
 * a roll when strafing, and look lag (the view trails the mouse slightly and leans into turns).
 *
 * Unreal runs camera modifiers on the PlayerCameraManager after it works out the base view each frame, so this
 * is the place for camera effects, rather than Tick on the character. Unity comparison: similar to a Cinemachine
 * extension or noise profile layered on top of the base camera.
 *
 * Only the *view* is changed. The control rotation (aim) is untouched, so it can never desync input.
 * Toggle at runtime with EnableModifier/DisableModifier (fades over AlphaInTime/AlphaOutTime). See
 * ASurvivorPlayerController::SetCameraMotionEnabled. Create a Blueprint subclass to tune values.
 *
 * Category names have no spaces (HeadBob, not "Head Bob") because the PrioritizeCategories list is space-separated.
 * The Details panel still displays them as "Head Bob" etc.
 */
UCLASS(Blueprintable, BlueprintType, PrioritizeCategories=(HeadBob, Breathing, StrafeRoll, LookLag))
class ENDLESSOCEAN_API USurvivorCameraModifier : public UCameraModifier
{
	GENERATED_BODY()

public:

	USurvivorCameraModifier();

	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;
	virtual void EnableModifier() override;

protected:

	// --- Head-bob ---

	/** Bob the camera while walking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HeadBob")
	bool bEnableHeadBob = true;

	/** Gait cycles per second at walking speed (each cycle is two footsteps). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HeadBob", meta=(EditCondition="bEnableHeadBob", ClampMin=0.1, ClampMax=5, Units="Hz"))
	float BobFrequency = 0.9f;

	/** How far the camera dips on each footstep. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HeadBob", meta=(EditCondition="bEnableHeadBob", ClampMin=0, ClampMax=10, Units="cm"))
	float BobVerticalAmplitude = 2.0f;

	/** Side-to-side sway per gait cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HeadBob", meta=(EditCondition="bEnableHeadBob", ClampMin=0, ClampMax=10, Units="cm"))
	float BobHorizontalAmplitude = 1.2f;

	/** Camera roll per gait cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HeadBob", meta=(EditCondition="bEnableHeadBob", ClampMin=0, ClampMax=5, Units="deg"))
	float BobRollAmplitude = 0.6f;

	/** Bob amplitude multiplier while sprinting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HeadBob", meta=(EditCondition="bEnableHeadBob", ClampMin=0, ClampMax=5))
	float SprintAmplitudeMultiplier = 1.8f;

	/** Bob frequency multiplier while sprinting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HeadBob", meta=(EditCondition="bEnableHeadBob", ClampMin=0.1, ClampMax=5))
	float SprintFrequencyMultiplier = 1.4f;

	/** Bob amplitude multiplier while crouched. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HeadBob", meta=(EditCondition="bEnableHeadBob", ClampMin=0, ClampMax=2))
	float CrouchAmplitudeMultiplier = 0.6f;

	/** How quickly bob fades in and out when starting and stopping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HeadBob", meta=(EditCondition="bEnableHeadBob", ClampMin=0.1, ClampMax=30))
	float BobBlendSpeed = 6.0f;

	// --- Breathing ---

	/** Slow vertical breathing motion, stronger when low on stamina. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breathing")
	bool bEnableBreathing = true;

	/** Breaths per second at rest. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breathing", meta=(EditCondition="bEnableBreathing", ClampMin=0.05, ClampMax=2, Units="Hz"))
	float BreathingFrequency = 0.25f;

	/** Vertical breathing motion at rest. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breathing", meta=(EditCondition="bEnableBreathing", ClampMin=0, ClampMax=5, Units="cm"))
	float BreathingAmplitude = 0.4f;

	/** Breathing amplitude multiplier at zero stamina (blends from 1 at full stamina). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breathing", meta=(EditCondition="bEnableBreathing", ClampMin=1, ClampMax=10))
	float ExhaustedAmplitudeMultiplier = 3.0f;

	/** Breathing rate multiplier at zero stamina (blends from 1 at full stamina). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breathing", meta=(EditCondition="bEnableBreathing", ClampMin=1, ClampMax=10))
	float ExhaustedRateMultiplier = 2.5f;

	// --- Strafe roll ---

	/** Roll the camera when moving sideways. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="StrafeRoll")
	bool bEnableStrafeRoll = true;

	/** Roll angle when strafing at walk speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="StrafeRoll", meta=(EditCondition="bEnableStrafeRoll", ClampMin=0, ClampMax=10, Units="deg"))
	float StrafeRollAngle = 1.5f;

	/** How quickly the roll follows the strafe input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="StrafeRoll", meta=(EditCondition="bEnableStrafeRoll", ClampMin=0.1, ClampMax=30))
	float StrafeRollBlendSpeed = 5.0f;

	// --- Look lag ---

	/** View rotation trails the aim slightly for a heavy feel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LookLag")
	bool bEnableLookLag = true;

	/** How quickly the view catches up with the aim. Higher means less lag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LookLag", meta=(EditCondition="bEnableLookLag", ClampMin=1, ClampMax=60))
	float LookLagSpeed = 14.0f;

	/** Maximum the view may trail behind the aim, so fast flicks don't feel floaty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LookLag", meta=(EditCondition="bEnableLookLag", ClampMin=0, ClampMax=20, Units="deg"))
	float MaxLookLagAngle = 5.0f;

	/** Degrees of roll per degree of yaw lag: leans into turns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LookLag", meta=(EditCondition="bEnableLookLag", ClampMin=0, ClampMax=1))
	float TurnRollScale = 0.25f;

private:

	float BobPhase = 0.0f;
	float BreathPhase = 0.0f;
	float CurrentBobIntensity = 0.0f;
	float CurrentSprintAlpha = 0.0f;
	float CurrentStrafeRoll = 0.0f;
	FRotator LaggedRotation = FRotator::ZeroRotator;
	bool bHasLaggedRotation = false;

	/** Cached so we don't search components every frame. Re-found if the view target changes. */
	TWeakObjectPtr<const ACharacter> CachedCharacter;
	TWeakObjectPtr<const UStaminaComponent> CachedStamina;
};
