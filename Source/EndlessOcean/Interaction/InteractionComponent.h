#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "InteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInteractionFocusChangedSignature, AActor*, NewFocus, AActor*, OldFocus);

/**
 * Finds the IInteractable the owner is looking at and interacts with it on request.
 *
 * Instead of tracing every frame in Tick, it traces on a timer (default ~15 times a second). That's plenty for
 * a "what am I looking at" prompt and costs a fraction of a per-frame trace.
 * Uses the owner's view point (the camera, including head-bob), so it matches what the player sees.
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
	float TraceDistance = 200.0f;

	/** Radius of the trace sphere. A small radius makes small items easier to aim at. 0 = a thin line trace. */
	UPROPERTY(EditAnywhere, Category="Interaction", meta=(ClampMin=0, ClampMax=30, Units="cm"))
	float TraceRadius = 4.0f;

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
};
