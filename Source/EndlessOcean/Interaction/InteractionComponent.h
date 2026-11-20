#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "InteractionComponent.generated.h"

struct FCollisionQueryParams;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInteractionFocusChangedSignature, AActor*, NewFocus, AActor*, OldFocus);

/**
 * Finds the IInteractable the owner is looking at and interacts with it on request.
 *
 * Instead of tracing every frame in Tick, it traces on a timer (default ~15 times a second). That's plenty for
 * a "what am I looking at" prompt and costs a fraction of a per-frame trace.
 * Uses the owner's view point (the camera, including head-bob), so it matches what the player sees.
 *
 * Two passes, so small items don't need pixel-perfect aim:
 * 1. A sphere sweep down the view. Whatever it hits first wins (a wall in front hides what's behind it).
 * 2. If that found nothing usable, "aim assist": every interactable within reach is scored by how far the view
 *    ray misses its bounds (visual-only parts included), and the closest miss within the assist cone wins,
 *    as long as nothing blocks the line of sight to it.
 */
UCLASS(ClassGroup=(EndlessOcean), PrioritizeCategories=(Interaction), meta=(BlueprintSpawnableComponent))
class ENDLESSOCEAN_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UInteractionComponent();

	/** Interacts with the focused actor, if any. Returns true if an interaction happened. */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	bool TryInteract();

	/** Runs the focus trace immediately instead of waiting for the next timer tick. */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void UpdateFocus();

	/** Returns the interactable actor currently in focus, or nullptr. */
	UFUNCTION(BlueprintPure, Category="Interaction")
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	/** Returns the prompt of the focused interactable, or empty text if nothing is focused. */
	UFUNCTION(BlueprintPure, Category="Interaction")
	FText GetFocusedPrompt() const;

	/** Called whenever the focused interactable changes (either value may be nullptr). */
	UPROPERTY(BlueprintAssignable, Category="Interaction")
	FInteractionFocusChangedSignature OnFocusChanged;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** How far from the eyes the player can interact. */
	UPROPERTY(EditAnywhere, Category="Interaction", meta=(ClampMin=10, ClampMax=1000, UIMin=50, UIMax=400, Units="cm"))
	float TraceDistance = 250.0f;

	/** Radius of the trace sphere. A bigger radius makes small items easier to aim at. 0 = a thin line trace. */
	UPROPERTY(EditAnywhere, Category="Interaction", meta=(ClampMin=0, ClampMax=30, Units="cm"))
	float TraceRadius = 6.0f;

	/** If the direct trace finds nothing, pick the interactable the view ray passes closest to (see the class comment). */
	UPROPERTY(EditAnywhere, Category="Interaction|AimAssist")
	bool bAimAssist = true;

	/** An item counts as "looked at" if the view ray passes within this angle of its bounds... */
	UPROPERTY(EditAnywhere, Category="Interaction|AimAssist", meta=(EditCondition="bAimAssist", EditConditionHides, ClampMin=0, ClampMax=45, UIMax=25, Units="deg"))
	float AssistAngle = 10.0f;

	/** ...or within this distance of them. Helps with items right at your feet, where a few degrees is only a few cm. */
	UPROPERTY(EditAnywhere, Category="Interaction|AimAssist", meta=(EditCondition="bAimAssist", EditConditionHides, ClampMin=0, ClampMax=100, UIMax=50, Units="cm"))
	float AssistRadius = 20.0f;

	/**
	 * The item already in focus keeps it until the miss grows past the limits above times this. Stops the prompt
	 * flickering between two items, or on and off, at the edge of the cone.
	 */
	UPROPERTY(EditAnywhere, Category="Interaction|AimAssist", meta=(EditCondition="bAimAssist", EditConditionHides, ClampMin=1, ClampMax=3))
	float FocusStickiness = 1.5f;

	/** Time between focus traces. Lower is more responsive but costs more. */
	UPROPERTY(EditAnywhere, Category="Interaction", meta=(ClampMin=0.01, ClampMax=0.5, Units="s"))
	float TraceInterval = 0.066f;

	/** Trace channel used to find interactables. "Interaction" (GameTraceChannel2) is set up in Project Settings > Collision. */
	UPROPERTY(EditAnywhere, Category="Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel2;

private:

	/** Weak pointer: we only observe the focused actor; if it's destroyed this simply becomes null. */
	TWeakObjectPtr<AActor> FocusedActor;

	/** Handle for the repeating focus trace. */
	FTimerHandle TraceTimer;

	/** Sets the focused actor and broadcasts if it changed. */
	void SetFocusedActor(AActor* NewFocus);

	/** True if Actor is an interactable the owner can use right now. */
	bool IsUsable(AActor* Actor) const;

	/** Pass 2 of the focus search: the usable interactable the view ray passes closest to, or nullptr. */
	AActor* FindAssistTarget(const FVector& ViewLocation, const FVector& ViewDirection, const FCollisionQueryParams& Params) const;
};
