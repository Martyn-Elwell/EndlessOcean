#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SurvivorMovementComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSurvivorSprintStateChangedSignature, bool, bIsSprinting);

/**
 * Character movement for the survivor: slow and heavy, with a sprint that's only allowed while moving
 * forward (on the ground or swimming), standing, and not blocked (e.g. out of stamina).
 *
 * Sprint is implemented by overriding GetMaxSpeed() rather than by poking MaxWalkSpeed from outside. That way
 * there's one source of truth for "how fast can I go right now", and walk/crouch/sprint can't fight each other.
 * The stamina cost lives elsewhere (UStaminaComponent); this class only reports when sprinting starts and stops.
 *
 * Swimming uses the engine's built-in swim mode (entered automatically inside a Physics Volume with "Water Volume"
 * ticked). This class adds: buoyancy that floats you up when near the surface (so you rest with your head above water)
 * but is neutral when deep, an underwater sprint, and climbing out onto a low ledge.
 */
UCLASS(PrioritizeCategories=(Sprint, Swimming))
class ENDLESSOCEAN_API USurvivorMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:

	USurvivorMovementComponent();

	/** Called from input: the player is holding (or released) the sprint key. */
	UFUNCTION(BlueprintCallable, Category="Survivor Movement")
	void SetWantsToSprint(bool bWants) { bWantsToSprint = bWants; }

	/** Blocks or unblocks sprinting regardless of input, e.g. while exhausted. */
	UFUNCTION(BlueprintCallable, Category="Survivor Movement")
	void SetSprintBlocked(bool bBlocked) { bSprintBlocked = bBlocked; }

	/** True while actually sprinting (not just holding the key). */
	UFUNCTION(BlueprintPure, Category="Survivor Movement")
	bool IsSprinting() const { return bIsSprinting; }

	/** Top speed while sprinting. */
	UFUNCTION(BlueprintPure, Category="Survivor Movement")
	float GetSprintSpeed() const { return SprintSpeed; }

	virtual float GetMaxSpeed() const override;

	/**
	 * At the surface and facing a ledge low enough to climb (MaxClimbOutHeight), launch up and onto it.
	 * Returns true if a climb started. Called while the rise key is held.
	 */
	UFUNCTION(BlueprintCallable, Category="Survivor Movement")
	bool TryClimbOutOfWater();

	/** Broadcast when the effective sprint state changes. UStaminaComponent listens to this. */
	UPROPERTY(BlueprintAssignable, Category="Survivor Movement")
	FSurvivorSprintStateChangedSignature OnSprintStateChanged;

protected:

	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

	/** Picks surface or deep buoyancy each frame before the engine's swim physics runs. */
	virtual void PhysSwimming(float DeltaTime, int32 Iterations) override;

	/** Top swim speed while sprinting underwater. Normal swim speed is the standard Max Swim Speed property. */
	UPROPERTY(EditAnywhere, Category="Swimming", meta=(ClampMin=0, ClampMax=1000, Units="cm/s"))
	float SwimSprintSpeed = 300.0f;

	/**
	 * Buoyancy while the eyes are near the surface. Above 1 you float up and rest with part of the body out of the
	 * water: at 1.3 about a quarter of the capsule (and the eyes) stays above the surface.
	 */
	UPROPERTY(EditAnywhere, Category="Swimming", meta=(ClampMin=0, ClampMax=3))
	float SurfaceBuoyancy = 1.3f;

	/** Buoyancy when deeper than SurfaceZoneDepth. 1 = neutral: you hover where you stop. */
	UPROPERTY(EditAnywhere, Category="Swimming", meta=(ClampMin=0, ClampMax=3))
	float DeepBuoyancy = 1.0f;

	/** How far below the surface the eyes can be and still count as "near the surface" (floating up to rest). */
	UPROPERTY(EditAnywhere, Category="Swimming", meta=(ClampMin=0, ClampMax=500, Units="cm"))
	float SurfaceZoneDepth = 120.0f;

	/** Highest ledge, measured from the water surface, that can be climbed out onto. */
	UPROPERTY(EditAnywhere, Category="Swimming", meta=(ClampMin=0, ClampMax=300, Units="cm"))
	float MaxClimbOutHeight = 110.0f;

	/** How far in front of the capsule to look for a ledge. */
	UPROPERTY(EditAnywhere, Category="Swimming", meta=(ClampMin=5, ClampMax=150, Units="cm"))
	float ClimbOutReach = 40.0f;

	/** Forward speed of the climb-out launch. */
	UPROPERTY(EditAnywhere, Category="Swimming", meta=(ClampMin=0, ClampMax=800, Units="cm/s"))
	float ClimbOutForwardSpeed = 220.0f;

	/** Extra height cleared above the ledge so the capsule doesn't clip its edge. */
	UPROPERTY(EditAnywhere, Category="Swimming", meta=(ClampMin=0, ClampMax=100, Units="cm"))
	float ClimbOutClearance = 25.0f;

	/** Minimum time between climb attempts. */
	UPROPERTY(EditAnywhere, Category="Swimming", meta=(ClampMin=0, ClampMax=5, Units="s"))
	float ClimbOutCooldown = 0.8f;

	/** Top speed while sprinting. Walk speed is the standard Max Walk Speed property. */
	UPROPERTY(EditAnywhere, Category="Sprint", meta=(ClampMin=0, ClampMax=1200, UIMax=800, Units="cm/s"))
	float SprintSpeed = 430.0f;

	/**
	 * How "forward" the movement input must be to allow sprinting. 1 = straight ahead only, 0 = up to sideways,
	 * -1 = any direction (including backwards).
	 */
	UPROPERTY(EditAnywhere, Category="Sprint", meta=(ClampMin=-1, ClampMax=1))
	float MinSprintForwardDot = 0.5f;

private:

	/** Whether the sprint input is held. */
	bool bWantsToSprint = false;

	/** Whether something (stamina) currently forbids sprinting. */
	bool bSprintBlocked = false;

	/** Cached effective sprint state, used to broadcast only on changes. */
	bool bIsSprinting = false;

	/** World time before which another climb-out isn't allowed. */
	double NextClimbOutTime = 0.0;

	/** All conditions for sprinting, based on input direction rather than velocity so it kicks in from a standstill. */
	bool CanSprintNow() const;
};
