#include "SurvivorCameraModifier.h"

#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "StaminaComponent.h"
#include "SurvivorMovementComponent.h"

USurvivorCameraModifier::USurvivorCameraModifier()
{
	// Fade in and out when toggled instead of snapping.
	AlphaInTime = 0.4f;
	AlphaOutTime = 0.4f;
}

void USurvivorCameraModifier::EnableModifier()
{
	Super::EnableModifier();

	// Start the look lag from the current view rather than wherever it was when last disabled.
	bHasLaggedRotation = false;
}

bool USurvivorCameraModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	// Base class updates Alpha (the fade in/out weight) and runs any Blueprint ModifyCamera override.
	Super::ModifyCamera(DeltaTime, InOutPOV);

	const ACharacter* Character = CameraOwner ? Cast<ACharacter>(CameraOwner->GetViewTarget()) : nullptr;
	if (!Character || Alpha <= 0.0f)
	{
		bHasLaggedRotation = false;
		return false;
	}

	if (CachedCharacter.Get() != Character)
	{
		CachedCharacter = Character;
		CachedStamina = Character->FindComponentByClass<UStaminaComponent>();
	}

	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const USurvivorMovementComponent* SurvivorMovement = Cast<USurvivorMovementComponent>(Movement);
	const FVector Velocity = Character->GetVelocity();
	const float WalkSpeed = Movement ? FMath::Max(Movement->MaxWalkSpeed, 1.0f) : 300.0f;
	const bool bGrounded = Movement && Movement->IsMovingOnGround();
	const bool bCrouching = Movement && Movement->IsCrouching();

	// Offsets are built in camera space (X forward, Y right, Z up), then scaled by Alpha and applied once at the end.
	FVector LocalOffset = FVector::ZeroVector;
	FRotator RotationOffset = FRotator::ZeroRotator;

	if (bEnableHeadBob)
	{
		const bool bSprinting = SurvivorMovement && SurvivorMovement->IsSprinting();
		CurrentSprintAlpha = FMath::FInterpTo(CurrentSprintAlpha, bSprinting ? 1.0f : 0.0f, DeltaTime, BobBlendSpeed);

		float TargetIntensity = bGrounded ? FMath::Clamp(Velocity.Size2D() / WalkSpeed, 0.0f, 1.0f) : 0.0f;
		if (bCrouching)
		{
			TargetIntensity *= CrouchAmplitudeMultiplier;
		}
		CurrentBobIntensity = FMath::FInterpTo(CurrentBobIntensity, TargetIntensity, DeltaTime, BobBlendSpeed);

		const float Amplitude = CurrentBobIntensity * FMath::Lerp(1.0f, SprintAmplitudeMultiplier, CurrentSprintAlpha);
		const float Frequency = BobFrequency * FMath::Lerp(1.0f, SprintFrequencyMultiplier, CurrentSprintAlpha);
		BobPhase = FMath::Fmod(BobPhase + DeltaTime * Frequency * UE_TWO_PI, UE_TWO_PI);

		// |sin| dips twice per cycle (one per footstep); plain sin sways left then right once per cycle.
		const float Sin = FMath::Sin(BobPhase);
		LocalOffset.Z -= FMath::Abs(Sin) * BobVerticalAmplitude * Amplitude;
		LocalOffset.Y += Sin * BobHorizontalAmplitude * Amplitude;
		RotationOffset.Roll += Sin * BobRollAmplitude * Amplitude;
	}

	if (bEnableBreathing)
	{
		// 1 at full stamina, 0 when empty.
		const float StaminaAlpha = CachedStamina.IsValid() ? CachedStamina->GetStaminaNormalized() : 1.0f;
		const float Rate = BreathingFrequency * FMath::Lerp(ExhaustedRateMultiplier, 1.0f, StaminaAlpha);
		const float Depth = BreathingAmplitude * FMath::Lerp(ExhaustedAmplitudeMultiplier, 1.0f, StaminaAlpha);

		BreathPhase = FMath::Fmod(BreathPhase + DeltaTime * Rate * UE_TWO_PI, UE_TWO_PI);
		LocalOffset.Z += FMath::Sin(BreathPhase) * Depth;
	}

	if (bEnableStrafeRoll)
	{
		const float RightSpeed = FVector::DotProduct(Velocity, Character->GetActorRightVector());
		const float TargetRoll = FMath::Clamp(RightSpeed / WalkSpeed, -1.5f, 1.5f) * StrafeRollAngle;
		CurrentStrafeRoll = FMath::FInterpTo(CurrentStrafeRoll, TargetRoll, DeltaTime, StrafeRollBlendSpeed);
		RotationOffset.Roll += CurrentStrafeRoll;
	}

	if (bEnableLookLag)
	{
		const FRotator AimRotation = InOutPOV.Rotation;
		if (!bHasLaggedRotation)
		{
			LaggedRotation = AimRotation;
			bHasLaggedRotation = true;
		}

		LaggedRotation = FMath::RInterpTo(LaggedRotation, AimRotation, DeltaTime, LookLagSpeed);

		// Clamp how far behind we can be, then rebuild the lagged rotation from the clamped difference.
		FRotator Lag = (LaggedRotation - AimRotation).GetNormalized();
		Lag.Pitch = FMath::Clamp(Lag.Pitch, -MaxLookLagAngle, MaxLookLagAngle);
		Lag.Yaw = FMath::Clamp(Lag.Yaw, -MaxLookLagAngle, MaxLookLagAngle);
		Lag.Roll = 0.0f;
		LaggedRotation = AimRotation + Lag;

		RotationOffset.Pitch += Lag.Pitch;
		RotationOffset.Yaw += Lag.Yaw;
		RotationOffset.Roll -= Lag.Yaw * TurnRollScale;
	}
	else
	{
		bHasLaggedRotation = false;
	}

	InOutPOV.Location += InOutPOV.Rotation.RotateVector(LocalOffset * Alpha);
	InOutPOV.Rotation = (InOutPOV.Rotation + RotationOffset * Alpha).GetNormalized();

	// false = let any later modifiers run too.
	return false;
}
