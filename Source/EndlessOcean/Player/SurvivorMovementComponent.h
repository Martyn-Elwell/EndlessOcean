#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SurvivorMovementComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSurvivorSprintStateChangedSignature, bool, bIsSprinting);

/**
 * Character movement for the survivor: slow and heavy, with a sprint that's only allowed while moving
 * forward on the ground, standing, and not blocked (e.g. out of stamina).
 *
 * Sprint is implemented by overriding GetMaxSpeed() rather than by poking MaxWalkSpeed from outside. That way
 * there's one source of truth for "how fast can I go right now", and walk/crouch/sprint can't fight each other.
 * The stamina cost lives elsewhere (UStaminaComponent); this class only reports when sprinting starts and stops.
 */
UCLASS(PrioritizeCategories=(Sprint))
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

	/** Broadcast when the effective sprint state changes. UStaminaComponent listens to this. */
	UPROPERTY(BlueprintAssignable, Category="Survivor Movement")
	FSurvivorSprintStateChangedSignature OnSprintStateChanged;

protected:

	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

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

	/** All conditions for sprinting, based on input direction rather than velocity so it kicks in from a standstill. */
	bool CanSprintNow() const;
};
