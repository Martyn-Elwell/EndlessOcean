#include "SurvivorMovementComponent.h"

#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PhysicsVolume.h"
#include "WaterQueryLibrary.h"

USurvivorMovementComponent::USurvivorMovementComponent()
{
	// Slow, heavy defaults. All of these show in the Details panel of the movement component, so tune them there.
	MaxWalkSpeed = 220.0f;
	MaxWalkSpeedCrouched = 120.0f;
	MaxAcceleration = 900.0f;
	BrakingDecelerationWalking = 900.0f;
	GroundFriction = 6.0f;

	// Swimming: slower than walking, with heavy braking so stopping in the water feels like pushing against it.
	MaxSwimSpeed = 180.0f;
	BrakingDecelerationSwimming = 400.0f;
	Buoyancy = SurfaceBuoyancy;

	// Crouching on, jumping off. Standing up is refused automatically if there's no head room.
	NavAgentProps.bCanCrouch = true;
	NavAgentProps.bCanJump = false;
	NavAgentProps.bCanSwim = true;
	bCanWalkOffLedgesWhenCrouching = true;
	SetCrouchedHalfHeight(50.0f);
}

bool USurvivorMovementComponent::CanSprintNow() const
{
	if (!bWantsToSprint || bSprintBlocked || IsCrouching() || !CharacterOwner)
	{
		return false;
	}

	const bool bSwimming = IsSwimming();
	if (!bSwimming && !IsMovingOnGround())
	{
		return false;
	}

	// Acceleration here is the player's movement input, scaled to MaxAcceleration. Underwater it's 3D and is compared
	// with where the camera looks; on land only the horizontal part matters, compared with the body's facing.
	const FVector InputDirection = bSwimming ? GetCurrentAcceleration().GetSafeNormal() : GetCurrentAcceleration().GetSafeNormal2D();
	if (InputDirection.IsNearlyZero())
	{
		return false;
	}

	const FVector Forward = bSwimming
		? CharacterOwner->GetControlRotation().Vector()
		: CharacterOwner->GetActorForwardVector().GetSafeNormal2D();
	return FVector::DotProduct(InputDirection, Forward) >= MinSprintForwardDot;
}

float USurvivorMovementComponent::GetMaxSpeed() const
{
	if (MovementMode == MOVE_Walking && CanSprintNow())
	{
		return SprintSpeed;
	}
	if (MovementMode == MOVE_Swimming && CanSprintNow())
	{
		return SwimSprintSpeed;
	}
	return Super::GetMaxSpeed();
}

void USurvivorMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	// Only count as sprinting (and costing stamina) once we're actually moving faster than normal.
	const bool bFastEnough = IsSwimming()
		? Velocity.SizeSquared() > FMath::Square(MaxSwimSpeed)
		: Velocity.SizeSquared2D() > FMath::Square(MaxWalkSpeed);
	const bool bNowSprinting = CanSprintNow() && bFastEnough;
	if (bNowSprinting != bIsSprinting)
	{
		bIsSprinting = bNowSprinting;
		OnSprintStateChanged.Broadcast(bIsSprinting);
	}
}

void USurvivorMovementComponent::PhysSwimming(float DeltaTime, int32 Iterations)
{
	// The engine's swim physics pushes up with (Buoyancy x how submerged the capsule is). With buoyancy above 1 you
	// settle with part of the body out of the water; at exactly 1 you hover. Use the floaty value only near the
	// surface, so the deep water still feels neutral and you choose when to rise.
	if (CharacterOwner)
	{
		const float EyeDepth = UWaterQueryLibrary::GetWaterDepthAt(this, CharacterOwner->GetPawnViewLocation());
		Buoyancy = EyeDepth < SurfaceZoneDepth ? SurfaceBuoyancy : DeepBuoyancy;
	}

	Super::PhysSwimming(DeltaTime, Iterations);
}

bool USurvivorMovementComponent::TryClimbOutOfWater()
{
	UWorld* World = GetWorld();
	if (!IsSwimming() || !CharacterOwner || !World || World->GetTimeSeconds() < NextClimbOutTime)
	{
		return false;
	}

	const FVector Location = UpdatedComponent->GetComponentLocation();
	APhysicsVolume* Water = nullptr;
	UWaterQueryLibrary::GetWaterDepthAt(this, Location, Water);
	if (!Water)
	{
		return false;
	}

	// Only from the surface: the eyes must be (nearly) out of the water.
	if (UWaterQueryLibrary::GetWaterDepthAt(this, CharacterOwner->GetPawnViewLocation()) > SurfaceZoneDepth * 0.25f)
	{
		return false;
	}

	const float SurfaceZ = UWaterQueryLibrary::GetWaterSurfaceZ(Water);
	const float Radius = CharacterOwner->GetSimpleCollisionRadius();
	const float HalfHeight = CharacterOwner->GetSimpleCollisionHalfHeight();
	const FVector Forward = CharacterOwner->GetActorForwardVector().GetSafeNormal2D();
	const ECollisionChannel Channel = UpdatedComponent->GetCollisionObjectType();
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(ClimbOutOfWater), false, CharacterOwner);

	// 1. Is there a wall right in front of us at water level?
	FHitResult WallHit;
	const FVector WallStart(Location.X, Location.Y, SurfaceZ);
	if (!World->LineTraceSingleByChannel(WallHit, WallStart, WallStart + Forward * (Radius + ClimbOutReach), Channel, Params))
	{
		return false;
	}

	// 2. Find the top of it: trace down just beyond the wall face, from the highest climbable height.
	const FVector LedgePoint = WallHit.ImpactPoint + Forward * (Radius + 5.0f);
	FHitResult FloorHit;
	const FVector DownStart(LedgePoint.X, LedgePoint.Y, SurfaceZ + MaxClimbOutHeight);
	const FVector DownEnd(LedgePoint.X, LedgePoint.Y, SurfaceZ - 5.0f);
	if (!World->LineTraceSingleByChannel(FloorHit, DownStart, DownEnd, Channel, Params) || FloorHit.bStartPenetrating || !IsWalkable(FloorHit))
	{
		return false;
	}

	// 3. Is there room to stand up there?
	const FVector StandLocation = FloorHit.ImpactPoint + FVector(0.0f, 0.0f, HalfHeight + 2.0f);
	if (World->OverlapBlockingTestByChannel(StandLocation, FQuat::Identity, Channel, GetPawnCapsuleCollisionShape(SHRINK_None), Params))
	{
		return false;
	}

	// 4. Launch with just enough vertical speed to lift the capsule over the edge (v = sqrt(2 g h)).
	const float Rise = FMath::Max(StandLocation.Z + ClimbOutClearance - Location.Z, 10.0f);
	const float UpSpeed = FMath::Sqrt(2.0f * FMath::Abs(GetGravityZ()) * Rise);
	CharacterOwner->LaunchCharacter(Forward * ClimbOutForwardSpeed + FVector::UpVector * UpSpeed, true, true);

	NextClimbOutTime = World->GetTimeSeconds() + ClimbOutCooldown;
	return true;
}
