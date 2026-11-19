#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FlareItem.h" // full include: a UFUNCTION takes EFlareState by value, and the generated code needs the complete enum
#include "SurvivorHUDWidget.generated.h"

class AHeldItem;
class UBreathComponent;
class UInteractionComponent;
class UInventoryComponent;
class UItemHolderComponent;
class UStaminaComponent;

/**
 * C++ base for the survivor HUD. It subscribes to the pawn's components and turns their delegates into simple
 * Blueprint events. The widget Blueprint (UI_SurvivorHUD) only does layout: implement the "On ..." events in
 * its Event Graph and update text and progress bars from them.
 *
 * Event-driven rather than property bindings, which UMG re-evaluates every frame.
 */
UCLASS(Abstract)
class ENDLESSOCEAN_API USurvivorHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Starts listening to a pawn's components (nullptr to stop). Called by the player controller. */
	void ObservePawn(APawn* Pawn);

protected:

	virtual void NativeDestruct() override;

	/** Stamina changed (0 to 1). */
	UFUNCTION(BlueprintImplementableEvent, Category="Survivor HUD")
	void OnStaminaChanged(float NormalizedStamina);

	/** Became exhausted, or recovered. */
	UFUNCTION(BlueprintImplementableEvent, Category="Survivor HUD")
	void OnExhaustedChanged(bool bExhausted);

	/** Show (bVisible) or hide the interaction prompt. */
	UFUNCTION(BlueprintImplementableEvent, Category="Survivor HUD")
	void OnInteractionPromptChanged(bool bVisible, const FText& Prompt);

	/** The item in hand changed. NewItem may be null (empty hands). */
	UFUNCTION(BlueprintImplementableEvent, Category="Survivor HUD")
	void OnHeldItemChanged(AHeldItem* NewItem, const FText& ItemName);

	/** The inventory changed. Slots may contain nulls (empty slots). ActiveSlot is where the held item came from, or -1. */
	UFUNCTION(BlueprintImplementableEvent, Category="Survivor HUD")
	void OnInventoryChanged(const TArray<AHeldItem*>& Slots, int32 ActiveSlot);

	/** Burn time of the held flare. Called repeatedly while a lit flare is in hand. bVisible false = hide the timer. */
	UFUNCTION(BlueprintImplementableEvent, Category="Survivor HUD")
	void OnFlareTimeChanged(bool bVisible, float SecondsRemaining, float NormalizedRemaining);

	/**
	 * Breath changed (0 to 1). bVisible is true while underwater or still refilling, so the bar can hide itself when
	 * it isn't relevant. There is deliberately no health event: health shows as screen effects, not a bar.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Survivor HUD")
	void OnBreathChanged(float NormalizedBreath, bool bVisible);

	/** How often the flare timer refreshes. */
	UPROPERTY(EditAnywhere, Category="Survivor HUD", meta=(ClampMin=0.05, ClampMax=1, Units="s"))
	float FlareTimerInterval = 0.1f;

private:

	UFUNCTION()
	void HandleStaminaChanged(float NormalizedStamina);

	UFUNCTION()
	void HandleExhaustedChanged(bool bExhausted);

	UFUNCTION()
	void HandleFocusChanged(AActor* NewFocus, AActor* OldFocus);

	UFUNCTION()
	void HandleHeldItemChanged(AHeldItem* NewItem, AHeldItem* OldItem);

	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION()
	void HandleFlareStateChanged(AFlareItem* Flare, EFlareState NewState);

	UFUNCTION()
	void HandleBreathChanged(float NormalizedBreath);

	UFUNCTION()
	void HandleSubmergedChanged(bool bSubmerged);

	/** Pushes the current flare burn time to Blueprint. */
	void UpdateFlareTimer();

	/** Unsubscribes from everything and stops the timer. */
	void StopObserving();

	/** Starts or stops the flare timer depending on whether a burning flare is in hand. */
	void RefreshFlareTimer();

	// Weak pointers: the HUD observes these but must never keep them alive.
	TWeakObjectPtr<UStaminaComponent> Stamina;
	TWeakObjectPtr<UInteractionComponent> Interaction;
	TWeakObjectPtr<UItemHolderComponent> ItemHolder;
	TWeakObjectPtr<UInventoryComponent> Inventory;
	TWeakObjectPtr<UBreathComponent> Breath;
	TWeakObjectPtr<AFlareItem> WatchedFlare;

	FTimerHandle FlareTimer;
};
