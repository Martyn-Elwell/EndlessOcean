#include "SurvivorMovementComponent.h"

#include "GameFramework/Character.h"

USurvivorMovementComponent::USurvivorMovementComponent()
{
	// Slow, heavy defaults. All of these show in the Details panel of the movement component, so tune them there.
	MaxWalkSpeed = 220.0f;
	MaxWalkSpeedCrouched = 120.0f;
	MaxAcceleration = 900.0f;
	BrakingDecelerationWalking = 900.0f;
	GroundFriction = 6.0f;

	// Crouching on, jumping off. Standing up is refused automatically if there's no head room.
	NavAgentProps.bCanCrouch = true;
	NavAgentProps.bCanJump = false;
	bCanWalkOffLedgesWhenCrouching = true;
	SetCrouchedHalfHeight(50.0f);
}

bool USurvivorMovementComponent::CanSprintNow() const
{
	if (!bWantsToSprint || bSprintBlocked || !IsMovingOnGround() || IsCrouching() || !CharacterOwner)
	{
		return false;
	}

	// Acceleration here is the player's movement input, scaled to MaxAcceleration.
	const FVector InputDirection = GetCurrentAcceleration().GetSafeNormal2D();
	if (InputDirection.IsNearlyZero())
	{
		return false;
	}

	const FVector Forward = CharacterOwner->GetActorForwardVector().GetSafeNormal2D();
	return FVector::DotProduct(InputDirection, Forward) >= MinSprintForwardDot;
}

float USurvivorMovementComponent::GetMaxSpeed() const
{
	if (MovementMode == MOVE_Walking && CanSprintNow())
	{
		return SprintSpeed;
	}
	return Super::GetMaxSpeed();
}

void USurvivorMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	// Only count as sprinting (and costing stamina) once we're actually moving faster than a walk.
	const bool bNowSprinting = CanSprintNow() && Velocity.SizeSquared2D() > FMath::Square(MaxWalkSpeed);
	if (bNowSprinting != bIsSprinting)
	{
		bIsSprinting = bNowSprinting;
		OnSprintStateChanged.Broadcast(bIsSprinting);
	}
}
